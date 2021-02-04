/*****************************************************************************
* \file      server.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX server class
*
* Copyright (c) 2017-2021 Conny Gustafsson
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
#include "apx/server.h"
//#include "apx/logging.h"
#include "apx/file_manager.h"
#include "apx/event_listener.h"
#include "apx/log_event.h"
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
static void apx_server_attach_and_start_connection(apx_server_t *self, apx_serverConnection_t *new_connection);
static void apx_server_trigger_connected_event(apx_server_t *self, apx_serverConnection_t * server_connection);
static void apx_server_trigger_disconnected_event(apx_server_t *self, apx_serverConnection_t *server_connection);
static void apx_server_trigger_log_event(apx_server_t *self, apx_logLevel_t level, const char *label, const char *msg);
static void apx_server_init_extensions(apx_server_t *self);
static void apx_server_shutdown_extensions(apx_server_t *self);
static void apx_server_handle_event(void *arg, apx_event_t *event);
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
      adt_list_create(&self->server_event_listeners, apx_serverEventListener_vdelete);
      apx_portSignatureMap_create(&self->port_signature_map);
      apx_connectionManager_create(&self->connection_manager);
      adt_list_create(&self->extension_manager, apx_serverExtension_vdelete);
      adt_ary_create(&self->modified_nodes, (void(*)(void*)) 0);
      soa_init(&self->allocator);
      apx_eventLoop_create(&self->event_loop);
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
      soa_destroy(&self->allocator);
      adt_list_destroy(&self->extension_manager);
      adt_ary_destroy(&self->modified_nodes);
      MUTEX_LOCK(self->event_listener_lock);
      adt_list_destroy(&self->server_event_listeners);
      MUTEX_UNLOCK(self->event_listener_lock);
      apx_connectionManager_destroy(&self->connection_manager);
      apx_portSignatureMap_destroy(&self->port_signature_map);
      MUTEX_UNLOCK(self->global_lock);
      apx_eventLoop_destroy(&self->event_loop);
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

   if( self != 0 )
   {
      apx_server_init_extensions(self);
#ifndef UNIT_TEST
      apx_connectionManager_start(&self->connection_manager);
      if (self->is_event_thread_valid == false)
      {
         apx_server_start_thread(self);
      }
#endif
   }
}

void apx_server_stop(apx_server_t *self)
{
   if( self != 0)
   {

#ifndef UNIT_TEST
      apx_connectionManager_stop(&self->connection_manager);
#endif
      apx_server_shutdown_extensions(self);
#ifndef UNIT_TEST
      apx_eventLoop_exit(&self->event_loop);
      if (self->is_event_thread_valid)
      {
         apx_server_stop_thread(self);
      }
#endif
   }
}

void* apx_server_register_event_listener(apx_server_t* self, apx_serverEventListener_t* event_listener)
{
   if ( (self != NULL) && (event_listener != NULL))
   {
      void *handle = (void*) apx_serverEventListener_clone(event_listener);
      if (handle != 0)
      {
         MUTEX_LOCK(self->event_listener_lock);
         adt_list_insert(&self->server_event_listeners, handle);
         MUTEX_UNLOCK(self->event_listener_lock);
      }
      return handle;
   }
   return (void*) 0;
}

void apx_server_unregister_event_listener(apx_server_t *self, void *handle)
{
   if ( (self != NULL) && (handle != 0))
   {
      bool isFound;
      MUTEX_LOCK(self->event_listener_lock);
      isFound = adt_list_remove(&self->server_event_listeners, handle);
      MUTEX_UNLOCK(self->event_listener_lock);
      if (isFound == true)
      {
         apx_serverEventListener_vdelete(handle);
      }
   }
}

void apx_server_accept_connection(apx_server_t* self, apx_serverConnection_t* server_connection)
{
   if ( (self != NULL) && (server_connection != NULL))
   {
      apx_server_attach_and_start_connection(self, server_connection);
   }
}

apx_error_t apx_server_detach_connection(apx_server_t* self, apx_serverConnection_t* server_connection)
{
   if ( (self != NULL) && (server_connection != NULL))
   {
      apx_error_t result;
      apx_connectionManager_detach(&self->connection_manager, server_connection);
      result = apx_serverConnection_disconnected_notification(server_connection);
      if (result == APX_NO_ERROR)
      {
         apx_server_trigger_disconnected_event(self, server_connection);
      }
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_add_extension(apx_server_t* self, const char* name, apx_serverExtensionHandler_t* handler, dtl_dv_t* config)
{
   if ( (self != NULL) && (handler != 0) )
   {
      apx_serverExtension_t *extension = apx_serverExtension_new(name, handler, config);
      if (extension == 0)
      {
         return APX_MEM_ERROR;
      }
      adt_list_insert(&self->extension_manager, (void*) extension);
      if (config != 0)
      {
         dtl_dv_inc_ref(config);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_server_log_event(apx_server_t* self, apx_logLevel_t level, const char* label, const char* msg)
{
   if ( (self != NULL) && (level <= APX_MAX_LOG_LEVEL) && (msg != 0) )
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
         MUTEX_LOCK(self->event_loop_lock);
         labelStr = soa_alloc(&self->allocator, labelSize+1);
         MUTEX_UNLOCK(self->event_loop_lock);
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
      apx_eventLoop_append(&self->event_loop, &event);
   }
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

apx_error_t apx_server_connect_node_instance_provide_ports(apx_server_t* self, apx_nodeInstance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != 0) )
   {
      return apx_portSignatureMap_connect_provide_ports(&self->port_signature_map, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_connect_node_instance_require_ports(apx_server_t* self, apx_nodeInstance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != 0) )
   {
      return apx_portSignatureMap_connect_require_ports(&self->port_signature_map, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_disconnect_node_instance_provide_ports(apx_server_t* self, apx_nodeInstance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != 0) )
   {
      return apx_portSignatureMap_disconnect_provide_ports(&self->port_signature_map, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_server_disconnect_node_instance_require_ports(apx_server_t* self, apx_nodeInstance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != 0) )
   {
      return apx_portSignatureMap_disconnect_require_ports(&self->port_signature_map, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Is is assumed that the server global lock is held by the caller of this function
 */
apx_error_t apx_server_process_require_port_connector_changes(apx_server_t* self, apx_nodeInstance_t* require_node_instance, apx_portConnectorChangeTable_t* connector_changes)
{
   if ( (self != NULL) && (require_node_instance != NULL) && (connector_changes != NULL) )
   {
      apx_size_t num_require_ports;
      apx_portId_t port_id;
      num_require_ports = apx_nodeInstance_get_num_require_ports(require_node_instance);
      assert(connector_changes->num_ports == num_require_ports);
      for (port_id = 0u; port_id < num_require_ports; port_id++)
      {
         apx_portInstance_t *require_port;
         apx_portConnectorChangeEntry_t *entry;
         require_port = apx_nodeInstance_get_require_port(require_node_instance, port_id);
         entry = apx_portConnectorChangeTable_get_entry(connector_changes, port_id);
         assert( (require_port != NULL) && (entry != NULL));
         if (entry->count > 0)
         {
            if (entry->count == 1)
            {
               apx_error_t rc;
               apx_portInstance_t *provide_port = entry->data.port_instance;
               assert(provide_port != NULL);
               rc = apx_nodeInstance_handle_require_port_connected_to_provide_port(require_port, provide_port);
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
apx_error_t apx_server_process_provide_port_connector_changes(apx_server_t* self, apx_nodeInstance_t* provide_node_instance, apx_portConnectorChangeTable_t* connector_changes)
{
   if ((self != NULL) && (provide_node_instance != NULL) && (connector_changes != NULL))
   {
      apx_portCount_t num_provide_ports;
      apx_portId_t port_id;
      num_provide_ports = apx_nodeInstance_get_num_provide_ports(provide_node_instance);
      assert(connector_changes->num_ports == num_provide_ports);
      apx_nodeInstance_lock_port_connector_table(provide_node_instance);
      for (port_id = 0u; port_id < num_provide_ports; port_id++)
      {
         apx_portInstance_t *provide_port;
         apx_portConnectorChangeEntry_t *entry;
         entry = apx_portConnectorChangeTable_get_entry(connector_changes, port_id);
         provide_port = apx_nodeInstance_get_provide_port(provide_node_instance, port_id);
         assert(entry != 0);
         assert(provide_port != 0);
         if (entry->count > 0)
         {
            if (entry->count == 1)
            {
               apx_error_t rc;
               apx_portInstance_t *require_port = entry->data.port_instance;
               assert(require_port != 0);
               rc = apx_nodeInstance_handle_provide_port_connected_to_require_port(provide_port, require_port);
               if (rc != APX_NO_ERROR)
               {
                  apx_nodeInstance_unlock_port_connector_table(provide_node_instance);
                  return rc;
               }
            }
            else
            {
               int32_t i;
               for(i=0; i < entry->count; i++)
               {
                  apx_error_t rc;
                  apx_portInstance_t *require_port = adt_ary_value(entry->data.array, i);
                  assert(require_port != 0);
                  rc = apx_nodeInstance_handle_provide_port_connected_to_require_port(provide_port, require_port);
                  if (rc != APX_NO_ERROR)
                  {
                     apx_nodeInstance_unlock_port_connector_table(provide_node_instance);
                     return rc;
                  }
               }
            }
         }
      }
      apx_nodeInstance_unlock_port_connector_table(provide_node_instance);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Note: Should only be used when caller holds globalLock
 */
apx_error_t apx_server_insert_modified_node_instance(apx_server_t* self, apx_nodeInstance_t* node_instance)
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
   return (adt_ary_t*) 0;
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
         apx_nodeInstance_t *node_instance = (apx_nodeInstance_t*) adt_ary_value(&self->modified_nodes, i);
         assert(node_instance != NULL);
         apx_nodeInstance_clear_provide_port_connector_changes(node_instance, true);
         apx_nodeInstance_clear_require_port_connector_changes(node_instance, true);
      }
      if (num_nodes > 0)
      {
         adt_ary_clear(&self->modified_nodes);
      }
   }
}

#ifdef UNIT_TEST
void apx_server_run(apx_server_t *self)
{
   if (self != NULL)
   {
      apx_eventLoop_runAll(&self->event_loop, apx_server_handle_event, (void*) self);
      apx_connectionManager_run(&self->connection_manager);
   }
}

apx_serverConnection_t* apx_server_get_last_connection(apx_server_t const* self)
{
   if (self != NULL)
   {
      return apx_connectionManager_get_last_connection(&self->connection_manager);
   }
   return (apx_serverConnection_t*) 0;
}

apx_portSignatureMap_t* apx_server_get_port_signature_map(apx_server_t const* self)
{
   if (self != NULL)
   {
      return (apx_portSignatureMap_t*) &self->port_signature_map;
   }
   return (apx_portSignatureMap_t*) 0;
}

#endif


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_server_attach_and_start_connection(apx_server_t* self, apx_serverConnection_t* new_connection)
{
   if (apx_connectionManager_get_num_connections(&self->connection_manager) < APX_SERVER_MAX_CONCURRENT_CONNECTIONS)
   {
      apx_connectionManager_attach(&self->connection_manager, new_connection);
      apx_serverConnection_set_server(new_connection, self);
      apx_server_trigger_connected_event(self, new_connection);
      apx_connectionBase_start(&new_connection->base);
   }
   else
   {
      printf("[SERVER] Concurrent connection limit exceeded\n");
   }
}
static void apx_server_trigger_connected_event(apx_server_t* self, apx_serverConnection_t* server_connection)
{
   assert(self != NULL);
   assert(server_connection != NULL);
   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(&self->server_event_listeners);
   while(iter != 0)
   {
      apx_serverEventListener_t *listener = (apx_serverEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->serverConnect1 != NULL) )
      {
         listener->serverConnect1(listener->arg, server_connection);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);
}

static void apx_server_trigger_disconnected_event(apx_server_t* self, apx_serverConnection_t* server_connection)
{
   assert(self != NULL);
   assert(server_connection != NULL);
   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(&self->server_event_listeners);
   while(iter != 0)
   {
      apx_serverEventListener_t *listener = (apx_serverEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->serverDisconnect1 != NULL) )
      {
         listener->serverDisconnect1(listener->arg, server_connection);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);
}

static void apx_server_trigger_log_event(apx_server_t* self, apx_logLevel_t level, const char* label, const char* msg)
{
   adt_list_elem_t *iter = adt_list_iter_first(&self->server_event_listeners);
   (void)level;
   (void)label;
   (void)msg;
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

static void apx_server_init_extensions(apx_server_t* self)
{
   if  (self != NULL)
   {
      adt_list_elem_t *iter = adt_list_iter_first(&self->extension_manager);
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

static void apx_server_shutdown_extensions(apx_server_t* self)
{
   if  (self != NULL)
   {
      adt_list_elem_t *iter = adt_list_iter_first(&self->extension_manager);
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

static void apx_server_handle_event(void* arg, apx_event_t* event)
{
   apx_server_t *self = (apx_server_t*) arg;
   if ( (self != NULL) && (event != NULL) )
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
            apx_server_trigger_log_event(self, level, label, msg);
            MUTEX_LOCK(self->event_loop_lock);
            soa_free(&self->allocator, label, labelSize+1);
            MUTEX_UNLOCK(self->event_loop_lock);
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

static apx_error_t apx_server_start_thread(apx_server_t* self);
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
      apx_eventLoop_run(&self->event_loop, apx_server_handleEvent, (void*) self);
   }
   THREAD_RETURN(0);
}
#endif
