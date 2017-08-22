#include "apx_stream.h"
#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pack.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif




typedef struct apx_headerLine_tag
{
   int majorVersion;
   int minorVersion;
}apx_headerLine_t;



/**************** Private Function Declarations *******************/
static void apx_istream_handler_open(const apx_istream_handler_t *handler);
static void apx_istream_handler_node(const apx_istream_handler_t *handler, const char *name); //N"<name>"
static void apx_istream_handler_datatype(const apx_istream_handler_t *handler,const char *name, const char *dsg, const char *attr); //T"<name>"<dsg>:<attr>
static void apx_istream_handler_require(const apx_istream_handler_t *handler,const char *name, const char *dsg, const char *attr); //R"<name>"<dsg>:<attr>
static void apx_istream_handler_provide(const apx_istream_handler_t *handler,const char *name, const char *dsg, const char *attr); //P"<name>"<dsg>:<attr>
static void apx_istream_handler_close(const apx_istream_handler_t *handler);

static const uint8_t* apx_istream_parseNodeName(apx_istream_t *self,const uint8_t *pBegin, const uint8_t *pEnd);
static const uint8_t *apx_stream_parse_textLine(apx_istream_t *self,const uint8_t *pLineBegin,const uint8_t *pLineEnd);
static const uint8_t *apx_stream_parseApxHeaderLine(const uint8_t *pBegin, const uint8_t *pEnd, apx_headerLine_t *data);
static const uint8_t * apx_splitDeclarationLine(const uint8_t *pBegin,const uint8_t *pEnd, apx_declarationLine_t *data);

/**************** Private Variable Declarations *******************/


/****************** Public Function Definitions *******************/
//constructor/destructor
apx_istream_t *apx_istream_new(apx_istream_handler_t *handler){
   apx_istream_t *self = (apx_istream_t*) malloc(sizeof(apx_istream_t));
   if(self != 0){
      apx_istream_create(self,handler);
   }
   else{
      errno = ENOMEM;
   }
   return self;
}

void apx_istream_delete(apx_istream_t *self){
   if(self != 0){
      apx_istream_destroy(self);
      free(self);
   }
}

void apx_istream_create(apx_istream_t *self, apx_istream_handler_t *handler){
   if(self != 0){
      memcpy(&self->handler,handler,sizeof(apx_istream_handler_t));
      adt_bytearray_create(&self->buf,APX_BUF_GROW_SIZE);
      self->parseState = APX_ISTREAM_STATE_HEADER;
      apx_declarationLine_create(&self->declarationLine);
   }
}

void apx_istream_destroy(apx_istream_t *self){
   if(self != 0){
      adt_bytearray_destroy(&self->buf);
      apx_declarationLine_destroy(&self->declarationLine);
   }
}


//istream functions
void apx_istream_open(apx_istream_t *self){
   if(self != 0){
      apx_istream_handler_open(&self->handler);
   }
}

/**
 * Writes data to the istream. This function parses the data and forwards to sub-handlers
 * Returns 0 on success, negative on error
 */
void apx_istream_write(apx_istream_t *self, const uint8_t *pChunk, uint32_t chunkLen){
   if( (self != 0) && (pChunk != 0) && (chunkLen != 0) ){
      const uint8_t *pEnd;
      const uint8_t *pBegin;
      const uint8_t *pNext;
      const uint8_t *pResult;
      const uint8_t *pNextOld = 0;
      adt_bytearray_append(&self->buf,(uint8_t*) pChunk,(uint32_t) chunkLen);
      pBegin = adt_bytearray_data(&self->buf);
      pEnd = pBegin+adt_bytearray_length(&self->buf);
      pNext = pBegin;
      while(pNext < pEnd){
         uint8_t firstByte;

         assert(pNextOld != pNext); //sanity-check for parser during development, this must never be true
         pNextOld = pNext;
         firstByte =  *pNext; //do not move pNext forward yet, it could be a single '\n' character
         if(firstByte < 128U){
            //ascii character if firstByte is in the range 0-127-
            //wait to parse until a complete line has been seen. lines end with a single \n (not with \r\n as in HTML)
            //if the line is empty it means end of data-block. This is the same principe as the empty \r\n at the end of an HTML request header.
            const uint8_t *pLineEnd = 0;
            const uint8_t *pLineBegin = pNext;

            pLineEnd = bstr_searchVal(pNext,pEnd,(uint8_t) '\n');

            if(pLineEnd == pLineBegin){
               pNext = pLineEnd+1;
               if(self->handler.node_end != 0){
                  //empty line '\n'
                  self->handler.node_end(self->handler.arg);
               }
            }
            else if(pLineEnd > pLineBegin){

               pNext = pLineEnd+1;
               assert( ((char) *pLineEnd) == '\n'); //check during development, remove later
               pResult = apx_stream_parse_textLine(self,pLineBegin,pLineEnd);
               if(pResult == 0){
                  //parse failure, ignore all data
                  adt_bytearray_clear(&self->buf);
                  printf("[APX_STREAM] parse error");
                  return;
               }
            }
            else{
               //'\n' not seen, trim byffer and try parsing again later
               break;
            }
         }
      }
      if (pNext <= pEnd)
      {
         adt_bytearray_trimLeft(&self->buf,pNext);
      }
      else
      {
         //pNext is outside array bounds
         assert(0);
      }
   }
}

/**
 * Closes the istream.
 */
void apx_istream_close(apx_istream_t *self){
   if(self != 0){
      apx_istream_handler_close(&self->handler);
   }
}

/**
 * same as apx_istream_open but uses void argument (used in handler tables)
 */
void apx_istream_vopen(void *arg){
   apx_istream_open((apx_istream_t*) arg);
}

/**
 * same as apx_istream_write but uses void argument (used in handler tables)
 */
void apx_istream_vwrite(void *arg, const uint8_t *pChunk, uint32_t chunkLen){
   apx_istream_write((apx_istream_t*) arg,pChunk,chunkLen);
}

/**
 * same as apx_istream_close but uses void argument (used in handler tables)
 */
void apx_istream_vclose(void *arg){
   apx_istream_close((apx_istream_t*) arg);
}


/**
 * allocates memory for an apx_declarationLine_t object
 */
void apx_declarationLine_create(apx_declarationLine_t *self)
{
   if ( (self != 0) )
   {
      self->pAlloc=0;
      self->allocLen=0;
      self->lineType=0;
      self->name=0;
      self->dsg=0;
      self->attr=0;
   }
}

/**
 * destructor for apx_declarationLine_t object
 */
void apx_declarationLine_destroy(apx_declarationLine_t *self)
{
   if ( (self != 0) && (self->pAlloc != 0))
   {
      free(self->pAlloc);
      self->pAlloc=0;
   }
}

/**
 * resizes the buffer for an apx_declarationLine_t object, all data in buffer will be lost
 */
int8_t apx_declarationLine_resize(apx_declarationLine_t *self, uint32_t len)
{
   if ( (self != 0) && (len > 0) )
   {
      if (len > self->allocLen)
      {
         if (self->pAlloc != 0)
         {
            free(self->pAlloc);
         }
         self->pAlloc = (char*) malloc(len);
         self->allocLen = len;
      }
      if (self->pAlloc != 0)
      {
         return 0;
      }
   }
   return -1;
}

void apx_istream_reset(apx_istream_t *self)
{
   if (self != 0)
   {
      adt_bytearray_clear(&self->buf);
      self->parseState = APX_ISTREAM_STATE_HEADER;
   }
}

/***************** Private Function Definitions *******************/
void apx_istream_handler_open(const apx_istream_handler_t *handler){
   if((handler != 0) && (handler->open != 0)){
      handler->open(handler->arg);
   }
}

void apx_istream_handler_node(const apx_istream_handler_t *handler, const char *name){ //N"<name>"
   if((handler != 0) && (name != 0) && (handler->node != 0)){
      handler->node(handler->arg,name);
   }
}

static void apx_istream_handler_datatype(const apx_istream_handler_t *handler,const char *name, const char *dsg, const char *attr) //T"<name>"<dsg>:<attr>
{
   if((handler != 0) && (handler->datatype != 0)){
      handler->datatype(handler->arg,name,dsg,attr);
   }
}

void apx_istream_handler_require(const apx_istream_handler_t *handler, const char *name, const char *dsg, const char *attr){ //R"<name>"<dsg>:<attr>
   if((handler != 0) && (handler->require != 0)){
      handler->require(handler->arg,name,dsg,attr);
   }
}

void apx_istream_handler_provide(const apx_istream_handler_t *handler, const char *name, const char *dsg, const char *attr){ //P"<name>"<dsg>:<attr>
   if((handler != 0) && (handler->provide != 0)){
      handler->provide(handler->arg,name,dsg,attr);
   }
}



void apx_istream_handler_close(const apx_istream_handler_t *handler){
   if( (handler != 0) && (handler->close != 0) ){
      handler->close(handler->arg);
   }
}

const uint8_t* apx_istream_parseNodeName(apx_istream_t *self, const uint8_t *pBegin, const uint8_t *pEnd){
   if (self != 0){
      const uint8_t *pNext;
      char name[APX_MAX_NAME_LEN+1];
      pNext = bstr_matchPair(pBegin,pEnd,(uint8_t) '\"', (uint8_t) '\"','\\');
      if( (pNext > pBegin) && (pNext<=pEnd) ){
         //pBegin[0] == '"'
         //pNext[0] == '"'
         //pBegin[1] == first character in string (unless empty)
         uint32_t len = (uint32_t) (pNext-pBegin-1);
         if(len <= APX_MAX_NAME_LEN){
            memcpy(name,pBegin+1,len);
            name[len]=0;
            apx_istream_handler_node(&self->handler,name);
         }
         return pNext+1; //return the first character after the right '"'
      }
   }
   return 0;
}


static const uint8_t *apx_stream_parse_textLine(apx_istream_t *self,const uint8_t *pLineBegin,const uint8_t *pLineEnd)
{
   if (self != 0)
   {
      const uint8_t *pNext = pLineBegin;
      const uint8_t *pResult=0;
      apx_headerLine_t header;
      char firstByte=0;
      if (pLineBegin+1<=pLineEnd)
      {
         firstByte=(char) pNext[0];
      }
      if (firstByte != 0)
      {
         switch(self->parseState)
         {
         case APX_ISTREAM_STATE_HEADER:
            pResult = apx_stream_parseApxHeaderLine(pLineBegin,pLineEnd,&header);
            if (pResult != 0)
            {
               if ( (header.majorVersion==1) && (header.minorVersion>=2))
               {
                  pNext=pResult;
                  self->parseState=APX_ISTREAM_STATE_NODE;
               }
            }
            break;
         case APX_ISTREAM_STATE_NODE:
            if (firstByte=='N')
            {
               pResult = apx_istream_parseNodeName(self,pLineBegin+1,pLineEnd);
               if (pResult != 0)
               {
                  pNext=pResult;
                  self->parseState=APX_ISTREAM_STATE_TYPES;
               }
            }
            else
            {
               pNext=0;
            }
            break;
         case APX_ISTREAM_STATE_TYPES:
            pResult = apx_splitDeclarationLine(pLineBegin,pLineEnd,&self->declarationLine);
            if (pResult != 0)
            {
               if (self->declarationLine.lineType==(uint8_t)'T')
               {
                  apx_istream_handler_datatype(&self->handler,self->declarationLine.name,self->declarationLine.dsg,self->declarationLine.attr);
               }
               else if (self->declarationLine.lineType==(uint8_t)'P')
               {
                  self->parseState=APX_ISTREAM_STATE_PORTS;
                  apx_istream_handler_provide(&self->handler,self->declarationLine.name,self->declarationLine.dsg,self->declarationLine.attr);
               }
               else if (self->declarationLine.lineType==(uint8_t)'R')
               {
                  self->parseState=APX_ISTREAM_STATE_PORTS;
                  apx_istream_handler_require(&self->handler,self->declarationLine.name,self->declarationLine.dsg,self->declarationLine.attr);
               }
               else
               {
                  printf("[apx_stream:%d] parse error\n",__LINE__);
                  return 0;
               }
            }
            break;
         case APX_ISTREAM_STATE_PORTS:
            pResult = apx_splitDeclarationLine(pLineBegin,pLineEnd,&self->declarationLine);
            if (pResult != 0)
            {
               if (self->declarationLine.lineType==(uint8_t)'P')
               {
                  apx_istream_handler_provide(&self->handler,self->declarationLine.name,self->declarationLine.dsg,self->declarationLine.attr);
               }
               else if (self->declarationLine.lineType==(uint8_t)'R')
               {
                  apx_istream_handler_require(&self->handler,self->declarationLine.name,self->declarationLine.dsg,self->declarationLine.attr);
               }
               else
               {
                  printf("[apx_stream:%d] parse error\n",__LINE__);
                  return 0;
               }
            }
            break;
         default:
            assert(0);
         }
         return pResult;
      }
      else
      {
         return pLineBegin;
      }
   }
   return 0;
}

static const uint8_t *apx_stream_parseApxHeaderLine(const uint8_t *pBegin, const uint8_t *pEnd, apx_headerLine_t *data)
{
   const uint8_t *pNext = pBegin;
   const uint8_t *pResult = 0;
   const char *str = "APX/";
   int len = (int) strlen(str);
   pResult = bstr_matchStr(pNext,pEnd,(const uint8_t*) str,((const uint8_t*) str)+len);
   if ( (pResult > pNext) && (pNext+len == pResult))
   {
      long number;
      pNext=pResult;
      number = strtol((const char*)pNext,(char **) &pResult,10);
      if (pResult > pNext)
      {
         data->majorVersion=(int) number;
         pNext=pResult;
         if (pNext<pEnd)
         {
            char c = (char) *pNext++;
            if ( (c == '.') && (pNext<pEnd) )
            {
               number = strtol((const char*)pNext,(char **) &pResult,10);
               if (pResult > pNext)
               {
                  data->minorVersion=(int) number;
                  pNext=pResult;
                  return pNext;
               }
            }
         }
      }
   }
   return 0;
}

static const uint8_t * apx_splitDeclarationLine(const uint8_t *pBegin,const uint8_t *pEnd, apx_declarationLine_t *data)
{
   const uint8_t *pNext = (uint8_t*) pBegin;
   const uint8_t *pResult = 0;
   const uint8_t *pNameBegin = 0;
   const uint8_t *pDsgBegin = 0;
   const uint8_t *pAttrBegin = 0;
   uint32_t nameLen=0;
   uint32_t dsgLen=0;
   uint32_t attrLen=0;
   if (pNext < pEnd)
   {
      data->lineType = *pNext++;
      if (pNext < pEnd)
      {
         uint8_t c = (uint8_t) *pNext;
         if (c == '"')
         {
            pResult = bstr_matchPair(pNext,pEnd,'"','"','\\');
            if (pResult > pNext)
            {
               nameLen = (uint32_t) (pResult-pNext-1); //compensate for the first '"' character
               pNameBegin=(pNext+1);
               pNext = pResult+1;
               pResult = bstr_searchVal(pNext,pEnd,':');
               if (pResult > pNext)
               {

                  dsgLen = (uint32_t) (pResult-pNext);
                  pDsgBegin=pNext;
                  pNext = pResult;
                  if (pNext<pEnd)
                  {
                     assert(':'==*pNext);
                     pNext++;
                     if (pNext<pEnd)
                     {
                        attrLen = (uint32_t) (pEnd-pNext);
                        pAttrBegin=pNext;
                        pNext=pEnd;
                     }
                     else
                     {
                        assert(0); //deal with this error later
                     }
                  }
               }
               else if(pResult == pNext) //OK, no ':' in string, put everything in dsg
               {
                  dsgLen = (uint32_t) (pEnd-pNext);
                  pDsgBegin=pNext;
                  pNext=pEnd;
               }
               else
               {
                  //parse failure
                  assert(pResult == 0);
                  return 0;
               }
            }
         }
         if ( (nameLen>0) && (dsgLen>0) )
         {
            int8_t result;
            uint32_t numTerminationChars = (attrLen>0)? 3u : 2u; //need extra bytes for NULL-terminators
            result = apx_declarationLine_resize(data,nameLen+dsgLen+attrLen+numTerminationChars); //this grows the internal buffer if it's too small otherwise the buffer stays the same
            if (result == 0)
            {
               char *pStrNext;
               char *pStrEnd;

               pStrNext= data->pAlloc;
               pStrEnd = pStrNext+data->allocLen;
               memcpy(pStrNext,pNameBegin,nameLen);
               data->name=pStrNext;
               pStrNext+=nameLen;
               *pStrNext++='\0';
               memcpy(pStrNext,pDsgBegin,dsgLen);
               data->dsg=pStrNext;
               pStrNext+=dsgLen;
               *pStrNext++='\0';
               if (attrLen>0)
               {
                  memcpy(pStrNext,pAttrBegin,attrLen);
                  data->attr=pStrNext;
                  pStrNext+=attrLen;
                  *pStrNext++='\0';
               }
               else
               {
                  data->attr = (char*) 0;
               }
               assert(pStrNext<=pStrEnd); //check pointer post conditions
               return pNext;
            }
         }
      }
   }
   return 0; //parse failure
}




