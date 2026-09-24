/*****************************************************************************
* \file      server_connection.c
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Base class for all APX server connections
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include "apx/server_connection.h"
#include "apx/port_connector_change_ref.h"
#include "bstr.h"
#include "apx/numheader.h"
#include "apx/file.h"
#include "apx/remotefile.h"
#include "apx/util.h"
#include "apx/server.h"
#include "apx/event.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#else
#define vfree free
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_HEADER_LEN 128
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t remote_file_published_notification(apx_server_connection_t* self, apx_file_t* file);
static apx_error_t process_new_definition_data_file(apx_server_connection_t* self, apx_file_t* file);
static apx_error_t process_new_provide_port_data_file(apx_server_connection_t* self, apx_file_t* file);
static apx_error_t create_new_node_instance(apx_server_connection_t* self, apx_node_manager_t* node_manager,  apx_file_t* definition_file);
static apx_error_t remote_file_write_notification(apx_server_connection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);
static uint8_t const* parse_message(apx_server_connection_t* self, uint8_t const* begin, uint8_t const* end, apx_error_t* error_code, apx_size_t* msg_size_hint);
static bool process_greeting_message(apx_server_connection_t* self, uint8_t const* msg_data, apx_size_t msg_size, apx_error_t* error_code);
static void apx_server_connection_node_created_notification(apx_server_connection_t* self, apx_node_instance_t* node_instance);
static apx_error_t detach_all_nodes(apx_server_connection_t* self);
static void remove_nodes_from_signature_map(apx_server_connection_t* self, adt_ary_t* node_instance_array);
static apx_error_t gather_provide_port_connector_changes(adt_ary_t* node_instance_array, adt_ary_t* provider_change_array);
static apx_error_t gather_require_port_connector_changes(adt_ary_t* node_instance_array, adt_ary_t* requester_change_array);
static apx_error_t process_disconnected_provider_nodes(adt_ary_t* provider_change_array);
static apx_error_t process_disconnected_requester_nodes(adt_ary_t* requester_change_array);
static void emit_remote_file_published_event(apx_server_connection_t* self, apx_file_t* file);
static void emit_protocol_header_accepted(apx_server_connection_t* self);
static apx_error_t parse_protocol_header_line(apx_server_connection_t* self, uint8_t const* begin, uint8_t const* end);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_server_connection_create(apx_server_connection_t* self, apx_connection_base_vtable_t* base_connection_vtable, apx_connection_interface_t* connection_interface)
{
   if (self != NULL)
   {
      apx_error_t error_code;
      //init non-overridable virtual functions
      base_connection_vtable->node_created_notification = apx_server_connection_vnode_created_notification;
      connection_interface->remote_file_published_notification = apx_server_connection_vremote_file_published_notification;
      connection_interface->remote_file_write_notification = apx_server_connection_vremote_file_write_notification;
      adt_list_create(&self->event_listeners, apx_connection_event_listener_vdelete);
      error_code = apx_connection_base_create(&self->base, APX_SERVER_MODE, base_connection_vtable, connection_interface);
      MUTEX_INIT(self->event_listener_lock);
      self->connection_state = APX_CONNECTION_STATE_CREATED;
      self->parent = NULL;
      self->last_error = APX_NO_ERROR;
      self->tag = NULL;
      return error_code;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_server_connection_destroy(apx_server_connection_t* self)
{
   if (self != NULL)
   {
      MUTEX_DESTROY(self->event_listener_lock);
      adt_list_destroy(&self->event_listeners);
      apx_connection_base_destroy(&self->base);
      if (self->tag != NULL)
      {
         adt_str_delete(self->tag);
      }
   }
}

apx_file_manager_t* apx_server_connection_get_file_manager(apx_server_connection_t* self)
{
   if (self != NULL)
   {
      return apx_connection_base_get_file_manager(&self->base);
   }
   return NULL;
}

void apx_server_connection_start(apx_server_connection_t* self)
{
   (void)self;
}


void apx_server_connection_close(apx_server_connection_t* self)
{
   (void)self;
}

uint32_t apx_server_connection_get_total_bytes_received(apx_server_connection_t* self)
{
   (void)self;
   return 0u;
}

uint32_t apx_server_connection_get_total_bytes_sent(apx_server_connection_t* self)
{
   (void)self;
   return 0u;
}

void* apx_server_connection_register_event_listener(apx_server_connection_t* self, apx_server_connection_event_listener_t* event_listener)
{
   if ((self != NULL) && (event_listener != NULL))
   {
      void* handle = (void*)apx_connection_event_listener_clone(event_listener);
      if (handle != NULL)
      {
         MUTEX_LOCK(self->event_listener_lock);
         adt_list_insert(&self->event_listeners, handle);
         MUTEX_UNLOCK(self->event_listener_lock);
      }
      return handle;
   }
   return NULL;
}

void apx_server_connection_unregister_event_listener(apx_server_connection_t* self, void* handle)
{
   if ((self != NULL) && (handle != NULL))
   {
      bool is_found;
      MUTEX_LOCK(self->event_listener_lock);
      is_found = adt_list_remove(&self->event_listeners, handle);
      MUTEX_UNLOCK(self->event_listener_lock);
      if (is_found)
      {
         apx_connection_event_listener_vdelete(handle);
      }
   }
}

void apx_server_connection_set_connection_type(apx_server_connection_t* self, apx_connection_type_t connection_type)
{
   if (self != NULL)
   {
      apx_connection_base_set_connection_type(&self->base, connection_type);
   }
}

apx_connection_type_t apx_server_connection_get_connection_type(apx_server_connection_t const* self)
{
   if (self != NULL)
   {
      return apx_connection_base_get_connection_type(&self->base);
   }
   return APX_CONNECTION_TYPE_DEFAULT;
}

void apx_server_connection_set_num_header_size(apx_server_connection_t* self, apx_size_t size)
{
   if (self != NULL)
   {
      apx_connection_base_set_num_header_size(&self->base, size);
   }
}

apx_size_t apx_server_connection_get_num_header_size(apx_server_connection_t const* self)
{
   if (self != NULL)
   {
      return apx_connection_base_get_num_header_size(&self->base);
   }
   return 0u;
}

void apx_server_connection_set_rmf_proto_id(apx_server_connection_t* self, rmf_version_id_t version_id)
{
   if (self != NULL)
   {
      apx_connection_base_set_rmf_proto_id(&self->base, version_id);
   }
}

rmf_version_id_t apx_server_connection_get_rmf_proto_id(apx_server_connection_t const* self)
{
   if (self != NULL)
   {
      return apx_connection_base_get_rmf_proto_id(&self->base);
   }
   return RMF_PROTOCOL_VERSION_ID_NONE;
}


void apx_server_connection_greeting_header_accepted_notification(apx_server_connection_t* self)
{
   if (self != NULL)
   {
      self->connection_state = APX_CONNECTION_STATE_ACCEPTED;
      apx_file_manager_connected(&self->base.file_manager);
      emit_protocol_header_accepted(self);
   }
}

void apx_server_connection_connected_notification(apx_server_connection_t* self)
{
   if (self != NULL)
   {
      self->connection_state = APX_CONNECTION_STATE_CONNECTING;
   }
}

apx_error_t apx_server_connection_disconnected_notification(apx_server_connection_t* self)
{
   if (self != NULL)
   {
      apx_connection_base_disconnect_notification(&self->base);
      return detach_all_nodes(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_server_connection_attach_node_manager(apx_server_connection_t* self, apx_node_manager_t* node_manager)
{
   if (self != NULL)
   {
      apx_connection_base_attach_node_manager(&self->base, node_manager);
   }
}

apx_node_manager_t* apx_server_connection_get_node_manager(apx_server_connection_t* self)
{
   if (self != NULL)
   {
      return apx_connection_base_get_node_manager(&self->base);
   }
   return NULL;
}

int apx_server_connection_on_data_received(apx_server_connection_t* self, uint8_t const* data, apx_size_t data_size, apx_size_t* parse_len, apx_size_t* msg_size_hint)
{
   if ((self != NULL) && (data != NULL) && (data_size > 0u) && (parse_len != NULL))
   {
      apx_size_t total_parse_len = 0u;
      uint8_t const* next = data;
      uint8_t const* end = data + data_size;
      if (msg_size_hint != NULL)
      {
         *msg_size_hint = 0u;
      }
      while (next < end)
      {
         uint8_t const* result;
         apx_error_t error_code = APX_NO_ERROR;
         result = parse_message(self, next, end, &error_code, msg_size_hint);
         if (error_code == APX_NO_ERROR)
         {
            assert((result >= next) && (result <= end));
            if (result == next)
            {
               // No more complete messages can be parsed. There may be a partial
               // message left in buffer, but we leave it in the buffer until
               // more data has arrived.
               break;
            }
            next = result;
            total_parse_len = (apx_size_t)(next - data);
            assert(total_parse_len <= data_size);
         }
         else
         {
            self->last_error = error_code;
            return -1;
         }
      }
      if (total_parse_len > 0u && msg_size_hint != NULL)
      {
         *msg_size_hint = 0u;
      }
      *parse_len = total_parse_len;
      return 0;
   }
   return -1;
}

void apx_server_connection_vnode_created_notification(void* arg, apx_node_instance_t* node_instance)
{
   apx_server_connection_node_created_notification((apx_server_connection_t*)arg, node_instance);
}

void apx_server_connection_set_connection_id(apx_server_connection_t* self, uint32_t connection_id)
{
   if (self != NULL)
   {
      apx_connection_base_set_connection_id(&self->base, connection_id);
   }
}

uint32_t apx_server_connection_get_connection_id(apx_server_connection_t* self)
{
   if (self != NULL)
   {
      return self->base.connection_id;
   }
   return APX_INVALID_CONNECTION_ID;
}

void apx_server_connection_set_server(apx_server_connection_t* self, struct apx_server_tag* server)
{
   if (self != NULL)
   {
      self->parent = server;
   }
}

void apx_server_connection_require_port_data_written(apx_server_connection_t* self, apx_node_instance_t* node_instance, apx_size_t offset, apx_size_t size)
{
   if (self != NULL)
   {
      (void)node_instance;
      (void)offset;
      (void)size;
   }
}

apx_error_t apx_server_connection_attach_node_instance(apx_server_connection_t* self, apx_node_instance_t* node_instance)
{
   if ((self != NULL) && (node_instance != NULL))
   {
      apx_file_manager_t* file_manager = apx_connection_base_get_file_manager(&self->base);
      assert(file_manager != NULL);
      return apx_node_instance_attach_to_file_manager(node_instance, file_manager);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_server_connection_set_tag(apx_server_connection_t* self, char const* tag)
{
   if ((self != NULL) && (tag != NULL))
   {
      self->tag = adt_str_new_cstr(tag);
   }
}

adt_str_t* apx_server_connection_get_tag(apx_server_connection_t const* self)
{
   if ( (self != NULL) && (self->tag != NULL))
   {
      return adt_str_clone(self->tag);
   }
   return NULL;
}

//ConnectionInterface API
apx_error_t apx_server_connection_vremote_file_published_notification(void* arg, apx_file_t* file)
{
   return remote_file_published_notification((apx_server_connection_t*)arg, file);
}

apx_error_t apx_server_connection_vremote_file_write_notification(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   return remote_file_write_notification((apx_server_connection_t*)arg, file, offset, data, size);
}

//Internal Event API
void apx_server_connection_process_protocol_header_accepted_event(apx_server_connection_t* self)
{
   adt_ary_t args;
   adt_ary_t callbacks;
   int32_t length = 0;
   int32_t i = 0;
   adt_ary_create(&args, NULL);
   adt_ary_create(&callbacks, NULL);
   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t* iter = adt_list_iter_first(&self->event_listeners);
   while (iter != NULL)
   {
      apx_server_connection_event_listener_t* listener = (apx_server_connection_event_listener_t*)iter->pItem;
      if ((listener != NULL) && (listener->protocol_header_accepted != NULL))
      {
         adt_ary_push(&args, (void*)listener->arg);
         adt_ary_push(&callbacks, (void*)listener->protocol_header_accepted);
         length++;
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);
   for (i = 0; i < length; i++)
   {
      void *arg = adt_ary_value(&args, i);
      apx_protocol_header_accepted_func_t* callback = (apx_protocol_header_accepted_func_t*)adt_ary_value(&callbacks, i);
      assert(callback != NULL);
      callback(arg, &self->base);
   }
   adt_ary_destroy(&args);
   adt_ary_destroy(&callbacks);
}

void apx_server_connection_process_remote_file_published_event(apx_server_connection_t* self, rmf_file_info_t* file_info)
{
   adt_ary_t args;
   adt_ary_t callbacks;
   int32_t length = 0;
   int32_t i = 0;
   adt_ary_create(&args, NULL);
   adt_ary_create(&callbacks, NULL);
   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t* iter = adt_list_iter_first(&self->event_listeners);
   while (iter != NULL)
   {
      apx_server_connection_event_listener_t* listener = (apx_server_connection_event_listener_t*)iter->pItem;
      if ((listener != NULL) && (listener->file_published != NULL))
      {
         adt_ary_push(&args, (void*)listener->arg);
         adt_ary_push(&callbacks, (void*)listener->file_published);
         length++;
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);
   for (i = 0; i < length; i++)
   {
      void* arg = adt_ary_value(&args, i);
      apx_file_event_func_t* callback = (apx_file_event_func_t*)adt_ary_value(&callbacks, i);
      assert(callback != NULL);
      callback(arg, &self->base, file_info);
   }
   adt_ary_destroy(&args);
   adt_ary_destroy(&callbacks);
}

apx_connection_state_t apx_server_connection_get_connection_state(apx_server_connection_t const* self)
{
   if (self != NULL)
   {
      return self->connection_state;
   }
   return APX_CONNECTION_STATE_CLOSED;
}

//Unit Test API
#ifdef UNIT_TEST
void apx_server_connection_run(apx_server_connection_t* self)
{
   if (self != NULL)
   {
      //apx_connection_base_run_all(&self->base);
      apx_file_manager_run(&self->base.file_manager);
   }
}
#endif



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t remote_file_published_notification(apx_server_connection_t* self, apx_file_t* file)
{
   if ((self != NULL) && (file != NULL))
   {
      apx_file_type_t file_type = apx_file_get_apx_file_type(file);
      emit_remote_file_published_event(self, file);
      if (file_type == APX_PROVIDE_PORT_DATA_FILE_TYPE)
      {
         return process_new_provide_port_data_file(self, file);
      }
      else if (file_type == APX_DEFINITION_FILE_TYPE)
      {
         return process_new_definition_data_file(self, file);
      }
      else
      {
         //TODO: Add generic handling of new file types
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t process_new_definition_data_file(apx_server_connection_t* self, apx_file_t* file)
{
   assert((self != NULL) && (file != NULL));
   char* base_name = rmf_file_info_base_name(apx_file_get_file_info(file));
   if (base_name != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_node_manager_t* node_manager = apx_connection_base_get_node_manager(&self->base);
      apx_node_instance_t* node_instance = apx_node_manager_find(node_manager, base_name);
      free(base_name);
      if (node_instance == NULL)
      {
         retval = create_new_node_instance(self, node_manager, file);
      }
      else
      {
         //node already exist with that name
         retval = APX_NODE_ALREADY_EXISTS_ERROR;
      }
      return retval;
   }
   return APX_MEM_ERROR;
}

static apx_error_t process_new_provide_port_data_file(apx_server_connection_t* self, apx_file_t* file)
{
   assert((self != NULL) && (file != NULL));
   char* base_name = rmf_file_info_base_name(apx_file_get_file_info(file));
   if (base_name != NULL)
   {
      apx_node_instance_t* node_instance = apx_node_manager_find(self->base.node_manager, base_name);
      free(base_name);
      if (node_instance != NULL)
      {
         return apx_node_instance_remote_file_published_notification(node_instance, file);
      }
   }
   else
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t create_new_node_instance(apx_server_connection_t* self, apx_node_manager_t* node_manager, apx_file_t* definition_file)
{
   apx_error_t retval = APX_NO_ERROR;
   assert( (self != NULL) && (node_manager != NULL) && (definition_file != NULL) );
   bool file_open_request = false;
   rmf_file_info_t const* file_info = apx_file_get_file_info(definition_file);
   retval = apx_node_manager_init_node_from_file_info(node_manager, file_info, &file_open_request);
   if (retval == APX_NO_ERROR)
   {
      apx_node_instance_t* node_instance = apx_node_manager_get_last_attached(node_manager);
      if (node_instance != NULL)
      {
         apx_node_instance_remote_file_published_notification(node_instance, definition_file);
      }
      if (file_open_request)
      {
         apx_file_manager_t* file_manager = apx_connection_base_get_file_manager(&self->base);
         if (file_manager != NULL)
         {
            apx_file_open(definition_file); //TODO: Should this be moved into file_manager?
            apx_node_instance_set_definition_data_state(node_instance, APX_DATA_STATE_WAITING_FOR_FILE_DATA);
            return apx_file_manager_send_open_file_request(file_manager, apx_file_get_address_without_flags(definition_file));
         }
      }
      else
      {
         retval = APX_NOT_IMPLEMENTED_ERROR; //Retriving data from cache not yet implemented
      }
   }
   return retval;
}


static apx_error_t remote_file_write_notification(apx_server_connection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   if ((self != NULL) && (file != NULL))
   {
      return apx_file_write_notify(file, offset, data, (uint32_t)size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static uint8_t const* parse_message(apx_server_connection_t* self, uint8_t const* begin, uint8_t const* end, apx_error_t* error_code, apx_size_t* msg_size_hint)
{
   uint8_t const* msg_end = NULL;
   *error_code = APX_NO_ERROR;
   if (begin < end)
   {
      uint32_t msg_size = 0u;
      uint8_t const* result = numheader_decode32(begin, end, &msg_size);
      if (result == NULL)
      {
         *error_code = APX_PARSE_ERROR;
         return NULL;
      }
      else if (result == begin)
      {
         //Not enough bytes in buffer, try later when more data has been received
         return begin;
      }
      else
      {
         uint8_t const* msg_data = result;
         msg_end = msg_data + msg_size;
#if APX_DEBUG_ENABLE
         apx_size_t const header_size = (apx_size_t)(msg_data - begin);
         printf("[SERVER-CONNECTION %d]: Received message: (%d+%d) bytes\n", (int)self->base.connection_id, (int)header_size, (int)msg_size);
#endif

         if (msg_end <= end)
         {
            if (self->connection_state == APX_CONNECTION_STATE_ACCEPTED)
            {
               *error_code = apx_connection_base_message_received(&self->base, msg_data, msg_size);
            }
            else if ((self->connection_state == APX_CONNECTION_STATE_CREATED) || (self->connection_state == APX_CONNECTION_STATE_CONNECTING) )
            {
               if (process_greeting_message(self, msg_data, msg_size, error_code))
               {
                  apx_server_connection_greeting_header_accepted_notification(self);
               }
               else
               {
                  fprintf(stderr, "[SERVER-CONNECTION] Failed to parse greeting message\n");
                  return NULL;
               }
            }
            else
            {
               fprintf(stderr, "[SERVER-CONNECTION] Invalid state detected\n");
               return NULL;
            }
         }
         else
         {
            //Message not complete, try again later
            if (msg_size_hint != NULL)
            {
               *msg_size_hint = (apx_size_t)((msg_data - begin) + msg_size);
            }
            return begin;
         }
      }
   }
   else
   {
      *error_code = APX_PARSE_ERROR;
      return NULL;
   }
   return msg_end;
}

static bool process_greeting_message(apx_server_connection_t* self, uint8_t const* msg_data, apx_size_t msg_size, apx_error_t* error_code)
{
   const uint8_t* next = msg_data;
   const uint8_t* end = msg_data + msg_size;
   assert( (msg_data != NULL) && (error_code != NULL) );
   while (next < end)
   {
      const uint8_t* result;
      result = bstr_find_line_feed(next, end);
      if (result < end)
      {
         //found a line ending with '\n'
         const uint8_t* mark = next;
         int32_t length_of_line = (int32_t)(result - next);
         //move next to beginning of next line (one byte after the '\n')
         next = result + 1;
         if (length_of_line == 0)
         {
            //this ends the header
            return true;
         }
         else
         {
            *error_code = parse_protocol_header_line(self, mark, mark + length_of_line);
            if (*error_code != APX_NO_ERROR)
            {
               break;
            }
         }
      }
      else
      {
         *error_code = APX_INTERNAL_ERROR;
         break;
      }
   }
   return false;
}

static void apx_server_connection_node_created_notification(apx_server_connection_t* self, apx_node_instance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL))
   {
      apx_node_instance_set_server(node_instance, self->parent);
   }
}

static apx_error_t detach_all_nodes(apx_server_connection_t* self)
{
   if (self->parent != NULL)
   {
      apx_error_t result = APX_NO_ERROR;
      int32_t num_nodes;
      adt_ary_t node_instance_array;
      adt_ary_t provide_connector_change_array;
      adt_ary_t require_connector_change_array;
      adt_ary_create(&node_instance_array, NULL);
      adt_ary_create(&provide_connector_change_array, apx_port_connector_change_ref_vdelete);
      adt_ary_create(&require_connector_change_array, apx_port_connector_change_ref_vdelete);
      //Take global lock server while calculating which nodes will be affected by disconnect event
      apx_server_take_global_lock(self->parent);
      num_nodes = apx_node_manager_values(apx_connection_base_get_node_manager(&self->base), &node_instance_array);
      if (num_nodes > 0)
      {
         remove_nodes_from_signature_map(self, &node_instance_array);
         if (result == APX_NO_ERROR)
         {
            result = gather_provide_port_connector_changes(&node_instance_array, &provide_connector_change_array);
         }
         if (result == APX_NO_ERROR)
         {
            result = gather_require_port_connector_changes(&node_instance_array, &require_connector_change_array);
         }
      }
      // We have now gathered all portConnectorTables belonging to this connection and placed them into providerConnectorChangeArray
      // and requesterConnectorChangeArray.
      // All other nodes that happened to be affected by port connector changes now need to have their port connector tables cleared.
      // Update port counts and trigger sending port count deltas to surviving clients:
      {
         int32_t i;
         int32_t const num_modified = adt_ary_length(&self->parent->modified_nodes);
         for (i = 0; i < num_modified; i++)
         {
            apx_node_instance_t* node_instance = (apx_node_instance_t*)adt_ary_value(&self->parent->modified_nodes, i);
            if ((node_instance != NULL) && (adt_ary_index_of(&node_instance_array, node_instance) < 0))
            {
               apx_port_connector_change_table_t* req_changes = apx_node_instance_get_require_port_connector_changes(node_instance, false);
               if (req_changes != NULL)
               {
                  apx_node_instance_handle_require_ports_disconnected(node_instance, req_changes);
               }
            }
         }
      }
      apx_server_clear_port_connector_changes(self->parent);
      //All information we need is now located in providerConnectorChangeArray and requesterConnectorChangeArray respectively
      //We can do further processing after releasing global lock
      apx_server_release_global_lock(self->parent);
      adt_ary_destroy(&node_instance_array);
      process_disconnected_provider_nodes(&provide_connector_change_array);
      process_disconnected_requester_nodes(&require_connector_change_array);
      adt_ary_destroy(&provide_connector_change_array);
      adt_ary_destroy(&require_connector_change_array);
      return result;
   }
   return APX_NULL_PTR_ERROR;
}

static void remove_nodes_from_signature_map(apx_server_connection_t* self, adt_ary_t* node_instance_array)
{
   int32_t i;
   int32_t num_nodes;
   assert(node_instance_array != NULL);
   num_nodes = adt_ary_length(node_instance_array);
   for (i = 0; i < num_nodes; i++)
   {
      apx_error_t result;
      apx_data_state_t require_port_data_state;
      apx_data_state_t provide_port_data_state;
      apx_node_instance_t* node_instance = (apx_node_instance_t*)adt_ary_value(node_instance_array, i);
      assert(node_instance != NULL);
      require_port_data_state = apx_node_instance_get_require_port_data_state(node_instance);
      provide_port_data_state = apx_node_instance_get_provide_port_data_state(node_instance);
      if (require_port_data_state == APX_DATA_STATE_SYNCHRONIZED)
      {
         result = apx_server_disconnect_node_instance_require_ports(self->parent, node_instance);
         if (result == APX_NO_ERROR)
         {
         }
         else
         {
            fprintf(stderr, "[SERVER-CONNECTION] apx_server_disconnect_node_instance_require_ports failed with %d\n", (int)result);
         }
      }
      if (provide_port_data_state == APX_DATA_STATE_SYNCHRONIZED)
      {
         result = apx_server_disconnect_node_instance_provide_ports(self->parent, node_instance);
         if (result == APX_NO_ERROR)
         {

         }
         else
         {
            fprintf(stderr, "[SERVER-CONNECTION] apx_server_disconnect_node_instance_provide_ports failed with %d\n", (int)result);
         }
      }
   }

}

static apx_error_t gather_provide_port_connector_changes(adt_ary_t* node_instance_array, adt_ary_t* provider_change_array)
{
   int32_t i;
   int32_t num_nodes;
   assert( (node_instance_array != NULL) && (provider_change_array != NULL));
   num_nodes = adt_ary_length(node_instance_array);
   for (i = 0; i < num_nodes; i++)
   {
      apx_data_state_t provide_port_data_state;
      apx_node_instance_t* node_instance = (apx_node_instance_t*)adt_ary_value(node_instance_array, i);
      assert(node_instance != NULL);
      provide_port_data_state = apx_node_instance_get_provide_port_data_state(node_instance);
      if (provide_port_data_state == APX_DATA_STATE_SYNCHRONIZED)
      {
         apx_port_connector_change_table_t* connector_changes = apx_node_instance_get_provide_port_connector_changes(node_instance, false);
         if (connector_changes != NULL)
         {
            apx_port_connector_change_ref_t* ref;
            adt_error_t rc;
            ref = apx_port_connector_change_ref_new(node_instance, connector_changes);
            if (ref == NULL)
            {
               return APX_MEM_ERROR;
            }
            rc = adt_ary_push(provider_change_array, (void*)ref);
            if (rc != ADT_NO_ERROR)
            {
               return convert_from_adt_to_apx_error(rc);
            }
            apx_node_instance_clear_provide_port_connector_changes(node_instance, false); //This moves ownership of the memory to the ref variable.
         }
         else
         {
            //This node was not connected to any require ports, no further processing needed
            apx_node_instance_set_provide_port_data_state(node_instance, APX_DATA_STATE_DISCONNECTED);
         }
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t gather_require_port_connector_changes(adt_ary_t* node_instance_array, adt_ary_t* requester_change_array)
{
   int32_t i;
   int32_t num_nodes;
   assert((node_instance_array != NULL) && (requester_change_array != NULL));
   num_nodes = adt_ary_length(node_instance_array);
   for (i = 0; i < num_nodes; i++)
   {
      apx_data_state_t require_port_data_state;
      apx_node_instance_t* node_instance = (apx_node_instance_t*)adt_ary_value(node_instance_array, i);
      assert(node_instance != NULL);
      require_port_data_state = apx_node_instance_get_require_port_data_state(node_instance);
      if (require_port_data_state == APX_DATA_STATE_SYNCHRONIZED)
      {
         apx_port_connector_change_table_t* connector_changes = apx_node_instance_get_require_port_connector_changes(node_instance, false);
         if (connector_changes != NULL)
         {
            apx_port_connector_change_ref_t* ref;
            adt_error_t rc;
            ref = apx_port_connector_change_ref_new(node_instance, connector_changes);
            if (ref == NULL)
            {
               return APX_MEM_ERROR;
            }
            rc = adt_ary_push(requester_change_array, (void*)ref);
            if (rc != ADT_NO_ERROR)
            {
               return convert_from_adt_to_apx_error(rc);
            }
            apx_node_instance_clear_require_port_connector_changes(node_instance, false); //This moves ownership of the memory to the ref variable.
         }
         else
         {
            //This node was not connected to any require ports, no further processing needed
            apx_node_instance_set_require_port_data_state(node_instance, APX_DATA_STATE_DISCONNECTED);
         }
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t process_disconnected_provider_nodes(adt_ary_t* provider_change_array)
{
   int32_t numNodes;
   int32_t i;
   assert(provider_change_array != NULL);
   numNodes = adt_ary_length(provider_change_array);
   for (i = 0; i < numNodes; i++)
   {
      apx_node_instance_t* provider_node_instance;
      apx_port_connector_change_table_t* connector_changes;
      apx_port_connector_change_ref_t* ref = (apx_port_connector_change_ref_t*)adt_ary_value(provider_change_array, i);
      provider_node_instance = ref->node_instance;
      connector_changes = ref->connector_changes;
      (void)connector_changes;
      assert(apx_node_instance_get_provide_port_data_state(provider_node_instance) == APX_DATA_STATE_SYNCHRONIZED);
      assert(connector_changes->num_ports == apx_node_instance_get_num_provide_ports(provider_node_instance));
      apx_node_instance_clear_connector_table(provider_node_instance);
      apx_node_instance_set_provide_port_data_state(provider_node_instance, APX_DATA_STATE_DISCONNECTED);
   }
   return APX_NO_ERROR;
}

static apx_error_t process_disconnected_requester_nodes(adt_ary_t* requester_change_array)
{
   int32_t num_nodes;
   int32_t i;
   assert(requester_change_array != NULL);
   num_nodes = adt_ary_length(requester_change_array);
   for (i = 0; i < num_nodes; i++)
   {
      apx_node_instance_t* require_node_instance;
      apx_port_connector_change_table_t* connector_changes;
      apx_port_connector_change_ref_t* ref = (apx_port_connector_change_ref_t*)adt_ary_value(requester_change_array, i);
      require_node_instance = ref->node_instance;
      connector_changes = ref->connector_changes;
      assert(apx_node_instance_get_require_port_data_state(require_node_instance) == APX_DATA_STATE_SYNCHRONIZED);
      assert(connector_changes->num_ports == apx_node_instance_get_num_require_ports(require_node_instance));
      apx_node_instance_handle_require_ports_disconnected(require_node_instance, connector_changes);
      apx_node_instance_set_require_port_data_state(require_node_instance, APX_DATA_STATE_DISCONNECTED);
   }
   return APX_NO_ERROR;
}

static void emit_remote_file_published_event(apx_server_connection_t* self, apx_file_t* file)
{
   if (self->parent != NULL)
   {
      rmf_file_info_t* file_info = apx_file_clone_file_info(file);
      if (file_info != NULL)
      {
         apx_event_t event;
         apx_event_pack_remote_file_published(&event, &self->base, file_info);
         apx_server_append_event(self->parent, &event);
      }
   }

}

static void emit_protocol_header_accepted(apx_server_connection_t* self)
{
   if (self->parent != NULL)
   {
      apx_event_t event;
      apx_event_pack_protocol_header_accepted(&event, &self->base);
      apx_server_append_event(self->parent, &event);
   }
}

static apx_error_t parse_protocol_header_line(apx_server_connection_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* result;
   result = bstr_match_cstr(begin, end, "RMFP/1.0");
   if (result > begin)
   {
      apx_server_connection_set_rmf_proto_id(self, RMF_PROTOCOL_VERSION_ID_1_0);
   }
   else
   {
      result = bstr_match_cstr(begin, end, "RMFP/1.1");
      if (result > begin)
      {
         apx_server_connection_set_rmf_proto_id(self, RMF_PROTOCOL_VERSION_ID_1_1);
      }
      else
      {
         result = bstr_match_cstr(begin, end, "Message-Size:");
         if (result > begin)
         {
            unsigned long size = 0u;
            uint8_t const* next = bstr_lstrip(result, end);
            result = bstr_parse_unsigned_long(next, end, 10, &size);
            if (result > next)
            {
               if ((size != 16) && (size != 32))
               {
                  return APX_INVALID_HEADER_ERROR;
               }
               apx_server_connection_set_num_header_size(self, (apx_size_t)(size/8u)); //convert from bits to bytes
            }
            else
            {
               return APX_INVALID_HEADER_ERROR;
            }
         }
         else
         {
            result = bstr_match_cstr(begin, end, "Connection-Type:");
            if (result > begin)
            {
               uint8_t const* next = bstr_lstrip(result, end);
               result = bstr_match_cstr(next, end, "Default");
               if (result == end)
               {
                  apx_server_connection_set_connection_type(self, APX_CONNECTION_TYPE_DEFAULT);
               }
               else
               {
                  result = bstr_match_cstr(next, end, "Monitor");
                  if (result == end)
                  {
                     apx_server_connection_set_connection_type(self, APX_CONNECTION_TYPE_MONITOR);
                  }
                  else
                  {
                     result = bstr_match_cstr(next, end, "Event");
                     if (result == end)
                     {
                        apx_server_connection_set_connection_type(self, APX_CONNECTION_TYPE_EVENT);
                     }
                     else
                     {
                        return APX_INVALID_HEADER_ERROR;
                     }
                  }
               }
            }
            else
            {
               int32_t length_of_line = (int32_t)(end - begin);
               if (length_of_line < MAX_HEADER_LEN)
               {
                  char tmp[MAX_HEADER_LEN + 1];
                  memcpy(tmp, begin, length_of_line);
                  tmp[length_of_line] = 0;
                  printf("\tUnprocessed header line: '%s'\n", tmp);
               }
               return APX_INVALID_HEADER_ERROR;
            }
         }
      }
   }
   return APX_NO_ERROR;
}