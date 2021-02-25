/*****************************************************************************
* \file      apx_client_socket_connection.c
* \author    Conny Gustafsson
* \date      2018-12-31
* \brief     Client socket connection class
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
#include "apx/socket_client_connection.h"
//#include "apx/logging.h"
#include "apx/file_manager.h"
#include "apx/numheader.h"
#include "bstr.h"
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
#define SOCKET_SET_HANDLER testsocket_setClientHandler
#define SOCKET_SEND testsocket_clientSend
#else
#define SOCKET_DELETE msocket_delete
#define SOCKET_TYPE msocket_t
#define SOCKET_START_IO(x) msocket_start_io(x)
#define SOCKET_SET_HANDLER msocket_sethandler
#define SOCKET_SEND msocket_send
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void register_msocket_handler(apx_clientSocketConnection_t *self, SOCKET_TYPE *socketObject);
static void create_connection_interface_vtable(apx_clientSocketConnection_t* self, apx_connectionInterface_t* interface);

//msocket API
static void on_socket_connected(void *arg, const char *addr, uint16_t port);
static void on_socket_disconnected(void* arg);
static int8_t on_socket_data(void* arg, const uint8_t* data, uint32_t data_size, uint32_t* parse_size);

//APX BaseConnection API
static void apx_clientSocketConnection_close(apx_clientSocketConnection_t *self);
static void apx_clientSocketConnection_vclose(void *arg);
static void apx_clientSocketConnection_start(apx_clientSocketConnection_t *self);
static void apx_clientSocketConnection_vstart(void *arg);

// ConnectionInterface API
static int32_t apx_clientSocketConnection_transmit_max_bytes_avaiable(apx_clientSocketConnection_t* self);
static int32_t apx_clientSocketConnection_transmit_current_bytes_avaiable(apx_clientSocketConnection_t* self);
static void apx_clientSocketConnection_transmit_begin(apx_clientSocketConnection_t* self);
static void apx_clientSocketConnection_transmit_end(apx_clientSocketConnection_t* self);
static apx_error_t apx_clientSocketConnection_transmit_data_message(apx_clientSocketConnection_t* self, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
static apx_error_t apx_clientSocketConnection_transmit_direct_message(apx_clientSocketConnection_t* self, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
static void send_packet(apx_clientSocketConnection_t* self);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientSocketConnection_create(apx_clientSocketConnection_t *self, SOCKET_TYPE * socket_object, apx_connectionType_t connection_type)
{
   if (self != 0)
   {
      apx_connectionBaseVTable_t base_connection_vtable;
      apx_connectionInterface_t connection_interface;
      MUTEX_INIT(self->lock);
      self->default_buffer_size = SEND_BUFFER_GROW_SIZE;
      self->pending_bytes = 0u;
      apx_connectionBaseVTable_create(&base_connection_vtable,
            apx_clientSocketConnection_vdestroy,
            apx_clientSocketConnection_vstart,
            apx_clientSocketConnection_vclose);
      create_connection_interface_vtable(self, &connection_interface);
      apx_error_t result = apx_clientConnection_create(&self->base, &base_connection_vtable, &connection_interface);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      apx_clientConnection_set_connection_type(&self->base, connection_type);
      adt_bytearray_create(&self->send_buffer, SEND_BUFFER_GROW_SIZE);
      register_msocket_handler(self, socket_object);
      apx_connectionBase_start(&self->base.base);///TODO: Don't call start from the constructor
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_clientSocketConnection_destroy(apx_clientSocketConnection_t *self)
{
   if (self != 0)
   {
      apx_clientConnection_destroy(&self->base);
      adt_bytearray_destroy(&self->send_buffer);
      SOCKET_DELETE(self->socket_object);
      MUTEX_DESTROY(self->lock);
   }
}

void apx_clientSocketConnection_vdestroy(void *arg)
{
   apx_clientSocketConnection_destroy((apx_clientSocketConnection_t*) arg);
}

apx_clientSocketConnection_t *apx_clientSocketConnection_new(SOCKET_TYPE *socket_object, apx_connectionType_t connection_type)
{
   apx_clientSocketConnection_t *self = (apx_clientSocketConnection_t*) malloc(sizeof(apx_clientSocketConnection_t));
   if (self != 0)
   {
      apx_error_t errorCode = apx_clientSocketConnection_create(self, socket_object, connection_type);
      if (errorCode != APX_NO_ERROR)
      {
         free(self);
         self = (apx_clientSocketConnection_t*) 0;
      }
   }
   return self;
}

apx_connectionType_t apx_clientSocketConnection_get_connection_type(apx_clientSocketConnection_t const* self)
{
   if (self != NULL)
   {
      return apx_clientConnection_get_connection_type(&self->base);
   }
   return APX_CONNECTION_TYPE_DEFAULT;
}

#ifndef UNIT_TEST
apx_error_t apx_clientConnection_tcp_connect(apx_clientSocketConnection_t *self, const char *address, uint16_t port)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      msocket_t *socketObject = msocket_new(AF_INET);
      if (socketObject != 0)
      {
         int8_t result = 0;
         register_msocket_handler(self, socketObject);
         result = msocket_connect(socketObject, address, port);
         if (result != 0)
         {
            msocket_delete(socketObject);
            self->socket_object = (SOCKET_TYPE*) 0;
            retval = APX_CONNECTION_ERROR;
         }
         else
         {

         }
      }
      else
      {
         retval = APX_MEM_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
# ifndef _WIN32
apx_error_t apx_clientConnection_unix_connect(apx_clientSocketConnection_t *self, const char *socket_path)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      msocket_t *socket_object = msocket_new(AF_LOCAL);
      if (socket_object != 0)
      {
         int8_t result = 0;
         register_msocket_handler(self, socket_object);
         result = msocket_unix_connect(socket_object, socket_path);
         if (result != 0)
         {
            msocket_delete(socket_object);
            self->socket_object = (SOCKET_TYPE*) 0;
            retval = APX_CONNECTION_ERROR;
         }
         else
         {

         }
      }
      else
      {
         retval = APX_MEM_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
# endif // _WIN32
#endif // UNIT_TEST

// ConnectionInterface API
int32_t apx_clientSocketConnection_vtransmit_max_bytes_avaiable(void* arg)
{
   return apx_clientSocketConnection_transmit_max_bytes_avaiable((apx_clientSocketConnection_t*)arg);
}

int32_t apx_clientSocketConnection_vtransmit_current_bytes_avaiable(void* arg)
{
   return apx_clientSocketConnection_transmit_current_bytes_avaiable((apx_clientSocketConnection_t*)arg);
}

void apx_clientSocketConnection_vtransmit_begin(void* arg)
{
   apx_clientSocketConnection_transmit_begin((apx_clientSocketConnection_t*)arg);
}

void apx_clientSocketConnection_vtransmit_end(void* arg)
{
   apx_clientSocketConnection_transmit_end((apx_clientSocketConnection_t*)arg);
}

apx_error_t apx_clientSocketConnection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   return apx_clientSocketConnection_transmit_data_message((apx_clientSocketConnection_t*)arg, write_address, more_bit, msg_data, msg_size, bytes_available);
}

apx_error_t apx_clientSocketConnection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
{
   return apx_clientSocketConnection_transmit_direct_message((apx_clientSocketConnection_t*)arg, msg_data, msg_size, bytes_available);
}


#ifdef UNIT_TEST

void apx_clientSocketConnection_attach_node_manager(apx_clientSocketConnection_t* self, apx_nodeManager_t* node_manager)
{
   if ((self != NULL) && (node_manager != NULL))
   {
      apx_clientConnection_attach_node_manager(&self->base, node_manager);
   }
}

apx_error_t apx_clientSocketConnection_build_node(apx_clientSocketConnection_t* self, char const* definition_text)
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

void apx_clientSocketConnection_run(apx_clientSocketConnection_t* self)
{
   if (self != NULL)
   {
      apx_clientConnection_run(&self->base);
   }
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////



static void register_msocket_handler(apx_clientSocketConnection_t* self, SOCKET_TYPE* socket_object)
{
   if (socket_object != NULL)
   {
      msocket_handler_t handler_table;
      memset(&handler_table,0,sizeof(handler_table));
      handler_table.tcp_connected = on_socket_connected;
      handler_table.tcp_data = on_socket_data;
      handler_table.tcp_disconnected = on_socket_disconnected;
      SOCKET_SET_HANDLER(socket_object, &handler_table, self);
      self->socket_object = socket_object;
   }
}


static void create_connection_interface_vtable(apx_clientSocketConnection_t* self, apx_connectionInterface_t* interface)
{
   memset(interface, 0, sizeof(apx_connectionInterface_t));
   interface->arg = (void*)self;
   interface->transmit_max_buffer_size = apx_clientSocketConnection_vtransmit_max_bytes_avaiable;
   interface->transmit_current_bytes_avaiable = apx_clientSocketConnection_vtransmit_current_bytes_avaiable;
   interface->transmit_begin = apx_clientSocketConnection_vtransmit_begin;
   interface->transmit_end = apx_clientSocketConnection_vtransmit_end;
   interface->transmit_data_message = apx_clientSocketConnection_vtransmit_data_message;
   interface->transmit_direct_message = apx_clientSocketConnection_vtransmit_direct_message;
}

//msocket API

static void on_socket_connected(void* arg, const char* addr, uint16_t port)
{
   apx_clientSocketConnection_t *self;
   (void) addr;
   (void) port;
#if APX_DEBUG_ENABLE
   printf("[CLIENT-SOCKET] Connected\n");
#endif
   self = (apx_clientSocketConnection_t*) arg;
   apx_clientConnection_connected_notification(&self->base);
}

static int8_t on_socket_data(void* arg, const uint8_t* data, uint32_t data_size, uint32_t* parse_size)
{
   apx_clientSocketConnection_t *self = (apx_clientSocketConnection_t*) arg;
   int8_t retval = (int8_t) apx_clientConnection_on_data_received(&self->base, data, data_size, parse_size);
   return retval;
}

static void on_socket_disconnected(void* arg)
{
   apx_clientSocketConnection_t *self = (apx_clientSocketConnection_t*) arg;
#if APX_DEBUG_ENABLE
   printf("[CLIENT-SOCKET] Disconnected\n");
#endif
   apx_clientConnection_disconnected_notification(&self->base);
}

static void apx_clientSocketConnection_close(apx_clientSocketConnection_t *self)
{
   (void)self;
}

static void apx_clientSocketConnection_vclose(void *arg)
{
   apx_clientSocketConnection_close((apx_clientSocketConnection_t*) arg);
}

static void apx_clientSocketConnection_start(apx_clientSocketConnection_t *self)
{
   apx_clientConnection_start(&self->base);
}

static void apx_clientSocketConnection_vstart(void *arg)
{
   apx_clientSocketConnection_start((apx_clientSocketConnection_t*) arg);
}

static int32_t apx_clientSocketConnection_transmit_max_bytes_avaiable(apx_clientSocketConnection_t* self)
{
   if (self != NULL)
   {
      return (int32_t)self->default_buffer_size;
   }
   return -1;
}

static int32_t apx_clientSocketConnection_transmit_current_bytes_avaiable(apx_clientSocketConnection_t* self)
{
   if (self != NULL)
   {
      return (int32_t)(adt_bytearray_length(&self->send_buffer) - self->pending_bytes);
   }
   return -1;
}

static void apx_clientSocketConnection_transmit_begin(apx_clientSocketConnection_t* self)
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

static void apx_clientSocketConnection_transmit_end(apx_clientSocketConnection_t* self)
{
   if (self != NULL)
   {
      if (self->pending_bytes > 0u)
      {
         send_packet(self);
      }
      MUTEX_UNLOCK(self->lock);
   }
}

static apx_error_t apx_clientSocketConnection_transmit_data_message(apx_clientSocketConnection_t* self, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
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
         send_packet(self);
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

static apx_error_t apx_clientSocketConnection_transmit_direct_message(apx_clientSocketConnection_t* self, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available)
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
         send_packet(self);
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

static void send_packet(apx_clientSocketConnection_t* self)
{
   if ((self->socket_object != NULL) && (self->pending_bytes > 0u))
   {
      uint8_t const* data = adt_bytearray_const_data(&self->send_buffer);
#if APX_DEBUG_ENABLE
      printf("[SOCKET-CLIENT-CONNECTION] Sending %d bytes\n", (int)self->pending_bytes);
#endif
      SOCKET_SEND(self->socket_object, data, (uint32_t)self->pending_bytes);
      adt_bytearray_clear(&self->send_buffer);
      self->pending_bytes = 0u;
   }
}
