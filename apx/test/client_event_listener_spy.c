//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "client_event_listener_spy.h"
#include "apx/client_connection_base.h"
#include "apx/event_listener.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_clientEventListenerSpy_onConnect(void *arg, apx_clientConnectionBase_t *clientConnection);
static void apx_clientEventListenerSpy_onDisconnect(void *arg, apx_clientConnectionBase_t *clientConnection);
static void apx_clientEventListenerSpy_onHeaderAccepted(void *arg, apx_connectionBase_t *clientConnection);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_clientEventListenerSpy_create(apx_clientEventListenerSpy_t *self)
{
   if (self != 0)
   {
      self->connectCount = 0u;
      self->disconnectCount = 0u;
      self->headerAcceptedCount = 0u;
   }
}

void apx_clientEventListenerSpy_destroy(apx_clientEventListenerSpy_t *self)
{
   //Nothing to do
}

apx_clientEventListenerSpy_t* apx_clientEventListenerSpy_new(void)
{
   apx_clientEventListenerSpy_t *self = (apx_clientEventListenerSpy_t*) malloc(sizeof(apx_clientEventListenerSpy_t));
   if (self != 0)
   {
      apx_clientEventListenerSpy_create(self);
   }
   return self;
}

void apx_clientEventListenerSpy_delete(apx_clientEventListenerSpy_t *self)
{
   if (self != 0)
   {
      apx_clientEventListenerSpy_destroy(self);
      free(self);
   }
}

void* apx_clientEventListenerSpy_register(apx_clientEventListenerSpy_t *self, apx_client_t *client)
{
   if ( (self != 0) && (client != 0) )
   {
      apx_clientEventListener_t handler;
      handler.arg = (void*) self;
      handler.clientConnect1 = apx_clientEventListenerSpy_onConnect;
      handler.clientDisconnect1 = apx_clientEventListenerSpy_onDisconnect;
      return apx_client_registerEventListener(client, &handler);
   }
   return (void*) 0;
}

uint32_t apx_clientEventListenerSpy_getConnectCount(apx_clientEventListenerSpy_t *self)
{
   if (self != 0)
   {
      return self->connectCount;
   }
   return 0;
}

uint32_t apx_clientEventListenerSpy_getDisconnectCount(apx_clientEventListenerSpy_t *self)
{
   if (self != 0)
   {
      return self->disconnectCount;
   }
   return 0;
}

uint32_t apx_clientEventListenerSpy_getHeaderAccepted(apx_clientEventListenerSpy_t *self)
{
   if (self != 0)
   {
      return self->headerAcceptedCount;
   }
   return 0;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_clientEventListenerSpy_onConnect(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   apx_clientEventListenerSpy_t *self = (apx_clientEventListenerSpy_t*) arg;
   if ( (self != 0) && (clientConnection != 0) )
   {
      apx_connectionEventListener_t handler;
      self->connectCount++;
      memset(&handler, 0, sizeof(handler));
      handler.arg = (void*) self;
      handler.headerAccepted2 = apx_clientEventListenerSpy_onHeaderAccepted;
      apx_clientConnectionBase_registerEventListener(clientConnection, &handler);
   }
}

static void apx_clientEventListenerSpy_onDisconnect(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   apx_clientEventListenerSpy_t *self = (apx_clientEventListenerSpy_t*) arg;
   if ( (self != 0) && (clientConnection != 0) )
   {
      self->disconnectCount++;
   }
}

static void apx_clientEventListenerSpy_onHeaderAccepted(void *arg, apx_connectionBase_t *clientConnection)
{
   apx_clientEventListenerSpy_t *self = (apx_clientEventListenerSpy_t*) arg;
   if ( (self != 0) && (clientConnection != 0) )
   {
      self->headerAcceptedCount++;
   }
}

