/*****************************************************************************
* \file      socket_server_connection.c
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     socket server connection class
*            Inherits from apx_serverConnection_t
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
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h> //Debug only
#ifdef _MSC_VER
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#endif
#ifdef UNIT_TEST
#include "testsocket.h"
#else
#include "msocket.h"
#endif
#include "apx/extension/socket_server_connection.h"
#include "apx/file_manager.h"
#include "apx/numheader.h"
#include "bstr.h"
#include "apx/server.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define SEND_BUFFER_GROW_SIZE 4096 //4KB
//#define MAX_DEBUG_BYTES 100
//#define MAX_DEBUG_MSG_SIZE 400
//#define HEX_DATA_LEN 3u

#ifdef UNIT_TEST
#define SOCKET_TYPE testsocket_t
#define SOCKET_DELETE testsocket_delete
#define SOCKET_START_IO(x)
#define SOCKET_SET_HANDLER testsocket_setServerHandler
#define SOCKET_SEND testsocket_serverSend
#define SOCKET_OBJECT_CLOSE(x)
#else
#define SOCKET_DELETE msocket_delete
#define SOCKET_TYPE msocket_t
#define SOCKET_START_IO(x) msocket_start_io(x)
#define SOCKET_SET_HANDLER msocket_sethandler
#define SOCKET_SEND msocket_send
#define SOCKET_OBJECT_CLOSE(x) msocket_close(x)
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void register_msocket_handler(apx_socketServerConnection_t* self, SOCKET_TYPE* socket_object);
static void create_connection_interface_vtable(apx_socketServerConnection_t* self, apx_connectionInterface_t* interface);

//msocket API
static void socket_disconnected_notification(void* arg);
static int8_t socket_data_notification(void* arg, const uint8_t* data, uint32_t data_size, uint32_t* parse_size);

//APX BaseConnection API
static void connection_close(apx_socketServerConnection_t* self);
static void connection_start(apx_socketServerConnection_t* self);

// ConnectionInterface API
static int32_t connection_transmit_max_bytes_avaiable(apx_socketServerConnection_t* self);
static int32_t connection_transmit_current_bytes_avaiable(apx_socketServerConnection_t* self);
static void connection_transmit_begin(apx_socketServerConnection_t* self);
static void connection_transmit_end(apx_socketServerConnection_t* self);
static apx_error_t connection_transmit_data_message(apx_socketServerConnection_t* self, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
static apx_error_t connection_transmit_direct_message(apx_socketServerConnection_t* self, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
static void connection_send_packet(apx_socketServerConnection_t* self);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_socketServerConnection_create(apx_socketServerConnection_t *self, SOCKET_TYPE *socket_object)
{
   if ( (self != NULL) && (socket_object != NULL) )
   {
      apx_connectionBaseVTable_t base_connection_vtable;
      apx_connectionInterface_t connection_interface;
      MUTEX_INIT(self->lock);
      self->default_buffer_size = SEND_BUFFER_GROW_SIZE;
      self->pending_bytes = 0u;
      apx_connectionBaseVTable_create(&base_connection_vtable,
         apx_socketServerConnection_vdestroy,
         apx_socketServerConnection_vstart,
         apx_socketServerConnection_vclose);
      create_connection_interface_vtable(self, &connection_interface);
      apx_error_t retval = apx_serverConnection_create(&self->base, &base_connection_vtable, &connection_interface);
      if (retval == APX_NO_ERROR)
      {
         adt_bytearray_create(&self->send_buffer, SEND_BUFFER_GROW_SIZE);
         register_msocket_handler(self, socket_object);
      }
      if (retval == APX_NO_ERROR)
      {
         apx_nodeManager_create(&self->node_manager, APX_SERVER_MODE);
         apx_serverConnection_attach_node_manager(&self->base, &self->node_manager);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_socketServerConnection_destroy(apx_socketServerConnection_t *self)
{
   if (self != NULL)
   {
      apx_serverConnection_destroy(&self->base);
      adt_bytearray_destroy(&self->send_buffer);
      apx_nodeManager_destroy(&self->node_manager);
      SOCKET_DELETE(self->socket_object);
      MUTEX_DESTROY(self->lock);
   }
}

void apx_socketServerConnection_vdestroy(void *arg)
{
   apx_socketServerConnection_destroy((apx_socketServerConnection_t*) arg);
}

apx_socketServerConnection_t *apx_socketServerConnection_new(SOCKET_TYPE *socket_object)
{
   if (socket_object != NULL)
   {
      apx_socketServerConnection_t *self = (apx_socketServerConnection_t*) malloc(sizeof(apx_socketServerConnection_t));
      if (self != NULL)
      {
         apx_error_t result = apx_socketServerConnection_create(self, socket_object);
         if (result != APX_NO_ERROR)
         {
            free(self);
            self = (apx_socketServerConnection_t*)NULL;
         }
      }
      return self;
   }
   return (apx_socketServerConnection_t*)NULL;
}

void apx_socketServerConnection_delete(apx_socketServerConnection_t *self)
{
   if (self != NULL)
   {
      apx_socketServerConnection_destroy(self);
      free(self);
   }
}

void apx_socketServerConnection_vdelete(void *arg)
{
   apx_socketServerConnection_delete((apx_socketServerConnection_t*) arg);
}

void apx_socketServerConnection_vstart(void *arg)
{
   connection_start((apx_socketServerConnection_t*) arg);
}

void apx_socketServerConnection_vclose(void *arg)
{
   connection_close((apx_socketServerConnection_t*) arg);
}

// ConnectionInterface API
int32_t apx_socketServerConnection_vtransmit_max_bytes_avaiable(void* arg)
{
   return connection_transmit_max_bytes_avaiable((apx_socketServerConnection_t*)arg);
}

int32_t apx_socketServerConnection_vtransmit_current_bytes_avaiable(void* arg)
{
   return connection_transmit_current_bytes_avaiable((apx_socketServerConnection_t*)arg);
}

void apx_socketServerConnection_vtransmit_begin(void* arg)
{
   connection_transmit_begin((apx_socketServerConnection_t*)arg);
}

void apx_socketServerConnection_vtransmit_end(void* arg)
{
   connection_transmit_end((apx_socketServerConnection_t*)arg);
}

apx_error_t apx_socketServerConnection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   return connection_transmit_data_message((apx_socketServerConnection_t*)arg, write_address, more_bit, msg_data, msg_size, bytes_available);
}

apx_error_t apx_socketServerConnection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   return connection_transmit_direct_message((apx_socketServerConnection_t*)arg, msg_data, msg_size, bytes_available);
}

#ifdef UNIT_TEST
void apx_socketServerConnection_run(apx_socketServerConnection_t* self)
{
   if (self != NULL)
   {
      apx_serverConnection_run(&self->base);
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void register_msocket_handler(apx_socketServerConnection_t* self, SOCKET_TYPE* socket_object)
{
   msocket_handler_t handler_table;
   memset(&handler_table, 0, sizeof(handler_table));
   handler_table.tcp_data = socket_data_notification;
   handler_table.tcp_disconnected = socket_disconnected_notification;
   self->socket_object = socket_object;
   SOCKET_SET_HANDLER(self->socket_object, &handler_table, self);
}

static void create_connection_interface_vtable(apx_socketServerConnection_t* self, apx_connectionInterface_t* interface)
{
   memset(interface, 0, sizeof(apx_connectionInterface_t));
   interface->arg = (void*)self;
   interface->transmit_max_buffer_size = apx_socketServerConnection_vtransmit_max_bytes_avaiable;
   interface->transmit_current_bytes_avaiable = apx_socketServerConnection_vtransmit_current_bytes_avaiable;
   interface->transmit_begin = apx_socketServerConnection_vtransmit_begin;
   interface->transmit_end = apx_socketServerConnection_vtransmit_end;
   interface->transmit_data_message = apx_socketServerConnection_vtransmit_data_message;
   interface->transmit_direct_message = apx_socketServerConnection_vtransmit_direct_message;
}

//msocket API
static void socket_disconnected_notification(void* arg)
{
   apx_socketServerConnection_t* self = (apx_socketServerConnection_t*)arg;
#if APX_DEBUG_ENABLE
   printf("[SERVER-SOCKET] Client disconnected\n");
#endif
   if (self != NULL)
   {
      assert(self->base.parent != NULL);
      apx_server_detach_connection(self->base.parent, &self->base);
   }
}

static int8_t socket_data_notification(void* arg, const uint8_t* data, uint32_t data_size, uint32_t* parse_size)
{
   apx_socketServerConnection_t* self = (apx_socketServerConnection_t*)arg;
   int8_t retval = (int8_t)apx_serverConnection_on_data_received(&self->base, data, data_size, parse_size);
   return retval;
}

//APX BaseConnection API
static void connection_close(apx_socketServerConnection_t* self)
{
   if (self != NULL)
   {
      SOCKET_OBJECT_CLOSE(self->socket_object);
   }
}

static void connection_start(apx_socketServerConnection_t* self)
{
   assert(self->socket_object != NULL);
   apx_serverConnection_start(&self->base);
   SOCKET_START_IO(self->socket_object);
}

// ConnectionInterface API
static int32_t connection_transmit_max_bytes_avaiable(apx_socketServerConnection_t* self)
{
   if (self != NULL)
   {
      return (int32_t)self->default_buffer_size;
   }
   return -1;
}

static int32_t connection_transmit_current_bytes_avaiable(apx_socketServerConnection_t* self)
{
   if (self != NULL)
   {
      return (int32_t)(adt_bytearray_length(&self->send_buffer) - self->pending_bytes);
   }
   return -1;
}

static void connection_transmit_begin(apx_socketServerConnection_t* self)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      if (adt_bytearray_length(&self->send_buffer) < self->default_buffer_size)
      {
         adt_bytearray_resize(&self->send_buffer, self->default_buffer_size);
      }
      self->pending_bytes = 0u;
      assert((adt_bytearray_length(&self->send_buffer) >= self->default_buffer_size));
   }
}

static void connection_transmit_end(apx_socketServerConnection_t* self)
{
   if (self != NULL)
   {
      if (self->pending_bytes > 0u)
      {
         connection_send_packet(self);
      }
      MUTEX_UNLOCK(self->lock);
   }
}

static apx_error_t connection_transmit_data_message(apx_socketServerConnection_t* self, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   if (self != NULL)
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
      apx_size_t const buffer_available = ((apx_size_t)adt_bytearray_length(&self->send_buffer)) - self->pending_bytes;
      if (bytes_to_send > buffer_available)
      {
         connection_send_packet(self);
         assert(self->pending_bytes == 0u);
      }
      memcpy(adt_bytearray_data(&self->send_buffer) + self->pending_bytes, header, header1_size + header2_size);
      self->pending_bytes += (header1_size + header2_size);
      memcpy(adt_bytearray_data(&self->send_buffer) + self->pending_bytes, msg_data, msg_size);
      self->pending_bytes += msg_size;
      *bytes_available = (int32_t)(((apx_size_t)adt_bytearray_length(&self->send_buffer)) - self->pending_bytes);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t connection_transmit_direct_message(apx_socketServerConnection_t* self, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   if (self != NULL)
   {
      uint8_t  header[NUMHEADER32_LONG_SIZE];
      if (msg_size > ((int32_t)self->default_buffer_size))
      {
         return APX_MSG_TOO_LARGE_ERROR;
      }
      apx_size_t const header_size = numheader_encode32(header, sizeof(header), msg_size);
      apx_size_t const bytes_to_send = header_size + msg_size;
      apx_size_t const buffer_available = ((apx_size_t)adt_bytearray_length(&self->send_buffer)) - self->pending_bytes;
      if (bytes_to_send > buffer_available)
      {
         connection_send_packet(self);
         assert(self->pending_bytes == 0u);
      }
      memcpy(adt_bytearray_data(&self->send_buffer), header, header_size);
      self->pending_bytes += header_size;
      memcpy(adt_bytearray_data(&self->send_buffer) + self->pending_bytes, msg_data, msg_size);
      self->pending_bytes += msg_size;
      *bytes_available = (int32_t)(((apx_size_t)adt_bytearray_length(&self->send_buffer)) - self->pending_bytes);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static void connection_send_packet(apx_socketServerConnection_t* self)
{
   if ((self->socket_object != NULL) && (self->pending_bytes > 0u))
   {
      uint8_t const* data = adt_bytearray_const_data(&self->send_buffer);
      SOCKET_SEND(self->socket_object, data, (uint32_t)self->pending_bytes);
      adt_bytearray_clear(&self->send_buffer);
      self->pending_bytes = 0u;
   }
}