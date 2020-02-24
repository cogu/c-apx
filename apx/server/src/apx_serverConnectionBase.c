/*****************************************************************************
* \file      apx_srvBaseConnection.c
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Description
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#include "apx_fileManager.h"
#include "apx_eventListener.h"
#include "apx_logging.h"
#include "apx_file.h"
#include "apx_nodeData.h"
#include "rmf.h"
#include "apx_server.h"
#include "apx_portConnectorChangeTable.h"
#include "apx_util.h"
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
static void apx_serverConnectionBase_fileInfoNotifyImpl(void *arg, const apx_fileInfo_t *fileInfo);
static apx_error_t apx_serverConnectionBase_processNewDefinitionFile(apx_serverConnectionBase_t *self, const apx_fileInfo_t *fileInfo);
static void apx_serverConnectionBase_processNewOutPortDataFile(apx_serverConnectionBase_t *self, const apx_fileInfo_t *fileInfo);
static void apx_serverConnectionBase_routeDataFromProvidePort(apx_serverConnectionBase_t *self, apx_nodeData_t *srcNodeData, uint32_t offset, uint32_t len);
static void apx_serverConnectionBase_definitionDataWriteNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, uint32_t offset, uint32_t len);
static apx_error_t apx_serverConnectionBase_providePortDataWriteNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, uint32_t offset, const uint8_t *data, uint32_t len);
static apx_error_t apx_serverConnectionBase_openOutPortDataFileIfExists(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance);
static apx_error_t  apx_serverConnectionBase_createRequirePortDataFileIfNeeded(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance);
static void apx_serverConnectionBase_nodeInstanceFileOpenNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType);
static void apx_serverConnectionBase_vnodeInstanceFileOpenNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType);
static apx_error_t apx_serverConnectionBase_RequirePortDataFileOpenNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance);
static void apx_serverConnectionBase_detachAllNodes(apx_serverConnectionBase_t *self);
static void apx_serverConnectionBase_nodeInstanceFileWriteNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);
static void apx_serverConnectionBase_vnodeInstanceFileWriteNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);

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
      vtable->fileInfoNotify = apx_serverConnectionBase_fileInfoNotifyImpl;
      vtable->nodeFileWriteNotify = apx_serverConnectionBase_vnodeInstanceFileWriteNotify;
      vtable->nodeFileOpenNotify = apx_serverConnectionBase_vnodeInstanceFileOpenNotify;
      result = apx_connectionBase_create(&self->base, APX_SERVER_MODE, vtable);
      self->server = (apx_server_t*) 0;
      self->isGreetingParsed = false;
      self->isActive = false;
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

apx_fileManager_t *apx_serverConnectionBase_getFileManager(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      return &self->base.fileManager;
   }
   return (apx_fileManager_t*) 0;
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
      apx_fileManager_start(&self->base.fileManager);
   }
}

void apx_serverConnectionBase_defaultEventHandler(void *arg, apx_event_t *event)
{
   apx_serverConnectionBase_t *self = (apx_serverConnectionBase_t*) arg;
   if (self != 0)
   {
      //apx_nodeData_t *nodeData;
      //apx_portConnectorChangeTable_t *portConnectionTable;
      //apx_portDataMap_t *portDataMap;
      apx_connectionBase_t *baseConnection;
      apx_fileInfo_t *fileInfo;
      void *caller;
      //bool isRemoteFile = false;
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
         nodeData = (apx_nodeData_t*) event->evData1;
         portConnectionTable = (apx_portConnectionTable_t*) event->evData2;
         apx_connectionBase_triggerRequirePortsConnected(&self->base, nodeData, portConnectionTable);
         break;
      case APX_EVENT_PROVIDE_PORT_CONNECT:
         nodeData = (apx_nodeData_t*) event->evData1;
         portConnectionTable = (apx_portConnectionTable_t*) event->evData2;
         portDataMap = apx_nodeData_getPortDataMap(nodeData);
         apx_portDataMap_updatePortTriggerList(portDataMap, portConnectionTable);
         apx_connectionBase_triggerProvidePortsConnected(&self->base, nodeData, portConnectionTable);
         break;
      case APX_EVENT_REQUIRE_PORT_DISCONNECT:
         nodeData = (apx_nodeData_t*) event->evData1;
         portConnectionTable = (apx_portConnectionTable_t*) event->evData2;
         apx_connectionBase_triggerRequirePortsDisconnected(&self->base, nodeData, portConnectionTable);
         break;
      case APX_EVENT_PROVIDE_PORT_DISCONNECT:
         nodeData = (apx_nodeData_t*) event->evData1;
         portConnectionTable = (apx_portConnectionTable_t*) event->evData2;
         portDataMap = apx_nodeData_getPortDataMap(nodeData);
         apx_portDataMap_updatePortTriggerList(portDataMap, portConnectionTable);
         apx_connectionBase_triggerProvidePortsDisconnected(&self->base, nodeData, portConnectionTable);
         break;
*/
      }
      apx_connectionBase_defaultEventHandler(&self->base, event);
   }
}

void apx_serverConnectionBase_activate(apx_serverConnectionBase_t *self, uint32_t connectionId)
{
   if (self != 0)
   {
      apx_connectionBase_setConnectionId(&self->base, connectionId);
      self->isActive = true;
   }
}

void apx_serverConnectionBase_deactivate(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      self->isActive = false;
      apx_serverConnectionBase_close(self);
      apx_serverConnectionBase_detachAllNodes(self);
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



uint32_t apx_serverConnectionBase_getTotalPortReferences(apx_serverConnectionBase_t *self)
{
   ///TODO: Implement
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
   apx_fileManager_headerReceived(&self->base.fileManager);
   apx_connectionBase_emitHeaderAccepted(&self->base);
}

apx_error_t apx_serverConnectionBase_fileInfoNotify(apx_serverConnectionBase_t *self, const rmf_fileInfo_t *remoteFileInfo)
{
   if ( (self != 0) && (remoteFileInfo != 0))
   {
      return apx_connectionBase_fileInfoNotify(&self->base, remoteFileInfo);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}



struct apx_server_tag* apx_serverConnectionBase_getServer(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      return self->server;
   }
   return (apx_server_t*) 0;
}

/*** UNIT TEST API ***/

#ifdef UNIT_TEST
void apx_serverConnectionBase_run(apx_serverConnectionBase_t *self)
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
            apx_error_t errorCode = apx_connectionBase_processMessage(&self->base, pNext, msgLen);
            if (errorCode != APX_NO_ERROR)
            {
               printf("[SERVER-CONNECTION-BASE] apx_connectionBase_processMessage failed with %d\n", (int) errorCode);
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
   *parseLen=totalParsed;
   return 0;
}



static void apx_serverConnectionBase_fileInfoNotifyImpl(void *arg, const apx_fileInfo_t *fileInfo)
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
         //apx_serverConnectionBase_processNewOutPortDataFile(self, fileInfo);
      }
      else
      {
         //ignore
      }
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
            //apx_fileManager_sendFileAlreadyExistsError(&self->base.fileManager, file);
         }
         else
         {
            apx_nodeInstance_t *nodeInstance = apx_nodeManager_createNode(&self->base.nodeManager, nodeName);
            if (nodeInstance != 0)
            {
               apx_nodeData_t *nodeData;
               apx_nodeInstance_setConnection(nodeInstance, &self->base);
               nodeData = apx_nodeInstance_getNodeData(nodeInstance);
               if (nodeData != 0)
               {
                  retval = apx_nodeInstance_createDefinitionBuffer(nodeInstance, fileInfo->length);
                  if (retval == APX_NO_ERROR)
                  {
                     apx_file_t *remoteFile = apx_fileManager_findFileByAddress(&self->base.fileManager, fileInfo->address);
                     if (remoteFile != 0)
                     {
                        apx_nodeInstance_registerDefinitionFileHandler(nodeInstance, remoteFile);
                        retval = apx_fileManager_requestOpenFile(&self->base.fileManager, fileInfo->address);
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
static void apx_serverConnectionBase_definitionDataWriteNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, uint32_t offset, uint32_t len)
{
   apx_programType_t errProgramType;
   apx_uniquePortId_t errPortId;
   apx_portCount_t numProvidePorts;
   apx_error_t rc = apx_nodeManager_parseDefinition(&self->base.nodeManager, nodeInstance);
   if (rc != APX_NO_ERROR )
   {
      printf("APX file parse failure (%d)\n", (int) rc);
      ///TODO: send error code back to client
      return;
   }
   rc = apx_nodeInstance_buildNodeInfo(nodeInstance, &errProgramType, &errPortId);
   if (rc != APX_NO_ERROR)
   {
      printf("APX build node info failure (%d)\n", (int) rc);
      ///TODO: send error code back to client
      return;
   }
   apx_nodeInstance_cleanParseTree(nodeInstance);
#if APX_DEBUG_ENABLE
   printf("%s.apx: Parse Success (%d bytes)\n", apx_nodeInstance_getName(nodeInstance), len);
#endif
   rc = apx_nodeInstance_createPortDataBuffers(nodeInstance);
   if (rc != APX_NO_ERROR)
   {
      printf("APX buffer creation failed with (%d)\n", (int) rc);
      ///TODO: send error code back to client
      return;
   }
   rc = apx_nodeInstance_buildPortRefs(nodeInstance);
   if (rc != APX_NO_ERROR)
   {
      ///TODO: send error code back to client
      return;
   }
   numProvidePorts = apx_nodeInstance_getNumProvidePorts(nodeInstance);
   if (numProvidePorts > 0)
   {
      rc = apx_nodeInstance_buildConnectorTable(nodeInstance);
      if (rc != APX_NO_ERROR)
      {
         ///TODO: send error code back to client
         return;
      }
   }
   rc = apx_serverConnectionBase_openOutPortDataFileIfExists(self, nodeInstance);
   if (rc != APX_NO_ERROR)
   {
      printf("Opening OutPortData file failed with (%d)\n", (int) rc);
      ///TODO: send error code back to client
      return;
   }
   rc = apx_serverConnectionBase_createRequirePortDataFileIfNeeded(self, nodeInstance);
   if (rc != APX_NO_ERROR)
   {
      printf("Creation of requirePortDataFile failed with (%d)\n", (int) rc);
      ///TODO: send error code back to client
      return;
   }
}

static apx_error_t apx_serverConnectionBase_providePortDataWriteNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, uint32_t offset, const uint8_t *data, uint32_t len)
{
   apx_error_t rc;
   apx_nodeData_t *nodeData;
   apx_providePortDataState_t portDataState = apx_nodeInstance_getProvidePortDataState(nodeInstance);
   nodeData = apx_nodeInstance_getNodeData(nodeInstance);
   assert(nodeData != 0);
   switch(portDataState)
   {
   case APX_PROVIDE_PORT_DATE_STATE_INIT:
      assert(0); //This is an internal error, no file handler should have been registered yet.
      break;
   case APX_PROVIDE_PORT_DATE_STATE_WAITING_FOR_FILE_INFO:
      assert(0); //This is an internal error, no file handler should have been registered yet.
      break;
   case APX_PROVIDE_PORT_DATA_STATE_WAITING_FOR_FILE_DATA:
      rc = apx_nodeData_writeProvidePortData(nodeData, data, offset, len);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      if (self->server != 0)
      {
         apx_server_takeGlobalLock(self->server);
         rc = apx_server_connectNodeInstanceProvidePorts(self->server, nodeInstance);
         if (rc == APX_NO_ERROR)
         {
            apx_portConnectorChangeTable_t *providePortChanges;
            apx_nodeInstance_setProvidePortDataState(nodeInstance, APX_PROVIDE_PORT_DATA_STATE_CONNECTED);
            providePortChanges = apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance, false);
            if (providePortChanges != 0)
            {
               rc = apx_server_processProvidePortConnectorChanges(self->server, nodeInstance, providePortChanges);
               if (rc != APX_NO_ERROR)
               {
                  apx_server_releaseGlobalLock(self->server);
                  return rc;
               }
            }
         }
         else
         {
            apx_server_releaseGlobalLock(self->server);
            return rc;
         }
         apx_nodeInstance_clearProvidePortConnectorChanges(nodeInstance, true); ///TODO: switch this to false once event handlers are working again
         apx_server_releaseGlobalLock(self->server);
      }
      break;
   case APX_PROVIDE_PORT_DATA_STATE_CONNECTED:
      printf("[%u] Consecutive write of %s.out(%d,%d)\n", (unsigned int) apx_connectionBase_getConnectionId(&self->base), apx_nodeInstance_getName(nodeInstance), (int) offset, (int) len);
      break;
   case APX_PROVIDE_PORT_DATA_STATE_DISCONNECTED:
      break; //Drop all data writes in this state, we are about to close connection
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_serverConnectionBase_openOutPortDataFileIfExists(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance)
{
   apx_error_t retval = APX_NO_ERROR;
   apx_nodeInfo_t *nodeInfo;
   apx_size_t providePortDataLen;
   assert(self != 0);
   assert(nodeInstance != 0);
   nodeInfo = apx_nodeInstance_getNodeInfo(nodeInstance);
   assert(nodeInfo != 0);
   providePortDataLen = apx_nodeInfo_getProvidePortDataLen(nodeInfo);
   if (providePortDataLen > 0)
   {
      apx_file_t *file;
      apx_nodeData_t *nodeData;
      char fileNameBuf[RMF_MAX_FILE_NAME+1];
      const char *nodeName;
      //Build file name using string buffer
      nodeName = apx_nodeInfo_getName(nodeInfo);
      assert(nodeName);
      if (strlen(nodeName)+APX_MAX_FILE_EXT_LEN > RMF_MAX_FILE_NAME)
      {
         return APX_NAME_TOO_LONG_ERROR;
      }
      strcpy(&fileNameBuf[0], nodeName);
      strcat(&fileNameBuf[0], APX_OUTDATA_FILE_EXT);

      // Validate that nodeData data buffer has been prepared
      nodeData = apx_nodeInstance_getNodeData(nodeInstance);
      assert(nodeData != 0);
      assert(providePortDataLen == apx_nodeData_getProvidePortDataLen(nodeData));

      //Search for file in fileManager
      file = apx_fileManager_findRemoteFileByName(&self->base.fileManager, &fileNameBuf[0]);
      if (file != 0)
      {
         apx_nodeInstance_registerProvidePortFileHandler(nodeInstance, file);
         apx_nodeInstance_setProvidePortDataState(nodeInstance, APX_PROVIDE_PORT_DATA_STATE_WAITING_FOR_FILE_DATA);
         retval = apx_fileManager_requestOpenFile(&self->base.fileManager, apx_file_getStartAddress(file) | RMF_REMOTE_ADDRESS_BIT);
      }
      else
      {
         //Not found, client might create it later
         apx_nodeInstance_setProvidePortDataState(nodeInstance, APX_PROVIDE_PORT_DATE_STATE_WAITING_FOR_FILE_INFO);
      }
   }
   else
   {
      apx_nodeInstance_setProvidePortDataState(nodeInstance, APX_PROVIDE_PORT_DATA_STATE_DISCONNECTED);
   }
   return retval;
}

static apx_error_t  apx_serverConnectionBase_createRequirePortDataFileIfNeeded(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance)
{
   apx_error_t retval = APX_NO_ERROR;
   apx_nodeInfo_t *nodeInfo;
   apx_size_t requirePortDataLen;
   assert(self != 0);
   assert(nodeInstance != 0);
   nodeInfo = apx_nodeInstance_getNodeInfo(nodeInstance);
   assert(nodeInfo != 0);
   requirePortDataLen = apx_nodeInfo_getRequirePortDataLen(nodeInfo);
   if (requirePortDataLen > 0u)
   {
      apx_fileInfo_t *fileInfo;
      apx_nodeData_t *nodeData;
      char fileNameBuf[RMF_MAX_FILE_NAME+1];
      const char *nodeName;
      //Build file name using string buffer
      nodeName = apx_nodeInfo_getName(nodeInfo);
      assert(nodeName);
      if (strlen(nodeName)+APX_MAX_FILE_EXT_LEN > RMF_MAX_FILE_NAME)
      {
         return APX_NAME_TOO_LONG_ERROR;
      }
      strcpy(&fileNameBuf[0], nodeName);
      strcat(&fileNameBuf[0], APX_INDATA_FILE_EXT);

      //Validate that nodeData data buffer has been prepared
      nodeData = apx_nodeInstance_getNodeData(nodeInstance);
      assert(nodeData != 0);
      assert(requirePortDataLen == apx_nodeData_getRequirePortDataLen(nodeData));

      //Create new fileInfo
      fileInfo = apx_fileInfo_new(RMF_INVALID_ADDRESS, requirePortDataLen, &fileNameBuf[0], RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, (const uint8_t*) 0);
      if (fileInfo != 0)
      {
         apx_file_t *file;
         file = apx_fileManager_createLocalFile(&self->base.fileManager, fileInfo);
         if (file == 0)
         {
            retval = APX_MEM_ERROR;
         }
         apx_fileInfo_setAddress(fileInfo, apx_file_getStartAddress(file));
         apx_nodeInstance_registerRequirePortFileHandler(nodeInstance, file);
         apx_nodeInstance_setRequirePortDataState(nodeInstance, APX_REQUIRE_PORT_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST);
         retval = apx_fileManager_sendFileInfo(&self->base.fileManager, fileInfo);
      }
   }
   else
   {
      apx_nodeInstance_setRequirePortDataState(nodeInstance, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED);
   }
   return retval;
}




static void apx_serverConnectionBase_nodeInstanceFileOpenNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      if (fileType == APX_INDATA_FILE_TYPE)
      {
         apx_error_t rc = apx_serverConnectionBase_RequirePortDataFileOpenNotify(self, nodeInstance);
         if (rc != APX_NO_ERROR)
         {
            printf("[SOCKET-SERVER-BASE] RequirePortDataFileOpenNotify failed with (%d)\n", rc);
         }
      }
   }
}

static apx_error_t apx_serverConnectionBase_RequirePortDataFileOpenNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance)
{
   assert(self != 0);
   assert(nodeInstance != 0);
   assert(nodeInstance->requirePortDataFile != 0);
   assert(apx_file_isOpen(nodeInstance->requirePortDataFile));
   assert(apx_nodeInstance_getRequirePortDataState(nodeInstance) == APX_REQUIRE_PORT_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST);
   if (self->server != 0)
   {
      apx_error_t rc;
      apx_portConnectorChangeTable_t *requirePortChanges;
      apx_server_takeGlobalLock(self->server);
      rc = apx_server_connectNodeInstanceRequirePorts(self->server, nodeInstance);
      if (rc != APX_NO_ERROR)
      {
         apx_server_releaseGlobalLock(self->server);
         return rc;
      }
      apx_nodeInstance_setRequirePortDataState(nodeInstance, APX_REQUIRE_PORT_DATA_STATE_CONNECTED);
      requirePortChanges = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance, false);
      if (requirePortChanges != 0)
      {
         rc = apx_server_processRequirePortConnectorChanges(self->server, nodeInstance, requirePortChanges);
         if (rc != APX_NO_ERROR)
         {
            apx_server_releaseGlobalLock(self->server);
            return rc;
         }
      }
      //Trigger transmission of .in file back to client
      rc = apx_nodeInstance_sendRequirePortDataToFileManager(nodeInstance);
      apx_server_releaseGlobalLock(self->server);
      return rc;
   }
   return APX_NO_ERROR;
}

static void apx_serverConnectionBase_vnodeInstanceFileOpenNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType)
{
   apx_serverConnectionBase_nodeInstanceFileOpenNotify((apx_serverConnectionBase_t*) arg, nodeInstance, fileType);
}

static void apx_serverConnectionBase_processNewOutPortDataFile(apx_serverConnectionBase_t *self, const apx_fileInfo_t *fileInfo)
{
   printf("[SOCKET-SERVER-BASE](%d) NOT IMPLEMENTED\n", __LINE__);
}

static void apx_serverConnectionBase_routeDataFromProvidePort(apx_serverConnectionBase_t *self, apx_nodeData_t *srcNodeData, uint32_t offset, uint32_t len)
{
/*
   apx_portDataMap_t *srcPortDataMap = apx_nodeData_getPortDataMap(srcNodeData);
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
            apx_portRef_t *portDataRef = apx_portTriggerList_get(portTriggerList, i);
            if (portDataRef != 0)
            {
               apx_nodeData_t *destNodeData = portDataRef->nodeData;
               const apx_portDataProps_t *destPortDataProps = portDataRef->portDataProps;
               if (apx_portDataProps_isPlainOldData(srcPortDataProps) )
               {
                  apx_nodeData_routePortData(destNodeData, destPortDataProps, srcNodeData, srcPortDataProps);
               }
            }
         }
      }
   }
*/
}

static void apx_serverConnectionBase_detachAllNodes(apx_serverConnectionBase_t *self)
{
   if (self != 0)
   {
      if (self->server != 0)
      {
         adt_ary_t nodeInstanceArray;
         int32_t numNodes;
         adt_ary_create(&nodeInstanceArray, (void (*)(void*)) 0);
         apx_server_takeGlobalLock(self->server);
         numNodes = apx_nodeManager_values(&self->base.nodeManager, &nodeInstanceArray);
         if (numNodes > 0)
         {
            int32_t i;
            for (i=0; i < numNodes; i++)
            {
               apx_requirePortDataState_t requirePortDataState;
               apx_providePortDataState_t providePortDataState;
               apx_nodeInstance_t *nodeInstance = (apx_nodeInstance_t*) adt_ary_value(&nodeInstanceArray, i);
               assert(nodeInstance != 0);
               requirePortDataState = apx_nodeInstance_getRequirePortDataState(nodeInstance);
               providePortDataState = apx_nodeInstance_getProvidePortDataState(nodeInstance);
               if (requirePortDataState == APX_REQUIRE_PORT_DATA_STATE_CONNECTED)
               {
                  apx_server_disconnectNodeInstanceRequirePorts(self->server, nodeInstance);
                  apx_nodeInstance_setRequirePortDataState(nodeInstance, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED);
               }
               if (providePortDataState == APX_PROVIDE_PORT_DATA_STATE_CONNECTED)
               {
                  apx_server_disconnectNodeInstanceProvidePorts(self->server, nodeInstance);
                  apx_nodeInstance_setProvidePortDataState(nodeInstance, APX_PROVIDE_PORT_DATA_STATE_DISCONNECTED);
               }
            }
         }
         apx_server_releaseGlobalLock(self->server);
         adt_ary_destroy(&nodeInstanceArray);
      }
   }
}

static void apx_serverConnectionBase_nodeInstanceFileWriteNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len)
{
   if ( (self != 0) && (nodeInstance != 0) && (data != 0) )
   {
      if (fileType == APX_DEFINITION_FILE_TYPE)
      {
         apx_serverConnectionBase_definitionDataWriteNotify(self, nodeInstance, offset, len);
      }
      else if (fileType == APX_OUTDATA_FILE_TYPE)
      {
         apx_error_t rc = apx_serverConnectionBase_providePortDataWriteNotify(self, nodeInstance, offset, data, len);
         if (rc != APX_NO_ERROR)
         {
            printf("[SERVER-CONNECTION-BASE] providePortDataWriteNotify failed with error %d\n", rc);
         }
      }
      else
      {
         printf("[SERVER-CONNECTION-BASE] NOT IMPLEMENTED\n");
      }
   }
}

static void apx_serverConnectionBase_vnodeInstanceFileWriteNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len)
{
   apx_serverConnectionBase_nodeInstanceFileWriteNotify((apx_serverConnectionBase_t*) arg, nodeInstance, fileType, offset, data, len);
}
