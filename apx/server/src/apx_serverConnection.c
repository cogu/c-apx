//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifndef APX_DEBUG_ENABLE
#define APX_DEBUG_ENABLE 0
#endif
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "apx_serverConnection.h"
#include "apx_logging.h"
#ifdef UNIT_TEST
#include "apx_testServer.h"
#else
#include "apx_server.h"
#endif
#include "headerutil.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_HEADER_LEN 128
#define SEND_BUFFER_GROW_SIZE 4096 //4KB
#define MAX_DEBUG_BYTES 100
#define MAX_DEBUG_MSG_SIZE 400
#define HEX_DATA_LEN 3u

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_serverConnection_parseGreeting(apx_serverConnection_t *self, const uint8_t *msgBuf, int32_t msgLen);
static uint8_t apx_serverConnection_parseMessage(apx_serverConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static uint8_t *apx_serverConnection_getSendBuffer(void *arg, int32_t msgLen);
static int32_t apx_serverConnection_send(void *arg, int32_t offset, int32_t msgLen);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
int8_t apx_serverConnection_create(apx_serverConnection_t *self, testsocket_t *socket, struct apx_testServer_tag *server)
#else
int8_t apx_serverConnection_create(apx_serverConnection_t *self, msocket_t *socket, struct apx_server_tag *server)
#endif
{
   if( (self != 0) && (socket != 0) )
   {
#ifdef UNIT_TEST
      self->testsocket=socket;
#else
      self->msocket = socket;
#endif
      self->server=server;
      self->isGreetingParsed = false;
      self->debugMode = APX_DEBUG_NONE;
      self->numHeaderMaxLen = (int8_t) sizeof(uint32_t); //currently only 4-byte header is supported. There might be a future version where we support both 16-bit and 32-bit message headers
      adt_bytearray_create(&self->sendBuffer, SEND_BUFFER_GROW_SIZE);
      return apx_fileManager_create(&self->fileManager, APX_FILEMANAGER_SERVER_MODE);
   }
   errno=EINVAL;
   return -1;
}

void apx_serverConnection_destroy(apx_serverConnection_t *self)
{
   if (self != 0)
   {
      apx_fileManager_destroy(&self->fileManager);
      adt_bytearray_destroy(&self->sendBuffer);
#ifdef UNIT_TEST
      testsocket_delete(self->testsocket);
#else
      msocket_delete(self->msocket);
#endif
   }
}

#ifdef UNIT_TEST
apx_serverConnection_t *apx_serverConnection_new(testsocket_t *socket, struct apx_testServer_tag *server)
#else
apx_serverConnection_t *apx_serverConnection_new(msocket_t *socket, struct apx_server_tag *server)
#endif
{
   if (socket != 0)
   {
      apx_serverConnection_t *self = (apx_serverConnection_t*) malloc(sizeof(apx_serverConnection_t));
      if(self != 0){
         int8_t result = apx_serverConnection_create(self, socket, server);
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
   return (apx_serverConnection_t*) 0;
}

void apx_serverConnection_delete(apx_serverConnection_t *self)
{
   if (self != 0)
   {
      apx_serverConnection_destroy(self);
      free(self);
   }
}

void apx_serverConnection_vdelete(void *arg)
{
   apx_serverConnection_delete((apx_serverConnection_t*) arg);
}

/**
 * attaches the servers nodeManager to the fileManager in our connection
 */
void apx_serverConnection_attachNodeManager(apx_serverConnection_t *self, apx_nodeManager_t *nodeManager)
{
   if ( (self != 0) && (nodeManager != 0) )
   {
      apx_nodeManager_attachFileManager(nodeManager, &self->fileManager);
   }
}

/**
 * detaches the servers nodeManager from the fileManager in our connection
 */
void apx_serverConnection_detachNodeManager(apx_serverConnection_t *self, apx_nodeManager_t *nodeManager)
{
   if ( (self != 0) && (nodeManager != 0) )
   {
      apx_nodeManager_shutdownFileManager(nodeManager, &self->fileManager);
   }
}

/**
 * activates a new server connection
 */
void apx_serverConnection_start(apx_serverConnection_t *self)
{
   if ( (self != 0) && (self->server != 0) )
   {
      apx_transmitHandler_t serverTransmitHandler;
      //register transmit handler with our fileManager
      serverTransmitHandler.arg = self;
      serverTransmitHandler.send = apx_serverConnection_send;
      serverTransmitHandler.getSendAvail = 0;
      serverTransmitHandler.getSendBuffer = apx_serverConnection_getSendBuffer;
      apx_fileManager_setTransmitHandler(&self->fileManager, &serverTransmitHandler);
      //register connection with the server nodeManager
      apx_nodeManager_attachFileManager(&self->server->nodeManager, &self->fileManager);
      apx_fileManager_start(&self->fileManager);      
   }
}

/**
 * called from apx_client when data has been received on the msocket
 */
int8_t apx_serverConnection_dataReceived(apx_serverConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   if ( (self != 0) && (dataBuf != 0) && (parseLen != 0) )
   {      
      uint32_t totalParseLen = 0;
      uint32_t remain = dataLen;
      const uint8_t *pNext = dataBuf;
      while(totalParseLen<dataLen)
      {
         uint32_t internalParseLen = 0;
         uint8_t result;         
         result = apx_serverConnection_parseMessage(self, pNext, remain, &internalParseLen);
         //check parse result
         if (result == 0)
         {
            //printf("\tinternalParseLen(%s): %d\n",parseFunc, internalParseLen);
            assert(internalParseLen<=dataLen);
            pNext+=internalParseLen;
            totalParseLen+=internalParseLen;
            remain-=internalParseLen;
            if(internalParseLen == 0)
            {
               break;
            }
         }
         else
         {
            return result;
         }
      }
      //no more complete messages can be parsed. There may be a partial message left in buffer, but we ignore it until more data has been recevied.
      //printf("\ttotalParseLen=%d\n", totalParseLen);
      *parseLen = totalParseLen;
      return 0;
   }
   return -1;
}

void apx_serverConnection_setDebugMode(apx_serverConnection_t *self, int8_t debugMode)
{
   if (self != 0)
   {
      self->debugMode = debugMode;
      if (debugMode > APX_DEBUG_2_LOW)
      {
         apx_fileManager_setDebugInfo(&self->fileManager, (void*) self);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * parses the greeting header. The header is similar to an HTTP header with an initial protocol line followed by one or more MIME-headers.
 * Instead of line ending \r\n we just just \n. The greeting ends when we encountered two consecutive \n\n.
 */
static void apx_serverConnection_parseGreeting(apx_serverConnection_t *self, const uint8_t *msgBuf, int32_t msgLen)
{
   const uint8_t *pNext = msgBuf;
   const uint8_t *pEnd = msgBuf + msgLen;
   while(pNext < pEnd)
   {
      const uint8_t *pResult;
      pResult = bstr_line(pNext, pEnd);
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
            if (self->debugMode > APX_DEBUG_NONE)
            {
               APX_LOG_INFO("[APX_SRV_CONNECTION] (%p) Greeting parsed", (void*) self);
            }
            else
            {
               APX_LOG_INFO("%s", "[APX_SRV_CONNECTION] Greeting parsed");
            }

            apx_fileManager_onConnected(&self->fileManager);
            break;
         }
         else
         {
            //TODO: parse greeting line
            if (lengthOfLine<MAX_HEADER_LEN)
            {
               char tmp[MAX_HEADER_LEN+1];
               memcpy(tmp,pMark,lengthOfLine);
               tmp[lengthOfLine]=0;
               //printf("\tgreeting-line: '%s'\n",tmp);
            }
         }
      }
      else
      {
         break;
      }
   }      
}

/**
 * a message consists of a message length (1 or 4 bytes) packed as binary integer (big endian). Then follows the message data followed by a new message length header etc.
 */
static uint8_t apx_serverConnection_parseMessage(apx_serverConnection_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   uint32_t totalParsed=0;
   uint32_t msgLen;
   const uint8_t *pBegin = dataBuf;
   const uint8_t *pResult;
   const uint8_t *pEnd = dataBuf+dataLen;
   const uint8_t *pNext = pBegin;
   pResult = headerutil_numDecode32(pNext, pEnd, &msgLen);
   if (pResult>pNext)
   {
      uint32_t headerLen = (uint32_t) (pResult-pNext);
      pNext+=headerLen;
      if (pNext+msgLen<=pEnd)
      {
         totalParsed+=headerLen+msgLen;
         if (self->debugMode >= APX_DEBUG_4_HIGH)
         {
            uint32_t i;
            char msg[MAX_DEBUG_MSG_SIZE];
            char *pMsg = &msg[0];
            char *pMsgEnd = pMsg + MAX_DEBUG_MSG_SIZE;
            pMsg += sprintf(msg, "(%p) Received %d+%d bytes:", (void*)self, (int)headerLen, (int)msgLen);            
            for (i = 0; i < MAX_DEBUG_BYTES; i++)
            {
               if ( ( i >= (msgLen + headerLen) ) || ( (pMsg + HEX_DATA_LEN) > pMsgEnd))
               {
                  break;
               }
               pMsg += sprintf(pMsg, " %02X", (int)pBegin[i]);
            }
            APX_LOG_DEBUG("[APX_SRV_CONNECTION] %s", msg);
         }
         
         if (self->isGreetingParsed == false)
         {
            apx_serverConnection_parseGreeting(self, pNext, msgLen);
         }
         else
         {
            apx_fileManager_parseMessage(&self->fileManager, pNext, msgLen);
         }
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
   *parseLen=totalParsed;
   return 0;
}

/**
 * callback for fileManager when it requests a send buffer
 */
static uint8_t *apx_serverConnection_getSendBuffer(void *arg, int32_t msgLen)
{
   apx_serverConnection_t *self = (apx_serverConnection_t*) arg;
   if (self != 0)
   {
      int8_t result=0;
      int32_t requestedLen;
      //create a buffer where we have room to encode the message header (the length of the message) in addition to the user requested length
      int32_t currentLen = adt_bytearray_length(&self->sendBuffer);
      requestedLen= msgLen + self->numHeaderMaxLen;
      if (currentLen<requestedLen)
      {
         result = adt_bytearray_resize(&self->sendBuffer, (uint32_t) requestedLen);
      }
      if (result == 0)
      {
         uint8_t *data = adt_bytearray_data(&self->sendBuffer);
         assert(data != 0);
         return &data[self->numHeaderMaxLen];
      }
   }
   return 0;
}

/**
 * callback for fileManager when it requests to send buffer (which it previously retreived by  apx_serverConnection_getSendBuffer
 */
static int32_t apx_serverConnection_send(void *arg, int32_t offset, int32_t msgLen)
{
   apx_serverConnection_t *self = (apx_serverConnection_t*) arg;
   if ( (self != 0) && (offset>=0) && (msgLen>=0))
   {
      int32_t sendBufferLen;
      uint8_t *sendBuffer = adt_bytearray_data(&self->sendBuffer);
      sendBufferLen = adt_bytearray_length(&self->sendBuffer);
      if ((sendBuffer != 0) && (msgLen+self->numHeaderMaxLen<=sendBufferLen) )
      {
         uint8_t header[sizeof(uint32_t)];
         uint8_t headerLen;
         uint8_t *headerEnd;
         uint8_t *pBegin;
         if (self->numHeaderMaxLen == (uint8_t) sizeof(uint32_t))
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
         pBegin = sendBuffer+(self->numHeaderMaxLen+offset-headerLen); //the part in the parenthesis is where the user data begins
         memcpy(pBegin, header, headerLen);
         if (self->debugMode >= APX_DEBUG_4_HIGH)
         {
            int i;
            char msg[MAX_DEBUG_MSG_SIZE];
            char *pMsg = &msg[0];
            char *pMsgEnd = pMsg + MAX_DEBUG_MSG_SIZE;
            pMsg += sprintf(msg, "(%p) Sending %d+%d bytes:", (void*)self, (int)headerLen, (int)msgLen);
            for (i = 0; i < MAX_DEBUG_BYTES; i++)
            {
               if ((i >= msgLen + headerLen) || ( (pMsg + HEX_DATA_LEN) > pMsgEnd))
               {
                  break;
               }
               pMsg += sprintf(pMsg, " %02X", (int)pBegin[i]);
            }
            APX_LOG_DEBUG("[APX_SRV_CONNECTION] %s", msg);
         }
		 
#ifdef UNIT_TEST
		 testsocket_serverSend(self->testsocket, pBegin, msgLen+headerLen);
#else
		 msocket_send(self->msocket, pBegin, msgLen+headerLen);
#endif
         return 0;
      }
      else
      {
         assert(0);
      }
   }
   return -1;
}
