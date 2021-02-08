/*****************************************************************************
* \file      server_connection_base.h
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Base class for all APX server connections
*
* Copyright (c) 2018-2021 Conny Gustafsson
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
static apx_error_t remote_file_published_notification(apx_serverConnection_t* self, apx_file_t* file);
static apx_error_t process_new_definition_data_file(apx_serverConnection_t* self, apx_file_t* file);
static apx_error_t process_new_provide_port_data_file(apx_serverConnection_t* self, apx_file_t* file);
static apx_error_t create_new_node_instance(apx_serverConnection_t* self, apx_nodeManager_t* node_manager,  apx_file_t* definition_file);
static apx_error_t remote_file_write_notification(apx_serverConnection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);
static uint8_t const* parse_message(apx_serverConnection_t* self, uint8_t const* begin, uint8_t const* end, apx_error_t* error_code);
static bool process_greeting_message(uint8_t const* msg_data, apx_size_t msg_size, apx_error_t* error_code);
static void apx_serverConnection_node_created_notification(apx_serverConnection_t* self, apx_nodeInstance_t* node_instance);
static apx_error_t detach_all_nodes(apx_serverConnection_t* self);
static void remove_nodes_from_signature_map(apx_serverConnection_t* self, adt_ary_t* node_instance_array);
static apx_error_t gather_provide_port_connector_changes(adt_ary_t* node_instance_array, adt_ary_t* provider_change_array);
static apx_error_t gather_require_port_connector_changes(adt_ary_t* node_instance_array, adt_ary_t* requester_change_array);
static apx_error_t process_disconnected_provider_nodes(adt_ary_t* provider_change_array);
static apx_error_t process_disconnected_requester_nodes(adt_ary_t* requester_change_array);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_serverConnection_create(apx_serverConnection_t* self, apx_connectionBaseVTable_t* base_connection_vtable, apx_connectionInterface_t* connection_interface)
{
   if (self != 0)
   {
      apx_error_t error_code;
      //init non-overridable virtual functions
      base_connection_vtable->node_created_notification = apx_serverConnection_vnode_created_notification;
      connection_interface->remote_file_published_notification = apx_serverConnection_vremote_file_published_notification;
      connection_interface->remote_file_write_notification = apx_serverConnection_vremote_file_write_notification;
      error_code = apx_connectionBase_create(&self->base, APX_SERVER_MODE, base_connection_vtable, connection_interface);
      self->is_greeting_accepted = false;
      self->parent = NULL;
      self->last_error = APX_NO_ERROR;
      //apx_connectionBase_setEventHandler(&self->base, apx_serverConnection_defaultEventHandler, (void*) self);
      return error_code;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_serverConnection_destroy(apx_serverConnection_t* self)
{
   if (self != 0)
   {
      apx_connectionBase_destroy(&self->base);
   }
}

apx_fileManager_t* apx_serverConnection_get_file_manager(apx_serverConnection_t* self)
{
   if (self != 0)
   {
      return apx_connectionBase_get_file_manager(&self->base);
   }
   return NULL;
}

void apx_serverConnection_start(apx_serverConnection_t* self)
{
   (void)self;
}


void apx_serverConnection_close(apx_serverConnection_t* self)
{
   (void)self;
}

uint32_t apx_serverConnection_get_total_bytes_received(apx_serverConnection_t* self)
{
   (void)self;
   return 0u;
}

uint32_t apx_serverConnection_get_total_bytes_sent(apx_serverConnection_t* self)
{
   (void)self;
   return 0u;
}

/*
void apx_serverConnection_default_event_handler(void* arg, apx_event_t* event)
{
   (void)arg;
   (void)event;
}

void* apx_serverConnection_register_event_listener(apx_serverConnection_t* self, apx_connectionEventListener_t* listener)
{
   (void)self;
   (void)listener;
   return NULL;
}

void apx_serverConnection_unregister_event_listener(apx_serverConnection_t* self, void* handle)
{
   (void)self;
   (void)handle;
}
*/

void apx_serverConnection_greeting_header_accepted_notification(apx_serverConnection_t* self)
{
   if (self != NULL)
   {
      self->is_greeting_accepted = true;
      apx_fileManager_connected(&self->base.file_manager);
   }
}

void apx_serverConnection_connected_notification(apx_serverConnection_t* self)
{
   if (self != NULL)
   {
      self->is_greeting_accepted = false;
   }
}

apx_error_t apx_serverConnection_disconnected_notification(apx_serverConnection_t* self)
{
   if (self != NULL)
   {
      apx_connectionBase_disconnect_notification(&self->base);
      return detach_all_nodes(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_serverConnection_attach_node_manager(apx_serverConnection_t* self, apx_nodeManager_t* node_manager)
{
   if (self != NULL)
   {
      apx_connectionBase_attach_node_manager(&self->base, node_manager);
   }
}

apx_nodeManager_t* apx_serverConnection_get_node_manager(apx_serverConnection_t* self)
{
   if (self != NULL)
   {
      return apx_connectionBase_get_node_manager(&self->base);
   }
   return NULL;
}

int apx_serverConnection_on_data_received(apx_serverConnection_t* self, uint8_t const* data, apx_size_t data_size, apx_size_t* parse_len)
{
   if ((self != NULL) && (data != NULL) && (data_size > 0u) && (parse_len != NULL))
   {
      apx_size_t total_parse_len = 0u;
      uint8_t const* next = data;
      uint8_t const* end = data + data_size;
      while (next < end)
      {
         uint8_t const* result;
         apx_error_t error_code = APX_NO_ERROR;
         result = parse_message(self, next, end, &error_code);
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
      *parse_len = total_parse_len;
      return 0;
   }
   return -1;
}

void apx_serverConnection_vnode_created_notification(void* arg, apx_nodeInstance_t* node_instance)
{
   apx_serverConnection_node_created_notification((apx_serverConnection_t*)arg, node_instance);
}

void apx_serverConnection_set_connection_id(apx_serverConnection_t* self, uint32_t connection_id)
{
   if (self != NULL)
   {
      apx_connectionBase_set_connection_id(&self->base, connection_id);
   }
}

uint32_t apx_serverConnection_get_connection_id(apx_serverConnection_t* self)
{
   if (self != NULL)
   {
      return self->base.connection_id;
   }
   return APX_INVALID_CONNECTION_ID;
}

void apx_serverConnection_set_server(apx_serverConnection_t* self, struct apx_server_tag* server)
{
   if (self != NULL)
   {
      self->parent = server;
   }
}

void apx_serverConnection_require_port_data_written(apx_serverConnection_t* self, apx_nodeInstance_t* node_instance, apx_size_t offset, apx_size_t size)
{
   if (self != NULL)
   {
      (void)node_instance;
      (void)offset;
      (void)size;
   }
}

apx_error_t apx_serverConnection_attach_node_instance(apx_serverConnection_t* self, apx_nodeInstance_t* node_instance)
{
   if ((self != NULL) && (node_instance != NULL))
   {
      apx_fileManager_t* file_manager = apx_connectionBase_get_file_manager(&self->base);
      assert(file_manager != NULL);
      return apx_nodeInstance_attach_to_file_manager(node_instance, file_manager);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//ConnectionInterface API
apx_error_t apx_serverConnection_vremote_file_published_notification(void* arg, apx_file_t* file)
{
   return remote_file_published_notification((apx_serverConnection_t*)arg, file);
}

apx_error_t apx_serverConnection_vremote_file_write_notification(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   return remote_file_write_notification((apx_serverConnection_t*)arg, file, offset, data, size);
}

#ifdef UNIT_TEST
void apx_serverConnection_run(apx_serverConnection_t* self)
{
   if (self != 0)
   {
      //apx_connectionBase_runAll(&self->base);
      apx_fileManager_run(&self->base.file_manager);
   }
}
#endif



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t remote_file_published_notification(apx_serverConnection_t* self, apx_file_t* file)
{
   if ((self != NULL) && (file != NULL))
   {
      apx_fileType_t file_type = apx_file_get_apx_file_type(file);
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

static apx_error_t process_new_definition_data_file(apx_serverConnection_t* self, apx_file_t* file)
{
   assert((self != NULL) && (file != NULL));
   char* base_name = rmf_fileInfo_base_name(apx_file_get_file_info(file));
   if (base_name != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_nodeManager_t* node_manager = apx_connectionBase_get_node_manager(&self->base);
      apx_nodeInstance_t* node_instance = apx_nodeManager_find(node_manager, base_name);
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

static apx_error_t process_new_provide_port_data_file(apx_serverConnection_t* self, apx_file_t* file)
{
   assert((self != NULL) && (file != NULL));
   char* base_name = rmf_fileInfo_base_name(apx_file_get_file_info(file));
   if (base_name != NULL)
   {
      apx_nodeInstance_t* node_instance = apx_nodeManager_find(self->base.node_manager, base_name);
      free(base_name);
      if (node_instance != NULL)
      {
         return apx_nodeInstance_remote_file_published_notification(node_instance, file);
      }
   }
   else
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t create_new_node_instance(apx_serverConnection_t* self, apx_nodeManager_t* node_manager, apx_file_t* definition_file)
{
   apx_error_t retval = APX_NO_ERROR;
   assert( (self != NULL) && (node_manager != NULL) && (definition_file != NULL) );
   bool file_open_request = false;
   rmf_fileInfo_t const* file_info = apx_file_get_file_info(definition_file);
   retval = apx_nodeManager_init_node_from_file_info(node_manager, file_info, &file_open_request);
   if (retval == APX_NO_ERROR)
   {
      apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(node_manager);
      if (node_instance != NULL)
      {
         apx_nodeInstance_remote_file_published_notification(node_instance, definition_file);
      }
      if (file_open_request)
      {
         apx_fileManager_t* file_manager = apx_connectionBase_get_file_manager(&self->base);
         if (file_manager != NULL)
         {
            apx_file_open(definition_file); //Should this be moved into file_manager?
            apx_nodeInstance_set_definition_data_state(node_instance, APX_DATA_STATE_WAITING_FOR_FILE_DATA);
            return apx_fileManager_send_open_file_request(file_manager, apx_file_get_address_without_flags(definition_file));
         }
      }
      else
      {
         retval = APX_NOT_IMPLEMENTED_ERROR; //Retriving data from cache not yet implemented
      }
   }
   return retval;
}


static apx_error_t remote_file_write_notification(apx_serverConnection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   if ((self != NULL) && (file != NULL))
   {
      return apx_file_write_notify(file, offset, data, (uint32_t)size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static uint8_t const* parse_message(apx_serverConnection_t* self, uint8_t const* begin, uint8_t const* end, apx_error_t* error_code)
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
            if (self->is_greeting_accepted)
            {
               *error_code = apx_connectionBase_message_received(&self->base, msg_data, msg_size);
            }
            else
            {
               if (process_greeting_message(msg_data, msg_size, error_code))
               {
                  apx_serverConnection_greeting_header_accepted_notification(self);
               }
               else
               {
                  return NULL;
               }
            }
         }
         else
         {
            //Message not complete, try again later
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

static bool process_greeting_message(uint8_t const* msg_data, apx_size_t msg_size, apx_error_t* error_code)
{
   const uint8_t* next = msg_data;
   const uint8_t* end = msg_data + msg_size;
   assert( (msg_data != NULL) && (error_code != NULL) );
   while (next < end)
   {
      const uint8_t* result;
      result = bstr_line(next, end);
      if ((result > next) || ((result == next) && *next == (uint8_t)'\n'))
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
            //TODO: parse greeting line
            if (length_of_line < MAX_HEADER_LEN)
            {
               char tmp[MAX_HEADER_LEN + 1];
               memcpy(tmp, mark, length_of_line);
               tmp[length_of_line] = 0;
               //printf("\tgreeting-line: '%s'\n",tmp);
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

static void apx_serverConnection_node_created_notification(apx_serverConnection_t* self, apx_nodeInstance_t* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL))
   {
      apx_nodeInstance_set_server(node_instance, self->parent);
   }
}

static apx_error_t detach_all_nodes(apx_serverConnection_t* self)
{
   if (self->parent != NULL)
   {
      apx_error_t result = APX_NO_ERROR;
      int32_t num_nodes;
      adt_ary_t node_instance_array;
      adt_ary_t provide_connector_change_array;
      adt_ary_t require_connector_change_array;
      adt_ary_create(&node_instance_array, NULL);
      adt_ary_create(&provide_connector_change_array, apx_portConnectorChangeRef_vdelete);
      adt_ary_create(&require_connector_change_array, apx_portConnectorChangeRef_vdelete);
      //Take global lock server while calculating which nodes will be affected by disconnect event
      apx_server_take_global_lock(self->parent);
      num_nodes = apx_nodeManager_values(apx_connectionBase_get_node_manager(&self->base), &node_instance_array);
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
      // TODO: before clearing the tables we should actually update the port count and also send out update port count deltas to clients
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

static void remove_nodes_from_signature_map(apx_serverConnection_t* self, adt_ary_t* node_instance_array)
{
   int32_t i;
   int32_t num_nodes;
   assert(node_instance_array != 0);
   num_nodes = adt_ary_length(node_instance_array);
   for (i = 0; i < num_nodes; i++)
   {
      apx_error_t result;
      apx_dataState_t require_port_data_state;
      apx_dataState_t provide_port_data_state;
      apx_nodeInstance_t* node_instance = (apx_nodeInstance_t*)adt_ary_value(node_instance_array, i);
      assert(node_instance != 0);
      require_port_data_state = apx_nodeInstance_get_require_port_data_state(node_instance);
      provide_port_data_state = apx_nodeInstance_get_provide_port_data_state(node_instance);
      if (require_port_data_state == APX_DATA_STATE_CONNECTED)
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
      if (provide_port_data_state == APX_DATA_STATE_CONNECTED)
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
      apx_dataState_t provide_port_data_state;
      apx_nodeInstance_t* node_instance = (apx_nodeInstance_t*)adt_ary_value(node_instance_array, i);
      assert(node_instance != 0);
      provide_port_data_state = apx_nodeInstance_get_provide_port_data_state(node_instance);
      if (provide_port_data_state == APX_DATA_STATE_CONNECTED)
      {
         apx_portConnectorChangeTable_t* connector_changes = apx_nodeInstance_get_provide_port_connector_changes(node_instance, false);
         if (connector_changes != NULL)
         {
            apx_portConnectorChangeRef_t* ref;
            adt_error_t rc;
            ref = apx_portConnectorChangeRef_new(node_instance, connector_changes);
            if (ref == 0)
            {
               return APX_MEM_ERROR;
            }
            rc = adt_ary_push(provider_change_array, (void*)ref);
            if (rc != ADT_MEM_ERROR)
            {
               return convert_from_adt_to_apx_error(rc);
            }
            apx_nodeInstance_clear_provide_port_connector_changes(node_instance, false); //This moves ownership of the memory to the ref variable.
         }
         else
         {
            //This node was not connected to any require ports, no further processing needed
            apx_nodeInstance_set_provide_port_data_state(node_instance, APX_DATA_STATE_DISCONNECTED);
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
      apx_dataState_t require_port_data_state;
      apx_nodeInstance_t* node_instance = (apx_nodeInstance_t*)adt_ary_value(node_instance_array, i);
      assert(node_instance != 0);
      require_port_data_state = apx_nodeInstance_get_require_port_data_state(node_instance);
      if (require_port_data_state == APX_DATA_STATE_CONNECTED)
      {
         apx_portConnectorChangeTable_t* connector_changes = apx_nodeInstance_get_require_port_connector_changes(node_instance, false);
         if (connector_changes != NULL)
         {
            apx_portConnectorChangeRef_t* ref;
            adt_error_t rc;
            ref = apx_portConnectorChangeRef_new(node_instance, connector_changes);
            if (ref == 0)
            {
               return APX_MEM_ERROR;
            }
            rc = adt_ary_push(requester_change_array, (void*)ref);
            if (rc != ADT_NO_ERROR)
            {
               return convert_from_adt_to_apx_error(rc);
            }
            apx_nodeInstance_clear_require_port_connector_changes(node_instance, false); //This moves ownership of the memory to the ref variable.
         }
         else
         {
            //This node was not connected to any require ports, no further processing needed
            apx_nodeInstance_set_require_port_data_state(node_instance, APX_DATA_STATE_DISCONNECTED);
         }
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t process_disconnected_provider_nodes(adt_ary_t* provider_change_array)
{
   int32_t numNodes;
   int32_t i;
   assert(provider_change_array != 0);
   numNodes = adt_ary_length(provider_change_array);
   for (i = 0; i < numNodes; i++)
   {
      apx_nodeInstance_t* provider_node_instance;
      apx_portConnectorChangeTable_t* connector_changes;
      apx_portConnectorChangeRef_t* ref = (apx_portConnectorChangeRef_t*)adt_ary_value(provider_change_array, i);
      provider_node_instance = ref->node_instance;
      connector_changes = ref->connector_changes;
      assert(apx_nodeInstance_get_provide_port_data_state(provider_node_instance) == APX_DATA_STATE_CONNECTED);
      assert(connector_changes->num_ports == apx_nodeInstance_get_num_provide_ports(provider_node_instance));
      apx_nodeInstance_clear_connector_table(provider_node_instance);
      apx_nodeInstance_set_provide_port_data_state(provider_node_instance, APX_DATA_STATE_DISCONNECTED);
   }
   return APX_NO_ERROR;
}

static apx_error_t process_disconnected_requester_nodes(adt_ary_t* requester_change_array)
{
   int32_t num_nodes;
   int32_t i;
   assert(requester_change_array != 0);
   num_nodes = adt_ary_length(requester_change_array);
   for (i = 0; i < num_nodes; i++)
   {
      apx_nodeInstance_t* require_node_instance;
      apx_portConnectorChangeTable_t* connector_changes;
      apx_portConnectorChangeRef_t* ref = (apx_portConnectorChangeRef_t*)adt_ary_value(requester_change_array, i);
      require_node_instance = ref->node_instance;
      connector_changes = ref->connector_changes;
      assert(apx_nodeInstance_get_require_port_data_state(require_node_instance) == APX_DATA_STATE_CONNECTED);
      assert(connector_changes->num_ports == apx_nodeInstance_get_num_require_ports(require_node_instance));
      apx_nodeInstance_handle_require_ports_disconnected(require_node_instance, connector_changes);
      apx_nodeInstance_set_require_port_data_state(require_node_instance, APX_DATA_STATE_DISCONNECTED);
   }
   return APX_NO_ERROR;
}

