/*****************************************************************************
* \file      client.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX client class
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "apx/client.h"
#include "apx/client_internal.h"
#include "apx/client_connection.h"
#include "apx/socket_client_connection.h"
#include "apx/node_manager.h"
#include "apx/file_manager.h"
#include "apx/parser.h"
#include "apx/node_instance.h"
#include "apx/vm.h"
#include "msocket.h"
#include "adt_ary.h"
#include "adt_list.h"
#include "adt_hash.h"
#include "apx/event_listener.h"
#include "apx/compiler.h"
#include "pack.h"
#ifdef UNIT_TEST
#include "testsocket.h"
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_STACK_BUFFER_SIZE 256u
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_client_trigger_connected_event_on_listeners(apx_client_t *self, apx_client_connection_t *connection);
static void apx_client_trigger_disconnected_event_on_listeners(apx_client_t *self, apx_client_connection_t *connection);
static void apx_client_trigger_port_write_event_on_listeners(apx_client_t* self, apx_client_connection_t* connection, apx_port_instance_t* port_instance, uint8_t const* data, apx_size_t size);
static void apx_client_attach_local_nodes_to_connection(apx_client_t *self);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_client_create(apx_client_t *self)
{
   if( self != NULL )
   {
      self->event_listeners = adt_list_new(apx_client_event_listener_vdelete);
      if (self->event_listeners == NULL)
      {
         return APX_MEM_ERROR;
      }
      self->connection = (apx_client_connection_t*) NULL;
      self->vm = (apx_vm_t*) NULL;
      self->node_manager = apx_node_manager_new(APX_CLIENT_MODE);
      self->is_connected = false;
      MUTEX_INIT(self->lock);
      MUTEX_INIT(self->event_listener_lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_client_destroy(apx_client_t *self)
{
   if (self != NULL)
   {
      bool isConnected;
      MUTEX_LOCK(self->lock);
      isConnected = self->is_connected;
      MUTEX_UNLOCK(self->lock);
      if (isConnected)
      {
         apx_client_disconnect(self);
      }
      adt_list_delete(self->event_listeners);
      if (self->connection != NULL)
      {
         apx_connection_base_delete(&self->connection->base);
      }
      if (self->node_manager != NULL)
      {
         apx_node_manager_delete(self->node_manager);
      }
      if (self->vm != NULL)
      {
         apx_vm_delete(self->vm);
      }
      MUTEX_DESTROY(self->lock);
      MUTEX_DESTROY(self->event_listener_lock);
   }
}

apx_client_t DLL_PUBLIC *apx_client_new(void)
{
   apx_client_t *self = (apx_client_t*) malloc(sizeof(apx_client_t));
   if(self != NULL)
   {
      apx_error_t result = apx_client_create(self);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void DLL_PUBLIC apx_client_delete(apx_client_t *self)
{
   if (self != NULL)
   {
      apx_client_destroy(self);
      free(self);
   }
}

void DLL_PUBLIC apx_client_vdelete(void *arg)
{
   apx_client_delete((apx_client_t*) arg);
}

#ifdef UNIT_TEST
apx_error_t apx_client_connect_testsocket(apx_client_t *self, struct testsocket_tag *socket_object)
{
   if (self != NULL)
   {
      apx_client_socket_connection_t *socketConnection = apx_client_socket_connection_new(socket_object, APX_CONNECTION_TYPE_DEFAULT);
      if (socketConnection)
      {
         apx_error_t result;
         apx_client_attach_connection(self, &socketConnection->base);
         result = APX_NO_ERROR;//apx_client_socket_connection_connect(socketConnection);
         if (result == APX_NO_ERROR)
         {
            MUTEX_LOCK(self->lock);
            self->is_connected = true;
            MUTEX_UNLOCK(self->lock);
            testsocket_on_connect(socket_object);
         }
         return result;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
#else

/**
 * On connection error, the user can retreive the actual error using errno on Linux and WSAGetLastError on Windows
 */
apx_error_t apx_client_connect_tcp(apx_client_t *self, const char *address, uint16_t port)
{
   if (self != NULL)
   {
      apx_client_socket_connection_t *socketConnection = apx_client_socket_connection_new(NULL, APX_CONNECTION_TYPE_DEFAULT);
      if (socketConnection != NULL)
      {
         apx_error_t result;
         apx_client_attach_connection(self, (apx_client_connection_t*) socketConnection);
         result = apx_client_socket_connection_connect_tcp(socketConnection, address, port);
         if (result == APX_NO_ERROR)
         {
            MUTEX_LOCK(self->lock);
            self->is_connected = true;
            MUTEX_UNLOCK(self->lock);
         }
         return result;
      }
      else
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

# ifndef _WIN32
apx_error_t apx_client_connect_unix(apx_client_t *self, const char *socket_path)
{
   if (self != NULL)
   {
      apx_client_socket_connection_t *socketConnection = apx_client_socket_connection_new(NULL, APX_CONNECTION_TYPE_DEFAULT);
      if (socketConnection != NULL)
      {
         apx_error_t result;
         apx_client_attach_connection(self, (apx_client_connection_t*) socketConnection);
         result = apx_client_socket_connection_connect_unix(socketConnection, socket_path);
         if (result == APX_NO_ERROR)
         {
            MUTEX_LOCK(self->lock);
            self->is_connected = true;
            MUTEX_UNLOCK(self->lock);
         }
         return result;
      }
      else
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
# endif

#endif

void apx_client_disconnect(apx_client_t *self)
{
   if ( (self != NULL) && (self->connection != NULL))
   {
      apx_connection_base_close(&self->connection->base);
      apx_connection_base_stop(&self->connection->base);
      MUTEX_LOCK(self->lock);
      self->is_connected = false;
      MUTEX_UNLOCK(self->lock);
   }
}


void* apx_client_register_event_listener(apx_client_t *self, struct apx_client_event_listener_tag *listener)
{
   if ( (self != NULL) && (listener != NULL))
   {
      void *handle = (void*) apx_client_event_listener_clone(listener);
      if (handle != NULL)
      {
         MUTEX_LOCK(self->event_listener_lock);
         adt_list_insert(self->event_listeners, handle);
         MUTEX_UNLOCK(self->event_listener_lock);
      }
      return handle;
   }
   return NULL;
}


void apx_client_unregister_event_listener(apx_client_t *self, void *handle)
{
   if ( (self != NULL) && (handle != NULL) )
   {
      bool deleteSuccess = false;
      MUTEX_LOCK(self->event_listener_lock);
      deleteSuccess = adt_list_remove(self->event_listeners, handle);
      MUTEX_UNLOCK(self->event_listener_lock);
      if (deleteSuccess)
      {
         apx_client_event_listener_vdelete(handle);
      }
   }
}

int32_t apx_client_get_num_attached_nodes(apx_client_t *self)
{
   if (self != NULL)
   {
      int32_t retval;
      MUTEX_LOCK(self->lock);
      retval = (int32_t)apx_node_manager_length(self->node_manager);
      MUTEX_UNLOCK(self->lock);
      return retval;
   }
   return -1;
}

int32_t apx_client_get_num_event_listeners(apx_client_t *self)
{
   if (self != NULL)
   {
      int32_t retval;
      MUTEX_LOCK(self->event_listener_lock);
      retval = adt_list_length(self->event_listeners);
      MUTEX_UNLOCK(self->event_listener_lock);
      return retval;
   }
   return -1;
}

void apx_client_attach_connection(apx_client_t *self, apx_client_connection_t *connection)
{
   if ( (self != NULL) && (connection != NULL) )
   {
      self->connection = connection;
      apx_client_connection_set_client(connection, self);
      apx_client_connection_attach_node_manager(connection, self->node_manager);
      apx_client_attach_local_nodes_to_connection(self); //TODO: This should not be necessary as an explicit step.
                                                         // Merge functionality with call to to apx_client_connection_attach_node_manager
   }
}

apx_client_connection_t *apx_client_get_connection(apx_client_t *self)
{
   if (self != NULL)
   {
      return self->connection;
   }
   return NULL;
}

apx_error_t apx_client_build_node(apx_client_t *self, const char *definition_text)
{
   if (self != NULL && definition_text != NULL)
   {
      return apx_node_manager_build_node(self->node_manager, definition_text);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_client_get_error_line(apx_client_t *self)
{
   if (self != NULL)
   {
      return apx_node_manager_get_error_line(self->node_manager);
   }
   return -1;
}

apx_node_instance_t *apx_client_get_last_attached_node(apx_client_t *self)
{
   if (self != NULL)
   {
      return apx_node_manager_get_last_attached(self->node_manager);
   }
   return NULL;
}

struct apx_file_manager_tag *apx_client_get_file_manager(apx_client_t *self)
{
   if ( (self != NULL) && (self->connection != NULL))
   {
      return &self->connection->base.file_manager;
   }
   return NULL;
}

struct apx_node_manager_tag *apx_client_get_node_manager(apx_client_t *self)
{
   if (self != NULL)
   {
      return self->node_manager;
   }
   return NULL;
}

/*** Port Handle API ***/
apx_port_instance_t* apx_client_get_port_instance_by_name(apx_client_t* self, const char* node_name, const char* port_name)
{
   if ( (self != NULL) && (port_name != NULL))
   {
      apx_node_instance_t *node_instance = NULL;
      if (node_name == NULL)
      {
         node_instance = apx_client_get_last_attached_node(self);
      }
      else
      {
         node_instance = apx_node_manager_find(self->node_manager, node_name);
      }
      if (node_instance != NULL)
      {
         return (void*) apx_node_instance_find_port_by_name(node_instance, port_name);
      }
   }
   return NULL;
}

apx_port_instance_t* apx_client_get_provide_port_instance_by_id(apx_client_t* self, const char* node_name, apx_port_id_t port_id)
{
   if ( (self != NULL) && (port_id >= 0))
   {
      apx_node_instance_t *node_instance = NULL;
      if (node_name == NULL)
      {
         node_instance = apx_client_get_last_attached_node(self);
      }
      else
      {
         node_instance = apx_node_manager_find(self->node_manager, node_name);
      }
      if (node_instance != NULL)
      {
         return (void*)apx_node_instance_get_provide_port(node_instance, port_id);
      }
   }
   return (void*) NULL;
}

apx_port_instance_t* apx_client_get_require_port_instance_by_id(apx_client_t* self, const char* node_name, apx_port_id_t port_id)
{
   if ( (self != NULL) && (port_id >= 0))
   {
      apx_node_instance_t *node_instance = NULL;
      if (node_name == NULL)
      {
         node_instance = apx_client_get_last_attached_node(self);
      }
      else
      {
         node_instance = apx_node_manager_find(self->node_manager, node_name);
      }
      if (node_instance != NULL)
      {
         return (void*)apx_node_instance_get_require_port(node_instance, port_id);
      }
   }
   return NULL;
}

apx_error_t apx_client_write_port_data(apx_client_t* self, apx_port_instance_t* port_instance, const dtl_dv_t* dv)
{
   if ((self != NULL) && (port_instance != NULL) && (dv != NULL))
   {
      uint8_t stack_buffer[MAX_STACK_BUFFER_SIZE];
      apx_error_t result;
      uint8_t* write_buffer;
      bool is_heap_allocated_buffer = false;
      uint32_t const data_size = apx_port_instance_data_size(port_instance);
      uint32_t const offset = apx_port_instance_data_offset(port_instance);
      apx_program_t const* pack_program = apx_port_instance_pack_program(port_instance);

      if (apx_port_instance_port_type(port_instance) != APX_PROVIDE_PORT)
      {
         return APX_INVALID_PORT_HANDLE_ERROR;
      }
      if (pack_program == NULL)
      {
         return APX_INVALID_PROGRAM_ERROR;
      }
      if (data_size > MAX_STACK_BUFFER_SIZE)
      {
         write_buffer = (uint8_t*)malloc(data_size);
         if (write_buffer == NULL)
         {
            return APX_MEM_ERROR;
         }
         is_heap_allocated_buffer = true;
      }
      else
      {
         write_buffer = &stack_buffer[0];
      }
      assert(write_buffer != NULL);
      MUTEX_LOCK(self->lock);
      if (self->vm == NULL)
      {
         self->vm = apx_vm_new();
         if (self->vm == NULL)
         {
            MUTEX_UNLOCK(self->lock);
            if (is_heap_allocated_buffer) free(write_buffer);
            return APX_MEM_ERROR;
         }
      }
      assert(self->vm != NULL);
      result = apx_vm_select_program(self->vm, pack_program);
      if (result == APX_NO_ERROR)
      {
         result = apx_vm_set_write_buffer(self->vm, write_buffer, data_size);
      }
      if (result == APX_NO_ERROR)
      {
         result = apx_vm_pack_value(self->vm, dv);
      }
      MUTEX_UNLOCK(self->lock);
      if (result == APX_NO_ERROR)
      {
         result = apx_node_instance_write_provide_port_data(apx_port_instance_parent(port_instance), offset, write_buffer, data_size);
      }
      if (is_heap_allocated_buffer) free(write_buffer);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_read_port_data(apx_client_t* self, apx_port_instance_t* port_instance, dtl_dv_t** dv)
{
   if ((self != NULL) && (port_instance != NULL) && (dv != NULL))
   {
      uint8_t stack_buffer[MAX_STACK_BUFFER_SIZE];
      apx_error_t result;
      uint8_t* read_buffer;
      apx_node_data_t* node_data = NULL;
      bool is_heap_allocated_buffer = false;
      uint32_t const data_size = apx_port_instance_data_size(port_instance);
      uint32_t const offset = apx_port_instance_data_offset(port_instance);
      apx_program_t const* unpack_program = apx_port_instance_unpack_program(port_instance);

      if (apx_port_instance_port_type(port_instance) != APX_REQUIRE_PORT)
      {
         return APX_INVALID_PORT_HANDLE_ERROR;
      }
      if (unpack_program == NULL)
      {
         return APX_INVALID_PROGRAM_ERROR;
      }
      if (data_size > MAX_STACK_BUFFER_SIZE)
      {
         read_buffer = (uint8_t*)malloc(data_size);
         if (read_buffer == NULL)
         {
            return APX_MEM_ERROR;
         }
         is_heap_allocated_buffer = true;
      }
      else
      {
         read_buffer = &stack_buffer[0];
      }
      assert(read_buffer != NULL);
      node_data = apx_node_instance_get_node_data(apx_port_instance_parent(port_instance));
      if (node_data == NULL)
      {
         if (is_heap_allocated_buffer) free(read_buffer);
         return APX_NULL_PTR_ERROR;
      }
      result = apx_node_data_read_require_port_data(node_data, offset, read_buffer, data_size);
      if (result != APX_NO_ERROR)
      {
         if (is_heap_allocated_buffer) free(read_buffer);
         return result;
      }
      MUTEX_LOCK(self->lock);
      if (self->vm == NULL)
      {
         self->vm = apx_vm_new();
         if (self->vm == NULL)
         {
            MUTEX_UNLOCK(self->lock);
            if (is_heap_allocated_buffer) free(read_buffer);
            return APX_MEM_ERROR;
         }
      }
      assert(self->vm != NULL);
      result = apx_vm_select_program(self->vm, unpack_program);
      if (result == APX_NO_ERROR)
      {
         result = apx_vm_set_read_buffer(self->vm, read_buffer, data_size);
      }
      if (result == APX_NO_ERROR)
      {
         result = apx_vm_unpack_value(self->vm, dv);
      }
      MUTEX_UNLOCK(self->lock);
      if (is_heap_allocated_buffer) free(read_buffer);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/////////////////////// BEGIN CLIENT INTERNAL API /////////////////////

void apx_client_internal_connect_notification(apx_client_t* self, apx_client_connection_t* connection)
{
   if ((self != NULL) && (connection != NULL))
   {
      apx_client_trigger_connected_event_on_listeners(self, connection);
   }
}

void apx_client_internal_disconnect_notification(apx_client_t* self, apx_client_connection_t* connection)
{
   if ((self != NULL) && (connection != NULL))
   {
      apx_client_trigger_disconnected_event_on_listeners(self, connection);
   }
}

void apx_client_internal_require_port_write_notification(apx_client_t* self, apx_client_connection_t* connection, apx_port_instance_t* port_instance, const uint8_t* data, apx_size_t size)
{
   if ((self != NULL) && (connection != NULL))
   {
      apx_client_trigger_port_write_event_on_listeners(self, connection, port_instance, data, size);
   }
}

/////////////////////// END CLIENT INTERNAL API /////////////////////

/////////////////////// BEGIN UNIT TEST API /////////////////////
#ifdef UNIT_TEST

#define APX_CLIENT_RUN_CYCLES 10

void apx_client_run(apx_client_t *self)
{
   if (self != NULL && (self->connection != NULL))
   {
      int32_t i;
      for(i=0;i<APX_CLIENT_RUN_CYCLES;i++)
      {
         apx_client_connection_run(self->connection);
      }
   }
}
#endif

/////////////////////// END UNIT TEST API /////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_client_trigger_connected_event_on_listeners(apx_client_t *self, apx_client_connection_t *connection)
{
   adt_ary_t args;
   adt_ary_t callbacks;
   int32_t length = 0;
   int32_t i = 0;

   assert(self != NULL);
   assert(connection != NULL);

   adt_ary_create(&args, NULL);
   adt_ary_create(&callbacks, NULL);

   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(self->event_listeners);
   while (iter != NULL)
   {
      apx_client_event_listener_t *listener = (apx_client_event_listener_t*) iter->pItem;
      if ( (listener != NULL) && (listener->connected != NULL) )
      {
         adt_ary_push(&args, (void*)listener->arg);
         adt_ary_push(&callbacks, (void*)listener->connected);
         length++;
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);

   for (i = 0; i < length; i++)
   {
      void *arg = adt_ary_value(&args, i);
      apx_client_connection_event_func_t *callback = (apx_client_connection_event_func_t*) adt_ary_value(&callbacks, i);
      assert(callback != NULL);
      callback(arg, connection);
   }
   adt_ary_destroy(&args);
   adt_ary_destroy(&callbacks);
}

static void apx_client_trigger_disconnected_event_on_listeners(apx_client_t *self, apx_client_connection_t *connection)
{
   adt_ary_t args;
   adt_ary_t callbacks;
   int32_t length = 0;
   int32_t i = 0;

   assert(self != NULL);
   assert(connection != NULL);

   adt_ary_create(&args, NULL);
   adt_ary_create(&callbacks, NULL);

   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(self->event_listeners);
   while (iter != NULL)
   {
      apx_client_event_listener_t *listener = (apx_client_event_listener_t*) iter->pItem;
      if ( (listener != NULL) && (listener->disconnected != NULL) )
      {
         adt_ary_push(&args, (void*)listener->arg);
         adt_ary_push(&callbacks, (void*)listener->disconnected);
         length++;
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);

   for (i = 0; i < length; i++)
   {
      void *arg = adt_ary_value(&args, i);
      apx_client_connection_event_func_t *callback = (apx_client_connection_event_func_t*) adt_ary_value(&callbacks, i);
      assert(callback != NULL);
      callback(arg, connection);
   }
   adt_ary_destroy(&args);
   adt_ary_destroy(&callbacks);
}

static void apx_client_trigger_port_write_event_on_listeners(apx_client_t* self, apx_client_connection_t* connection, apx_port_instance_t* port_instance, uint8_t const* data, apx_size_t size)
{
   adt_ary_t args;
   adt_ary_t callbacks;
   int32_t length = 0;
   int32_t i = 0;

   (void)connection;

   adt_ary_create(&args, NULL);
   adt_ary_create(&callbacks, NULL);

   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(self->event_listeners);
   while (iter != NULL)
   {
      apx_client_event_listener_t *listener = (apx_client_event_listener_t*) iter->pItem;
      if ( (listener != NULL) && (listener->require_port_write != NULL) )
      {
         adt_ary_push(&args, (void*)listener->arg);
         adt_ary_push(&callbacks, (void*)listener->require_port_write);
         length++;
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);

   for (i = 0; i < length; i++)
   {
      void *arg = adt_ary_value(&args, i);
      apx_port_data_write_func_t *callback = (apx_port_data_write_func_t*) adt_ary_value(&callbacks, i);
      assert(callback != NULL);
      callback(arg, port_instance, data, size);
   }
   adt_ary_destroy(&args);
   adt_ary_destroy(&callbacks);
}


static void apx_client_attach_local_nodes_to_connection(apx_client_t *self)
{
   if (self->connection != NULL)
   {
      adt_ary_t *nodeList = adt_ary_new( (void(*)(void*)) 0);
      if (nodeList != NULL)
      {
         int32_t i;
         int32_t numNodes;
         numNodes = apx_node_manager_values(self->node_manager, nodeList);
         for (i=0; i<numNodes; i++)
         {
            apx_node_instance_t *nodeInstance = (apx_node_instance_t*) adt_ary_value(nodeList, i);
            apx_client_connection_attach_node_instance(self->connection, nodeInstance);
         }
         adt_ary_delete(nodeList);
      }
   }
}
