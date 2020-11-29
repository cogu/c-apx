/*****************************************************************************
* \file      apx_clientConnectionBase.c
* \author    Conny Gustafsson
* \date      2018-12-31
* \brief     Base class for client connections
*
* Copyright (c) 2018-2019 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h> //debug only
#include "apx_clientConnectionBase.h"
#include "bstr.h"
#include "numheader.h"
#include "apx_logging.h"
#include "apx_file.h"
#include "rmf.h"
#include "apx_clientInternal.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#else
#define vfree free
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_HEADER_LEN 128
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_clientConnectionBase_parseMessage(apx_clientConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void apx_clientConnectionBase_sendGreeting(apx_clientConnectionBase_t *self);
static void apx_clientConnectionBase_fileInfoNotifyImpl(void *arg, const apx_fileInfo_t *fileInfo);
static void apx_clientConnectionBase_nodeInstanceFileWriteNotify(apx_clientConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);
static void apx_clientConnectionBase_vnodeInstanceFileWriteNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);
static void apx_clientConnectionBase_nodeInstanceFileOpenNotify(apx_clientConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType);
static void apx_clientConnectionBase_vnodeInstanceFileOpenNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType);
static apx_error_t apx_clientConnectionBase_processNewRequirePortDataFile(apx_clientConnectionBase_t *self, const apx_fileInfo_t *fileInfo);
static apx_error_t apx_clientConnectionBase_requirePortDataWriteNotify(apx_clientConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, uint32_t offset, const uint8_t *data, uint32_t len);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientConnectionBase_create(apx_clientConnectionBase_t *self, apx_connectionBaseVTable_t *vtable)
{
   if (self != 0)
   {
      apx_error_t errorCode;
      //init non-overridable virtual functions
      vtable->fileInfoNotify = apx_clientConnectionBase_fileInfoNotifyImpl;
      vtable->nodeFileWriteNotify = apx_clientConnectionBase_vnodeInstanceFileWriteNotify;
      vtable->nodeFileOpenNotify = apx_clientConnectionBase_vnodeInstanceFileOpenNotify;
      errorCode = apx_connectionBase_create(&self->base, APX_CLIENT_MODE, vtable);
      self->isAcknowledgeSeen = false;
      self->client = (apx_client_t*) 0;
      apx_connectionBase_setEventHandler(&self->base, apx_clientConnectionBase_defaultEventHandler, (void*) self);
      return errorCode;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_clientConnectionBase_destroy(apx_clientConnectionBase_t *self)
{
   if (self != 0)
   {
      apx_connectionBase_destroy(&self->base);
   }
}

apx_fileManager_t *apx_clientConnectionBase_getFileManager(apx_clientConnectionBase_t *self)
{
   if (self != 0)
   {
      return &self->base.fileManager;
   }
   return (apx_fileManager_t*) 0;
}

void apx_clientConnectionBase_connectedCbk(apx_clientConnectionBase_t *self)
{
   apx_event_t event;
   self->isAcknowledgeSeen = false;
   apx_clientConnectionBase_sendGreeting(self);
   apx_event_create_clientConnected(&event, self);
   apx_eventLoop_append(&self->base.eventLoop, &event);
}

void apx_clientConnectionBase_disconnectedCbk(apx_clientConnectionBase_t *self)
{
   apx_event_t event;
   self->isAcknowledgeSeen = false;
   apx_event_create_clientDisconnected(&event, self);
   apx_eventLoop_append(&self->base.eventLoop, &event);
}

int8_t apx_clientConnectionBase_onDataReceived(apx_clientConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   if ( (self != 0) && (dataBuf != 0) && (parseLen != 0) )
   {
      uint32_t totalParseLen = 0;
      uint32_t remain = dataLen;
      const uint8_t *pNext = dataBuf;
      self->base.totalBytesReceived+=dataLen;
      while(totalParseLen<dataLen)
      {
         uint32_t internalParseLen = 0;
         apx_error_t errorCode;
         errorCode = apx_clientConnectionBase_parseMessage(self, pNext, remain, &internalParseLen);
         //check parse result
         if (errorCode == APX_NO_ERROR)
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
            //TODO: deal with errorCode here
            return -1;
         }
      }
      //no more complete messages can be parsed. There may be a partial message left in buffer, but we ignore it until more data has been recevied.
      //printf("\ttotalParseLen=%d\n", totalParseLen);
      *parseLen = totalParseLen;
      return 0;
   }
   return -1;
}

void apx_clientConnectionBase_start(apx_clientConnectionBase_t *self)
{
   if ( self != 0)
   {
      apx_fileManager_start(&self->base.fileManager);
   }
}

void apx_clientConnectionBase_defaultEventHandler(void *arg, apx_event_t *event)
{
   apx_clientConnectionBase_t *self = (apx_clientConnectionBase_t*) arg;
   if (self != 0)
   {
      apx_connectionBase_t *baseConnection;
      switch(event->evType)
      {
      case APX_EVENT_RMF_HEADER_ACCEPTED:
         baseConnection = (apx_connectionBase_t*) event->evData1;
         apx_connectionBase_onHeaderAccepted(&self->base, baseConnection);
         break;
      case APX_EVENT_CLIENT_CONNECTED:
         apx_clientInternal_onConnect(self->client, self);
         break;
      case APX_EVENT_CLIENT_DISCONNECTED:
         apx_clientInternal_onDisconnect(self->client, self);
         break;
      case APX_EVENT_NODE_COMPLETE:
//         nodeData = (apx_nodeData_t*) event->evData1;
//         apx_clientInternal_onNodeComplete(self->client, nodeData);
         break;
      default:
         apx_connectionBase_defaultEventHandler(&self->base, event);
      }
   }
}

void apx_clientConnectionBase_setConnectionId(apx_clientConnectionBase_t *self, uint32_t connectionId)
{
   if (self != 0)
   {
      apx_connectionBase_setConnectionId(&self->base, connectionId);
   }
}

uint32_t apx_clientConnectionBase_getConnectionId(apx_clientConnectionBase_t *self)
{
   if (self != 0)
   {
      return apx_connectionBase_getConnectionId(&self->base);
   }
   return 0;
}

void apx_clientConnectionBase_close(apx_clientConnectionBase_t *self)
{
   if (self != 0)
   {
      apx_connectionBase_close(&self->base);
   }
}

uint32_t apx_clientConnectionBase_getTotalBytesReceived(apx_clientConnectionBase_t *self)
{
   if (self != 0)
   {
      return self->base.totalBytesReceived;
   }
   return 0;
}

uint32_t apx_clientConnectionBase_getTotalBytesSent(apx_clientConnectionBase_t *self)
{
   if (self != 0)
   {
      return self->base.totalBytesSent;
   }
   return 0;
}

void* apx_clientConnectionBase_registerEventListener(apx_clientConnectionBase_t *self, apx_connectionEventListener_t *listener)
{
   if (self != 0)
   {
      return apx_connectionBase_registerEventListener(&self->base, listener);
   }
   return (void*) 0;
}

void apx_clientConnectionBase_unregisterEventListener(apx_clientConnectionBase_t *self, void *handle)
{
   if (self != 0)
   {
      apx_connectionBase_unregisterEventListener(&self->base, handle);
   }
}

void apx_clientConnectionBase_attachNodeInstance(apx_clientConnectionBase_t *self, struct apx_nodeInstance_tag *nodeInstance)
{
   if (self != 0)
   {
#if APX_DEBUG_ENABLE
      assert(apx_nodeInstance_getName(nodeInstance) != 0);
      printf("[CLIENT-CONNECTION] Attaching %s\n", apx_nodeInstance_getName(nodeInstance));
#endif
      apx_connectionBase_attachNodeInstance(&self->base, nodeInstance);
   }
}

//Internal API

void apx_clientConnectionBaseInternal_headerAccepted(apx_clientConnectionBase_t *self)
{
   if (self != 0)
   {
#if APX_DEBUG_ENABLE
      printf("[CLIENT-CONNECTION] Header accepted\n");
#endif
      self->isAcknowledgeSeen = true;
      apx_fileManager_headerAccepted(&self->base.fileManager);
      apx_connectionBase_emitHeaderAccepted(&self->base);
   }
}

apx_error_t apx_clientConnectionBase_fileOpenNotify(apx_clientConnectionBase_t *self, const rmf_cmdOpenFile_t *openFileCmd)
{
   if ( (self != 0) && (openFileCmd != 0))
   {
      return apx_connectionBase_fileOpenNotify(&self->base, openFileCmd->address);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_clientConnectionBase_fileInfoNotify(apx_clientConnectionBase_t *self, const rmf_fileInfo_t *remoteFileInfo)
{
   if ( (self != 0) && (remoteFileInfo != 0))
   {
      return apx_connectionBase_fileInfoNotify(&self->base, remoteFileInfo);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

#ifdef UNIT_TEST
void apx_clientConnectionBase_run(apx_clientConnectionBase_t *self)
{
   if (self != 0)
   {
      apx_connectionBase_runAll(&self->base);
      apx_fileManager_run(&self->base.fileManager);
   }
}
#endif
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_clientConnectionBase_parseMessage(apx_clientConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   uint32_t msgLen;
   const uint8_t *pBegin = dataBuf;
   const uint8_t *pResult;
   const uint8_t *pEnd = dataBuf+dataLen;
   const uint8_t *pNext = pBegin;

   pResult = numheader_decode32(pNext, pEnd, &msgLen);
   if (pResult>pNext)
   {
      uint32_t headerLen = (uint32_t) (pResult-pNext);
      if (msgLen > APX_MAX_FILE_SIZE)
      {
         return APX_MSG_TOO_LARGE_ERROR;
      }
      pNext = pResult;
      if (pNext+msgLen<=pEnd)
      {
         if (parseLen != 0)
         {
            *parseLen=headerLen+msgLen;
         }
         if (self->isAcknowledgeSeen == false)
         {
            if (msgLen == 8)
            {
               if ( (pNext[0] == 0xbf) &&
                    (pNext[1] == 0xff) &&
                    (pNext[2] == 0xfc) &&
                    (pNext[3] == 0x00) &&
                    (pNext[4] == 0x00) &&
                    (pNext[5] == 0x00) &&
                    (pNext[6] == 0x00) &&
                    (pNext[7] == 0x00) )
               {
                  apx_clientConnectionBaseInternal_headerAccepted(self);
               }
            }
         }
         else
         {
            //printf("processing %d bytes\n", msgLen);
            apx_error_t processResult = apx_connectionBase_processMessage(&self->base, pNext, msgLen);
            if (processResult != APX_NO_ERROR)
            {
               printf("[CLIENT-CONNECTION] Processing message failed with: %d\n", (int) processResult);
            }
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
   return APX_NO_ERROR;
}


static void apx_clientConnectionBase_sendGreeting(apx_clientConnectionBase_t *self)
{
   uint8_t *sendBuffer;
   uint32_t greetingLen;
   apx_transmitHandler_t transmitHandler;
   int numheaderFormat = 32;
   char greeting[RMF_GREETING_MAX_LEN];
   char *p = &greeting[0];
   strcpy(greeting, RMF_GREETING_START);
   p += strlen(greeting);
   p += sprintf(p, "%s%d\n\n", RMF_NUMHEADER_FORMAT_HDR, numheaderFormat);
   greetingLen = (uint32_t) (p-greeting);
   apx_connectionBase_getTransmitHandler(&self->base, &transmitHandler);
   if ( (transmitHandler.getSendBuffer != 0) && (transmitHandler.send != 0) )
   {
      sendBuffer = transmitHandler.getSendBuffer((void*) self, greetingLen);
      if (sendBuffer != 0)
      {
         memcpy(sendBuffer, greeting, greetingLen);
         transmitHandler.send((void*) self, 0, greetingLen);
      }
      else
      {
         fprintf(stderr, "Failed to acquire sendBuffer while trying to send greeting\n");
      }
   }
}

static void apx_clientConnectionBase_fileInfoNotifyImpl(void *arg, const apx_fileInfo_t *fileInfo)
{
   apx_clientConnectionBase_t *self = (apx_clientConnectionBase_t*) arg;
   if (self != 0)
   {
      if (apx_fileInfo_nameEndsWith(fileInfo, APX_INDATA_FILE_EXT) )
      {
         apx_clientConnectionBase_processNewRequirePortDataFile(self, fileInfo);
      }
      else
      {
         //ignore
      }
   }
}

static void apx_clientConnectionBase_nodeInstanceFileWriteNotify(apx_clientConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len)
{
   if ( (self != 0) && (nodeInstance != 0) && (data != 0) )
   {
      if (fileType == APX_INDATA_FILE_TYPE)
      {
         apx_error_t rc = apx_clientConnectionBase_requirePortDataWriteNotify(self, nodeInstance, offset, data, len);
         if (rc != APX_NO_ERROR)
         {
            printf("[CLIENT-CONNECTION-BASE] requirePortDataWriteNotify failed with error %d\n", rc);
         }
      }
      else
      {
         //Ignore
      }
   }
}

static void apx_clientConnectionBase_vnodeInstanceFileWriteNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len)
{
   apx_clientConnectionBase_nodeInstanceFileWriteNotify((apx_clientConnectionBase_t*) arg, nodeInstance, fileType, offset, data, len);
}

static void apx_clientConnectionBase_nodeInstanceFileOpenNotify(apx_clientConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      if (fileType == APX_OUTDATA_FILE_TYPE)
      {
         printf("[CLIENT-CONNECTION-BASE] OUTDATA opened\n");
      }
   }
}

static void apx_clientConnectionBase_vnodeInstanceFileOpenNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType)
{
   apx_clientConnectionBase_nodeInstanceFileOpenNotify((apx_clientConnectionBase_t*) arg, nodeInstance, fileType);
}

static apx_error_t apx_clientConnectionBase_processNewRequirePortDataFile(apx_clientConnectionBase_t *self, const apx_fileInfo_t *fileInfo)
{
   char baseNameBuf[RMF_MAX_FILE_NAME+1];
   apx_nodeInstance_t *nodeInstance;
   apx_fileInfo_copyBaseName(fileInfo, baseNameBuf, RMF_MAX_FILE_NAME);
   nodeInstance = apx_nodeManager_find(&self->base.nodeManager, baseNameBuf);
   if (nodeInstance != 0)
   {
      assert(apx_nodeInstance_getRequirePortDataState(nodeInstance) == APX_REQUIRE_PORT_DATA_STATE_WAITING_FILE_INFO);
      //Search for file in fileManager
      apx_file_t *file = apx_fileManager_findFileByAddress(&self->base.fileManager, fileInfo->address);
      if (file != 0)
      {
         apx_nodeInstance_registerRequirePortFileHandler(nodeInstance, file);
         apx_nodeInstance_setRequirePortDataState(nodeInstance, APX_REQUIRE_PORT_DATA_STATE_WAITING_FOR_FILE_DATA);
         return apx_fileManager_requestOpenFile(&self->base.fileManager, fileInfo->address);
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_clientConnectionBase_requirePortDataWriteNotify(apx_clientConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, uint32_t offset, const uint8_t *data, uint32_t len)
{
   apx_error_t rc;
   assert(self != 0);
   assert(nodeInstance != 0);
   rc = apx_nodeInstance_writeRequirePortData(nodeInstance, data, offset, len);
   if (rc != APX_NO_ERROR)
   {
      return rc;
   }
   if (self->client != 0)
   {
      apx_clientInternal_requirePortDataWriteNotify(self->client, self, nodeInstance, offset, data, len);
   }
   return APX_NO_ERROR;
}
