/*****************************************************************************
* \file      server_monitor.c
* \author    Conny Gustafsson
* \date      2021-02-28
* \brief     Server Monitor State
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "apx/extension/server_monitor.h"
#include "apx/extension/observed_connection.h"
#include "apx/server.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_serverMonitor_t* self);
static void on_new_connection(apx_serverMonitor_t* self, apx_serverConnection_t* connection);
static void on_connection_closed(apx_serverMonitor_t* self, apx_serverConnection_t* connection);
static void on_protocol_header_accepted(apx_serverMonitor_t* self, apx_serverConnection_t* connection);
static void register_connection_listener(apx_serverMonitor_t* self, apx_serverConnection_t* connection);
static void mutex_lock(apx_serverMonitor_t* self);
static void mutex_unlock(apx_serverMonitor_t* self);
static apx_error_t transmit_connection_info_to_new_monitor_connection(apx_serverMonitor_t* self, apx_serverConnection_t* monitor_connection);
static void delete_observed_connection(apx_serverMonitor_t* self, apx_serverConnection_t* server_connection);
static apx_observedConnection_t* find_observed_connection(apx_serverMonitor_t* self, apx_serverConnection_t* server_connection);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_serverMonitor_create(apx_serverMonitor_t* self, struct apx_server_tag* server)
{
   if (self != NULL)
   {
      printf("[MONITOR_STATE] Init\n");
      self->server = server;
      adt_list_create(&self->connection_observers, apx_observedConnection_vdelete);
      adt_list_create(&self->monitor_connections, NULL);
      register_server_listener(self);
      MUTEX_INIT(self->lock);
   }
}

void apx_serverMonitor_destroy(apx_serverMonitor_t* self)
{
   if (self != NULL)
   {
      printf("[MONITOR_STATE] Destroy\n");
      adt_list_destroy(&self->connection_observers);
      adt_list_destroy(&self->monitor_connections);
      MUTEX_DESTROY(self->lock);
   }
}

apx_serverMonitor_t* apx_serverMonitor_new(struct apx_server_tag* server)
{
   apx_serverMonitor_t* self = (apx_serverMonitor_t*)malloc(sizeof(apx_serverMonitor_t));
   if (self != NULL)
   {
      apx_serverMonitor_create(self, server);
   }
   return self;
}

void apx_serverMonitor_delete(apx_serverMonitor_t* self)
{
   if (self != NULL)
   {
      apx_serverMonitor_destroy(self);
      free(self);
   }
}

int32_t apx_serverMonitor_num_connections(apx_serverMonitor_t* self)
{
   if (self != NULL)
   {
      return adt_list_length(&self->connection_observers);
   }
   return -1;
}

apx_observedConnection_t* apx_serverMonitor_get_last_observed_connection(apx_serverMonitor_t* self)
{
   if (self != NULL)
   {
      return adt_list_last(&self->connection_observers);
   }
   return NULL;
}

//Virtual call points
void apx_serverMonitor_virtual_on_new_connection(void* arg, apx_serverConnection_t* connection)
{
   apx_serverMonitor_t* self = (apx_serverMonitor_t*)arg;
   if ( (self != NULL) && (connection != NULL) )
   {
      on_new_connection(self, connection);
   }
}

void apx_serverMonitor_virtual_on_connection_closed(void* arg, apx_serverConnection_t* connection)
{
   apx_serverMonitor_t* self = (apx_serverMonitor_t*)arg;
   if ( (self != NULL) && (connection != NULL) )
   {
      on_connection_closed(self, connection);
   }
}

void apx_serverMonitor_virtual_on_protocol_header_accepted(void* arg, apx_connectionBase_t* connection)
{
   apx_serverMonitor_t* self = (apx_serverMonitor_t*)arg;
   if ((self != NULL) && (connection != NULL))
   {
      on_protocol_header_accepted(self, (apx_serverConnection_t*) connection);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_serverMonitor_t* self)
{
   if (self->server != NULL)
   {
      apx_serverEventListener_t eventListener;
      memset(&eventListener, 0, sizeof(apx_serverEventListener_t));
      eventListener.arg = (void*)self;
      eventListener.new_connection = apx_serverMonitor_virtual_on_new_connection;
      eventListener.connection_closed = apx_serverMonitor_virtual_on_connection_closed;
      apx_server_register_event_listener(self->server, &eventListener);
   }
}

static void on_new_connection(apx_serverMonitor_t* self, apx_serverConnection_t* connection)
{
   assert((self != NULL) && (connection != NULL));
   apx_observedConnection_t* observed_connection = apx_observedConnection_new(connection);
   if (observed_connection != NULL)
   {
      mutex_lock(self);
      adt_list_insert(&self->connection_observers, (void*)observed_connection);
      mutex_unlock(self);
      register_connection_listener(self, connection);
   }
}

static void on_connection_closed(apx_serverMonitor_t* self, apx_serverConnection_t* connection)
{
   int32_t num_monitors = -1;
   apx_connectionType_t connection_type = apx_serverConnection_get_connection_type(connection);
   mutex_lock(self);
   delete_observed_connection(self, connection);
   if (connection_type == APX_CONNECTION_TYPE_MONITOR)
   {
      adt_list_remove(&self->monitor_connections, (void*)connection);
      num_monitors = adt_list_length(&self->monitor_connections);
   }
   mutex_unlock(self);
   if (num_monitors >= 0)
   {
      printf("[MONITOR_STATE] Number of monitor connections: %d\n", (int)num_monitors);
   }
}

static void register_connection_listener(apx_serverMonitor_t* self, apx_serverConnection_t* connection)
{
   apx_serverConnectionEventListener_t listener;
   memset(&listener, 0, sizeof(listener));
   listener.arg = (void*)self;
   listener.protocol_header_accepted = apx_serverMonitor_virtual_on_protocol_header_accepted;
   apx_serverConnection_register_event_listener(connection, &listener);
}

static void on_protocol_header_accepted(apx_serverMonitor_t* self, apx_serverConnection_t* server_connection)
{
   (void)self;
   apx_connectionType_t connection_type = apx_serverConnection_get_connection_type(server_connection);
   apx_observedConnection_t* observed_connection = NULL;
   mutex_lock(self);
   observed_connection = find_observed_connection(self, server_connection);
   if (observed_connection != NULL)
   {
      apx_observedConnection_set_connection_type(observed_connection, connection_type);
      apx_observedConnection_set_connection_state(observed_connection, APX_CONNECTION_STATE_ACCEPTED);
      if (connection_type == APX_CONNECTION_TYPE_MONITOR)
      {         
         adt_list_insert(&self->monitor_connections, (void*)server_connection);
         transmit_connection_info_to_new_monitor_connection(self, server_connection);
      }
   }
   mutex_unlock(self);
}

static void mutex_lock(apx_serverMonitor_t* self)
{
   assert(self != NULL);
   MUTEX_LOCK(self->lock);
}

static void mutex_unlock(apx_serverMonitor_t* self)
{
   assert(self != NULL);
   MUTEX_UNLOCK(self->lock);
}

//Requires lock to be held before calling function
static apx_error_t transmit_connection_info_to_new_monitor_connection(apx_serverMonitor_t* self, apx_serverConnection_t* monitor_connection)
{
   assert( (self != NULL) && (monitor_connection != NULL) );
   adt_list_elem_t* iter = adt_list_iter_first(&self->connection_observers);
   while (iter != NULL)
   {
      apx_observedConnection_t* observed_connecton = (apx_observedConnection_t*)iter->pItem;
      assert(observed_connecton != NULL);
      {
         if ((observed_connecton->server_connection != monitor_connection))
         {
            apx_error_t result;
            apx_connectionId_t connection_id;
            apx_connectionState_t connection_state;
            char const* tag;
            apx_fileManager_t* file_manager = apx_serverConnection_get_file_manager(monitor_connection);
            assert(file_manager != NULL);
            connection_id = apx_observedConnection_connection_id(observed_connecton);
            connection_state = apx_observedConnection_get_connection_state(observed_connecton);
            tag = apx_observedConnection_tag(observed_connecton);
            result = apx_fileManager_send_connection_create(file_manager, connection_id, connection_state, tag);
            if (result != APX_NO_ERROR)
            {
               return result;
            }
         }
         iter = adt_list_iter_next(iter);
      }
   }
   return APX_NO_ERROR;
}

static void delete_observed_connection(apx_serverMonitor_t* self, apx_serverConnection_t* server_connection)
{
   assert((self != NULL) && (server_connection != NULL));
   adt_list_elem_t* iter = adt_list_iter_first(&self->connection_observers);
   while (iter != NULL)
   {
      apx_observedConnection_t* observed_connecton = (apx_observedConnection_t*)iter->pItem;
      assert(observed_connecton != NULL);
      if (observed_connecton->server_connection == server_connection)
      {
         adt_list_erase(&self->connection_observers, iter);
         apx_observedConnection_delete(observed_connecton);
         break;
      }
      iter = adt_list_iter_next(iter);
   }
}

static apx_observedConnection_t* find_observed_connection(apx_serverMonitor_t* self, apx_serverConnection_t* server_connection)
{
   assert((self != NULL) && (server_connection != NULL));
   adt_list_elem_t* iter = adt_list_iter_first(&self->connection_observers);
   while (iter != NULL)
   {
      apx_observedConnection_t* observed_connecton = (apx_observedConnection_t*)iter->pItem;
      assert(observed_connecton != NULL);
      if (observed_connecton->server_connection == server_connection)
      {
         return observed_connecton;
      }
      iter = adt_list_iter_next(iter);
   }
   return NULL;
}