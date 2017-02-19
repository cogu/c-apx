//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "apx_clientConnection.h"
#include "apx_client.h"
#include "scan.h"
#include "headerutil.h"
#include <time.h> //DEBUG
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_HEADER_LEN 128
#define SEND_BUFFER_GROW_SIZE 4096 //4KB

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_clientConnection_parseGreeting(apx_clientConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static int8_t apx_clientConnection_parseMessage(apx_clientConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static uint8_t *apx_clientConnection_getSendBuffer(void *arg, int32_t msgLen);
static int32_t apx_clientConnection_send(void *arg, int32_t offset, int32_t msgLen);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int8_t apx_clientConnection_create(apx_clientConnection_t *self, msocket_t *msocket, struct apx_client_tag *client)
{
   if( (self != 0) && (msocket != 0) &&  (client != 0))
   {
      self->msocket = msocket;
      self->isGreetingParsed = false;
      self->client = client;
      self->maxMsgHeaderSize = (uint8_t) sizeof(uint32_t);
      apx_fileManager_create(&self->fileManager, APX_FILEMANAGER_CLIENT_MODE);
      adt_bytearray_create(&self->sendBuffer, SEND_BUFFER_GROW_SIZE);
      return 0;
   }
   errno=EINVAL;
   return -1;
}

void apx_clientConnection_destroy(apx_clientConnection_t *self)
{
   if (self != 0)
   {
      if (self->msocket != 0)
      {
         msocket_delete(self->msocket);
      }

      apx_fileManager_destroy(&self->fileManager);
      adt_bytearray_destroy(&self->sendBuffer);
   }
}

apx_clientConnection_t *apx_clientConnection_new(msocket_t *msocket, struct apx_client_tag *client)
{
   if ( (msocket != 0) && (client != 0))
   {
      apx_clientConnection_t *self = (apx_clientConnection_t*) malloc(sizeof(apx_clientConnection_t));
      if(self != 0){
         int8_t result = apx_clientConnection_create(self,msocket,client);
         if (result != 0)
         {
            free(self);
            self=0;
         }
      }
      else{
         errno = ENOMEM;
      }
      return self;
   }
   return (apx_clientConnection_t*) 0;
}

void apx_clientConnection_delete(apx_clientConnection_t *self)
{
   if (self != 0)
   {
      apx_clientConnection_destroy(self);
      free(self);
   }
}

void apx_clientConnection_vdelete(void *arg)
{
   apx_clientConnection_delete((apx_clientConnection_t*) arg);
}

/**
 * activates the server connection and sends out the greeting
 */
void apx_clientConnection_start(apx_clientConnection_t *self)
{
   if (self != 0)
   {
      apx_transmitHandler_t serverTransmitHandler;
      //register transmit handler with our fileManager
      serverTransmitHandler.arg = self;
      serverTransmitHandler.send = apx_clientConnection_send;
      serverTransmitHandler.getSendAvail = 0;
      serverTransmitHandler.getSendBuffer = apx_clientConnection_getSendBuffer;
      apx_fileManager_setTransmitHandler(&self->fileManager, &serverTransmitHandler);
      //register connection with the server nodeManager
      apx_nodeManager_attachFileManager(&self->client->nodeManager, &self->fileManager);
      apx_fileManager_start(&self->fileManager);
      //send greeting
      {
         char greeting[RMF_GREETING_MAX_LEN];
         strcpy(greeting, RMF_GREETING_START);
         //headers end with an additional newline
         strcat(greeting, "\n");
         msocket_send(self->msocket, (void*) greeting, (uint32_t) strlen(greeting));
      }
      apx_fileManager_triggerConnectEvent(&self->fileManager);
   }
}


/**
 * called from apx_client when data has been received on the msocket
 */
int8_t apx_clientConnection_dataReceived(apx_clientConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   if ( (self != 0) && (dataBuf != 0) )
   {
      struct timespec timestamp;
      uint32_t totalParseLen = 0;
      uint32_t remain = dataLen;
      int8_t result = 0;
      const uint8_t *pNext = dataBuf;
      clock_gettime(CLOCK_MONOTONIC, &timestamp);
      printf("%0.6f: ", ((double)timestamp.tv_sec)+((double)timestamp.tv_nsec+500000000)/1000000000.0); //FIXME: remove debug print
      printf("apx_clientConnection_dataReceived: %d\n",dataLen); //FIXME: remove debug print
      while(totalParseLen<dataLen)
      {
         uint32_t internalParseLen = 0;
         const char *parseFunc;
         result=0;

         //parse data
         if (self->isGreetingParsed == false)
         {
            parseFunc="greeting";
            result = apx_clientConnection_parseGreeting(self, pNext, remain, &internalParseLen);
         }
         else
         {
            parseFunc="message";
            result = apx_clientConnection_parseMessage(self, pNext, remain, &internalParseLen);
         }
         //check parse result
         if ( (result == 0) && (internalParseLen!=0) )
         {
            printf("\tinternalParseLen(%s): %d\n",parseFunc, internalParseLen); ///FIXME: remove debug print
            assert(internalParseLen<=dataLen);
            pNext+=internalParseLen;
            totalParseLen+=internalParseLen;
            remain-=internalParseLen;
         }
         else
         {
            break;
         }
      }
      //no more complete messages can be parsed. There may be a partial message left in buffer, but we ignore it until more data has been recevied.
      printf("\ttotalParseLen=%d\n", totalParseLen); ///FIXME: remove debug print
      *parseLen = totalParseLen;
      return result;
   }
   return -1;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * parses the greeting header. this is very similar to an HTTP header with an initial protocol line followed by one or more MIME-headers.
 * Instead of line ending \r\n we just just \n. The greeting ends when we encountered two consecutive \n\n.
 * Returns 0 on parse success, -1 on parse failure
 */
static int8_t apx_clientConnection_parseGreeting(apx_clientConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   const uint8_t *pBegin = dataBuf;
   const uint8_t *pNext = dataBuf;
   const uint8_t *pEnd = dataBuf + dataLen;
   while(pNext < pEnd)
   {
      const uint8_t *pResult;
      pResult = scan_line(pNext, pEnd);
      if ( (pResult > pNext) || ((pResult==pNext) && *pNext==(uint8_t) '\n') )
      {
         //found a line ending with '\n'
         const uint8_t *pMark = pNext;
         int32_t lengthOfLine = (int32_t) (pResult-pNext);
         //move pNext to beginning of next line (one byte after the '\n')
         pNext = pResult+1;
         if (lengthOfLine == 0)
         {
            //this ends the header
            self->isGreetingParsed = true;
            printf("\tparse greeting complete\n"); //FIXME: this is just a debug printout, can be removed
            break;
         }
         else
         {
            //TODO: parse greeting line
            if (lengthOfLine<=MAX_HEADER_LEN)
            {
               char tmp[MAX_HEADER_LEN+1];
               memcpy(tmp,pMark,lengthOfLine);
               tmp[lengthOfLine]=0;
               printf("\tgreeting-line: '%s'\n",tmp); //FIXME: this is just a debug printout, can be removed
            }
         }
      }
      else
      {
         break;
      }
   }
   *parseLen = (uint32_t) (pNext-pBegin);
   return 0;
}

/**
 * a message consists of a message length (first 1 or 4 bytes) packed as binary integer (big endian).
 * Then follows the message data followed by a new message length header etc.
 * Returns 0 on parse success, -1 on parse failure.
 */
static int8_t apx_clientConnection_parseMessage(apx_clientConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   uint32_t msgLen;
   const uint8_t *pBegin = dataBuf;
   const uint8_t *pResult;
   const uint8_t *pEnd = dataBuf+dataLen;
   const uint8_t *pNext = pBegin;
   pResult = headerutil_numDecode32(pNext, pEnd, &msgLen);
   if (pResult>pNext)
   {
      uint32_t headerLen = (uint32_t) (pResult-pNext);
      pNext = pResult;
      //TODO: implement sanity check for too long messages? (by checking value of msgLen here)
      if (pNext+msgLen<=pEnd)
      {
         if (parseLen != 0)
         {
            *parseLen=headerLen+msgLen;
         }
         printf("\tmessage %d+%d bytes\n",headerLen,msgLen); ///FIXME: remove debug print
         apx_fileManager_parseMessage(&self->fileManager, pNext, msgLen);
         pNext+=msgLen;
      }
      else
      {
         //we have to wait until entire message is in the buffer
      }
   }
   else
   {
      //there is not enough bytes in buffer to parse header
   }
   return 0;
}

/**
 * callback for fileManager when it requests a send buffer
 * returns pointer to allocated buffer, or NULL on failure
 */
static uint8_t *apx_clientConnection_getSendBuffer(void *arg, int32_t msgLen)
{
   apx_clientConnection_t *self = (apx_clientConnection_t*) arg;
   if ( (self != 0) && (msgLen>0) )
   {
      int8_t result=0;
      int32_t requestedLen;
      //create a buffer where we have room to encode the message header (the length of the message) in addition to the user requested length
      int32_t currentLen = adt_bytearray_length(&self->sendBuffer);
      requestedLen = msgLen + self->maxMsgHeaderSize;
      if (currentLen<requestedLen)
      {
         result = adt_bytearray_resize(&self->sendBuffer, (uint32_t) requestedLen);
      }
      if (result == 0)
      {
         uint8_t *data = adt_bytearray_data(&self->sendBuffer);
         assert(data != 0);
         return &data[self->maxMsgHeaderSize]; //return a pointer directly after the message header size.
      }
   }
   return (uint8_t*) 0;
}

/**
 * callback for fileManager when it requests to send buffer (which it previously retreived by  apx_serverConnection_getSendBuffer
 */
static int32_t apx_clientConnection_send(void *arg, int32_t offset, int32_t msgLen)
{
   apx_clientConnection_t *self = (apx_clientConnection_t*) arg;
   if ( (self != 0) && (offset>=0) && (msgLen>=0) )
   {
      int32_t sendBufferLen;
      uint8_t *sendBuffer = adt_bytearray_data(&self->sendBuffer);
      sendBufferLen = adt_bytearray_length(&self->sendBuffer);
      if ((sendBuffer != 0) && (msgLen+self->maxMsgHeaderSize<=sendBufferLen) )
      {
         uint8_t header[sizeof(uint32_t)];
         uint8_t headerLen;
         uint8_t *headerEnd;
         uint8_t *pBegin;
         if (self->maxMsgHeaderSize == (uint8_t) sizeof(uint32_t))
         {
            headerEnd = headerutil_numEncode32(header, (uint32_t) sizeof(header), msgLen);
            if (headerEnd>header)
            {
               headerLen=headerEnd-header;
            }
            else
            {
               assert(0);
               return -1; //header buffer too small
            }
         }
         else
         {
            return -1; //not yet implemented
         }
         //place header just before user data begin
         pBegin = sendBuffer+(self->maxMsgHeaderSize+offset-headerLen); //the part in the parenthesis is where the user data begins
         memcpy(pBegin, header, headerLen);
         printf("sending %d bytes\n",msgLen+headerLen);
         msocket_send(self->msocket, pBegin, msgLen+headerLen);
         return 0;
      }
      else
      {
         assert(0);
      }
   }
   return -1;
}
