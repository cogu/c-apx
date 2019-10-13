/*****************************************************************************
* \file      apx_connectionBase.c
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
#include "apx_connectionBase.h"
#include "apx_portDataRef.h"
#include "apx_nodeData.h"
#include "apx_logging.h"
#include "apx_portConnectionTable.h"
#include <string.h>
#include <stdio.h> //DEBUG only
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
static void apx_connectionBase_createNodeCompleteEvent(apx_event_t *event, apx_nodeData_t *nodeData);
static void apx_connectionBase_handlePortConnectEvent(apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable, apx_portType_t portType);
static void apx_connectionBase_handlePortDisconnectEvent(apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable, apx_portType_t portType);

#ifndef UNIT_TEST
static THREAD_PROTO(eventHandlerWorkThread,arg);
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_connectionBaseVTable_create(apx_connectionBaseVTable_t *self, void (*destructor)(void *arg), void (*start)(void *arg), void (*close)(void *arg))
{
   if (self != 0)
   {
      memset(self, 0, sizeof(apx_connectionBaseVTable_t));
      self->destructor = destructor;
      self->start = start;
      self->close = close;
   }
}

apx_error_t apx_connectionBase_create(apx_connectionBase_t *self, apx_mode_t mode, apx_connectionBaseVTable_t *vtable)
{
   if (self != 0)
   {
      adt_buf_err_t bufResult;
      int8_t i8result;
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
      MUTEX_INIT(self->eventListenerMutex);
      adt_list_create(&self->nodeDataEventListeners, apx_nodeDataEventListener_vdelete);

      apx_nodeDataManager_create(&self->nodeDataManager, mode);
#ifdef _WIN32
      self->workerThread = INVALID_HANDLE_VALUE;
#else
      self->workerThread = 0;
#endif
      self->workerThreadValid=false;
      bufResult = apx_eventLoop_create(&self->eventLoop);
      if (bufResult != BUF_E_OK)
      {
         apx_nodeDataManager_destroy(&self->nodeDataManager);
         return APX_MEM_ERROR;
      }
      i8result = apx_fileManager_create(&self->fileManager, mode, self);
      if (i8result != 0)
      {
         apx_nodeDataManager_destroy(&self->nodeDataManager);
      }
      return i8result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_connectionBase_destroy(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      apx_fileManager_destroy(&self->fileManager);
      apx_eventLoop_destroy(&self->eventLoop);
      apx_nodeDataManager_destroy(&self->nodeDataManager);
      MUTEX_DESTROY(self->eventListenerMutex);
      adt_list_destroy(&self->nodeDataEventListeners);
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

void apx_connectionBase_emitFileManagerHeaderCompleteEvent(apx_connectionBase_t *self)
{
   if (self != 0)
   {
      apx_event_t event;
      apx_fileManager_createHeaderCompleteEvent(&event, &self->fileManager);
      apx_eventLoop_append(&self->eventLoop, &event);
   }
}

void apx_connectionBase_emitFileCreatedEvent(apx_connectionBase_t *self, struct apx_file2_tag *file, const void *caller)
{
   if (self != 0)
   {
      apx_event_t event;
      apx_fileManager_createFileCreatedEvent(&event, &self->fileManager, file, caller);
      apx_eventLoop_append(&self->eventLoop, &event);
   }
}

void apx_connectionBase_emitFileRevokedEvent(apx_connectionBase_t *self, struct apx_file2_tag *file, const void *caller)
{
   if (self != 0)
   {
      apx_event_t event;
      apx_fileManager_createFileRevokedEvent(&event, &self->fileManager, file, caller);
      apx_eventLoop_append(&self->eventLoop, &event);
   }
}

void apx_connectionBase_emitFileOpenedEvent(apx_connectionBase_t *self, struct apx_file2_tag *file, const void *caller)
{
   if (self != 0)
   {
      apx_event_t event;
      apx_fileManager_createFileOpenedEvent(&event, &self->fileManager, file, caller);
      apx_eventLoop_append(&self->eventLoop, &event);
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
      apx_event_t event;
      apx_connectionBase_createNodeCompleteEvent(&event, nodeData);
      apx_eventLoop_append(&self->eventLoop, &event);
   }
}

void apx_connectionBase_defaultEventHandler(apx_connectionBase_t *self, apx_event_t *event)
{
   if ( (event->evFlags & APX_EVENT_FLAG_FILE_MANAGER_EVENT) != 0)
   {
      //Play this event through the event handler of the fileManager
      apx_fileManager_t *fileManager = (apx_fileManager_t*) event->evData1;
      apx_fileManager_eventHandler(fileManager, event);
   }
   else
   {
      apx_portConnectionTable_t *connectionTable;
      apx_nodeData_t *nodeData;


      switch(event->evType)
      {
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
      fileManager_setID(&self->fileManager, connectionId);
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
      apx_fileManager_getTransmitHandler(&self->fileManager, transmitHandler);
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

void* apx_connectionBase_registerNodeDataEventListener(apx_connectionBase_t *self, apx_nodeDataEventListener_t *listener)
{
   if ( (self != 0) && (listener != 0))
   {
      void *handle = (void*) apx_nodeDataEventListener_clone(listener);
      if (handle != 0)
      {
         MUTEX_LOCK(self->eventListenerMutex);
         adt_list_insert(&self->nodeDataEventListeners, handle);
         MUTEX_UNLOCK(self->eventListenerMutex);
      }
      return handle;
   }
   return (void*) 0;
}

void apx_connectionBase_unregisterNodeDataEventListener(apx_connectionBase_t *self, void *handle)
{
   if ( (self != 0) && (handle != 0))
   {
      MUTEX_LOCK(self->eventListenerMutex);
      bool isFound = adt_list_remove(&self->nodeDataEventListeners, handle);
      if (isFound == true)
      {
         apx_nodeDataEventListener_vdelete(handle);
      }
      MUTEX_UNLOCK(self->eventListenerMutex);
   }
}

//Type 1 event
void apx_connectionBase_triggerDefinitionDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->nodeDataEventListeners);
   while (iter != 0)
   {
      apx_nodeDataEventListener_t *eventListener = (apx_nodeDataEventListener_t*) iter->pItem;
      if (eventListener->definitionDataWritten != 0)
      {
         eventListener->definitionDataWritten(eventListener->arg, nodeData, offset, len);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 1 event
void apx_connectionBase_triggerInPortDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->nodeDataEventListeners);
   while (iter != 0)
   {
      apx_nodeDataEventListener_t *eventListener = (apx_nodeDataEventListener_t*) iter->pItem;
      if (eventListener->inPortDataWritten != 0)
      {
         eventListener->inPortDataWritten(eventListener->arg, nodeData, offset, len);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 1 event
void apx_connectionBase_triggerOutPortDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->nodeDataEventListeners);
   while (iter != 0)
   {
      apx_nodeDataEventListener_t *eventListener = (apx_nodeDataEventListener_t*) iter->pItem;
      if (eventListener->outPortDataWritten != 0)
      {
         eventListener->outPortDataWritten(eventListener->arg, nodeData, offset, len);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 2 event trigger
void apx_connectionBase_triggerRequirePortsConnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->nodeDataEventListeners);
   while (iter != 0)
   {
      apx_nodeDataEventListener_t *eventListener = (apx_nodeDataEventListener_t*) iter->pItem;
      if (eventListener->requirePortsConnected != 0)
      {
         eventListener->requirePortsConnected(eventListener->arg, nodeData, portConnectionTable);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 2 event trigger
void apx_connectionBase_triggerProvidePortsConnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->nodeDataEventListeners);
   while (iter != 0)
   {
      apx_nodeDataEventListener_t *eventListener = (apx_nodeDataEventListener_t*) iter->pItem;
      if (eventListener->providePortsConnected != 0)
      {
         eventListener->providePortsConnected(eventListener->arg, nodeData, portConnectionTable);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 2 event trigger
void apx_connectionBase_triggerRequirePortsDisconnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->nodeDataEventListeners);
   while (iter != 0)
   {
      apx_nodeDataEventListener_t *eventListener = (apx_nodeDataEventListener_t*) iter->pItem;
      if (eventListener->requirePortsDisconnected != 0)
      {
         eventListener->requirePortsDisconnected(eventListener->arg, nodeData, portConnectionTable);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}

//Type 2 event trigger
void apx_connectionBase_triggerProvidePortsDisconnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable)
{
   adt_list_elem_t *iter;
   MUTEX_LOCK(self->eventListenerMutex);
   iter = adt_list_iter_first(&self->nodeDataEventListeners);
   while (iter != 0)
   {
      apx_nodeDataEventListener_t *eventListener = (apx_nodeDataEventListener_t*) iter->pItem;
      if (eventListener->providePortsDisconnected != 0)
      {
         eventListener->providePortsDisconnected(eventListener->arg, nodeData, portConnectionTable);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->eventListenerMutex);
}




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

static void apx_connectionBase_createNodeCompleteEvent(apx_event_t *event, apx_nodeData_t *nodeData)
{
   memset(event, 0, APX_EVENT_SIZE);
   event->evType = APX_EVENT_NODE_COMPLETE;
   event->evData1 = (void*) nodeData;
}

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
            apx_portDataRef_t *remotePortRef = adt_ary_value(array, i);
            portIncCountFunc(remotePortRef->nodeData, apx_portDataRef_getPortId(remotePortRef));
         }
      }
      else if (entry->count == 1)
      {
         apx_portDataRef_t *remotePortRef = (apx_portDataRef_t*) entry->pAny;
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
            apx_portDataRef_t *remotePortRef = adt_ary_value(array, i);
            portDecCountFunc(remotePortRef->nodeData, apx_portDataRef_getPortId(remotePortRef));
         }
      }
      else if (entry->count == -1)
      {
         apx_portDataRef_t *remotePortRef = (apx_portDataRef_t*) entry->pAny;
         portDecCountFunc(remotePortRef->nodeData, apx_portDataRef_getPortId(remotePortRef));
      }
      else
      {
         //DO NOTHING
      }
   }
}
