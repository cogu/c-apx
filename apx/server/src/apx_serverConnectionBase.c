/*****************************************************************************
* \file      apx_srvBaseConnection.c
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Description
*
* Copyright (c) 2018 Conny Gustafsson
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
#include "apx_serverConnectionBase.h"
#include "bstr.h"
#include "numheader.h"
#include "apx_fileManager2.h"
#include "apx_eventListener2.h"
#include "apx_logging.h"
#include "apx_file2.h"
#include "apx_nodeData2.h"
#include "rmf.h"
#include "apx_server.h"
#include "apx_routingTable.h"
#include "apx_portConnectionTable.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_HEADER_LEN 128

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_serverConnectionBase_parseGreeting(apx_serverConnectionBase_t *self, const uint8_t *msgBuf, int32_t msgLen);
static uint8_t apx_serverConnectionBase_parseMessage(apx_serverConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void apx_serverConnectionBase_onFileCreated(void *arg, const apx_fileInfo_t *fileInfo);
static void apx_serverConnectionBase_onFileOpen(void *arg, apx_fileManager2_t *fileManager, const apx_fileInfo_t *fileInfo);
static void apx_serverConnectionBase_onDefinitionDataWritten(void *arg, apx_nodeData2_t *nodeData, uint32_t offset, uint32_t len);
static void apx_serverConnectionBase_onOutPortDataWritten(void *arg, apx_nodeData2_t *nodeData, uint32_t offset, uint32_t len);
static apx_error_t apx_serverConnectionBase_processNewDefinitionFile(apx_serverConnectionBase_t *self, const apx_fileInfo_t *fileInfo);
static void apx_serverConnectionBase_processOutPortDataFile(apx_serverConnectionBase_t *self, const apx_fileInfo_t *fileInfo);
static apx_error_t apx_serverConnectionBase_createInPortDataFile(apx_serverConnectionBase_t *self, apx_nodeData2_t *nodeData, apx_file2_t *definitionFile);
static void apx_serverConnectionBase_routeDataFromProvidePort(apx_serverConnectionBase_t *self, apx_nodeData2_t *srcNodeData, uint32_t offset, uint32_t len);


//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_serverConnectionBase_create(apx_serverConnectionBase_t *self, apx_connectionBaseVTable_t *vtable)
{
   if (self != 0)
   {
      apx_error_t result;

      //init non-overridable virtual functions
      vtable->onFileCreated = apx_serverConnectionBase_onFileCreated;

      result = apx_connectionBase_create(&self->base, APX_SERVER_MODE, vtable);
      self->server = (apx_server_t*) 0;
      self->isGreetingParsed = false;
      self->isActive = true;
      apx_connectionBase_setEventHandler(&self->base, apx_serverConnectionBase_defaultEventHandler, (void*) self);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_serverConnectionBase_destroy(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      apx_connectionBase_destroy(&self->base);
   }
}

apx_fileManager2_t *apx_serverConnectionBase_getFileManager(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      return &self->base.fileManager;
   }
   return (apx_fileManager2_t*) 0;
}

int8_t apx_serverConnectionBase_dataReceived(apx_serverConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   if ( (self != 0) && (dataBuf != 0) && (parseLen != 0) )
   {
      uint32_t totalParseLen = 0;
      uint32_t remain = dataLen;
      const uint8_t *pNext = dataBuf;
      self->base.totalBytesReceived+=dataLen;
      //printf("total received: %d\n", self->base.totalBytesReceived);
      while(totalParseLen<dataLen)
      {
         uint32_t internalParseLen = 0;
         uint8_t result;
         result = apx_serverConnectionBase_parseMessage(self, pNext, remain, &internalParseLen);
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

void apx_serverConnectionBase_start(apx_serverConnectionBase_t *self)
{
   if ( self != 0)
   {
      apx_fileManager2_start(&self->base.fileManager);
   }
}

void apx_serverConnectionBase_defaultEventHandler(void *arg, apx_event_t *event)
{
   apx_serverConnectionBase_t *self = (apx_serverConnectionBase_t*) arg;
   if (self != 0)
   {
      apx_nodeData2_t *nodeData;
      apx_portConnectionTable_t *portConnectionTable;
      //apx_portDataMap_t *portDataMap;
      apx_connectionBase_t *baseConnection;
      apx_fileInfo_t *fileInfo;
      void *caller;
      bool isRemoteFile = false;
      switch(event->evType)
      {
      case APX_EVENT_RMF_HEADER_ACCEPTED:
         baseConnection = (apx_connectionBase_t*) event->evData1;
         apx_connectionBase_onHeaderAccepted(&self->base, baseConnection);
         break;
      case APX_EVENT_FILE_CREATED:
         baseConnection = (apx_connectionBase_t*) event->evData1;
         fileInfo = (apx_fileInfo_t*) event->evData2;
         caller = event->evData3;
         apx_connectionBase_onFileCreated(&self->base, baseConnection, fileInfo, caller);
         if (fileInfo != 0)
         {
            apx_fileInfo_delete(fileInfo);
         }
         break;
/*
      case APX_EVENT_REQUIRE_PORT_CONNECT:
         nodeData = (apx_nodeData2_t*) event->evData1;
         portConnectionTable = (apx_portConnectionTable_t*) event->evData2;
         apx_connectionBase_triggerRequirePortsConnected(&self->base, nodeData, portConnectionTable);
         break;
      case APX_EVENT_PROVIDE_PORT_CONNECT:
         nodeData = (apx_nodeData2_t*) event->evData1;
         portConnectionTable = (apx_portConnectionTable_t*) event->evData2;
         portDataMap = apx_nodeData2_getPortDataMap(nodeData);
         apx_portDataMap_updatePortTriggerList(portDataMap, portConnectionTable);
         apx_connectionBase_triggerProvidePortsConnected(&self->base, nodeData, portConnectionTable);
         break;
      case APX_EVENT_REQUIRE_PORT_DISCONNECT:
         nodeData = (apx_nodeData2_t*) event->evData1;
         portConnectionTable = (apx_portConnectionTable_t*) event->evData2;
         apx_connectionBase_triggerRequirePortsDisconnected(&self->base, nodeData, portConnectionTable);
         break;
      case APX_EVENT_PROVIDE_PORT_DISCONNECT:
         nodeData = (apx_nodeData2_t*) event->evData1;
         portConnectionTable = (apx_portConnectionTable_t*) event->evData2;
         portDataMap = apx_nodeData2_getPortDataMap(nodeData);
         apx_portDataMap_updatePortTriggerList(portDataMap, portConnectionTable);
         apx_connectionBase_triggerProvidePortsDisconnected(&self->base, nodeData, portConnectionTable);
         break;
*/
      }
      apx_connectionBase_defaultEventHandler(&self->base, event);
   }
}

void apx_serverConnectionBase_setConnectionId(apx_serverConnectionBase_t *self, uint32_t connectionId)
{
   if (self != 0)
   {
      return apx_connectionBase_setConnectionId(&self->base, connectionId);
   }
}

uint32_t apx_serverConnectionBase_getConnectionId(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      return apx_connectionBase_getConnectionId(&self->base);
   }
   return 0;
}

void apx_serverConnectionBase_setServer(apx_serverConnectionBase_t *self, struct apx_server_tag *server)
{
   if ( (self != 0) && (server != 0) )
   {
      self->server = server;
   }
}

void apx_serverConnectionBase_close(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      apx_connectionBase_close(&self->base);
   }
}

void apx_serverConnectionBase_detachNodes(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
#if 0
      apx_routingTable_t *routingTable;
      routingTable = apx_server_getRoutingTable(self->server);
      if (routingTable != 0)
      {

         adt_ary_t *nodeDataList = adt_ary_new(NULL);
         if (nodeDataList != 0)
         {
            int32_t numNodes;
            int32_t i;
            numNodes = apx_nodeManager_values(&self->base.nodeManager, nodeDataList);
            for(i=0;i<numNodes;i++)
            {
               apx_nodeData2_t *nodeData = (apx_nodeData2_t*) adt_ary_value(nodeDataList, i);
               apx_routingTable_detachNodeData(routingTable, nodeData);
            }
            adt_ary_delete(nodeDataList);
         }
      }
#endif
   }
}

uint32_t apx_serverConnectionBase_getTotalPortReferences(apx_serverConnectionBase_t *self)
{
#if 0
   if (self != 0)
   {
      int32_t numNodes;
      int32_t i;
      uint32_t tortReferences = 0;
      adt_ary_t *nodeDataList = adt_ary_new(NULL);
      numNodes = apx_nodeManager_values(&self->base.nodeManager, nodeDataList);
      for(i=0; i<numNodes; i++)
      {
         apx_nodeData2_t *nodeData = (apx_nodeData2_t*) adt_ary_value(nodeDataList, i);
         tortReferences+= apx_nodeData2_getPortConnectionsTotal(nodeData);
      }
      adt_ary_delete(nodeDataList);
      return tortReferences;
   }
#endif
   return 0;
}

void* apx_serverConnectionBase_registerEventListener(apx_serverConnectionBase_t *self, apx_connectionEventListener_t *listener)
{
   if (self != 0)
   {
      return apx_connectionBase_registerEventListener(&self->base, listener);
   }
   return (void*) 0;
}

void apx_serverConnectionBase_unregisterEventListener(apx_serverConnectionBase_t *self, void *handle)
{
   apx_connectionBase_unregisterEventListener(&self->base, handle);
}

/*** Internal API (used only internally and by test classes) ***/

void apx_serverConnectionBase_onRemoteFileHeaderReceived(apx_serverConnectionBase_t *self)
{
   self->isGreetingParsed = true;
   apx_fileManager2_headerReceived(&self->base.fileManager);
   apx_connectionBase_emitHeaderAccepted(&self->base);
}

apx_error_t apx_serverConnectionBase_onFileInfoMsgReceived(apx_serverConnectionBase_t *self, const rmf_fileInfo_t *remoteFileInfo)
{
   if ( (self != 0) && (remoteFileInfo != 0))
   {
      return apx_connectionBase_onFileInfoMsgReceived(&self->base, remoteFileInfo);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/*** UNIT TEST API ***/

#ifdef UNIT_TEST
void apx_serverConnectionBase_run(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      apx_connectionBase_runAll(&self->base);
      apx_fileManager2_run(&self->base.fileManager);
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_serverConnectionBase_parseGreeting(apx_serverConnectionBase_t *self, const uint8_t *msgBuf, int32_t msgLen)
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
            apx_serverConnectionBase_onRemoteFileHeaderReceived(self);
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
static uint8_t apx_serverConnectionBase_parseMessage(apx_serverConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   uint32_t totalParsed=0;
   uint32_t msgLen;
   const uint8_t *pBegin = dataBuf;
   const uint8_t *pResult;
   const uint8_t *pEnd = dataBuf+dataLen;
   const uint8_t *pNext = pBegin;
   ///TODO: support both 16-bit and 32-bit length header
   pResult = numheader_decode32(pNext, pEnd, &msgLen);
   if (pResult>pNext)
   {
      uint32_t headerLen = (uint32_t) (pResult-pNext);
      pNext+=headerLen;
      if (pNext+msgLen<=pEnd)
      {
         totalParsed+=headerLen+msgLen;
         if (self->isGreetingParsed == false)
         {
            apx_serverConnectionBase_parseGreeting(self, pNext, msgLen);
         }
         else
         {
            apx_error_t processResult = apx_connectionBase_processMessage(&self->base, pNext, msgLen);
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



static void apx_serverConnectionBase_onFileCreated(void *arg, const apx_fileInfo_t *fileInfo)
{
   apx_serverConnectionBase_t *self = (apx_serverConnectionBase_t*) arg;
   //printf("[SERVER CONNECTION] file created: %s\n", fileInfo->name);
   if (self != 0)
   {
      if (apx_fileInfo_nameEndsWith(fileInfo, APX_DEFINITION_FILE_EXT) )
      {
         apx_serverConnectionBase_processNewDefinitionFile(self, fileInfo);
      }
      else if (apx_fileInfo_nameEndsWith(fileInfo, APX_OUTDATA_FILE_EXT) )
      {
         apx_serverConnectionBase_processOutPortDataFile(self, fileInfo);
      }
      else
      {
         //ignore
      }
   }
}

static void apx_serverConnectionBase_onFileOpen(void *arg, apx_fileManager2_t *fileManager, const apx_fileInfo_t *fileInfo)
{
   apx_serverConnectionBase_t *self = (apx_serverConnectionBase_t*) arg;
   if (self != 0)
   {
/*
      const char *basename = apx_file2_basename(file);
      apx_nodeData2_t *nodeData = apx_nodeManager_find(&self->base.nodeManager, basename);
      if (nodeData != 0)
      {
         bool isComplete = apx_nodeData2_isComplete(nodeData);
         if (isComplete == true)
         {

         }
      }
*/
   }
}


static void apx_serverConnectionBase_onDefinitionDataWritten(void *arg, apx_nodeData2_t *nodeData, uint32_t offset, uint32_t len)
{
   apx_serverConnectionBase_t *self = (apx_serverConnectionBase_t*) arg;
   if (self != 0)
   {
/*
      apx_error_t result = apx_nodeManager_parseDefinition(&self->base.nodeManager, nodeData);
      if (result == APX_NO_ERROR)
      {
         result = apx_nodeData2_createPortDataBuffers(nodeData);
         if (result == APX_NO_ERROR)
         {
            apx_file2_t *definitionFile;
            definitionFile = apx_nodeData2_getDefinitionFile(nodeData);
            if ( definitionFile != 0 )
            {
               if (strlen(definitionFile->fileInfo.name)<=RMF_MAX_FILE_NAME)
               {
                  result = apx_nodeData2_createPortDataMap(nodeData, APX_SERVER_MODE);
                  if (result == APX_NO_ERROR)
                  {
                     uint32_t inPortDataLen;
                     apx_routingTable_attachNodeData(&self->server->routingTable, nodeData);
                     inPortDataLen = apx_nodeData2_getInPortDataLen(nodeData);
                     if (inPortDataLen > 0)
                     {
                        result = apx_serverConnectionBase_createInPortDataFile(self, nodeData, definitionFile);
                        if (result == APX_NO_ERROR)
                        {
                           apx_routingTable_copyInitData(&self->server->routingTable, nodeData);
                           apx_fileManager2_attachLocalFile(&self->base.fileManager, apx_nodeData2_getInPortDataFile(nodeData), (void*) self);
                        }
                     }
                     if (result == APX_NO_ERROR)
                     {
                        uint32_t outPortDataLen = apx_nodeData2_getOutPortDataLen(nodeData);
                        if (outPortDataLen > 0)
                        {
                           char fileName[RMF_MAX_FILE_NAME+1];
                           strcpy(fileName, apx_file2_basename(definitionFile));
                           strcat(fileName, APX_OUTDATA_FILE_EXT);
                           apx_file2_t *outPortDataFile = apx_fileManager2_findRemoteFileByName(&self->base.fileManager, fileName);
                           if (outPortDataFile != 0)
                           {
                              //TODO: Verify file length here
                              apx_nodeData2_setOutPortDataFile(nodeData, outPortDataFile);
                              apx_fileManager2_openRemoteFile(&self->base.fileManager, outPortDataFile->fileInfo.address, self);
                           }
                        }
                     }
                  }
               }
               else
               {
                  result = APX_NAME_TOO_LONG_ERROR;
               }
            }
            else
            {
               result = APX_MISSING_FILE_ERROR;
            }
         }
      }
      if (result != APX_NO_ERROR)
      {
         printf("APX error %d\n", (int) result);
         apx_fileManager2_sendApxErrorCode(&self->base.fileManager, (uint32_t) result);
      }
      */
   }
}

static apx_error_t apx_serverConnectionBase_processNewDefinitionFile(apx_serverConnectionBase_t *self, const apx_fileInfo_t *fileInfo)
{
   if ( (fileInfo->fileType == RMF_FILE_TYPE_FIXED) && ( (fileInfo->address & RMF_REMOTE_ADDRESS_BIT) != 0))
   {
      apx_error_t retval = APX_NO_ERROR;
      char *nodeName = apx_fileInfo_getBaseName(fileInfo);
      if (nodeName != 0)
      {
         if (apx_nodeManager_find(&self->base.nodeManager, nodeName) != 0)
         {
            printf("APX node already exits: %s", nodeName);
            //apx_fileManager2_sendFileAlreadyExistsError(&self->base.fileManager, file);
         }
         else
         {
            apx_nodeInstance_t *nodeInstance = apx_nodeManager_createNode(&self->base.nodeManager, nodeName);
            if (nodeInstance != 0)
            {
               apx_nodeData2_t *nodeData;
               apx_nodeInstance_setConnection(nodeInstance, &self->base);
               nodeData = apx_nodeInstance_getNodeData(nodeInstance);
               if (nodeData != 0)
               {
                  retval = apx_nodeInstance_createDefinitionBuffer(nodeInstance, fileInfo->length);
                  if (retval == APX_NO_ERROR)
                  {
                     apx_file2_t *remoteFile = apx_fileManager2_findFileByAddress(&self->base.fileManager, fileInfo->address);
                     if (remoteFile != 0)
                     {
                        apx_nodeInstance_registerDefinitionFileHandler(nodeInstance, remoteFile);
                        retval = apx_fileManager2_requestOpenFile(&self->base.fileManager, fileInfo->address);
                     }
                     else
                     {
                        retval = APX_FILE_NOT_FOUND_ERROR;
                     }
                  }
               }
            }
         }
         free(nodeName);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static void apx_serverConnectionBase_processOutPortDataFile(apx_serverConnectionBase_t *self, const apx_fileInfo_t *fileInfo)
{
/*
   if (file->fileInfo.fileType == RMF_FILE_TYPE_FIXED)
   {

      apx_nodeData2_t *nodeData = apx_nodeManager_find(&self->base.nodeManager, apx_file2_basename(file));
      if ( (nodeData != 0) && (nodeData->outPortDataBuf != 0))
      {
         printf("requesting opening of .out file\n");
      }
   }
   */
}


//Type 2 event trigger
static apx_error_t apx_serverConnectionBase_createInPortDataFile(apx_serverConnectionBase_t *self, apx_nodeData2_t *nodeData, apx_file2_t *definitionFile)
{
   apx_error_t retval = APX_NO_ERROR;
/*
   rmf_fileInfo_t info;
   apx_file2_t *inPortDataFile;
   char fileName[RMF_MAX_FILE_NAME+1];
   uint32_t inPortDataLen = apx_nodeData2_getInPortDataLen(nodeData);
   apx_fileInfo_copyBaseName(&definitionFile->fileInfo, fileName, (uint32_t) sizeof(fileName));
   strcat(fileName, APX_INDATA_FILE_EXT);
   rmf_fileInfo_create(&info, fileName, RMF_INVALID_ADDRESS, inPortDataLen, RMF_FILE_TYPE_FIXED);
   inPortDataFile = apx_file2_newLocal(&info, NULL);
   if (inPortDataFile != 0)
   {
      apx_nodeData2_setInPortDataFile(nodeData, inPortDataFile);
   }
   else
   {
      retval = APX_MEM_ERROR;
   }
*/
   return retval;
}

//Type 1 event
static void apx_serverConnectionBase_onOutPortDataWritten(void *arg, apx_nodeData2_t *nodeData, uint32_t offset, uint32_t len)
{
/*
   apx_serverConnectionBase_t *self = (apx_serverConnectionBase_t*) arg;
   if (self != 0 && nodeData != 0)
   {
      apx_file2_t *file = apx_nodeData2_getOutPortDataFile(nodeData);
      if (file != 0)
      {
         if (apx_file2_isRemoteFile(file))
         {
            apx_serverConnectionBase_routeDataFromProvidePort(self, nodeData, offset, len);
         }
      }
   }
*/
}

static void apx_serverConnectionBase_routeDataFromProvidePort(apx_serverConnectionBase_t *self, apx_nodeData2_t *srcNodeData, uint32_t offset, uint32_t len)
{
/*
   apx_portDataMap_t *srcPortDataMap = apx_nodeData2_getPortDataMap(srcNodeData);
   if (srcPortDataMap != 0)
   {
      apx_portId_t providePortId = apx_portDataMap_findProvidePortIdFromByteOffset(srcPortDataMap, offset);
      if (providePortId != -1)
      {
         const apx_portDataProps_t *srcPortDataProps;
         apx_portTriggerList_t *portTriggerList;
         int32_t numConnections;
         int32_t i;
         srcPortDataProps = apx_portDataMap_getProvidePortDataProps(srcPortDataMap, providePortId);
         portTriggerList = apx_portDataMap_getPortTriggerList(srcPortDataMap, providePortId);
         numConnections = apx_portTriggerList_length(portTriggerList);
         for(i = 0; i < numConnections; i++)
         {
            apx_portDataRef_t *portDataRef = apx_portTriggerList_get(portTriggerList, i);
            if (portDataRef != 0)
            {
               apx_nodeData2_t *destNodeData = portDataRef->nodeData;
               const apx_portDataProps_t *destPortDataProps = portDataRef->portDataProps;
               if (apx_portDataProps_isPlainOldData(srcPortDataProps) )
               {
                  apx_nodeData2_routePortData(destNodeData, destPortDataProps, srcNodeData, srcPortDataProps);
               }
            }
         }
      }
   }
*/
}
