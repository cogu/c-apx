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
static void apx_client_event_listener_spy_on_connect(void *arg, apx_client_connection_base_t *clientConnection);
static void apx_client_event_listener_spy_on_disconnect(void *arg, apx_client_connection_base_t *clientConnection);
static void apx_client_event_listener_spy_on_header_accepted(void *arg, apx_connection_base_t *clientConnection);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_client_event_listener_spy_create(apx_client_event_listener_spy_t *self)
{
   if (self != NULL)
   {
      self->connectCount = 0u;
      self->disconnectCount = 0u;
      self->headerAcceptedCount = 0u;
   }
}

void apx_client_event_listener_spy_destroy(apx_client_event_listener_spy_t *self)
{
   //Nothing to do
}

apx_client_event_listener_spy_t* apx_client_event_listener_spy_new(void)
{
   apx_client_event_listener_spy_t *self = (apx_client_event_listener_spy_t*) malloc(sizeof(apx_client_event_listener_spy_t));
   if (self != NULL)
   {
      apx_client_event_listener_spy_create(self);
   }
   return self;
}

void apx_client_event_listener_spy_delete(apx_client_event_listener_spy_t *self)
{
   if (self != NULL)
   {
      apx_client_event_listener_spy_destroy(self);
      free(self);
   }
}

void* apx_client_event_listener_spy_register(apx_client_event_listener_spy_t *self, apx_client_t *client)
{
   if ( (self != NULL) && (client != NULL) )
   {
      apx_client_event_listener_t handler;
      handler.arg = (void*) self;
      handler.connected = apx_client_event_listener_spy_on_connect;
      handler.disconnected = apx_client_event_listener_spy_on_disconnect;
      handler.require_port_write = NULL;
      return apx_client_register_event_listener(client, &handler);
   }
   return NULL;
}

uint32_t apx_client_event_listener_spy_get_connect_count(apx_client_event_listener_spy_t *self)
{
   if (self != NULL)
   {
      return self->connectCount;
   }
   return 0;
}

uint32_t apx_client_event_listener_spy_get_disconnect_count(apx_client_event_listener_spy_t *self)
{
   if (self != NULL)
   {
      return self->disconnectCount;
   }
   return 0;
}

uint32_t apx_client_event_listener_spy_get_header_accepted(apx_client_event_listener_spy_t *self)
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
static void apx_client_event_listener_spy_on_connect(void *arg, apx_client_connection_base_t *clientConnection)
{
   apx_client_event_listener_spy_t *self = (apx_client_event_listener_spy_t*) arg;
   if ( (self != NULL) && (clientConnection != NULL) )
   {
      apx_connection_event_listener_t handler;
      self->connectCount++;
      memset(&handler, 0, sizeof(handler));
      handler.arg = (void*) self;
      handler.headerAccepted2 = apx_client_event_listener_spy_on_header_accepted;
      apx_client_connection_base_register_event_listener(clientConnection, &handler);
   }
}

static void apx_client_event_listener_spy_on_disconnect(void *arg, apx_client_connection_base_t *clientConnection)
{
   apx_client_event_listener_spy_t *self = (apx_client_event_listener_spy_t*) arg;
   if ( (self != NULL) && (clientConnection != NULL) )
   {
      self->disconnectCount++;
   }
}

static void apx_client_event_listener_spy_on_header_accepted(void *arg, apx_connection_base_t *clientConnection)
{
   apx_client_event_listener_spy_t *self = (apx_client_event_listener_spy_t*) arg;
   if ( (self != NULL) && (clientConnection != NULL) )
   {
      self->headerAcceptedCount++;
   }
}

