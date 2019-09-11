
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_server.h"
#include "apx_logging.h"
#include "apx_fileManager.h"
#include "apx_eventListener.h"
#include "apx_logEvent.h"
#include <string.h>
#include <assert.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_server_attach_and_start_connection(apx_server_t *self, apx_serverConnectionBase_t *newConnection);
static void apx_server_triggerConnectedEvent(apx_server_t *self, apx_serverConnectionBase_t *connection);
static void apx_server_triggerDisconnectedEvent(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
static void apx_server_shutdown_extensions(apx_server_t *self);
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
      apx_routingTable_create(&self->routingTable);
      apx_connectionManager_create(&self->connectionManager);
      adt_list_create(&self->extensionManager, apx_serverExtension_vdelete);
      soa_init(&self->soa);
      apx_eventLoop_create(&self->eventLoop);
      self->isWorkerThreadValid = false;
      MUTEX_INIT(self->mutex);
#ifdef _MSC_VER
      self->threadId = 0u;
#endif
   }
}

void apx_server_start(apx_server_t *self)
{

   if( self != 0 )
   {
#ifndef UNIT_TEST
      apx_connectionManager_start(&self->connectionManager);
      if (self->isWorkerThreadValid == false)
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
      apx_server_shutdown_extensions(self);
#ifndef UNIT_TEST
      apx_eventLoop_exit(&self->eventLoop);
      if (self->isWorkerThreadValid)
      {
         apx_server_stopThread(self);
      }
#endif
   }
}

void apx_server_destroy(apx_server_t *self)
{
   if (self != 0)
   {
      apx_server_stop(self);
      apx_eventLoop_destroy(&self->eventLoop);
      soa_destroy(&self->soa);
      adt_list_destroy(&self->serverEventListeners);
      apx_connectionManager_destroy(&self->connectionManager);
      apx_routingTable_destroy(&self->routingTable);
      adt_list_destroy(&self->extensionManager);
      MUTEX_DESTROY(self->mutex);
   }
}

void* apx_server_registerEventListener(apx_server_t *self, apx_serverEventListener_t *eventListener)
{
   if ( (self != 0) && (eventListener != 0))
   {
      void *handle = (void*) apx_serverEventListener_clone(eventListener);
      if (handle != 0)
      {
         //TODO: Add multi-thread lock
         adt_list_insert(&self->serverEventListeners, handle);
      }
      return handle;
   }
   return (void*) 0;
}

void apx_server_unregisterEventListener(apx_server_t *self, void *handle)
{
   if ( (self != 0) && (handle != 0))
   {
      //TODO: Add multi-thread lock
      bool isFound = adt_list_remove(&self->serverEventListeners, handle);
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

void apx_server_closeConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection)
{
   if ( (self != 0) && (serverConnection != 0))
   {
      apx_server_triggerDisconnectedEvent(self, serverConnection);
      apx_connectionManager_closeConnection(&self->connectionManager, serverConnection);
   }
}

apx_routingTable_t* apx_server_getRoutingTable(apx_server_t *self)
{
   if (self != 0)
   {
      return &self->routingTable;
   }
   return (apx_routingTable_t*) 0;
}

apx_error_t apx_server_addExtension(apx_server_t *self, apx_serverExtension_t *extension, dtl_dv_t *config)
{
   if ( (self != 0) && (extension != 0) )
   {
      apx_serverExtension_t *clone = apx_serverExtension_clone(extension);
      if (clone == 0)
      {
         return APX_MEM_ERROR;
      }
      adt_list_insert(&self->extensionManager, (void*) clone);
      if (clone->init != 0)
      {
         clone->init(self, config);
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
         MUTEX_LOCK(self->mutex);
         labelStr = soa_alloc(&self->soa, labelSize+1);
         MUTEX_UNLOCK(self->mutex);
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
#endif


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_server_attach_and_start_connection(apx_server_t *self, apx_serverConnectionBase_t *newConnection)
{
   if (apx_connectionManager_getNumConnections(&self->connectionManager) < APX_SERVER_MAX_CONCURRENT_CONNECTIONS)
   {
      apx_connectionManager_attach(&self->connectionManager, newConnection);
      apx_server_triggerConnectedEvent(self, newConnection);
      apx_connectionBase_start(&newConnection->base);
   }
}

static void apx_server_triggerConnectedEvent(apx_server_t *self, apx_serverConnectionBase_t *serverConnection)
{
   adt_list_elem_t *iter = adt_list_iter_first(&self->serverEventListeners);
   while(iter != 0)
   {
      apx_serverEventListener_t *listener = (apx_serverEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->serverConnected != 0) )
      {
         listener->serverConnected(listener->arg, serverConnection);
      }
      iter = adt_list_iter_next(iter);
   }
}

static void apx_server_triggerDisconnectedEvent(apx_server_t *self, apx_serverConnectionBase_t *serverConnection)
{
   adt_list_elem_t *iter = adt_list_iter_first(&self->serverEventListeners);
   while(iter != 0)
   {
      apx_serverEventListener_t *listener = (apx_serverEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->serverDisconnected != 0) )
      {
         listener->serverDisconnected(listener->arg, serverConnection);
      }
      iter = adt_list_iter_next(iter);
   }
}

static void apx_server_shutdown_extensions(apx_server_t *self)
{
   if  (self != 0)
   {
      adt_list_elem_t *iter = adt_list_iter_first(&self->extensionManager);
      while(iter != 0)
      {
        apx_serverExtension_t *extension = (apx_serverExtension_t*) iter->pItem;
        if (extension->shutdown != 0)
        {
           extension->shutdown();
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
            printf("[%s] %s\n", label, msg);
            MUTEX_LOCK(self->mutex);
            soa_free(&self->soa, label, labelSize+1);
            MUTEX_UNLOCK(self->mutex);
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
   self->isWorkerThreadValid = true;
#ifdef _MSC_VER
   THREAD_CREATE(self->workerThread, threadTask, self, self->threadId);
   if(self->workerThread == INVALID_HANDLE_VALUE)
   {
      self->workerThreadValid = false;
      return APX_THREAD_CREATE_ERROR;
   }
#else
   int rc = THREAD_CREATE(self->workerThread,threadTask,self);
   if(rc != 0)
   {
      self->isWorkerThreadValid = false;
      return APX_THREAD_CREATE_ERROR;
   }
#endif
   return APX_NO_ERROR;
}

static apx_error_t apx_server_stopThread(apx_server_t *self)
{
   if (self->isWorkerThreadValid)
   {
#ifdef _MSC_VER
      result = WaitForSingleObject(self->workerThread, 5000);
      if (result == WAIT_TIMEOUT)
      {
         return APX_THREAD_JOIN_TIMEOUT_ERROR;
      }
      else if (result == WAIT_FAILED)
      {
         return APX_THREAD_JOIN_ERROR;
      }
      CloseHandle(self->workerThread);
      self->workerThread = INVALID_HANDLE_VALUE;
#else
      if(pthread_equal(pthread_self(),self->workerThread) == 0)
      {
         void *status;
         int s = pthread_join(self->workerThread, &status);
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
   self->isWorkerThreadValid = false;
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
