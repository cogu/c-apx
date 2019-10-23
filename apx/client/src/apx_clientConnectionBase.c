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
#include "apx_fileManager.h"
#include "apx_eventListener.h"
#include "apx_logging.h"
#include "apx_file2.h"
#include "rmf.h"
#include "apx_portDataMap.h"
#include "apx_routingTable.h"
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
static void apx_clientConnectionBase_onFileCreate(void *arg, apx_fileManager_t *fileManager, struct apx_file2_tag *file);
static apx_error_t apx_clientConnectionBase_parseMessage(apx_clientConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void apx_clientConnectionBase_processNewInPortDataFile(apx_clientConnectionBase_t *self, struct apx_file2_tag *file);
static void apx_clientConnectionBase_sendGreeting(apx_clientConnectionBase_t *self);
static void apx_clientConnectionBase_registerLocalFiles(apx_clientConnectionBase_t *self);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientConnectionBase_create(apx_clientConnectionBase_t *self, struct apx_client_tag *client, apx_connectionBaseVTable_t *vtable)
{
   if (self != 0)
   {
      apx_error_t errorCode = apx_connectionBase_create(&self->base, APX_CLIENT_MODE, vtable);
      self->client = client;
      self->isAcknowledgeSeen = false;
      if ( (errorCode == APX_NO_ERROR) && (self->client != 0))
      {
         errorCode = apx_clientInternal_attachLocalNodes(self->client, &self->base.nodeDataManager);
         if (errorCode == APX_NO_ERROR)
         {
            apx_clientConnectionBase_registerLocalFiles(self);
         }
      }
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

void apx_clientConnectionBase_onConnected(apx_clientConnectionBase_t *self)
{
   apx_event_t event;
   self->isAcknowledgeSeen = false;
   apx_clientConnectionBase_sendGreeting(self);
   apx_event_create_clientConnected(&event, self);
   apx_eventLoop_append(&self->base.eventLoop, &event);
}

void apx_clientConnectionBase_onDisconnected(apx_clientConnectionBase_t *self)
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
      apx_fileManagerEventListener_t listener;
      memset(&listener, 0, sizeof(listener));
      listener.fileCreate = apx_clientConnectionBase_onFileCreate;
      listener.arg = (void*) self;
      apx_fileManager_start(&self->base.fileManager);
      apx_fileManager_registerEventListener(&self->base.fileManager, &listener);
   }
}

void apx_clientConnectionBase_defaultEventHandler(void *arg, apx_event_t *event)
{
   apx_clientConnectionBase_t *self = (apx_clientConnectionBase_t*) arg;
   if (self != 0)
   {
      apx_nodeData_t *nodeData;
      switch(event->evType)
      {
      case APX_EVENT_CLIENT_CONNECTED:
         apx_clientInternal_onConnect(self->client, self);
         break;
      case APX_EVENT_CLIENT_DISCONNECTED:
         apx_clientInternal_onDisconnect(self->client, self);
         break;
      case APX_EVENT_NODE_COMPLETE:
         nodeData = (apx_nodeData_t*) event->evData1;
         apx_clientInternal_onNodeComplete(self->client, nodeData);
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
      return apx_connectionBase_setConnectionId(&self->base, connectionId);
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

void* apx_clientConnectionBase_registerNodeDataEventListener(apx_clientConnectionBase_t *self, apx_nodeDataEventListener_t *listener)
{
   if (self != 0)
   {
      return apx_connectionBase_registerNodeDataEventListener(&self->base, listener);
   }
   return (void*) 0;
}

void apx_clientConnectionBase_unregisterNodeDataEventListener(apx_clientConnectionBase_t *self, void *handle)
{
   apx_connectionBase_unregisterNodeDataEventListener(&self->base, handle);
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
static void apx_clientConnectionBase_onFileCreate(void *arg, apx_fileManager_t *fileManager, struct apx_file2_tag *file)
{
   apx_clientConnectionBase_t *self = (apx_clientConnectionBase_t*) arg;
   printf("file created: %s\n", file->fileInfo.name);
   if (self != 0)
   {
      if ( strcmp(apx_file2_extension(file), APX_INDATA_FILE_EXT) == 0)
      {
         apx_clientConnectionBase_processNewInPortDataFile(self,  file);
      }
   }
}

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
      if (msgLen > APX_MAX_MESSAGE_SIZE)
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
                  self->isAcknowledgeSeen = true;
                  apx_fileManager_onHeaderAccepted(&self->base.fileManager);
               }
            }
         }
         else
         {
            //printf("processing %d bytes\n", msgLen);
            int32_t result = apx_fileManager_processMessage(&self->base.fileManager, pNext, msgLen);
            if (result != msgLen)
            {
               //TODO: do error handling here
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

static void apx_clientConnectionBase_processNewInPortDataFile(apx_clientConnectionBase_t *self, struct apx_file2_tag *inPortDataFile)
{
   if (inPortDataFile->fileInfo.fileType == RMF_FILE_TYPE_FIXED)
   {
      apx_nodeData_t *nodeData = apx_nodeDataManager_find(&self->base.nodeDataManager, apx_file2_basename(inPortDataFile));
      if ( (nodeData != 0) && (nodeData->inPortDataBuf != 0))
      {
         if (inPortDataFile->fileInfo.length == nodeData->inPortDataLen)
         {
            apx_nodeData_setInPortDataFile(nodeData, inPortDataFile);
            apx_fileManager_openRemoteFile(&self->base.fileManager, inPortDataFile->fileInfo.address, self);
            if (apx_nodeData_isComplete(nodeData))
            {
               apx_connectionBase_emitNodeComplete(&self->base, nodeData);
            }
         }
      }
   }
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

static void apx_clientConnectionBase_registerLocalFiles(apx_clientConnectionBase_t *self)
{
   adt_ary_t nodeNames;
   int32_t numNodes;
   int32_t i;
   adt_ary_create(&nodeNames, vfree);
   numNodes = apx_nodeDataManager_keys(&self->base.nodeDataManager, &nodeNames);

   for(i=0;i<numNodes;i++)
   {
      apx_nodeData_t *nodeData;
      const char *nodeName = (const char*) adt_ary_value(&nodeNames, i);
      nodeData = apx_nodeDataManager_find(&self->base.nodeDataManager, nodeName);
      if (nodeData != 0)
      {
         if (nodeData->definitionDataBuf != 0)
         {
            apx_file2_t *outDataFile;
            apx_file2_t *definitionFile = apx_nodeData_createLocalDefinitionFile(nodeData);
            if (definitionFile != 0)
            {
               apx_fileManager_t *fileManager = &self->base.fileManager;
               apx_nodeData_setFileManager(nodeData, fileManager);
               apx_fileManager_attachLocalFile(fileManager, definitionFile, (void*) self);
            }
            outDataFile  = apx_nodeData_createLocalOutPortDataFile(nodeData);
            if (outDataFile != 0)
            {
               apx_fileManager_t *fileManager = &self->base.fileManager;
               apx_nodeData_setFileManager(nodeData, fileManager);
               apx_fileManager_attachLocalFile(fileManager, outDataFile, (void*) self);
            }
         }
      }
   }
   adt_ary_destroy(&nodeNames);
}
