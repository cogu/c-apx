/*****************************************************************************
* \file      server_monitor_state.c
* \author    Conny Gustafsson
* \date      2021-02-28
* \brief     Server Monitor State
*
* Copyright (c) 2021 Conny Gustafsson
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
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "apx/extension/server_monitor_state.h"
#include "apx/server.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_serverMonitorState_t* self);
static void on_new_connection(apx_serverMonitorState_t* self, apx_serverConnection_t* connection);
static void on_connection_closed(apx_serverMonitorState_t* self, apx_serverConnection_t* connection);
static void on_protocol_header_accepted(apx_serverMonitorState_t* self, struct apx_connectionBase_tag* connection);
static void register_connection_listener(apx_serverMonitorState_t* self, apx_serverConnection_t* connection);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_serverMonitorState_create(apx_serverMonitorState_t* self, struct apx_server_tag* server)
{
   if (self != NULL)
   {
      printf("[MONITOR_STATE] Init\n");
      self->server = server;
      adt_list_create(&self->client_connections, NULL);
      register_server_listener(self);
      MUTEX_INIT(self->lock);
   }
}

void apx_serverMonitorState_destroy(apx_serverMonitorState_t* self)
{
   if (self != NULL)
   {
      printf("[MONITOR_STATE] Destroy\n");
      adt_list_destroy(&self->client_connections);
      MUTEX_DESTROY(self->lock);      
   }
}

apx_serverMonitorState_t* apx_serverMonitorState_new(struct apx_server_tag* server)
{
   apx_serverMonitorState_t* self = (apx_serverMonitorState_t*)malloc(sizeof(apx_serverMonitorState_t));
   if (self != NULL)
   {
      apx_serverMonitorState_create(self, server);
   }
   return self;
}

void apx_serverMonitorState_delete(apx_serverMonitorState_t* self)
{
   if (self != NULL)
   {
      apx_serverMonitorState_destroy(self);
      free(self);
   }
}


//Virtual call points
void apx_serverMonitorState_virtual_on_new_connection(void* arg, apx_serverConnection_t* connection)
{
   apx_serverMonitorState_t* self = (apx_serverMonitorState_t*)arg;
   if ( (self != NULL) && (connection != NULL) )
   {
      on_new_connection(self, connection);
   }   
}

void apx_serverMonitorState_virtual_on_connection_closed(void* arg, apx_serverConnection_t* connection)
{
   apx_serverMonitorState_t* self = (apx_serverMonitorState_t*)arg;
   if ( (self != NULL) && (connection != NULL) )
   {
      on_connection_closed(self, connection);
   }
}

void apx_serverMonitorState_virtual_on_protocol_header_accepted(void* arg, struct apx_connectionBase_tag* connection)
{
   apx_serverMonitorState_t* self = (apx_serverMonitorState_t*)arg;
   if ((self != NULL) && (connection != NULL))
   {
      on_protocol_header_accepted(self, connection);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_serverMonitorState_t* self)
{
   apx_serverEventListener_t eventListener;
   memset(&eventListener, 0, sizeof(apx_serverEventListener_t));
   eventListener.arg = (void*)self;
   eventListener.new_connection2 = apx_serverMonitorState_virtual_on_new_connection;
   eventListener.connection_closed2 = apx_serverMonitorState_virtual_on_connection_closed;
   apx_server_register_event_listener(self->server, &eventListener);
}

static void on_new_connection(apx_serverMonitorState_t* self, apx_serverConnection_t* connection)
{
   MUTEX_LOCK(self->lock);
   adt_list_insert(&self->client_connections, (void*)connection);
   MUTEX_UNLOCK(self->lock);
   register_connection_listener(self, connection);
}

static void on_connection_closed(apx_serverMonitorState_t* self, apx_serverConnection_t* connection)
{
   MUTEX_LOCK(self->lock);
   adt_list_remove(&self->client_connections, (void*)connection);
   MUTEX_UNLOCK(self->lock);
}

static void register_connection_listener(apx_serverMonitorState_t* self, apx_serverConnection_t* connection)
{
   apx_connectionEventListener_t listener;
   memset(&listener, 0, sizeof(listener));
   listener.arg = (void*)self;
   listener.protocol_header_accepted = apx_serverMonitorState_virtual_on_protocol_header_accepted;
   apx_serverConnection_register_event_listener(connection, &listener);
}

static void on_protocol_header_accepted(apx_serverMonitorState_t* self, struct apx_connectionBase_tag* connection)
{
   (void)self;
   apx_connectionType_t connection_type = apx_connectionBase_get_connection_type(connection);
   if (connection_type == APX_CONNECTION_TYPE_MONITOR)
   {
      printf("[MONITOR_STATE] New monitor connection detected\n");
   }
}