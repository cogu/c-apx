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
static void register_server_listener(apx_server_monitor_t* self);
static void on_new_connection(apx_server_monitor_t* self, apx_server_connection_t* connection);
static void on_connection_closed(apx_server_monitor_t* self, apx_server_connection_t* connection);
static void on_protocol_header_accepted(apx_server_monitor_t* self, apx_server_connection_t* connection);
static void register_connection_listener(apx_server_monitor_t* self, apx_server_connection_t* connection);
static void mutex_lock(apx_server_monitor_t* self);
static void mutex_unlock(apx_server_monitor_t* self);
static apx_error_t transmit_connection_info_to_new_monitor_connection(apx_server_monitor_t* self, apx_server_connection_t* monitor_connection);
static void delete_observed_connection(apx_server_monitor_t* self, apx_server_connection_t* server_connection);
static apx_observed_connection_t* find_observed_connection(apx_server_monitor_t* self, apx_server_connection_t* server_connection);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_server_monitor_create(apx_server_monitor_t* self, struct apx_server_tag* server)
{
   if (self != NULL)
   {
      self->server = server;
      adt_list_create(&self->connection_observers, apx_observed_connection_vdelete);
      adt_list_create(&self->monitor_connections, NULL);
      register_server_listener(self);
      MUTEX_INIT(self->lock);
   }
}

void apx_server_monitor_destroy(apx_server_monitor_t* self)
{
   if (self != NULL)
   {
      adt_list_destroy(&self->connection_observers);
      adt_list_destroy(&self->monitor_connections);
      MUTEX_DESTROY(self->lock);
   }
}

apx_server_monitor_t* apx_server_monitor_new(struct apx_server_tag* server)
{
   apx_server_monitor_t* self = (apx_server_monitor_t*)malloc(sizeof(apx_server_monitor_t));
   if (self != NULL)
   {
      apx_server_monitor_create(self, server);
   }
   return self;
}

void apx_server_monitor_delete(apx_server_monitor_t* self)
{
   if (self != NULL)
   {
      apx_server_monitor_destroy(self);
      free(self);
   }
}

int32_t apx_server_monitor_num_connections(apx_server_monitor_t* self)
{
   if (self != NULL)
   {
      return adt_list_length(&self->connection_observers);
   }
   return -1;
}

apx_observed_connection_t* apx_server_monitor_get_last_observed_connection(apx_server_monitor_t* self)
{
   if (self != NULL)
   {
      return adt_list_last(&self->connection_observers);
   }
   return NULL;
}

//Virtual call points
void apx_server_monitor_virtual_on_new_connection(void* arg, apx_server_connection_t* connection)
{
   apx_server_monitor_t* self = (apx_server_monitor_t*)arg;
   if ( (self != NULL) && (connection != NULL) )
   {
      on_new_connection(self, connection);
   }
}

void apx_server_monitor_virtual_on_connection_closed(void* arg, apx_server_connection_t* connection)
{
   apx_server_monitor_t* self = (apx_server_monitor_t*)arg;
   if ( (self != NULL) && (connection != NULL) )
   {
      on_connection_closed(self, connection);
   }
}

void apx_server_monitor_virtual_on_protocol_header_accepted(void* arg, apx_connection_base_t* connection)
{
   apx_server_monitor_t* self = (apx_server_monitor_t*)arg;
   if ((self != NULL) && (connection != NULL))
   {
      on_protocol_header_accepted(self, (apx_server_connection_t*) connection);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_server_monitor_t* self)
{
   if (self->server != NULL)
   {
      apx_server_event_listener_t eventListener;
      memset(&eventListener, 0, sizeof(apx_server_event_listener_t));
      eventListener.arg = (void*)self;
      eventListener.new_connection = apx_server_monitor_virtual_on_new_connection;
      eventListener.connection_closed = apx_server_monitor_virtual_on_connection_closed;
      apx_server_register_event_listener(self->server, &eventListener);
   }
}

static void on_new_connection(apx_server_monitor_t* self, apx_server_connection_t* connection)
{
   assert((self != NULL) && (connection != NULL));
   apx_observed_connection_t* observed_connection = apx_observed_connection_new(connection);
   if (observed_connection != NULL)
   {
      mutex_lock(self);
      adt_list_insert(&self->connection_observers, (void*)observed_connection);
      mutex_unlock(self);
      register_connection_listener(self, connection);
   }
}

static void on_connection_closed(apx_server_monitor_t* self, apx_server_connection_t* connection)
{
   int32_t num_monitors = -1;
   apx_connection_type_t connection_type = apx_server_connection_get_connection_type(connection);
   mutex_lock(self);
   delete_observed_connection(self, connection);
   if (connection_type == APX_CONNECTION_TYPE_MONITOR)
   {
      adt_list_remove(&self->monitor_connections, (void*)connection);
      num_monitors = adt_list_length(&self->monitor_connections);
   }
   mutex_unlock(self);
#if APX_DEBUG_ENABLE
   if (num_monitors >= 0)
   {
      printf("[MONITOR_STATE] Number of monitor connections: %d\n", (int)num_monitors);
   }
#endif
}

static void register_connection_listener(apx_server_monitor_t* self, apx_server_connection_t* connection)
{
   apx_server_connection_event_listener_t listener;
   memset(&listener, 0, sizeof(listener));
   listener.arg = (void*)self;
   listener.protocol_header_accepted = apx_server_monitor_virtual_on_protocol_header_accepted;
   apx_server_connection_register_event_listener(connection, &listener);
}

static void on_protocol_header_accepted(apx_server_monitor_t* self, apx_server_connection_t* server_connection)
{
   (void)self;
   apx_connection_type_t connection_type = apx_server_connection_get_connection_type(server_connection);
   apx_observed_connection_t* observed_connection = NULL;
   mutex_lock(self);
   observed_connection = find_observed_connection(self, server_connection);
   if (observed_connection != NULL)
   {
      apx_observed_connection_set_connection_type(observed_connection, connection_type);
      apx_observed_connection_set_connection_state(observed_connection, APX_CONNECTION_STATE_ACCEPTED);
      if (connection_type == APX_CONNECTION_TYPE_MONITOR)
      {
         adt_list_insert(&self->monitor_connections, (void*)server_connection);
         transmit_connection_info_to_new_monitor_connection(self, server_connection);
      }
   }
   mutex_unlock(self);
}

static void mutex_lock(apx_server_monitor_t* self)
{
   assert(self != NULL);
   MUTEX_LOCK(self->lock);
}

static void mutex_unlock(apx_server_monitor_t* self)
{
   assert(self != NULL);
   MUTEX_UNLOCK(self->lock);
}

//Requires lock to be held before calling function
static apx_error_t transmit_connection_info_to_new_monitor_connection(apx_server_monitor_t* self, apx_server_connection_t* monitor_connection)
{
   assert( (self != NULL) && (monitor_connection != NULL) );
   adt_list_elem_t* iter = adt_list_iter_first(&self->connection_observers);
   while (iter != NULL)
   {
      apx_observed_connection_t* observed_connecton = (apx_observed_connection_t*)iter->pItem;
      assert(observed_connecton != NULL);
      {
         if ((observed_connecton->server_connection != monitor_connection))
         {
            apx_error_t result;
            apx_connection_id_t connection_id;
            apx_connection_state_t connection_state;
            char const* tag;
            apx_file_manager_t* file_manager = apx_server_connection_get_file_manager(monitor_connection);
            assert(file_manager != NULL);
            connection_id = apx_observed_connection_connection_id(observed_connecton);
            connection_state = apx_observed_connection_get_connection_state(observed_connecton);
            tag = apx_observed_connection_tag(observed_connecton);
            result = apx_file_manager_send_connection_create(file_manager, connection_id, connection_state, tag);
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

static void delete_observed_connection(apx_server_monitor_t* self, apx_server_connection_t* server_connection)
{
   assert((self != NULL) && (server_connection != NULL));
   adt_list_elem_t* iter = adt_list_iter_first(&self->connection_observers);
   while (iter != NULL)
   {
      apx_observed_connection_t* observed_connecton = (apx_observed_connection_t*)iter->pItem;
      assert(observed_connecton != NULL);
      if (observed_connecton->server_connection == server_connection)
      {
         adt_list_erase(&self->connection_observers, iter);
         apx_observed_connection_delete(observed_connecton);
         break;
      }
      iter = adt_list_iter_next(iter);
   }
}

static apx_observed_connection_t* find_observed_connection(apx_server_monitor_t* self, apx_server_connection_t* server_connection)
{
   assert((self != NULL) && (server_connection != NULL));
   adt_list_elem_t* iter = adt_list_iter_first(&self->connection_observers);
   while (iter != NULL)
   {
      apx_observed_connection_t* observed_connecton = (apx_observed_connection_t*)iter->pItem;
      assert(observed_connecton != NULL);
      if (observed_connecton->server_connection == server_connection)
      {
         return observed_connecton;
      }
      iter = adt_list_iter_next(iter);
   }
   return NULL;
}