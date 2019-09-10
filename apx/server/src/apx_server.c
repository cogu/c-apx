
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_server.h"
#include "apx_logging.h"
#include "apx_fileManager.h"
#include "apx_eventListener.h"
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

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_server_create(apx_server_t *self, uint16_t maxNumEvents)
{
   if (self != 0)
   {
      adt_list_create(&self->serverEventListeners, apx_serverEventListener_vdelete);
      apx_routingTable_create(&self->routingTable);
      apx_connectionManager_create(&self->connectionManager);
      adt_list_create(&self->extensionManager, apx_serverExtension_vdelete);
      apx_allocator_create(&self->allocator, maxNumEvents);
      apx_eventLoop_create(&self->eventLoop);
      self->isRunning = false;
   }
}

void apx_server_start(apx_server_t *self)
{

   if (self != 0)
   {
#ifndef UNIT_TEST
      apx_connectionManager_start(&self->connectionManager);
      apx_allocator_start(&self->allocator);
#endif
      self->isRunning = true;
   }
}

void apx_server_stop(apx_server_t *self)
{
   if ( (self != 0) && (self->isRunning) )
   {
      apx_server_shutdown_extensions(self);
#ifndef UNIT_TEST
      apx_eventLoop_exit(&self->eventLoop);
      apx_allocator_stop(&self->allocator);
      apx_connectionManager_stop(&self->connectionManager);
#endif
      self->isRunning = false;
   }
}

void apx_server_destroy(apx_server_t *self)
{
   if (self != 0)
   {
      apx_server_stop(self);
      apx_eventLoop_destroy(&self->eventLoop);
      apx_allocator_destroy(&self->allocator);
      adt_list_destroy(&self->serverEventListeners);
      apx_connectionManager_destroy(&self->connectionManager);
      apx_routingTable_destroy(&self->routingTable);
      adt_list_destroy(&self->extensionManager);
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

#ifdef UNIT_TEST

void apx_server_run(apx_server_t *self)
{
   if (self != 0)
   {
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
