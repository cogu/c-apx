
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_server.h"
#include "apx_logging.h"
#include "apx_fileManager.h"
#include "apx_eventListener.h"
#include "apx_logEvent.h"
#include <string.h>
#include <malloc.h>
#include <stdio.h> //DEBUG ONLY
#include <assert.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_LOG_LEN 1024

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_server_attach_and_start_connection(apx_server_t *self, apx_serverConnectionBase_t *newConnection);
static void apx_server_triggerConnectedEvent(apx_server_t *self, apx_serverConnectionBase_t *connection);
static void apx_server_triggerDisconnectedEvent(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
static void apx_server_triggerLogEvent(apx_server_t *self, apx_logLevel_t level, const char *label, const char *msg);
static void apx_server_initExtensions(apx_server_t *self);
static void apx_server_shutdownExtensions(apx_server_t *self);
static void apx_server_handleEvent(void *arg, apx_event_t *event);
#ifndef UNIT_TEST
static apx_error_t apx_server_startThread(apx_server_t *self);
static apx_error_t apx_server_stopThread(apx_server_t *self);
static THREAD_PROTO(threadTask,arg);
#endif

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_server_create(apx_server_t *self)
{
   if (self != 0)
   {
      adt_list_create(&self->serverEventListeners, apx_serverEventListener_vdelete);
      apx_portSignatureMap_create(&self->portSignatureMap);
      apx_connectionManager_create(&self->connectionManager);
      adt_list_create(&self->extensionManager, apx_serverExtension_vdelete);
      adt_ary_create(&self->modifiedNodes, (void(*)(void*)) 0);
      soa_init(&self->soa);
      apx_eventLoop_create(&self->eventLoop);
      self->isEventThreadValid = false;
      MUTEX_INIT(self->eventLoopLock);
      MUTEX_INIT(self->globalLock);
      SPINLOCK_INIT(self->eventListenerLock);
#ifdef _MSC_VER
      self->threadId = 0u;
#endif
   }
}

void apx_server_destroy(apx_server_t *self)
{
   if (self != 0)
   {
      apx_server_stop(self);
      MUTEX_LOCK(self->globalLock);
      soa_destroy(&self->soa);
      adt_list_destroy(&self->extensionManager);
      adt_ary_destroy(&self->modifiedNodes);
      SPINLOCK_ENTER(self->eventListenerLock);
      adt_list_destroy(&self->serverEventListeners);
      SPINLOCK_LEAVE(self->eventListenerLock);
      apx_connectionManager_destroy(&self->connectionManager);
      apx_portSignatureMap_destroy(&self->portSignatureMap);
      MUTEX_UNLOCK(self->globalLock);
      apx_eventLoop_destroy(&self->eventLoop);
      MUTEX_DESTROY(self->eventLoopLock);
      MUTEX_DESTROY(self->globalLock);
      SPINLOCK_DESTROY(self->eventListenerLock);
   }
}

apx_server_t *apx_server_new(void)
{
   apx_server_t *self = (apx_server_t*) malloc(sizeof(apx_server_t));
   if (self != 0)
   {
      apx_server_create(self);
   }
   return self;
}

void apx_server_delete(apx_server_t *self)
{
   if (self != 0)
   {
      apx_server_destroy(self);
      free(self);
   }
}

void apx_server_start(apx_server_t *self)
{

   if( self != 0 )
   {
      apx_server_initExtensions(self);
#ifndef UNIT_TEST
      apx_connectionManager_start(&self->connectionManager);
      if (self->isEventThreadValid == false)
      {
         apx_server_startThread(self);
      }
#endif
   }
}

void apx_server_stop(apx_server_t *self)
{
   if( self != 0)
   {

#ifndef UNIT_TEST
      apx_connectionManager_stop(&self->connectionManager);
#endif
      apx_server_shutdownExtensions(self);
#ifndef UNIT_TEST
      apx_eventLoop_exit(&self->eventLoop);
      if (self->isEventThreadValid)
      {
         apx_server_stopThread(self);
      }
#endif
   }
}





void* apx_server_registerEventListener(apx_server_t *self, apx_serverEventListener_t *eventListener)
{
   if ( (self != 0) && (eventListener != 0))
   {
      void *handle = (void*) apx_serverEventListener_clone(eventListener);
      if (handle != 0)
      {
         SPINLOCK_ENTER(self->eventListenerLock);
         adt_list_insert(&self->serverEventListeners, handle);
         SPINLOCK_LEAVE(self->eventListenerLock);
      }
      return handle;
   }
   return (void*) 0;
}

void apx_server_unregisterEventListener(apx_server_t *self, void *handle)
{
   if ( (self != 0) && (handle != 0))
   {
      bool isFound;
      SPINLOCK_ENTER(self->eventListenerLock);
      isFound = adt_list_remove(&self->serverEventListeners, handle);
      SPINLOCK_LEAVE(self->eventListenerLock);
      if (isFound == true)
      {
         apx_serverEventListener_vdelete(handle);
      }
   }
}

void apx_server_acceptConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection)
{
   if ( (self != 0) && (serverConnection != 0))
   {
      apx_server_attach_and_start_connection(self, serverConnection);
   }
}

void apx_server_detachConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection)
{
   if ( (self != 0) && (serverConnection != 0))
   {
      apx_connectionManager_detach(&self->connectionManager, serverConnection);
      apx_serverConnectionBase_disconnectNotify(serverConnection);
      apx_server_triggerDisconnectedEvent(self, serverConnection);
   }
}

apx_error_t apx_server_addExtension(apx_server_t *self, const char *name, apx_serverExtensionHandler_t *handler, dtl_dv_t *config)
{
   if ( (self != 0) && (handler != 0) )
   {
      apx_serverExtension_t *extension = apx_serverExtension_new(name, handler, config);
      if (extension == 0)
      {
         return APX_MEM_ERROR;
      }
      adt_list_insert(&self->extensionManager, (void*) extension);
      if (config != 0)
      {
         dtl_dv_inc_ref(config);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_server_logEvent(apx_server_t *self, apx_logLevel_t level, const char *label, const char *msg)
{
   if ( (self != 0) && (level <= APX_MAX_LOG_LEVEL) && (msg != 0) )
   {
      apx_event_t event;
      char *labelStr = 0;
      adt_str_t *msgStr = adt_str_new_cstr(msg);

      if (msgStr == 0)
      {
         return;
      }

      if (label != 0)
      {
         size_t labelSize = strlen(label);
         if (labelSize > APX_LOG_LABEL_MAX_LEN)
         {
            labelSize = APX_LOG_LABEL_MAX_LEN;
         }
         MUTEX_LOCK(self->eventLoopLock);
         labelStr = soa_alloc(&self->soa, labelSize+1);
         MUTEX_UNLOCK(self->eventLoopLock);
         if (labelStr == 0)
         {
            adt_str_delete(msgStr);
            return;
         }
         memcpy(labelStr, label, labelSize);
         labelStr[labelSize] = 0;
      }
      memset(&event, 0, sizeof(event));
      apx_logEvent_pack(&event, level, labelStr, msgStr);
      apx_eventLoop_append(&self->eventLoop, &event);
   }
}

/**
 * Acquires the server global lock
 */
void apx_server_takeGlobalLock(apx_server_t *self)
{
   if (self != 0)
   {
      MUTEX_LOCK(self->globalLock);
   }
}

/**
 * Releases the server global lock
 */
void apx_server_releaseGlobalLock(apx_server_t *self)
{
   if (self != 0)
   {
      MUTEX_UNLOCK(self->globalLock);
   }
}

apx_error_t apx_server_connectNodeInstanceProvidePorts(apx_server_t *self, apx_nodeInstance_t *nodeInstance)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      return apx_portSignatureMap_connectProvidePorts(&self->portSignatureMap, nodeInstance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_connectNodeInstanceRequirePorts(apx_server_t *self, apx_nodeInstance_t *nodeInstance)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      return apx_portSignatureMap_connectRequirePorts(&self->portSignatureMap, nodeInstance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_disconnectNodeInstanceProvidePorts(apx_server_t *self, apx_nodeInstance_t *nodeInstance)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      return apx_portSignatureMap_disconnectProvidePorts(&self->portSignatureMap, nodeInstance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_disconnectNodeInstanceRequirePorts(apx_server_t *self, apx_nodeInstance_t *nodeInstance)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {
      return apx_portSignatureMap_disconnectRequirePorts(&self->portSignatureMap, nodeInstance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Is is assumed that the server global lock is held by the caller of this function
 */
apx_error_t apx_server_processRequirePortConnectorChanges(apx_server_t *self, apx_nodeInstance_t *requireNodeInstance, apx_portConnectorChangeTable_t *connectorChanges)
{
   if ( (requireNodeInstance != 0) && (connectorChanges != 0 ) )
   {
      apx_portCount_t numRequirePorts;
      apx_portId_t requirePortId;
//      apx_requirePortDataState_t requirePortState;
//      requirePortState = apx_nodeInstance_getRequirePortDataState(requireNodeInstance);
      numRequirePorts = apx_nodeInstance_getNumRequirePorts(requireNodeInstance);
      assert(connectorChanges->numPorts == numRequirePorts);
      for (requirePortId = 0u; requirePortId < numRequirePorts; requirePortId++)
      {
         apx_portRef_t *requirePortRef;
         apx_portConnectorChangeEntry_t *entry;
         requirePortRef = apx_nodeInstance_getRequirePortRef(requireNodeInstance, requirePortId);
         entry = apx_portConnectorChangeTable_getEntry(connectorChanges, requirePortId);
         assert(requirePortRef != 0);
         assert(entry != 0);
         if (entry->count > 0)
         {
            if (entry->count == 1)
            {
               apx_error_t rc;
               apx_portRef_t *providePortRef = entry->data.portRef;
               assert(providePortRef != 0);
               rc = apx_nodeInstance_handleRequirePortWasConnectedToProvidePort(requirePortRef, providePortRef);
               if (rc != APX_NO_ERROR)
               {
                  return rc;
               }
            }
            else
            {
               //Multiple providers are available. This needs to be handled later
               return APX_NOT_IMPLEMENTED_ERROR;
            }
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Is is assumed that the server global lock is held by the caller of this function
 */
apx_error_t apx_server_processProvidePortConnectorChanges(apx_server_t *self, apx_nodeInstance_t *provideNodeInstance, apx_portConnectorChangeTable_t *connectorChanges)
{
   if ( (provideNodeInstance != 0) && (connectorChanges != 0 ) )
   {
      apx_portCount_t numProvidePorts;
      apx_portId_t providePortId;
      numProvidePorts = apx_nodeInstance_getNumProvidePorts(provideNodeInstance);
      assert(connectorChanges->numPorts == numProvidePorts);
      apx_nodeInstance_lockPortConnectorTable(provideNodeInstance);
      for (providePortId = 0u; providePortId < numProvidePorts; providePortId++)
      {
         apx_portRef_t *providePortRef;
         apx_portConnectorChangeEntry_t *entry;
         entry = apx_portConnectorChangeTable_getEntry(connectorChanges, providePortId);
         providePortRef = apx_nodeInstance_getProvidePortRef(provideNodeInstance, providePortId);
         assert(entry != 0);
         assert(providePortRef != 0);
         if (entry->count > 0)
         {
            if (entry->count == 1)
            {
               apx_error_t rc;
               apx_portRef_t *requirePortRef = entry->data.portRef;
               assert(requirePortRef != 0);
               rc = apx_nodeInstance_handleProvidePortWasConnectedToRequirePort(providePortRef, requirePortRef);
               if (rc != APX_NO_ERROR)
               {
                  apx_nodeInstance_unlockPortConnectorTable(provideNodeInstance);
                  return rc;
               }
            }
            else
            {
               int32_t i;
               for(i=0; i < entry->count; i++)
               {
                  apx_error_t rc;
                  apx_portRef_t *requirePortRef = adt_ary_value(entry->data.array, i);
                  assert(requirePortRef != 0);
                  rc = apx_nodeInstance_handleProvidePortWasConnectedToRequirePort(providePortRef, requirePortRef);
                  if (rc != APX_NO_ERROR)
                  {
                     apx_nodeInstance_unlockPortConnectorTable(provideNodeInstance);
                     return rc;
                  }
               }
            }
         }
      }
      apx_nodeInstance_unlockPortConnectorTable(provideNodeInstance);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Note: Should only be used when caller holds globalLock
 */
apx_error_t apx_server_insertModifiedNode(apx_server_t *self, apx_nodeInstance_t *nodeInstance)
{
   if (self != 0)
   {
      adt_error_t rc = adt_ary_push_unique(&self->modifiedNodes, (void*) nodeInstance);
      if (rc == ADT_MEM_ERROR)
      {
         return APX_MEM_ERROR;
      }
      else if(rc != APX_NO_ERROR)
      {
         return APX_GENERIC_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Note: Should only be used when caller holds globalLock
 */
adt_ary_t *apx_server_getModifiedNodes(const apx_server_t *self)
{
   if (self != 0)
   {
      return (adt_ary_t*) &self->modifiedNodes;
   }
   return (adt_ary_t*) 0;
}

/**
 * Note: Should only be used when caller holds globalLock
 */
void apx_server_clearPortConnectorChanges(apx_server_t *self)
{
   if (self != 0)
   {
      int32_t i;
      int32_t numNodes = adt_ary_length(&self->modifiedNodes);
      for(i=0; i<numNodes; i++)
      {
         apx_nodeInstance_t *nodeInstance = (apx_nodeInstance_t*) adt_ary_value(&self->modifiedNodes, i);
         assert(nodeInstance != 0);
         apx_nodeInstance_clearProvidePortConnectorChanges(nodeInstance, true);
         apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance, true);
      }
      if (numNodes > 0)
      {
         adt_ary_clear(&self->modifiedNodes);
      }
   }
}


#ifdef UNIT_TEST
void apx_server_run(apx_server_t *self)
{
   if (self != 0)
   {
      apx_eventLoop_runAll(&self->eventLoop, apx_server_handleEvent, (void*) self);
      apx_connectionManager_run(&self->connectionManager);
   }
}

apx_serverConnectionBase_t *apx_server_getLastConnection(apx_server_t *self)
{
   if (self != 0)
   {
      return apx_connectionManager_getLastConnection(&self->connectionManager);
   }
   return (apx_serverConnectionBase_t*) 0;
}

apx_portSignatureMap_t *apx_server_getPortSignatureMap(apx_server_t *self)
{
   if (self != 0)
   {
      return &self->portSignatureMap;
   }
   return (apx_portSignatureMap_t*) 0;
}

#endif


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_server_attach_and_start_connection(apx_server_t *self, apx_serverConnectionBase_t *newConnection)
{
   if (apx_connectionManager_getNumConnections(&self->connectionManager) < APX_SERVER_MAX_CONCURRENT_CONNECTIONS)
   {
      apx_connectionManager_attach(&self->connectionManager, newConnection);
      apx_serverConnectionBase_setServer(newConnection, self);
      apx_server_triggerConnectedEvent(self, newConnection);
      apx_connectionBase_start(&newConnection->base);
   }
   else
   {
      printf("[SERVER] Concurrent connection limit exceeded\n");
   }
}

static void apx_server_triggerConnectedEvent(apx_server_t *self, apx_serverConnectionBase_t *serverConnection)
{
   assert(self != 0);
   assert(serverConnection != 0);
   SPINLOCK_ENTER(self->eventListenerLock);
   adt_list_elem_t *iter = adt_list_iter_first(&self->serverEventListeners);
   while(iter != 0)
   {
      apx_serverEventListener_t *listener = (apx_serverEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->serverConnect1 != 0) )
      {
         listener->serverConnect1(listener->arg, serverConnection);
      }
      iter = adt_list_iter_next(iter);
   }
   SPINLOCK_LEAVE(self->eventListenerLock);
}

static void apx_server_triggerDisconnectedEvent(apx_server_t *self, apx_serverConnectionBase_t *serverConnection)
{
   assert(self != 0);
   assert(serverConnection != 0);
   SPINLOCK_ENTER(self->eventListenerLock);
   adt_list_elem_t *iter = adt_list_iter_first(&self->serverEventListeners);
   while(iter != 0)
   {
      apx_serverEventListener_t *listener = (apx_serverEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->serverDisconnect1 != 0) )
      {
         listener->serverDisconnect1(listener->arg, serverConnection);
      }
      iter = adt_list_iter_next(iter);
   }
   SPINLOCK_LEAVE(self->eventListenerLock);
}

static void apx_server_triggerLogEvent(apx_server_t *self, apx_logLevel_t level, const char *label, const char *msg)
{
   adt_list_elem_t *iter = adt_list_iter_first(&self->serverEventListeners);
   while(iter != 0)
   {
/*
      apx_serverEventListener_t *listener = (apx_serverEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->serverConnected != 0) )
      {
         listener->logEvent(listener->arg, level, label, msg);
      }
*/
      iter = adt_list_iter_next(iter);
   }
}

static void apx_server_initExtensions(apx_server_t *self)
{
   if  (self != 0)
   {
      adt_list_elem_t *iter = adt_list_iter_first(&self->extensionManager);
      while(iter != 0)
      {
         apx_serverExtension_t *extension = (apx_serverExtension_t*) iter->pItem;
         if (extension->handler.init != 0)
         {
            extension->handler.init(self, extension->config);
            if (extension->config != 0)
            {
               dtl_dv_dec_ref(extension->config);
               extension->config = (dtl_dv_t*) 0;
            }
            if (extension->name != 0)
            {
/*               char msg[MAX_LOG_LEN];
               sprintf(msg, "Started extension %s", extension->name);
               apx_server_logEvent(self, APX_LOG_LEVEL_INFO, "SERVER", msg);*/
            }
         }
         iter = adt_list_iter_next(iter);
      }
   }
}


static void apx_server_shutdownExtensions(apx_server_t *self)
{
   if  (self != 0)
   {
      adt_list_elem_t *iter = adt_list_iter_first(&self->extensionManager);
      while(iter != 0)
      {
        apx_serverExtension_t *extension = (apx_serverExtension_t*) iter->pItem;
        if (extension->handler.shutdown != 0)
        {
           extension->handler.shutdown();
        }
        iter = adt_list_iter_next(iter);
      }
   }
}

static void apx_server_handleEvent(void *arg, apx_event_t *event)
{
   apx_server_t *self = (apx_server_t*) arg;
   if ( (self != 0) && (event != 0) )
   {
      apx_logLevel_t level;
      size_t labelSize;
      char *label;
      adt_str_t *str;
      const char *msg;
      switch(event->evType)
      {
      case APX_EVENT_LOG_EVENT:
         apx_logEvent_unpack(event, &level, &label, &str);
         msg = adt_str_cstr(str);
         if (label != 0)
         {
            labelSize = strlen(label);
            apx_server_triggerLogEvent(self, level, label, msg);
            MUTEX_LOCK(self->eventLoopLock);
            soa_free(&self->soa, label, labelSize+1);
            MUTEX_UNLOCK(self->eventLoopLock);
         }
         else
         {
            printf("%s\n", msg);
         }
         adt_str_delete(str);
         break;
      }
   }
}
#ifndef UNIT_TEST
static apx_error_t apx_server_startThread(apx_server_t *self)
{
   self->isEventThreadValid = true;
#ifdef _MSC_VER
   THREAD_CREATE(self->eventThread, threadTask, self, self->threadId);
   if(self->eventThread == INVALID_HANDLE_VALUE)
   {
      self->eventThreadValid = false;
      return APX_THREAD_CREATE_ERROR;
   }
#else
   int rc = THREAD_CREATE(self->eventThread,threadTask,self);
   if(rc != 0)
   {
      self->isEventThreadValid = false;
      return APX_THREAD_CREATE_ERROR;
   }
#endif
   return APX_NO_ERROR;
}

static apx_error_t apx_server_stopThread(apx_server_t *self)
{
   if (self->isEventThreadValid)
   {
#ifdef _MSC_VER
      result = WaitForSingleObject(self->eventThread, 5000);
      if (result == WAIT_TIMEOUT)
      {
         return APX_THREAD_JOIN_TIMEOUT_ERROR;
      }
      else if (result == WAIT_FAILED)
      {
         return APX_THREAD_JOIN_ERROR;
      }
      CloseHandle(self->eventThread);
      self->eventThread = INVALID_HANDLE_VALUE;
#else
      if(pthread_equal(pthread_self(),self->eventThread) == 0)
      {
         void *status;
         int s = pthread_join(self->eventThread, &status);
         if (s != 0)
         {
            return APX_THREAD_JOIN_ERROR;
         }
      }
      else
      {
         return APX_THREAD_JOIN_ERROR;
      }
#endif
   self->isEventThreadValid = false;
   }
   return APX_NO_ERROR;
}

static THREAD_PROTO(threadTask,arg)
{
   apx_server_t *self = (apx_server_t*) arg;
   if (self != 0)
   {
      apx_eventLoop_run(&self->eventLoop, apx_server_handleEvent, (void*) self);
   }
   THREAD_RETURN(0);
}
#endif
