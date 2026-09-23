/*****************************************************************************
* \file      server.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX server class
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/server.h"
//#include "apx/logging.h"
#include "apx/file_manager.h"
#include "apx/event_listener.h"
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <stdio.h> //DEBUG ONLY
#ifdef _WIN32
#include <process.h>
#endif
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
static void apx_server_attach_and_start_connection(apx_server_t *self, apx_server_connection_t *new_connection);
static void apx_server_trigger_connected_event(apx_server_t *self, apx_server_connection_t * server_connection);
static void apx_server_trigger_disconnected_event(apx_server_t *self, apx_server_connection_t *server_connection);
static void apx_server_trigger_log_write_event(apx_server_t *self, apx_log_level_t level, const char *label, const char *msg);
static void apx_server_init_extensions(apx_server_t *self);
static void apx_server_shutdown_extensions(apx_server_t *self);
static void apx_server_handle_event(void *arg, apx_event_t *event);
static void apx_server_destroy_event(apx_server_t* self, apx_event_t* event);
#ifndef UNIT_TEST
static apx_error_t apx_server_start_thread(apx_server_t *self);
static apx_error_t apx_server_stop_thread(apx_server_t *self);
static THREAD_PROTO(thread_task,arg);
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
   if (self != NULL)
   {
      adt_list_create(&self->server_event_listeners, apx_server_event_listener_vdelete);
      apx_port_signature_map_create(&self->port_signature_map);
      apx_connection_manager_create(&self->connection_manager);
      adt_list_create(&self->extension_manager, apx_server_extension_vdelete);
      adt_ary_create(&self->modified_nodes, (void(*)(void*)) 0);
      soa_init(&self->allocator);
      apx_event_loop_create(&self->event_loop);
      self->is_event_thread_valid = false;
      MUTEX_INIT(self->event_loop_lock);
      MUTEX_INIT(self->global_lock);
      MUTEX_INIT(self->event_listener_lock);
#ifdef _WIN32
      self->thread_id = 0u;
#endif
   }
}

void apx_server_destroy(apx_server_t *self)
{
   if (self != NULL)
   {
      apx_server_stop(self);
      MUTEX_LOCK(self->global_lock);
      adt_list_destroy(&self->extension_manager);
      adt_ary_destroy(&self->modified_nodes);
      MUTEX_LOCK(self->event_listener_lock);
      adt_list_destroy(&self->server_event_listeners);
      MUTEX_UNLOCK(self->event_listener_lock);
      apx_connection_manager_destroy(&self->connection_manager);
      apx_port_signature_map_destroy(&self->port_signature_map);
      MUTEX_UNLOCK(self->global_lock);
      apx_event_loop_destroy(&self->event_loop, apx_server_vdestroy_event, (void*)self);
      soa_destroy(&self->allocator);
      MUTEX_DESTROY(self->event_loop_lock);
      MUTEX_DESTROY(self->global_lock);
      MUTEX_DESTROY(self->event_listener_lock);
   }
}

apx_server_t *apx_server_new(void)
{
   apx_server_t *self = (apx_server_t*) malloc(sizeof(apx_server_t));
   if (self != NULL)
   {
      apx_server_create(self);
   }
   return self;
}

void apx_server_delete(apx_server_t *self)
{
   if (self != NULL)
   {
      apx_server_destroy(self);
      free(self);
   }
}

void apx_server_start(apx_server_t *self)
{

   if( self != NULL )
   {
      apx_server_init_extensions(self);
#ifndef UNIT_TEST
      apx_connection_manager_start(&self->connection_manager);
      if (self->is_event_thread_valid == false)
      {
         apx_server_start_thread(self);
      }
#endif
   }
}

void apx_server_stop(apx_server_t *self)
{
   if( self != NULL)
   {

#ifndef UNIT_TEST
      apx_connection_manager_stop(&self->connection_manager);
#endif
      apx_server_shutdown_extensions(self);
#ifndef UNIT_TEST
      apx_event_loop_exit(&self->event_loop);
      if (self->is_event_thread_valid)
      {
         apx_server_stop_thread(self);
      }
#endif
   }
}

void* apx_server_register_event_listener(apx_server_t* self, apx_server_event_listener_t* event_listener)
{
   if ( (self != NULL) && (event_listener != NULL))
   {
      void *handle = (void*) apx_server_event_listener_clone(event_listener);
      if (handle != NULL)
      {
         MUTEX_LOCK(self->event_listener_lock);
         adt_list_insert(&self->server_event_listeners, handle);
         MUTEX_UNLOCK(self->event_listener_lock);
      }
      return handle;
   }
   return NULL;
}

void apx_server_unregister_event_listener(apx_server_t *self, void *handle)
{
   if ( (self != NULL) && (handle != NULL))
   {
      bool isFound;
      MUTEX_LOCK(self->event_listener_lock);
      isFound = adt_list_remove(&self->server_event_listeners, handle);
      MUTEX_UNLOCK(self->event_listener_lock);
      if (isFound == true)
      {
         apx_server_event_listener_vdelete(handle);
      }
   }
}

void apx_server_accept_connection(apx_server_t* self, apx_server_connection_t* server_connection)
{
   if ( (self != NULL) && (server_connection != NULL))
   {
      apx_server_attach_and_start_connection(self, server_connection);
   }
}

apx_error_t apx_server_detach_connection(apx_server_t* self, apx_server_connection_t* server_connection)
{
   if ( (self != NULL) && (server_connection != NULL))
   {
      apx_error_t result;
      apx_connection_manager_detach(&self->connection_manager, server_connection);
      result = apx_server_connection_disconnected_notification(server_connection);
      if (result == APX_NO_ERROR)
      {
         apx_server_trigger_disconnected_event(self, server_connection);
      }
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_add_extension(apx_server_t* self, const char* name, apx_server_extension_handler_t* handler, dtl_dv_t* config)
{
   if ( (self != NULL) && (handler != NULL) )
   {
      apx_server_extension_t *extension = apx_server_extension_new(name, handler, config);
      if (extension == NULL)
      {
         return APX_MEM_ERROR;
      }
      adt_list_insert(&self->extension_manager, (void*) extension);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_server_log_write(apx_server_t* self, apx_log_level_t level, const char* label, const char* msg)
{
   if ( (self != NULL) && (level <= APX_MAX_LOG_LEVEL) && (msg != NULL) )
   {
      apx_event_t event;
      char *labelStr = NULL;
      adt_str_t *msgStr = adt_str_new_cstr(msg);

      if (msgStr == NULL)
      {
         return;
      }

      if (label != NULL)
      {
         size_t labelSize = strlen(label);
         if (labelSize > APX_LOG_LABEL_MAX_LEN)
         {
            labelSize = APX_LOG_LABEL_MAX_LEN;
         }
         MUTEX_LOCK(self->event_loop_lock);
         labelStr = soa_alloc(&self->allocator, labelSize+1);
         MUTEX_UNLOCK(self->event_loop_lock);
         if (labelStr == NULL)
         {
            adt_str_delete(msgStr);
            return;
         }
         memcpy(labelStr, label, labelSize);
         labelStr[labelSize] = 0;
      }
      memset(&event, 0, sizeof(event));
      apx_event_pack_log_write(&event, level, labelStr, msgStr);
      apx_event_loop_append(&self->event_loop, &event);
   }
}


apx_error_t apx_server_append_event(apx_server_t* self, apx_event_t* event)
{
   apx_event_loop_append(&self->event_loop, event);
   return APX_NO_ERROR;
}

/**
 * Acquires the server global lock
 */
void apx_server_take_global_lock(apx_server_t* self)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->global_lock);
   }
}

/**
 * Releases the server global lock
 */
void apx_server_release_global_lock(apx_server_t* self)
{
   if (self != NULL)
   {
      MUTEX_UNLOCK(self->global_lock);
   }
}

apx_error_t apx_server_connect_node_instance_provide_ports(apx_server_t* self, apx_node_instance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      return apx_port_signature_map_connect_provide_ports(&self->port_signature_map, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_connect_node_instance_require_ports(apx_server_t* self, apx_node_instance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      return apx_port_signature_map_connect_require_ports(&self->port_signature_map, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_disconnect_node_instance_provide_ports(apx_server_t* self, apx_node_instance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      return apx_port_signature_map_disconnect_provide_ports(&self->port_signature_map, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_disconnect_node_instance_require_ports(apx_server_t* self, apx_node_instance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      return apx_port_signature_map_disconnect_require_ports(&self->port_signature_map, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static uint16_t cap_port_count(int32_t count)
{
   if (count <= 0)
   {
      return 0u;
   }
   if (count > (int32_t)UINT16_MAX)
   {
      return UINT16_MAX;
   }
   return (uint16_t)count;
}

static uint16_t calculate_provide_port_count(apx_port_connector_list_t const* connectors)
{
   if (connectors != NULL)
   {
      return cap_port_count(apx_port_connector_list_length((apx_port_connector_list_t*)connectors));
   }
   return 0u;
}

uint16_t apx_server_calculate_require_port_count(apx_server_t* self, apx_port_instance_t* require_port)
{
   if ((self != NULL) && (require_port != NULL))
   {
      bool has_dynamic_data = false;
      const char* port_signature = apx_port_instance_get_port_signature(require_port, &has_dynamic_data);
      if (port_signature != NULL)
      {
         apx_port_signature_map_entry_t* entry = apx_port_signature_map_find(&self->port_signature_map, port_signature);
         if (entry != NULL)
         {
            int32_t num_providers = apx_port_signature_map_entry_get_num_providers(entry);
            return cap_port_count(num_providers);
         }
      }
   }
   return 0u;
}

/**
 * Is is assumed that the server global lock is held by the caller of this function
 */
apx_error_t apx_server_process_require_port_connector_changes(apx_server_t* self, apx_node_instance_t* require_node_instance, apx_port_connector_change_table_t* connector_changes)
{
   if ( (self != NULL) && (require_node_instance != NULL) && (connector_changes != NULL) )
   {
      apx_size_t num_require_ports;
      apx_port_id_t port_id;
      num_require_ports = apx_node_instance_get_num_require_ports(require_node_instance);
      assert(connector_changes->num_ports == num_require_ports);
      for (port_id = 0u; port_id < num_require_ports; port_id++)
      {
         apx_port_instance_t *require_port;
         apx_port_connector_change_entry_t *entry;
         require_port = apx_node_instance_get_require_port(require_node_instance, port_id);
         entry = apx_port_connector_change_table_get_entry(connector_changes, port_id);
         assert( (require_port != NULL) && (entry != NULL));
         if (entry->count > 0)
         {
            if (entry->count == 1)
            {
               apx_error_t rc;
               apx_port_instance_t *provide_port = entry->data.port_instance;
               assert(provide_port != NULL);
               rc = apx_node_instance_handle_require_port_connected_to_provide_port(require_port, provide_port);
               if (rc != APX_NO_ERROR)
               {
                  return rc;
               }
               uint16_t require_count = apx_server_calculate_require_port_count(self, require_port);
               apx_node_instance_send_require_port_count_data(require_node_instance, port_id, require_count);
               apx_node_instance_t* provide_node_instance = apx_port_instance_parent(provide_port);
               apx_port_id_t provide_port_id = apx_port_instance_port_id(provide_port);
               apx_port_connector_list_t* connectors = apx_node_instance_get_provide_port_connectors(provide_node_instance, provide_port_id);
               if (connectors != NULL)
               {
                  uint16_t count = calculate_provide_port_count(connectors);
                  apx_node_instance_send_provide_port_count_data(provide_node_instance, provide_port_id, count);
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
apx_error_t apx_server_process_provide_port_connector_changes(apx_server_t* self, apx_node_instance_t* provide_node_instance, apx_port_connector_change_table_t* connector_changes)
{
   if ((self != NULL) && (provide_node_instance != NULL) && (connector_changes != NULL))
   {
      apx_size_t num_provide_ports;
      apx_port_id_t port_id;
      num_provide_ports = apx_node_instance_get_num_provide_ports(provide_node_instance);
      assert(connector_changes->num_ports == num_provide_ports);
      apx_node_instance_lock_port_connector_table(provide_node_instance);
      for (port_id = 0u; port_id < num_provide_ports; port_id++)
      {
         apx_port_instance_t *provide_port;
         apx_port_connector_change_entry_t *entry;
         entry = apx_port_connector_change_table_get_entry(connector_changes, port_id);
         provide_port = apx_node_instance_get_provide_port(provide_node_instance, port_id);
         assert(entry != NULL);
         assert(provide_port != NULL);
         if (entry->count > 0)
         {
            if (entry->count == 1)
            {
               apx_error_t rc;
               apx_port_instance_t *require_port = entry->data.port_instance;
               assert(require_port != NULL);
               rc = apx_node_instance_handle_provide_port_connected_to_require_port(provide_port, require_port);
               if (rc != APX_NO_ERROR)
               {
                  apx_node_instance_unlock_port_connector_table(provide_node_instance);
                  return rc;
               }
               apx_node_instance_t* require_node_instance = apx_port_instance_parent(require_port);
               apx_port_id_t require_port_id = apx_port_instance_port_id(require_port);
               uint16_t require_count = apx_server_calculate_require_port_count(self, require_port);
               apx_node_instance_send_require_port_count_data(require_node_instance, require_port_id, require_count);
            }
            else
            {
               int32_t i;
               for(i=0; i < entry->count; i++)
               {
                  apx_error_t rc;
                  apx_port_instance_t *require_port = adt_ary_value(entry->data.array, i);
                  assert(require_port != NULL);
                  rc = apx_node_instance_handle_provide_port_connected_to_require_port(provide_port, require_port);
                  if (rc != APX_NO_ERROR)
                  {
                     apx_node_instance_unlock_port_connector_table(provide_node_instance);
                     return rc;
                  }
                  apx_node_instance_t* require_node_instance = apx_port_instance_parent(require_port);
                  apx_port_id_t require_port_id = apx_port_instance_port_id(require_port);
                  uint16_t require_count = apx_server_calculate_require_port_count(self, require_port);
                  apx_node_instance_send_require_port_count_data(require_node_instance, require_port_id, require_count);
               }
            }
            apx_port_connector_list_t* connectors = &provide_node_instance->connector_table[port_id];
            uint16_t count = calculate_provide_port_count(connectors);
            apx_node_instance_send_provide_port_count_data(provide_node_instance, port_id, count);
         }
      }
      apx_node_instance_unlock_port_connector_table(provide_node_instance);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Note: Should only be used when caller holds globalLock
 */
apx_error_t apx_server_insert_modified_node_instance(apx_server_t* self, apx_node_instance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL))
   {
      adt_error_t rc = adt_ary_push_unique(&self->modified_nodes, (void*)node_instance);
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
adt_ary_t* apx_server_get_modified_node_instance(const apx_server_t* self)
{
   if (self != NULL)
   {
      return (adt_ary_t*) &self->modified_nodes;
   }
   return NULL;
}

/**
 * Note: Should only be used when caller holds globalLock
 */
void apx_server_clear_port_connector_changes(apx_server_t* self)
{
   if (self != NULL)
   {
      int32_t i;
      int32_t num_nodes = adt_ary_length(&self->modified_nodes);
      for(i=0; i < num_nodes; i++)
      {
         apx_node_instance_t *node_instance = (apx_node_instance_t*) adt_ary_value(&self->modified_nodes, i);
         assert(node_instance != NULL);
         apx_node_instance_clear_provide_port_connector_changes(node_instance, true);
         apx_node_instance_clear_require_port_connector_changes(node_instance, true);
      }
      if (num_nodes > 0)
      {
         adt_ary_clear(&self->modified_nodes);
      }
   }
}

void apx_server_vdestroy_event(void* arg, apx_event_t* event)
{
   apx_server_t* self = (apx_server_t*)arg;
   apx_server_destroy_event(self, event);
}

#ifdef UNIT_TEST
void apx_server_run(apx_server_t *self)
{
   if (self != NULL)
   {
      apx_event_loop_run_all(&self->event_loop, apx_server_handle_event, (void*) self);
      apx_connection_manager_run(&self->connection_manager);
   }
}

apx_server_connection_t* apx_server_get_last_connection(apx_server_t const* self)
{
   if (self != NULL)
   {
      return apx_connection_manager_get_last_connection(&self->connection_manager);
   }
   return NULL;
}

apx_port_signature_map_t* apx_server_get_port_signature_map(apx_server_t const* self)
{
   if (self != NULL)
   {
      return (apx_port_signature_map_t*) &self->port_signature_map;
   }
   return NULL;
}

#endif


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_server_attach_and_start_connection(apx_server_t* self, apx_server_connection_t* new_connection)
{
   if (apx_connection_manager_get_num_connections(&self->connection_manager) < APX_SERVER_MAX_CONCURRENT_CONNECTIONS)
   {
      apx_connection_manager_attach(&self->connection_manager, new_connection);
      apx_server_connection_set_server(new_connection, self);
      apx_server_trigger_connected_event(self, new_connection);
      apx_connection_base_start(&new_connection->base);
   }
   else
   {
      printf("[SERVER] Concurrent connection limit exceeded\n");
   }
}
static void apx_server_trigger_connected_event(apx_server_t* self, apx_server_connection_t* server_connection)
{
   adt_ary_t args;
   adt_ary_t callbacks;
   int32_t length = 0;
   int32_t i = 0;

   assert(self != NULL);
   assert(server_connection != NULL);

   adt_ary_create(&args, NULL);
   adt_ary_create(&callbacks, NULL);

   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(&self->server_event_listeners);
   while(iter != NULL)
   {
      apx_server_event_listener_t *listener = (apx_server_event_listener_t*) iter->pItem;
      if ( (listener != NULL) && (listener->new_connection != NULL) )
      {
         adt_ary_push(&args, (void*)listener->arg);
         adt_ary_push(&callbacks, (void*)listener->new_connection);
         length++;
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);

   for (i = 0; i < length; i++)
   {
      void *arg = adt_ary_value(&args, i);
      apx_server_connection_event_func_t *callback = (apx_server_connection_event_func_t*) adt_ary_value(&callbacks, i);
      assert(callback != NULL);
      callback(arg, server_connection);
   }
   adt_ary_destroy(&args);
   adt_ary_destroy(&callbacks);
}

static void apx_server_trigger_disconnected_event(apx_server_t* self, apx_server_connection_t* server_connection)
{
   adt_ary_t args;
   adt_ary_t callbacks;
   int32_t length = 0;
   int32_t i = 0;

   assert(self != NULL);
   assert(server_connection != NULL);

   adt_ary_create(&args, NULL);
   adt_ary_create(&callbacks, NULL);

   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(&self->server_event_listeners);
   while(iter != NULL)
   {
      apx_server_event_listener_t *listener = (apx_server_event_listener_t*) iter->pItem;
      if ( (listener != NULL) && (listener->connection_closed != NULL) )
      {
         adt_ary_push(&args, (void*)listener->arg);
         adt_ary_push(&callbacks, (void*)listener->connection_closed);
         length++;
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);

   for (i = 0; i < length; i++)
   {
      void *arg = adt_ary_value(&args, i);
      apx_server_connection_event_func_t *callback = (apx_server_connection_event_func_t*) adt_ary_value(&callbacks, i);
      assert(callback != NULL);
      callback(arg, server_connection);
   }
   adt_ary_destroy(&args);
   adt_ary_destroy(&callbacks);
}

static void apx_server_init_extensions(apx_server_t* self)
{
   if  (self != NULL)
   {
      adt_list_elem_t *iter = adt_list_iter_first(&self->extension_manager);
      while(iter != NULL)
      {
         apx_server_extension_t *extension = (apx_server_extension_t*) iter->pItem;
         if (extension->handler.init != NULL)
         {
            extension->handler.init(self, extension->config);
            if (extension->config != NULL)
            {
               dtl_dv_dec_ref(extension->config);
               extension->config = NULL;
            }
            if (extension->name != NULL)
            {
/*               char msg[MAX_LOG_LEN];
               sprintf(msg, "Started extension %s", extension->name);
               apx_server_log_event(self, APX_LOG_LEVEL_INFO, "SERVER", msg);*/
            }
         }
         iter = adt_list_iter_next(iter);
      }
   }
}

static void apx_server_shutdown_extensions(apx_server_t* self)
{
   if  (self != NULL)
   {
      adt_list_elem_t *iter = adt_list_iter_first(&self->extension_manager);
      while(iter != NULL)
      {
        apx_server_extension_t *extension = (apx_server_extension_t*) iter->pItem;
        if (extension->handler.shutdown != NULL)
        {
           extension->handler.shutdown();
        }
        iter = adt_list_iter_next(iter);
      }
   }
}

static void apx_server_handle_event(void* arg, apx_event_t* event)
{
   apx_server_t *self = (apx_server_t*) arg;
   if ( (self != NULL) && (event != NULL) )
   {
      apx_log_level_t level;
      char *label;
      adt_str_t *str;
      const char *msg = NULL;
      rmf_file_info_t* file_info = NULL;
      apx_server_connection_t* server_connection = NULL;
      switch(event->ev_type)
      {
      case APX_EVENT_LOG_WRITE:
         apx_event_unpack_log_write(event, &level, &label, &str);
         msg = adt_str_cstr(str);
         if (label != NULL)
         {
            size_t label_size = strlen(label);
            apx_server_trigger_log_write_event(self, level, label, msg);
            MUTEX_LOCK(self->event_loop_lock);
            soa_free(&self->allocator, label, label_size + 1);
            MUTEX_UNLOCK(self->event_loop_lock);
         }
         else
         {
            printf("%s\n", msg);
         }
         adt_str_delete(str);
         break;
      case APX_EVENT_PROTOCOL_HEADER_ACCEPTED:
         apx_event_unpack_protocol_header_accepted(event, (apx_connection_base_t**)&server_connection);
         if (server_connection != NULL)
         {
            apx_server_connection_process_protocol_header_accepted_event(server_connection);
         }
         else
         {
            assert(0);
         }
         break;
      case APX_EVENT_REMOTE_FILE_PUBLISHED:
         apx_event_unpack_remote_file_published(event, (apx_connection_base_t**)&server_connection, &file_info);
         if ((server_connection != NULL) && (file_info != NULL) )
         {
            apx_server_connection_process_remote_file_published_event(server_connection, file_info);
            rmf_file_info_delete(file_info);
         }
         else
         {
            assert(0);
         }
         break;
      default:
         printf("[SERVER] Unhandled event %d\n", (int)event->ev_type);
      }
   }
}

static void apx_server_trigger_log_write_event(apx_server_t* self, apx_log_level_t level, const char* label, const char* msg)
{
   adt_ary_t args;
   adt_ary_t callbacks;
   int32_t length = 0;
   int32_t i = 0;

   adt_ary_create(&args, NULL);
   adt_ary_create(&callbacks, NULL);

   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t* iter = adt_list_iter_first(&self->server_event_listeners);
   while (iter != NULL)
   {
      apx_server_event_listener_t *listener = (apx_server_event_listener_t*) iter->pItem;
      if ( (listener != NULL) && (listener->server_write_log != NULL) )
      {
         adt_ary_push(&args, (void*)listener->arg);
         adt_ary_push(&callbacks, (void*)listener->server_write_log);
         length++;
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);
   for (i = 0; i < length; i++)
   {
      void* arg = adt_ary_value(&args, i);
      apx_server_log_write_event_func_t* callback = (apx_server_log_write_event_func_t*)adt_ary_value(&callbacks, i);
      assert(callback != NULL);
      callback(arg, level, label, msg);
   }
   adt_ary_destroy(&args);
   adt_ary_destroy(&callbacks);
}

static void apx_server_destroy_event(apx_server_t* self, apx_event_t* event)
{
   if ((self != NULL) && (event != NULL))
   {
      apx_event_destroy(event, &self->allocator);
   }
}
#ifndef UNIT_TEST

static apx_error_t apx_server_start_thread(apx_server_t* self)
{
   self->is_event_thread_valid = true;
#ifdef _MSC_VER
   THREAD_CREATE(self->event_thread, thread_task, self, self->thread_id);
   if(self->event_thread == INVALID_HANDLE_VALUE)
   {
      self->is_event_thread_valid = false;
      return APX_THREAD_CREATE_ERROR;
   }
#else
   int rc = THREAD_CREATE(self->event_thread, thread_task, self);
   if(rc != 0)
   {
      self->is_event_thread_valid = false;
      return APX_THREAD_CREATE_ERROR;
   }
#endif
   return APX_NO_ERROR;
}

static apx_error_t apx_server_stop_thread(apx_server_t* self)
{
   if (self->is_event_thread_valid)
   {
#ifdef _MSC_VER
      DWORD result = WaitForSingleObject(self->event_thread, 5000);
      if (result == WAIT_TIMEOUT)
      {
         return APX_THREAD_JOIN_TIMEOUT_ERROR;
      }
      else if (result == WAIT_FAILED)
      {
         return APX_THREAD_JOIN_ERROR;
      }
      CloseHandle(self->event_thread);
      self->event_thread = INVALID_HANDLE_VALUE;
#else
      if(pthread_equal(pthread_self(),self->event_thread) == 0)
      {
         void *status;
         int s = pthread_join(self->event_thread, &status);
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
   self->is_event_thread_valid = false;
   }
   return APX_NO_ERROR;
}

static THREAD_PROTO(thread_task, arg)
{
   apx_server_t *self = (apx_server_t*) arg;
   if (self != NULL)
   {
      printf("[SERVER] Starting event loop\n");
      apx_event_loop_run(&self->event_loop, apx_server_handle_event, (void*) self);
   }
   THREAD_RETURN(0);
}
#endif
