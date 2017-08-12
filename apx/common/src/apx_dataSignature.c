#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include "apx_cfg.h"
#include "apx_dataSignature.h"
#include "bstr.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif


/**************** Private Function Declarations *******************/

static int8_t parseDataSignature(apx_dataSignature_t *self, const uint8_t *dsg);
static const uint8_t *parseDataElement(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseName(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseType(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseArrayLength(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static const uint8_t *parseLimit(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement);
static void calcPackLen(apx_dataElement_t *pDataElement);
/**************** Private Variable Declarations *******************/


/****************** Public Function Definitions *******************/
apx_dataSignature_t *apx_dataSignature_new(const char *dsg)
{
   apx_dataSignature_t *self = (apx_dataSignature_t*) malloc(sizeof(apx_dataSignature_t));
   if(self != 0)
   {
      int8_t result = apx_dataSignature_create(self,dsg);
      if (result<0) //apx_dataSignature_create should have already set errno
      {
         free(self);
         self=0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_dataSignature_delete(apx_dataSignature_t *self)
{
   if(self != 0)
   {
      apx_dataSignature_destroy(self);
      free(self);
   }
}

void apx_dataSignature_vdelete(void *arg)
{
   apx_dataSignature_delete((apx_dataSignature_t*) arg);
}

/**
 * returns 0 on sucess, -1 on failure (also sets errno)
 */
int8_t apx_dataSignature_create(apx_dataSignature_t *self, const char *dsg)
{
   if (self != 0)
   {
      if (dsg != 0)
      {
         self->str=STRDUP(dsg);
         if (self->str == 0)
         {
            errno = ENOMEM;
            return -1;
         }
      }
      else
      {
         self->str = 0;
      }
      if (self->str != 0)
      {
         self->dataElement=apx_dataElement_new(APX_BASE_TYPE_NONE,0);
         parseDataSignature(self,(const uint8_t*) self->str);
      }
      else
      {
         self->dataElement=0;
      }
      self->dsgType = APX_DSG_TYPE_SENDER_RECEIVER; //No support for client/server yet
   }
   return 0;
}

void apx_dataSignature_destroy(apx_dataSignature_t *self)
{
   if (self != 0)
   {
      if (self->dataElement != 0)
      {
         apx_dataElement_delete(self->dataElement);
      }
      if (self->str != 0)
      {
         free(self->str);
      }
   }
}

uint32_t apx_dataSignature_packLen(apx_dataSignature_t *self)
{
   if (self != 0)
   {
      if (self->dataElement != 0)
      {
         return self->dataElement->packLen;
      }
   }
   return 0;
}

/**
 * updates the datasignature object with new dataSignature string.
 * the internal dataelement object will also be updated to reflect the new string.
 */
int8_t apx_dataSignature_update(apx_dataSignature_t *self,const char *dsg)
{
   if ( (self != 0) )
   {
      if (dsg == 0)
      {
         if (self->str != 0)
         {
            free(self->str);
            self->str=0;
            if (self->dataElement != 0)
            {
               apx_dataElement_destroy(self->dataElement);
               apx_dataElement_create(self->dataElement,APX_BASE_TYPE_NONE,0);
            }
            else
            {
               self->dataElement = apx_dataElement_new(APX_BASE_TYPE_NONE,0);
            }
         }
      }
      else
      {
         if ( (self->str != 0) && (strcmp(self->str,dsg)==0)  )
         {
            return 0; //no change
         }
         if ( self->str != 0)
         {
            free(self->str);
         }
         self->str=STRDUP(dsg);
         if (self->str == 0)
         {
            errno = ENOMEM;
            return -1;
         }
         if (self->dataElement != 0)
         {
            apx_dataElement_destroy(self->dataElement);
            apx_dataElement_create(self->dataElement,APX_BASE_TYPE_NONE,0);
         }
         else
         {
            self->dataElement = apx_dataElement_new(APX_BASE_TYPE_NONE,0);
         }
         parseDataSignature(self,(const uint8_t*) self->str);
      }
   }
   return 0;
}

/***************** Private Function Definitions *******************/
/**
 * returns 0 on success, -1 on error
 */
static int8_t parseDataSignature(apx_dataSignature_t *self, const uint8_t *dsg)
{
   const uint8_t *pNext=dsg;
   const uint8_t *pEnd=pNext+strlen((const char*)dsg);
   const uint8_t *pResult;
   char c = (char) *pNext;

   if (c =='{')
   {
      const uint8_t *pRecordBegin = pNext;
      const uint8_t *pRecordEnd=bstr_matchPair(pRecordBegin,pEnd,'{','}','\\');
      if (pRecordEnd > pRecordBegin)
      {
         apx_dataElement_create(self->dataElement,APX_BASE_TYPE_RECORD,0);

         pNext = pRecordBegin+1; //point to the first character after '{'
         while (pNext<pRecordEnd)
         {
            apx_dataElement_t *pChildElement;

            pChildElement = apx_dataElement_new(APX_BASE_TYPE_NONE,0);

            pResult = parseDataElement(pNext,pRecordEnd,pChildElement);
            if (pResult > pNext)
            {
               adt_ary_push(self->dataElement->childElements,pChildElement);
               pNext=pResult;
            }
            else
            {
               apx_dataElement_delete(pChildElement);
               apx_dataElement_delete(self->dataElement);
               self->dataElement=0;
               return -1;
            }
         }
      }
      else
      {
         //not enough bytes in buffer, trigger parse error
         return -1;
      }
   }
   else
   {
      pResult = parseDataElement(pNext,pEnd,self->dataElement);
      if (pResult <= pNext)
      {
         return -1;
      }
   }
   calcPackLen(self->dataElement);
   return 0;
}


static const uint8_t *parseDataElement(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext=pBegin;
   //optional name
   pNext = parseName(pNext,pEnd,pDataElement);
   if (pNext == 0)
   {
      return NULL;
   }
   //mandatory type byte
   pNext = parseType(pNext,pEnd,pDataElement);
   if (pNext == 0)
   {
      return NULL;
   }
   //optional array length [arrayLen]
   pNext = parseArrayLength(pNext,pEnd,pDataElement);
   if (pNext == 0)
   {
      return NULL;
   }
   //optional range limit (min,max)
   pNext = parseLimit(pNext,pEnd,pDataElement);
   if (pNext == 0)
   {
      return NULL;
   }
   calcPackLen(pDataElement);
   return pNext;
}


static const uint8_t *parseName(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext=pBegin;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      errno = EINVAL;
      return 0;
   }
   if (pNext < pEnd)
   {
      char c = (char) *pNext;
      if(c == '\"')
      {
         const uint8_t *pResult;
         pResult=bstr_matchPair(pNext,pEnd,'"','"','\\');
         if (pResult>pNext)
         {
            uint32_t nameLen=(uint32_t) (pResult-pNext-1);
            assert(*pResult=='"');
            if (nameLen<APX_MAX_NAME_LEN)
            {
               pDataElement->name = (char*) bstr_make(pNext+1,pResult); //copy string start from the first byte after left '"' until (but not including) the right '"'
            }
            pNext=pResult+1;
         }
         else
         {
            return 0;
         }
      }
   }
   return pNext;
}


static const uint8_t *parseType(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{   
   const uint8_t *pNext;
   char c;
   (void)pEnd;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      errno = EINVAL;
      return 0;
   }
   pNext =pBegin;
   c = (char) *pNext++;
   //check for name (name is optional)
   switch(c)
   {
   case 'C':
      pDataElement->baseType=APX_BASE_TYPE_UINT8;
      break;
   case 'S':
      pDataElement->baseType=APX_BASE_TYPE_UINT16;
      break;
   case 'L':
      pDataElement->baseType=APX_BASE_TYPE_UINT32;
      break;
   case 'a':
      pDataElement->baseType=APX_BASE_TYPE_STRING;
      break;
   case 'c':
      pDataElement->baseType=APX_BASE_TYPE_SINT8;
      break;
   case 's':
      pDataElement->baseType=APX_BASE_TYPE_SINT16;
      break;
   case 'l':
      pDataElement->baseType=APX_BASE_TYPE_SINT32;
      break;
   case '{':
      apx_dataElement_initRecordType(pDataElement);
      //TODO: implement child record parsing here
      return NULL;
   default:
      return NULL;
   }
   return pNext;
}

static const uint8_t *parseArrayLength(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext=pBegin;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      errno = EINVAL;
      return 0;
   }
   if (pNext < pEnd)
   {
      char c = (char) *pNext;
      if(c == '[')
      {
         const uint8_t *pResult;
         pResult=bstr_matchPair(pNext,pEnd,'[',']','\\');
         if (pResult>pNext)
         {
            long value;
            if (bstr_toLong(pNext+1,pResult,&value) == 0)
            {
               return NULL;
            }
            pDataElement->arrayLen = (uint32_t) value;
            pNext=pResult+1;
         }
         else
         {
            return NULL;
         }
      }
   }
   return pNext;
}

static const uint8_t *parseLimit(const uint8_t *pBegin, const uint8_t *pEnd, apx_dataElement_t *pDataElement)
{
   const uint8_t *pNext;
   (void) pDataElement;
   if ( (pBegin == 0) || (pEnd == 0) || (pDataElement == 0) || (pEnd < pBegin) )
   {
      errno = EINVAL;
      return 0;
   }
   pNext =pBegin;
   if (pNext < pEnd)
   {
      char c = (char) *pNext;
      if(c == '(')
      {
         const uint8_t *pParenBegin = pNext; //'('
         const uint8_t *pParenEnd; //')'
         pParenEnd=bstr_matchPair(pParenBegin,pEnd,'(',')','\\');
         if (pParenEnd>pParenBegin)
         {
            const uint8_t *pMid;
            pParenBegin++; //move past the first '('
            pMid = bstr_searchVal(pParenBegin,pParenEnd,',');
            if (pMid > pParenBegin)
            {
               long min;
               long max;
               uint8_t isParseOK = 0;
               const uint8_t *pResult;
               pResult = bstr_toLong(pParenBegin,pMid,&min);
               if (pResult > pParenBegin)
               {
                  pResult = bstr_toLong(pMid+1,pParenEnd,&max);
                  if (pResult > pMid+1)
                  {
                     isParseOK = 1;
                  }
               }

               if (isParseOK != 0)
               {
                  pNext=pParenEnd+1;
                  switch(pDataElement->baseType)
                  {
                  case APX_BASE_TYPE_NONE:
                     break;
                  case APX_BASE_TYPE_UINT8:
                  case APX_BASE_TYPE_UINT16:
                  case APX_BASE_TYPE_UINT32:
                     pDataElement->min.u32 = (uint32_t) min; //TODO: implement support for unsigned parsing of min/max
                     pDataElement->max.u32 = (uint32_t) max;
                     break;
                  case APX_BASE_TYPE_SINT8:
                  case APX_BASE_TYPE_SINT16:
                  case APX_BASE_TYPE_SINT32:
                     pDataElement->min.s32 = min;
                     pDataElement->max.s32 = max;
                     break;
                  case APX_BASE_TYPE_STRING:
                     break;
                  default:
                     break;
                  }
               }
            }
         }
      }
   }
   return pNext;
}

static void calcPackLen(apx_dataElement_t *pDataElement)
{
   if (pDataElement->baseType == APX_BASE_TYPE_RECORD)
   {
      int32_t i;
      int32_t end = adt_ary_length(pDataElement->childElements);
      pDataElement->packLen=0;
      for(i=0;i<end;i++)
      {
         void **ptr;
         ptr = adt_ary_get(pDataElement->childElements,i);
         if (ptr != 0)
         {
            apx_dataElement_t *pChildElement = (apx_dataElement_t*) *ptr;
            pDataElement->packLen+=pChildElement->packLen;
         }
      }
   }
   else
   {
      uint32_t elemLen=0;
      switch(pDataElement->baseType)
      {
      case APX_BASE_TYPE_NONE:
         break;
      case APX_BASE_TYPE_UINT8:
         elemLen=1;
         break;
      case APX_BASE_TYPE_UINT16:
         elemLen=2;
         break;
      case APX_BASE_TYPE_UINT32:
         elemLen=4;
         break;
      case APX_BASE_TYPE_SINT8:
         elemLen=1;
         break;
      case APX_BASE_TYPE_SINT16:
         elemLen=2;
         break;
      case APX_BASE_TYPE_SINT32:
         elemLen=4;
         break;
      case APX_BASE_TYPE_STRING:
         elemLen=1;
         break;
      default:
         break;
      }
      if (elemLen > 0)
      {
         if (pDataElement->arrayLen > 0)
         {
            pDataElement->packLen=(uint32_t)elemLen*pDataElement->arrayLen;
         }
         else
         {
            pDataElement->packLen=elemLen;
         }
      }
   }
}

