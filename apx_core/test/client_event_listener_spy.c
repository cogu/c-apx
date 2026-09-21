/*****************************************************************************
* \file      client_event_listener_spy.c
* \author    Conny Gustafsson
* \date      2019-11-29
* \brief     Client event listener spy
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
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
   if (self != NULL)
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
   if (self != NULL)
   {
      apx_clientEventListenerSpy_create(self);
   }
   return self;
}

void apx_clientEventListenerSpy_delete(apx_clientEventListenerSpy_t *self)
{
   if (self != NULL)
   {
      apx_clientEventListenerSpy_destroy(self);
      free(self);
   }
}

void* apx_clientEventListenerSpy_register(apx_clientEventListenerSpy_t *self, apx_client_t *client)
{
   if ( (self != NULL) && (client != NULL) )
   {
      apx_clientEventListener_t handler;
      handler.arg = (void*) self;
      handler.connected = apx_clientEventListenerSpy_onConnect;
      handler.disconnected = apx_clientEventListenerSpy_onDisconnect;
      handler.require_port_write = NULL;
      return apx_client_register_event_listener(client, &handler);
   }
   return NULL;
}

uint32_t apx_clientEventListenerSpy_getConnectCount(apx_clientEventListenerSpy_t *self)
{
   if (self != NULL)
   {
      return self->connectCount;
   }
   return 0;
}

uint32_t apx_clientEventListenerSpy_getDisconnectCount(apx_clientEventListenerSpy_t *self)
{
   if (self != NULL)
   {
      return self->disconnectCount;
   }
   return 0;
}

uint32_t apx_clientEventListenerSpy_getHeaderAccepted(apx_clientEventListenerSpy_t *self)
{
   if (self != NULL)
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
   if ( (self != NULL) && (clientConnection != NULL) )
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
   if ( (self != NULL) && (clientConnection != NULL) )
   {
      self->disconnectCount++;
   }
}

static void apx_clientEventListenerSpy_onHeaderAccepted(void *arg, apx_connectionBase_t *clientConnection)
{
   apx_clientEventListenerSpy_t *self = (apx_clientEventListenerSpy_t*) arg;
   if ( (self != NULL) && (clientConnection != NULL) )
   {
      self->headerAcceptedCount++;
   }
}

