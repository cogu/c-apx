/*****************************************************************************
* \file      client_test_connection.c
* \author    Conny Gustafsson
* \date      2018-01-15
* \brief     Unit Test connection for APX clients
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
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "apx/client_test_connection.h"
//#include "apx/client_internal.h"
#include "apx/numheader.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void create_connection_interface_vtable(apx_clientTestConnection_t* self, apx_connectionInterface_t* interface);
static int32_t apx_clientTestConnection_transmit_max_bytes_avaiable(apx_clientTestConnection_t* self);
static int32_t apx_clientTestConnection_transmit_current_bytes_avaiable(apx_clientTestConnection_t* self);
static void apx_clientTestConnection_transmit_begin(apx_clientTestConnection_t* self);
static void apx_clientTestConnection_transmit_end(apx_clientTestConnection_t* self);
static apx_error_t apx_clientTestConnection_transmit_data_message(apx_clientTestConnection_t* self, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
static apx_error_t apx_clientTestConnection_transmit_direct_message(apx_clientTestConnection_t* self, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
static void send_packet(apx_clientTestConnection_t* self);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

// Constructor/Destructor
apx_error_t apx_clientTestConnection_create(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      apx_connectionBaseVTable_t base_connection_vtable;
      apx_connectionInterface_t connection_interface;
      self->default_buffer_size = 1024u;
      self->pending_bytes = 0u;
      apx_connectionBaseVTable_create(&base_connection_vtable,
            apx_clientTestConnection_vdestroy,
            apx_clientTestConnection_vstart,
            apx_clientTestConnection_vclose);
      create_connection_interface_vtable(self, &connection_interface);
      apx_error_t result = apx_clientConnection_create(&self->base, &base_connection_vtable, &connection_interface);
      if (result == APX_NO_ERROR)
      {
         self->transmit_log = adt_ary_new(adt_bytearray_vdelete);
         if (self->transmit_log == NULL)
         {
            result = APX_MEM_ERROR;
         }
         else
         {
            adt_bytearray_create(&self->transmit_buffer, 0u);
         }
      }
      if (result == APX_NO_ERROR)
      {
         apx_nodeManager_create(&self->node_manager, APX_CLIENT_MODE);
         apx_clientConnection_attach_node_manager(&self->base, &self->node_manager);
      }
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_clientTestConnection_destroy(apx_clientTestConnection_t *self)
{
   if (self != NULL)
   {
      adt_bytearray_destroy(&self->transmit_buffer);
      if (self->transmit_log != 0)
      {
         adt_ary_delete(self->transmit_log);
      }
      apx_clientConnection_destroy(&self->base);
      apx_nodeManager_destroy(&self->node_manager);
   }
}

void apx_clientTestConnection_vdestroy(void *arg)
{
   apx_clientTestConnection_destroy((apx_clientTestConnection_t*) arg);
}

apx_clientTestConnection_t *apx_clientTestConnection_new(void)
{
   apx_clientTestConnection_t *self = (apx_clientTestConnection_t*) malloc(sizeof(apx_clientTestConnection_t));
   if (self != NULL)
   {
      apx_error_t result = apx_clientTestConnection_create(self);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = 0;
      }
   }
   return self;
}

void apx_clientTestConnection_delete(apx_clientTestConnection_t *self)
{
   if (self != NULL)
   {
      apx_clientTestConnection_destroy(self);
      free(self);
   }
}

// BaseConnection API
void apx_clientTestConnection_start(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
   }
}

void apx_clientTestConnection_vstart(void* arg)
{
   apx_clientTestConnection_start((apx_clientTestConnection_t*)arg);
}

void apx_clientTestConnection_close(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
   }
}

void apx_clientTestConnection_vclose(void* arg)
{
   apx_clientTestConnection_close((apx_clientTestConnection_t*)arg);
}

// ClientConnection API
void apx_clientTestConnection_greeting_header_accepted_notification(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      apx_clientConnection_greeting_header_accepted_notification(&self->base);
   }
}

void apx_clientTestConnection_connected_notification(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      apx_clientConnection_connected_notification(&self->base);
   }
}

void apx_clientTestConnection_disconnected_notification(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      apx_clientConnection_disconnected_notification(&self->base);
   }
}


// ConnectionInterface API
int32_t apx_clientTestConnection_vtransmit_max_bytes_avaiable(void* arg)
{
   return apx_clientTestConnection_transmit_max_bytes_avaiable((apx_clientTestConnection_t*)arg);
}

int32_t apx_clientTestConnection_vtransmit_current_bytes_avaiable(void* arg)
{
   return apx_clientTestConnection_transmit_current_bytes_avaiable((apx_clientTestConnection_t*)arg);
}

void apx_clientTestConnection_vtransmit_begin(void* arg)
{
   apx_clientTestConnection_transmit_begin((apx_clientTestConnection_t*)arg);
}

void apx_clientTestConnection_vtransmit_end(void* arg)
{
   apx_clientTestConnection_transmit_end((apx_clientTestConnection_t*)arg);
}

apx_error_t apx_clientTestConnection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   return apx_clientTestConnection_transmit_data_message((apx_clientTestConnection_t*)arg, write_address, more_bit, msg_data, msg_size, bytes_available);
}

apx_error_t apx_clientTestConnection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   return apx_clientTestConnection_transmit_direct_message((apx_clientTestConnection_t*)arg, msg_data, msg_size, bytes_available);
}

apx_error_t apx_clientTestConnection_remote_file_published_notification(apx_clientTestConnection_t* self, apx_file_t* file)
{
   (void)self;
   (void)file;
   return APX_NOT_IMPLEMENTED_ERROR;
}

apx_error_t apx_clientTestConnection_remote_file_write_notification(apx_clientTestConnection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   (void)self;
   (void)file;
   (void)offset;
   (void)data;
   (void)size;
   return APX_NOT_IMPLEMENTED_ERROR;
}


//Log API
int32_t apx_clientTestConnection_log_length(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      return adt_ary_length(self->transmit_log);
   }
   return -1;
}

adt_bytearray_t* apx_clientTestConnection_get_log_packet(apx_clientTestConnection_t* self, int32_t index)
{
   if (self != NULL)
   {
      return (adt_bytearray_t*)adt_ary_value(self->transmit_log, index);
   }
   return NULL;
}

void apx_clientTestConnection_clear_log(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      adt_ary_clear(self->transmit_log);
   }
}

//Test-case API
apx_fileManager_t* apx_clientTestConnection_get_file_manager(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      return apx_clientConnection_get_file_manager(&self->base);
   }
   return NULL;
}

apx_error_t apx_clientTestConnection_request_open_local_file(apx_clientTestConnection_t* self, char const* file_name)
{
   if ( (self != NULL) && (file_name != NULL) )
   {
      uint8_t  buffer[RMF_HIGH_ADDR_SIZE + RMF_CMD_TYPE_SIZE + RMF_FILE_OPEN_CMD_SIZE];
      apx_fileManager_t* file_manager = apx_clientConnection_get_file_manager(&self->base);
      assert(file_manager != NULL);
      apx_file_t* file = apx_fileManager_find_local_file_by_name(file_manager, file_name);
      if (file == NULL)
      {
         return APX_FILE_NOT_FOUND_ERROR;
      }
      if (rmf_address_encode(buffer, RMF_HIGH_ADDR_SIZE, RMF_CMD_AREA_START_ADDRESS, false) != RMF_HIGH_ADDR_SIZE)
      {
         return APX_INTERNAL_ERROR;
      }
      apx_size_t const cmd_size = RMF_CMD_TYPE_SIZE + RMF_FILE_OPEN_CMD_SIZE;
      apx_size_t result = rmf_encode_open_file_cmd(buffer + RMF_HIGH_ADDR_SIZE, cmd_size, apx_file_get_address_without_flags(file));
      if (result == 0)
      {
         return APX_INTERNAL_ERROR;
      }
      return apx_fileManager_message_received(file_manager, buffer, sizeof(buffer));
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_clientTestConnection_publish_remote_file(apx_clientTestConnection_t* self, uint32_t address, char const* file_name, apx_size_t file_size)
{
   if (self != NULL && file_name != NULL)
   {
      uint8_t  buffer[RMF_HIGH_ADDR_SIZE + RMF_CMD_TYPE_SIZE + RMF_FILE_INFO_HEADER_SIZE + RMF_FILE_NAME_MAX_SIZE];
      rmf_fileInfo_t *file_info;
      apx_fileManager_t* file_manager = apx_clientConnection_get_file_manager(&self->base);
      assert(file_manager != NULL);


      if (rmf_address_encode(buffer, RMF_HIGH_ADDR_SIZE, RMF_CMD_AREA_START_ADDRESS, false) != RMF_HIGH_ADDR_SIZE)
      {
         return APX_INTERNAL_ERROR;
      }
      file_info = rmf_fileInfo_make_fixed(file_name, file_size, address);
      apx_size_t const max_cmd_size = sizeof(buffer) - RMF_HIGH_ADDR_SIZE;
      apx_size_t const cmd_size = rmf_encode_publish_file_cmd(buffer + RMF_HIGH_ADDR_SIZE, max_cmd_size, file_info);
      rmf_fileInfo_delete(file_info);
      if (cmd_size == 0)
      {
         return APX_INTERNAL_ERROR;
      }
      return apx_fileManager_message_received(file_manager, buffer, RMF_HIGH_ADDR_SIZE + cmd_size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_clientTestConnection_write_remote_data(apx_clientTestConnection_t* self, uint32_t address, uint8_t const* payload_data, apx_size_t payload_size)
{
   if ( (self != NULL) && (payload_size > 0))
   {
      uint8_t header[RMF_HIGH_ADDR_SIZE];
      apx_fileManager_t* file_manager = apx_clientConnection_get_file_manager(&self->base);
      assert(file_manager != NULL);
      apx_size_t header_size = (apx_size_t) rmf_address_encode(header, sizeof(header), address, false);
      if (header_size == 0u)
      {
         return APX_INTERNAL_ERROR;
      }
      apx_error_t retval = APX_MEM_ERROR;
      uint8_t* msg = (uint8_t*)malloc(((size_t)header_size) + payload_size);
      if (msg != NULL)
      {
         memcpy(msg, &header[0], header_size);
         memcpy(msg + header_size, payload_data, payload_size);
         retval = apx_fileManager_message_received(file_manager, msg, header_size + payload_size);
         free(msg);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_nodeInstance_t* apx_clientTestConnection_find_node(apx_clientTestConnection_t* self, char const* name)
{
   if (self != NULL)
   {
      return apx_nodeManager_find(&self->node_manager, name);
   }
   return NULL;
}

apx_error_t apx_clientTestConnection_build_node(apx_clientTestConnection_t* self, char const* definition_text)
{
   if (self != NULL)
   {
      apx_nodeManager_t* node_manager = apx_clientConnection_get_node_manager(&self->base);
      if (node_manager == NULL)
      {
         return APX_NULL_PTR_ERROR;
      }
      apx_error_t retval = apx_nodeManager_build_node(node_manager, definition_text);
      if (retval == APX_NO_ERROR)
      {
         apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(node_manager);
         assert(node_instance != NULL);
         apx_clientConnection_attach_node_instance(&self->base, node_instance);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_clientTestConnection_run(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      apx_clientConnection_run(&self->base);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void create_connection_interface_vtable(apx_clientTestConnection_t* self, apx_connectionInterface_t* interface)
{
   memset(interface, 0, sizeof(apx_connectionInterface_t));
   interface->arg = (void*)self;
   interface->transmit_max_buffer_size = apx_clientTestConnection_vtransmit_max_bytes_avaiable;
   interface->transmit_current_bytes_avaiable = apx_clientTestConnection_vtransmit_current_bytes_avaiable;
   interface->transmit_begin = apx_clientTestConnection_vtransmit_begin;
   interface->transmit_end = apx_clientTestConnection_vtransmit_end;
   interface->transmit_data_message = apx_clientTestConnection_vtransmit_data_message;
   interface->transmit_direct_message = apx_clientTestConnection_vtransmit_direct_message;
}

static int32_t apx_clientTestConnection_transmit_max_bytes_avaiable(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      return (int32_t)self->default_buffer_size;
   }
   return -1;
}

static int32_t apx_clientTestConnection_transmit_current_bytes_avaiable(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      return (int32_t)adt_bytearray_length(&self->transmit_buffer);
   }
   return -1;
}

static void apx_clientTestConnection_transmit_begin(apx_clientTestConnection_t* self)
{
   if (self != NULL)
   {
      if (adt_bytearray_length(&self->transmit_buffer) < self->default_buffer_size)
      {
         adt_bytearray_resize(&self->transmit_buffer, self->default_buffer_size);
      }
      self->pending_bytes = 0u;
      assert((adt_bytearray_length(&self->transmit_buffer) >= self->default_buffer_size));
   }
}

static void apx_clientTestConnection_transmit_end(apx_clientTestConnection_t* self)
{
   if ( (self != NULL) && (self->pending_bytes > 0u) )
   {
      send_packet(self);
   }
}

static apx_error_t apx_clientTestConnection_transmit_data_message(apx_clientTestConnection_t* self, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   uint8_t header[NUMHEADER32_LONG_SIZE + RMF_HIGH_ADDR_SIZE];
   apx_size_t const address_size = rmf_needed_encoding_size(write_address);
   apx_size_t const payload_size = address_size + msg_size;
   if (payload_size > self->default_buffer_size)
   {
      return APX_MSG_TOO_LARGE_ERROR;
   }
   apx_size_t const header1_size = numheader_encode32(header, sizeof(header), payload_size);
   assert(header1_size > 0);
   apx_size_t const header2_size = rmf_address_encode(header + header1_size, sizeof(header) - header1_size, write_address, more_bit);
   assert(header2_size == address_size);
   apx_size_t const bytes_to_send = header1_size + header2_size + payload_size;
   apx_size_t const buffer_available = ((apx_size_t)adt_bytearray_length(&self->transmit_buffer)) - self->pending_bytes;
   if (bytes_to_send > buffer_available)
   {
      send_packet(self);
      assert(self->pending_bytes == 0u);
   }
   memcpy(adt_bytearray_data(&self->transmit_buffer) + self->pending_bytes, header, header1_size + header2_size);
   self->pending_bytes += (header1_size + header2_size);
   memcpy(adt_bytearray_data(&self->transmit_buffer) + self->pending_bytes, msg_data, msg_size);
   self->pending_bytes += msg_size;
   *bytes_available = (int32_t)(((apx_size_t)adt_bytearray_length(&self->transmit_buffer)) - self->pending_bytes);
   return APX_NO_ERROR;
}

static apx_error_t apx_clientTestConnection_transmit_direct_message(apx_clientTestConnection_t* self, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   uint8_t  header [NUMHEADER32_LONG_SIZE];
   if (msg_size > ((int32_t)self->default_buffer_size))
   {
      return APX_MSG_TOO_LARGE_ERROR;
   }
   apx_size_t const header_size = numheader_encode32(header, sizeof(header), msg_size);
   apx_size_t const bytes_to_send = header_size + msg_size;
   apx_size_t const buffer_available = ((apx_size_t)adt_bytearray_length(&self->transmit_buffer)) - self->pending_bytes;
   if (bytes_to_send > buffer_available)
   {
      send_packet(self);
      assert(self->pending_bytes == 0u);
   }
   memcpy(adt_bytearray_data(&self->transmit_buffer), header, header_size);
   self->pending_bytes += header_size;
   memcpy(adt_bytearray_data(&self->transmit_buffer) + self->pending_bytes, msg_data, msg_size);
   self->pending_bytes += msg_size;
   *bytes_available = (int32_t)(((apx_size_t)adt_bytearray_length(&self->transmit_buffer)) - self->pending_bytes);
   return APX_NO_ERROR;
}

static void send_packet(apx_clientTestConnection_t* self)
{
   adt_bytearray_t* packet = adt_bytearray_new(0u);
   if (packet != NULL)
   {
      adt_error_t result = adt_bytearray_resize(packet, (uint32_t) self->pending_bytes);
      if (result == ADT_NO_ERROR)
      {
         memcpy(adt_bytearray_data(packet), adt_bytearray_data(&self->transmit_buffer), self->pending_bytes);
         adt_ary_push(self->transmit_log, packet);
      }
   }
   adt_bytearray_clear(&self->transmit_buffer);
   self->pending_bytes = 0u;
}
