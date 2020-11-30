/*****************************************************************************
* \file      apx_connection_base.h
* \author    Conny Gustafsson
* \date      2018-12-09
* \brief     Base class for all connections (client and server)
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
#include <string.h>
#include <assert.h>
#include <stdio.h> //DEBUG only
#include "apx/connection_base.h"
#include "apx/port_data_ref.h"
#include "apx/node_data.h"
#include "apx/logging.h"
#include "apx/port_connector_change_table.h"
#include "apx/util.h"
#ifdef _WIN32
#include <process.h>
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_connectionBase_startWorkerThread(apx_connectionBase_t *self);
static void apx_connectionBase_stopWorkerThread(apx_connectionBase_t *self);
static void apx_connectionBase_stopWorkerThread(apx_connectionBase_t *self);
static apx_error_t apx_connectionBase_initTransmitHandler(apx_connectionBase_t *self);

//Internal event emit API

static void apx_connectionBase_emitFileCreatedEvent(apx_connectionBase_t *self, const apx_fileInfo_t *fileInfo);

//static void apx_connectionBase_createNodeCompleteEvent(apx_event_t *event, apx_nodeData_t *nodeData);
//static void apx_connectionBase_handlePortConnectEvent(apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable, apx_portType_t portType);
//static void apx_connectionBase_handlePortDisconnectEvent(apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable, apx_portType_t portType);


#ifndef UNIT_TEST
static THREAD_PROTO(eventHandlerWorkThread,arg);
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_connectionBaseVTable_create(apx_connectionBaseVTable_t *self, apx_voidPtrFunc *destructor, apx_voidPtrFunc *start, apx_voidPtrFunc *close, apx_fillTransmitHandlerFunc *fillTransmitHandler)
{
   if (self != 0)
   {
      memset(self, 0, sizeof(apx_connectionBaseVTable_t));
      self->destructor = destructor;
      self->start = start;
      self->close = close;
      self->fillTransmitHandler = fillTransmitHandler;
   }
}

apx_error_t apx_connectionBase_create(apx_connectionBase_t *self, apx_mode_t mode, apx_connectionBaseVTable_t *vtable)
{
   if (self != 0)
   {
      adt_buf_err_t bufResult;
      apx_error_t rc;
      self->connectionId = 0;
      if (vtable != 0)
      {
         memcpy(&self->vtable, vtable, sizeof(apx_connectionBaseVTable_t));
      }
      else
      {
         memset(&self->vtable, 0, sizeof(apx_connectionBaseVTable_t));
      }
      self->numHeaderLen = (int8_t) sizeof(uint32_t);
      self->eventHandler = (apx_eventHandlerFunc_t*) 0;
      self->eventHandlerArg = (void*) 0;
      self->totalBytesReceived = 0u;
      self->totalBytesSent = 0u;
      self->mode = mode;
      rc = apx_allocator_create(&self->allocator, APX_MAX_NUM_MESSAGES);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      apx_nodeManager_create(&self->nodeManager, mode, (bool) (mode==APX_CLIENT_MODE));
#ifdef _WIN32
      self->workerThread = INVALID_HANDLE_VALUE;
#else
      self->workerThread = 0;
#endif
      self->workerThreadValid=false;
      bufResult = apx_eventLoop_create(&self->eventLoop);
      if (bufResult != BUF_E_OK)
      {
         apx_allocator_destroy(&self->allocator);
         apx_nodeManager_destroy(&self->nodeManager);
         return APX_MEM_ERROR;
      }
      rc = apx_fileManager_create(&self->fileManager, mode, self);
      if (rc != APX_NO_ERROR)
      {
         apx_allocator_destroy(&self->allocator);
         apx_nodeManager_destroy(&self->nodeManager);
         apx_eventLoop_destroy(&self->eventLoop);
         return rc;
      }
      rc = apx_connectionBase_initTransmitHandler(self);
      if (rc != APX_NO_ERROR)
      {
         apx_allocator_destroy(&self->allocator);
         apx_nodeManager_destroy(&self->nodeManager);
         apx_fileManager_destroy(&self->fileManager);
         apx_eventLoop_destroy(&self->eventLoop);
         return rc;
      }
      adt_list_create(&self->connectionEventListeners, apx_connectionEventListener_vdelete);
      MUTEX_INIT(self->eventListenerMutex);
      apx_allocator_start(&self->allocator);
      return rc;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_connectionBase_destroy(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      apx_fileManager_destroy(&self->fileManager);
      apx_eventLoop_destroy(&self->eventLoop);
      apx_nodeManager_destroy(&self->nodeManager);
      MUTEX_DESTROY(self->eventListenerMutex);
      adt_list_destroy(&self->connectionEventListeners);
      apx_allocator_stop(&self->allocator);
      apx_allocator_destroy(&self->allocator);
   }
}

void apx_connectionBase_delete(apx_connectionBase_t *self)
{
   if(self != 0)
   {
      if (self->vtable.destructor != 0)
      {
         self->vtable.destructor((void*) self);
      }
      free(self);
   }
}

void apx_connectionBase_vdelete(void *arg)
{
   apx_connectionBase_delete((apx_connectionBase_t*) arg);
}

apx_fileManager_t *apx_connectionBase_getFileManager(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      return &self->fileManager;
   }
   return (apx_fileManager_t*) 0;
}

void apx_connectionBase_setEventHandler(apx_connectionBase_t *self, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg)
{
   if (self != 0)
   {
      self->eventHandler = eventHandler;
      self->eventHandlerArg = eventHandlerArg;
   }
}

void apx_connectionBase_start(apx_connectionBase_t *self)
{
   if ( self != 0 )
   {
      apx_connectionBase_startWorkerThread(self);
      apx_fileManager_start(&self->fileManager);
      if ( self->vtable.start != 0 )
      {
         self->vtable.start((void*) self);
      }
   }
}

void apx_connectionBase_stop(apx_connectionBase_t *self)
{
   if ( self != 0 )
   {
      apx_fileManager_stop(&self->fileManager);
      apx_connectionBase_stopWorkerThread(self);
   }
}

void apx_connectionBase_close(apx_connectionBase_t *self)
{
   if ( (self != 0) && (self->vtable.close != 0) )
   {
      self->vtable.close((void*) self);
   }
}

void apx_connectionBase_attachNodeInstance(apx_connectionBase_t *self, apx_nodeInstance_t *nodeInstance)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      apx_fileInfo_t fileInfo;
      apx_error_t rc = APX_NO_ERROR;
      apx_portCount_t numRequirePorts;
      apx_portCount_t numProvidePorts;
      numRequirePorts = apx_nodeInstance_getNumRequirePorts(nodeInstance);
      numProvidePorts = apx_nodeInstance_getNumProvidePorts(nodeInstance);
      apx_nodeManager_attachNode(&self->nodeManager, nodeInstance);
      apx_nodeInstance_setConnection(nodeInstance, self);
      if (numProvidePorts > 0)
      {
         rc = apx_nodeInstance_fillProvidePortDataFileInfo(nodeInstance, &fileInfo);
         if (rc == APX_NO_ERROR)
         {
            apx_file_t *localFile = apx_fileManager_createLocalFile(&self->fileManager, &fileInfo);
            apx_fileInfo_destroy(&fileInfo);
            if (localFile != 0)
            {
               apx_nodeInstance_registerProvidePortFileHandler(nodeInstance, localFile);
            }
         }
      }
      if (numRequirePorts > 0)
      {
         apx_nodeInstance_setRequirePortDataState(nodeInstance, APX_REQUIRE_PORT_DATA_STATE_WAITING_FILE_INFO);
      }
      rc = apx_nodeInstance_fillDefinitionFileInfo(nodeInstance, &fileInfo);
      if (rc == APX_NO_ERROR)
      {
         apx_file_t *localFile = apx_fileManager_createLocalFile(&self->fileManager, &fileInfo);
         apx_fileInfo_destroy(&fileInfo);
         if (localFile != 0)
         {
            apx_nodeInstance_registerDefinitionFileHandler(nodeInstance, localFile);
         }
      }
   }
}

apx_error_t apx_connectionBase_processMessage(apx_connectionBase_t *self, const uint8_t *msgBuf, int32_t msgLen)
{
   if (self != 0)
   {
      return apx_fileManager_messageReceived(&self->fileManager, msgBuf, msgLen);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

uint8_t *apx_connectionBase_alloc(apx_connectionBase_t *self, size_t size)
{
   if (self != 0)
   {
      return apx_allocator_alloc(&self->allocator, size);
   }
   return (uint8_t*) 0;
}

void apx_connectionBase_free(apx_connectionBase_t *self, uint8_t *ptr, size_t size)
{
   if (self != 0)
   {
      apx_allocator_free(&self->allocator, ptr, size);
   }
}


/*** Internal Callback API ***/
//Callbacks triggered due to events happening remotely

/**
 * New fileInfo has been received
 */
apx_error_t apx_connectionBase_fileInfoNotify(apx_connectionBase_t *self, const rmf_fileInfo_t *remoteFileInfo)
{
   if ( (self != 0) && (remoteFileInfo != 0) )
   {
      apx_fileInfo_t fileInfo;
      apx_error_t rc = apx_fileInfo_create_rmf(&fileInfo, remoteFileInfo, true);
      if (rc == APX_NO_ERROR)
      {
         apx_file_t *file = apx_fileManager_fileInfoNotify(&self->fileManager, &fileInfo);
         if (file == 0)
         {
            apx_fileInfo_destroy(&fileInfo);
            return APX_MEM_ERROR; ///TODO: Investigate if we can get the error state from the file manager.
         }
         if ( self->vtable.fileInfoNotify != 0 )
         {
            //In server case: Start preparing new nodeInstance or a new file belonging to a nodeInstance
            //In client case: Start preparing for a new file belonging to a nodeInstance
            self->vtable.fileInfoNotify((void*) self, &fileInfo);
         }
         if(self->mode == APX_SERVER_MODE) ///TODO: fix memory leak in client mode
         {
            apx_connectionBase_emitFileCreatedEvent(self, &fileInfo); ///TODO: Perhaps remove cloning of fileInfo inside call?
         }
         apx_fileInfo_destroy(&fileInfo);
         return APX_NO_ERROR;
      }
      else
      {
         return rc;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_connectionBase_fileOpenNotify(apx_connectionBase_t *self, uint32_t address)
{
   if (self != 0 )
   {
      apx_file_t *file = apx_fileManager_findFileByAddress(&self->fileManager, address & RMF_ADDRESS_MASK_INTERNAL);
      if (file != 0)
      {
         apx_file_open(file);
         return apx_file_fileOpenNotify(file);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_connectionBase_fileWriteNotify(apx_connectionBase_t *self, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t len)
{
   if ( (self != 0) && (file != 0) && (data != 0) )
   {
      apx_error_t retval = apx_file_fileWriteNotify(file, offset, data, len);
      if (retval == APX_NO_ERROR)
      {
         ///TODO: trigger generic event listeners here?
      }
      else
      {
#if APX_DEBUG_ENABLED
         printf("[CONNECTION-BASE] apx_file_fileWriteNotify failed with error %d\n", retval);
#endif
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_connectionBase_nodeInstanceFileWriteNotify(apx_connectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len)
{
   if ( (self != 0) && (nodeInstance != 0) && (data != 0) )
   {
      if (self->vtable.nodeFileWriteNotify != 0)
      {
         self->vtable.nodeFileWriteNotify((void*) self, nodeInstance, fileType, offset, data, len);
      }
      else
      {
         printf("No callback\n");
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_connectionBase_nodeInstanceFileOpenNotify(apx_connectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      if (self->vtable.nodeFileOpenNotify != 0)
      {
         self->vtable.nodeFileOpenNotify((void*) self, nodeInstance, fileType);
      }
      else
      {
         printf("No callback\n");
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//Callbacks triggered due to events happening locally

apx_error_t apx_connectionBase_updateProvidePortDataDirect(apx_connectionBase_t *self, apx_file_t *file, const uint8_t *data, uint32_t offset, uint32_t len)
{
   if ( (self != 0) && (file != 0) )
   {
      if (self->mode == APX_CLIENT_MODE)
      {
         bool isFileOpen;
         assert(file != 0);
         isFileOpen = apx_file_isOpen(file);
         if (isFileOpen)
         {
            uint8_t *dataBuf;
            uint32_t address;
            uint32_t startAddress = apx_file_getStartAddress(file);
            address = startAddress + offset;
            dataBuf = apx_allocator_alloc(&self->allocator, len);
            if (dataBuf == 0)
            {
               return APX_MEM_ERROR;
            }
            memcpy(dataBuf, data, len);
            return apx_fileManager_writeDynamicData(&self->fileManager, address, len, dataBuf);
         }
         else
         {
            //We have a connection by file has not yet been opened on remote side
         }
         return APX_NO_ERROR;
      }
      else
      {
         //Server mode
         printf("apx_connectionBase_updateProvidePortDataDirect in server mode not implemented\n");
         return APX_NOT_IMPLEMENTED_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_connectionBase_updateRequirePortDataDirect(apx_connectionBase_t *self, apx_file_t *file, const uint8_t *data, uint32_t offset, uint32_t len)
{
   if ( (self != 0) && (file != 0) )
   {
      if (self->mode == APX_CLIENT_MODE)
      {
         printf("apx_connectionBase_updateRequirePortDataDirect in client mode not implemented\n");
         return APX_NOT_IMPLEMENTED_ERROR;
      }
      else
      {
         //Server mode
         bool isFileOpen;
         assert(file != 0);
         isFileOpen = apx_file_isOpen(file);
         if (isFileOpen)
         {
            uint8_t *dataBuf;
            uint32_t address;
            uint32_t startAddress = apx_file_getStartAddress(file);
            address = startAddress + offset;
            dataBuf = apx_allocator_alloc(&self->allocator, len);
            if (dataBuf == 0)
            {
               return APX_MEM_ERROR;
            }
            memcpy(dataBuf, data, len);
            return apx_fileManager_writeDynamicData(&self->fileManager, address, len, dataBuf);
         }
         else
         {
            //We have a connection by file has not yet been opened on remote side
         }
         return APX_NO_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_connectionBase_disconnectNotify(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      apx_fileManager_disconnectNotify(&self->fileManager);
   }
}

void apx_connectionBase_portConnectorChangeCreateNotify(apx_connectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_portType_t portType)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      if (self->vtable.portConnectorChangeCreateNotify != 0)
      {
         self->vtable.portConnectorChangeCreateNotify((void*) self, nodeInstance, portType);
      }
   }
}


/*** Event triggering API ***/
void apx_connectionBase_triggerRemoteFileHeaderCompleteEvent(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      apx_event_t event;
      apx_event_fillRemoteFileHeaderComplete(&event, self);
      apx_eventLoop_append(&self->eventLoop, &event);
   }
}

/*
void apx_connectionBase_emitFileManagerPreStartEvent(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      apx_event_t event;
      apx_fileManager_createPreStartEvent(&event, &self->fileManager);
      apx_eventLoop_append(&self->eventLoop, &event);
   }
}

void apx_connectionBase_emitFileManagerPostStopEvent(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      apx_event_t event;
      apx_fileManager_createPostStopEvent(&event, &self->fileManager);
      apx_eventLoop_append(&self->eventLoop, &event);
   }
}
*/

void apx_connectionBase_emitFileRevokedEvent(apx_connectionBase_t *self, struct apx_file_tag *file, const void *caller)
{
   if (self != 0)
   {
/*      apx_event_t event;
      apx_fileManager_createFileRevokedEvent(&event, &self->fileManager, file, caller);
      apx_eventLoop_append(&self->eventLoop, &event);
*/
   }
}

void apx_connectionBase_emitFileOpenedEvent(apx_connectionBase_t *self, struct apx_file_tag *file, const void *caller)
{
   if (self != 0)
   {
/*      apx_event_t event;
      apx_fileManager_createFileOpenedEvent(&event, &self->fileManager, file, caller);
      apx_eventLoop_append(&self->eventLoop, &event);
*/
   }
}

void apx_connectionBase_emitGenericEvent(apx_connectionBase_t *self, apx_event_t *event)
{
   if (self != 0)
   {
      apx_eventLoop_append(&self->eventLoop, event);
   }
}

void apx_connectionBase_emitNodeComplete(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData)
{
   if (self != 0)
   {
/*
      apx_event_t event;
      apx_connectionBase_createNodeCompleteEvent(&event, nodeData);
      apx_eventLoop_append(&self->eventLoop, &event);
*/
   }
}

void apx_connectionBase_emitHeaderAccepted(apx_connectionBase_t *self)
{
   apx_event_t event;
   apx_event_createHeaderAccepted(&event, self);
   apx_eventLoop_append(&self->eventLoop, &event);
}

void apx_connectionBase_emitRemoteFileWrittenType1(apx_connectionBase_t *self, struct apx_file_tag *remoteFile, struct apx_file_tag *file, uint32_t offset, const uint8_t *data, uint32_t len, bool moreBit)
{
   if (self != 0)
   {

   }
}

void apx_connectionBase_onHeaderAccepted(apx_connectionBase_t *self, apx_connectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      adt_list_elem_t *iter;
      MUTEX_LOCK(self->eventListenerMutex);
      iter = adt_list_iter_first(&self->connectionEventListeners);
      while (iter != 0)
      {
         apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;

         if (eventListener->headerAccepted2 != 0)
         {
            eventListener->headerAccepted2(eventListener->arg, connection);
         }

         iter = adt_list_iter_next(iter);
      }
      MUTEX_UNLOCK(self->eventListenerMutex);
   }
}

void apx_connectionBase_onFileCreated(apx_connectionBase_t *self, apx_connectionBase_t *connection, struct apx_fileInfo_tag *fileInfo, void *caller)
{
   if ( (self != 0) && (connection != 0) )
   {
      adt_list_elem_t *iter;
      MUTEX_LOCK(self->eventListenerMutex);
      iter = adt_list_iter_first(&self->connectionEventListeners);
      while (iter != 0)
      {
         if (iter->pItem != caller)
         {
            apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;
            if (eventListener->fileCreate2 != 0)
            {
               eventListener->fileCreate2(eventListener->arg, connection, fileInfo);
            }
         }
         iter = adt_list_iter_next(iter);
      }
      MUTEX_UNLOCK(self->eventListenerMutex);
   }
}


void apx_connectionBase_defaultEventHandler(apx_connectionBase_t *self, apx_event_t *event)
{
   if ( (self != 0) && (event != 0) )
   {
      apx_portConnectorChangeTable_t *connectorChanges;
      apx_nodeData_t *nodeData;


      switch(event->evType)
      {
/*
      case APX_EVENT_REQUIRE_PORT_CONNECT:
         nodeData = (apx_nodeData_t*) event->evData1;
         connectionTable = (apx_portConnectionTable_t*) event->evData2;
         apx_connectionBase_handlePortConnectEvent(nodeData, connectionTable, APX_REQUIRE_PORT);
         apx_portConnectionTable_delete(connectionTable);
         break;
      case APX_EVENT_PROVIDE_PORT_CONNECT:
         nodeData = (apx_nodeData_t*) event->evData1;
         connectionTable = (apx_portConnectionTable_t*) event->evData2;
         apx_connectionBase_handlePortConnectEvent(nodeData, connectionTable, APX_PROVIDE_PORT);
         apx_portConnectionTable_delete(connectionTable);
         break;
      case APX_EVENT_REQUIRE_PORT_DISCONNECT:
         nodeData = (apx_nodeData_t*) event->evData1;
         connectionTable = (apx_portConnectionTable_t*) event->evData2;
         apx_connectionBase_handlePortDisconnectEvent(nodeData, connectionTable, APX_REQUIRE_PORT);
         apx_portConnectionTable_delete((apx_portConnectionTable_t*) event->evData2);
         break;
      case APX_EVENT_PROVIDE_PORT_DISCONNECT:
         nodeData = (apx_nodeData_t*) event->evData1;
         connectionTable = (apx_portConnectionTable_t*) event->evData2;
         apx_connectionBase_handlePortDisconnectEvent(nodeData, connectionTable, APX_PROVIDE_PORT);
         apx_portConnectionTable_delete((apx_portConnectionTable_t*) event->evData2);
         break;
*/
      case APX_EVENT_NODE_COMPLETE:
         break;
      }
   }
}


void apx_connectionBase_setConnectionId(apx_connectionBase_t *self, uint32_t connectionId)
{
   if (self != 0)
   {
      self->connectionId = connectionId;
      apx_fileManager_setConnectionId(&self->fileManager, connectionId);
   }
}

uint32_t apx_connectionBase_getConnectionId(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      return self->connectionId;
   }
   return APX_INVALID_CONNECTION_ID;
}

void apx_connectionBase_getTransmitHandler(apx_connectionBase_t *self, apx_transmitHandler_t *transmitHandler)
{
   if (self != 0)
   {
      apx_fileManager_copyTransmitHandler(&self->fileManager, transmitHandler);
   }
}

uint16_t apx_connectionBase_getNumPendingEvents(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      return apx_eventLoop_numPendingEvents(&self->eventLoop);
   }
   return 0;
}

uint16_t apx_connectionBase_getNumPendingWorkerMessages(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      return apx_fileManager_getNumPendingWorkerMessages(&self->fileManager);
   }
   return 0u;
}

void* apx_connectionBase_registerEventListener(apx_connectionBase_t *self, apx_connectionEventListener_t *listener)
{
   if ( (self != 0) && (listener != 0))
   {
      void *handle = (void*) apx_connectionEventListener_clone(listener);
      if (handle != 0)
      {
         MUTEX_LOCK(self->eventListenerMutex);
         adt_list_insert(&self->connectionEventListeners, handle);
         MUTEX_UNLOCK(self->eventListenerMutex);
      }
      return handle;
   }
   return (void*) 0;
}

void apx_connectionBase_unregisterEventListener(apx_connectionBase_t *self, void *handle)
{
   if ( (self != 0) && (handle != 0))
   {
      MUTEX_LOCK(self->eventListenerMutex);
      bool isFound = adt_list_remove(&self->connectionEventListeners, handle);
      if (isFound == true)
      {
         apx_connectionEventListener_vdelete(handle);
      }
      MUTEX_UNLOCK(self->eventListenerMutex);
   }
}



void apx_connectionBase_triggerDefinitionDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->connectionEventListeners);
   while (iter != 0)
   {
      apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;
      /*
      if (eventListener->definitionDataWritten != 0)
      {
         eventListener->definitionDataWritten(eventListener->arg, nodeData, offset, len);
      }
      */
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 1 event
void apx_connectionBase_triggerInPortDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->connectionEventListeners);
   while (iter != 0)
   {
      apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;
      /*
      if (eventListener->inPortDataWritten != 0)
      {
         eventListener->inPortDataWritten(eventListener->arg, nodeData, offset, len);
      }
      */
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 1 event
void apx_connectionBase_triggerOutPortDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->connectionEventListeners);
   while (iter != 0)
   {
      apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;
      /*
      if (eventListener->outPortDataWritten != 0)
      {
         eventListener->outPortDataWritten(eventListener->arg, nodeData, offset, len);
      }
      */
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 2 event trigger
void apx_connectionBase_triggerRequirePortsConnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->connectionEventListeners);
   while (iter != 0)
   {
      apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;
      /*
      if (eventListener->requirePortsConnected != 0)
      {
         eventListener->requirePortsConnected(eventListener->arg, nodeData, portConnectionTable);
      }
      */
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 2 event trigger
void apx_connectionBase_triggerProvidePortsConnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->connectionEventListeners);
   while (iter != 0)
   {
      apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;
      /*
      if (eventListener->providePortsConnected != 0)
      {
         eventListener->providePortsConnected(eventListener->arg, nodeData, portConnectionTable);
      }
      */
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 2 event trigger
void apx_connectionBase_triggerRequirePortsDisconnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->connectionEventListeners);
   while (iter != 0)
   {
      apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;
      /*
      if (eventListener->requirePortsDisconnected != 0)
      {
         eventListener->requirePortsDisconnected(eventListener->arg, nodeData, portConnectionTable);
      }
      */
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 2 event trigger
void apx_connectionBase_triggerProvidePortsDisconnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->connectionEventListeners);
   while (iter != 0)
   {
      apx_connectionEventListener_t *eventListener = (apx_connectionEventListener_t*) iter->pItem;
      /*
      if (eventListener->providePortsDisconnected != 0)
      {
         eventListener->providePortsDisconnected(eventListener->arg, nodeData, portConnectionTable);
      }
      */
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

/*** Internal callback API ***/




#ifdef UNIT_TEST
void apx_connectionBase_runAll(apx_connectionBase_t *self)
{
   if ( (self != 0) && (self->eventHandler != 0) )
   {
      apx_eventLoop_runAll(&self->eventLoop, self->eventHandler, self->eventHandlerArg);
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_connectionBase_startWorkerThread(apx_connectionBase_t *self)
{
#ifndef UNIT_TEST
   if( self->workerThreadValid == false ){
      self->workerThreadValid = true;
# ifdef _MSC_VER
      THREAD_CREATE(self->workerThread, eventHandlerWorkThread, self, self->threadId);
      if(self->workerThread == INVALID_HANDLE_VALUE){
         self->workerThreadValid = false;
         return APX_THREAD_CREATE_ERROR;
      }
# else
      int rc = THREAD_CREATE(self->workerThread, eventHandlerWorkThread, self);
      if(rc != 0){
         self->workerThreadValid = false;
         return APX_THREAD_CREATE_ERROR;
      }
# endif //_MSC_VER
   }
#endif //UNIT_TEST
   return APX_NO_ERROR;
}

static void apx_connectionBase_stopWorkerThread(apx_connectionBase_t *self)
{
   if ( self->workerThreadValid == true )
   {
   #ifdef _MSC_VER
         DWORD result;
   #endif
         apx_eventLoop_exit(&self->eventLoop);
   #ifdef _MSC_VER
         result = WaitForSingleObject(self->workerThread, 5000);
         if (result == WAIT_TIMEOUT)
         {
            APX_LOG_ERROR("[APX_FILE_MANAGER] timeout while joining workerThread");
         }
         else if (result == WAIT_FAILED)
         {
            DWORD lastError = GetLastError();
            APX_LOG_ERROR("[APX_FILE_MANAGER]  joining workerThread failed with %d", (int)lastError);
         }
         CloseHandle(self->workerThread);
         self->workerThread = INVALID_HANDLE_VALUE;
   #else
         if (pthread_equal(pthread_self(), self->workerThread) == 0)
         {
            void *status;
            int s = pthread_join(self->workerThread, &status);
            if (s != 0)
            {
               APX_LOG_ERROR("[APX_FILE_MANAGER] pthread_join error %d\n", s);
            }
         }
         else
         {
            APX_LOG_ERROR("[APX_FILE_MANAGER] pthread_join attempted on pthread_self()\n");
         }
   #endif
   self->workerThreadValid = false;
   }
}

static apx_error_t apx_connectionBase_initTransmitHandler(apx_connectionBase_t *self)
{
   apx_transmitHandler_t handler;
   if (self->vtable.fillTransmitHandler != 0)
   {
      apx_error_t rc = self->vtable.fillTransmitHandler((void*) self, &handler);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
      apx_fileManager_setTransmitHandler(&self->fileManager, &handler);
   }
   return APX_NO_ERROR;
}
/*
static void apx_connectionBase_createNodeCompleteEvent(apx_event_t *event, apx_nodeData_t *nodeData)
{
   memset(event, 0, APX_EVENT_SIZE);
   event->evType = APX_EVENT_NODE_COMPLETE;
   event->evData1 = (void*) nodeData;
}
*/

#ifndef UNIT_TEST
static THREAD_PROTO(eventHandlerWorkThread,arg)
{
   apx_connectionBase_t *self = (apx_connectionBase_t*) arg;
   if(self != 0)
   {
      apx_eventLoop_run(&self->eventLoop, self->eventHandler, self->eventHandlerArg); //This function will not return until someone calls apx_connectionBase_stop
   }
   THREAD_RETURN(0);
}
#endif

/*
static void apx_connectionBase_handlePortConnectEvent(apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable, apx_portType_t portType)
{
   int32_t portId;
   void (*portIncCountFunc)(apx_nodeData_t*, apx_portId_t) = portType==APX_REQUIRE_PORT? apx_nodeData_incProvidePortConnectionCount : apx_nodeData_incRequirePortConnectionCount;
   for (portId=0; portId<connectionTable->numPorts; portId++)
   {
      apx_portConnectionEntry_t *entry = apx_portConnectionTable_getEntry(connectionTable, portId);
      if (entry->count > 1)
      {
         int32_t numConnections;
         int32_t i;
         adt_ary_t *array = (adt_ary_t*) entry->pAny;
         numConnections = adt_ary_length(array);
         for(i=0;i<numConnections;i++)
         {
            apx_portRef_t *remotePortRef = adt_ary_value(array, i);
            portIncCountFunc(remotePortRef->nodeData, apx_portDataRef_getPortId(remotePortRef));
         }
      }
      else if (entry->count == 1)
      {
         apx_portRef_t *remotePortRef = (apx_portRef_t*) entry->pAny;
         portIncCountFunc(remotePortRef->nodeData, apx_portDataRef_getPortId(remotePortRef));
      }
      else
      {
         //DO NOTHING
      }
   }
}

static void apx_connectionBase_handlePortDisconnectEvent(apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable, apx_portType_t portType)
{
   int32_t portId;
   void (*portDecCountFunc)(apx_nodeData_t*, apx_portId_t) = portType==APX_REQUIRE_PORT? apx_nodeData_decProvidePortConnectionCount : apx_nodeData_decRequirePortConnectionCount;
   for (portId=0; portId<connectionTable->numPorts; portId++)
   {
      apx_portConnectionEntry_t *entry = apx_portConnectionTable_getEntry(connectionTable, portId);
      if (entry->count < -1)
      {
         int32_t numConnections;
         int32_t i;
         adt_ary_t *array = (adt_ary_t*) entry->pAny;
         numConnections = adt_ary_length(array);
         for(i=0;i<numConnections;i++)
         {
            apx_portRef_t *remotePortRef = adt_ary_value(array, i);
            portDecCountFunc(remotePortRef->nodeData, apx_portDataRef_getPortId(remotePortRef));
         }
      }
      else if (entry->count == -1)
      {
         apx_portRef_t *remotePortRef = (apx_portRef_t*) entry->pAny;
         portDecCountFunc(remotePortRef->nodeData, apx_portDataRef_getPortId(remotePortRef));
      }
      else
      {
         //DO NOTHING
      }
   }
}
*/

static void apx_connectionBase_emitFileCreatedEvent(apx_connectionBase_t *self, const apx_fileInfo_t *fileInfo)
{
   if (self != 0)
   {
      apx_event_t event;
      apx_fileInfo_t *fileInfo2;
      fileInfo2 = apx_fileInfo_clone(fileInfo);
      if (fileInfo2 != 0)
      {
         apx_event_fillFileCreatedEvent(&event, self, fileInfo2);
         apx_eventLoop_append(&self->eventLoop, &event);
      }
   }
}
