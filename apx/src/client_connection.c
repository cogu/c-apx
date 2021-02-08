/*****************************************************************************
* \file      client_connection_base.c
* \author    Conny Gustafsson
* \date      2018-12-31
* \brief     Base class for client connections
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#include <stdio.h>
#include "apx/client_connection.h"
#include "apx/numheader.h"
#include "apx/file.h"
#include "apx/remotefile.h"
#include "apx/client.h"
#include "apx/client_internal.h"
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
static void send_greeting_header(apx_clientConnection_t* self);
static apx_error_t remote_file_published_notification(apx_clientConnection_t* self, apx_file_t* file);
static apx_error_t process_new_require_port_data_file(apx_clientConnection_t* self, apx_file_t* file);
static apx_error_t remote_file_write_notification(apx_clientConnection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);
static uint8_t const* parse_message(apx_clientConnection_t* self, uint8_t const* begin, uint8_t const* end, apx_error_t* error_code);
static bool is_greeting_accepted(uint8_t const* msg_data, apx_size_t msg_size, apx_error_t* error_code);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_clientConnection_create(apx_clientConnection_t* self, apx_connectionBaseVTable_t* base_connection_vtable, apx_connectionInterface_t* connection_interface)
{
   if (self != 0)
   {
      apx_error_t error_code;
      //init non-overridable virtual functions
      base_connection_vtable->node_created_notification = NULL; //Only used in server mode
      base_connection_vtable->require_port_write_notification = apx_clientConnection_vrequire_port_write_notification;
      connection_interface->remote_file_published_notification = apx_clientConnection_vremote_file_published_notification;
      connection_interface->remote_file_write_notification = apx_clientConnection_vremote_file_write_notification;
      error_code = apx_connectionBase_create(&self->base, APX_CLIENT_MODE, base_connection_vtable, connection_interface);
      self->is_greeting_accepted = false;
      self->client = NULL;
      self->last_error = APX_NO_ERROR;
      //apx_connectionBase_setEventHandler(&self->base, apx_clientConnection_defaultEventHandler, (void*) self);
      return error_code;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_clientConnection_destroy(apx_clientConnection_t *self)
{
   if (self != 0)
   {
      apx_connectionBase_destroy(&self->base);
   }
}

apx_fileManager_t* apx_clientConnection_get_file_manager(apx_clientConnection_t* self)
{
   if (self != 0)
   {
      return apx_connectionBase_get_file_manager(&self->base);
   }
   return NULL;
}

void apx_clientConnection_start(apx_clientConnection_t* self)
{
   (void)self;
}


void apx_clientConnection_close(apx_clientConnection_t* self)
{
   (void)self;
}

uint32_t apx_clientConnection_get_total_bytes_received(apx_clientConnection_t* self)
{
   (void)self;
   return 0u;
}

uint32_t apx_clientConnection_get_total_bytes_sent(apx_clientConnection_t* self)
{
   (void)self;
   return 0u;
}

/*
void apx_clientConnection_default_event_handler(void* arg, apx_event_t* event)
{
   (void)arg;
   (void)event;
}

void* apx_clientConnection_register_event_listener(apx_clientConnection_t* self, apx_connectionEventListener_t* listener)
{
   (void)self;
   (void)listener;
   return NULL;
}

void apx_clientConnection_unregister_event_listener(apx_clientConnection_t* self, void* handle)
{
   (void)self;
   (void)handle;
}
*/

void apx_clientConnection_greeting_header_accepted_notification(apx_clientConnection_t* self)
{
   if (self != NULL)
   {
      self->is_greeting_accepted = true;
      apx_fileManager_connected(&self->base.file_manager);
   }
}

void apx_clientConnection_connected_notification(apx_clientConnection_t* self)
{
   if (self != NULL)
   {
      self->is_greeting_accepted = false;
      send_greeting_header(self);
      if (self->client != NULL)
      {
         apx_clientInternal_connect_notification(self->client, self);
      }
   }
}

void apx_clientConnection_disconnected_notification(apx_clientConnection_t* self)
{
   if (self != NULL)
   {
      if (self->client != NULL)
      {
         apx_clientInternal_disconnect_notification(self->client, self);
      }
   }
}

void apx_clientConnection_attach_node_manager(apx_clientConnection_t* self, apx_nodeManager_t* node_manager)
{
   if (self != NULL)
   {
      apx_connectionBase_attach_node_manager(&self->base, node_manager);
   }
}

apx_nodeManager_t* apx_clientConnection_get_node_manager(apx_clientConnection_t* self)
{
   if (self != NULL)
   {
      return apx_connectionBase_get_node_manager(&self->base);
   }
   return NULL;
}

void apx_clientConnection_set_client(apx_clientConnection_t* self, struct apx_client_tag* client)
{
   if (self != NULL)
   {
      self->client = client;
   }
}

int apx_clientConnection_on_data_received(apx_clientConnection_t* self, uint8_t const* data, apx_size_t data_size, apx_size_t* parse_len)
{
   if ( (self != NULL) && (data != NULL) && (data_size > 0u) && (parse_len != NULL))
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
            total_parse_len = (apx_size_t) (next - data);
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

apx_error_t apx_clientConnection_attach_node_instance(apx_clientConnection_t* self, apx_nodeInstance_t* node_instance)
{
   if ((self != NULL) && (node_instance != NULL))
   {
      apx_fileManager_t* file_manager = apx_connectionBase_get_file_manager(&self->base);
      assert(file_manager != NULL);
      return apx_nodeInstance_attach_to_file_manager(node_instance, file_manager);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_clientConnection_vrequire_port_write_notification(void* arg, apx_portInstance_t* port_instance, uint8_t const* data, apx_size_t size)
{
   apx_clientConnection_t* self = (apx_clientConnection_t*)arg;
   if ( (self != NULL) && (port_instance != NULL) && (self->client != NULL))
   {
      apx_clientInternal_require_port_write_notification(self->client, self, port_instance, data, size);
   }
}

//ConnectionInterface API
apx_error_t apx_clientConnection_vremote_file_published_notification(void* arg, apx_file_t* file)
{
   return remote_file_published_notification((apx_clientConnection_t*)arg, file);
}

apx_error_t apx_clientConnection_vremote_file_write_notification(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   return remote_file_write_notification((apx_clientConnection_t*)arg, file, offset, data, size);
}

#ifdef UNIT_TEST
void apx_clientConnection_run(apx_clientConnection_t* self)
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

static void send_greeting_header(apx_clientConnection_t* self)
{
   int32_t greeting_size;
   apx_connectionInterface_t const* connection;
   int num_header_format = 32;
   char greeting[RMF_GREETING_MAX_LEN];
   char* p = &greeting[0];
   strcpy(greeting, RMF_GREETING_START);
   p += strlen(greeting);
   p += sprintf(p, "%s%d\n\n", RMF_NUMHEADER_FORMAT_HDR, num_header_format);
   greeting_size = (int32_t)(p - greeting);
   connection = apx_connectionBase_get_connection(&self->base);
   if (connection != NULL)
   {
      int32_t bytes_available = 0;
      assert((connection->transmit_begin != NULL) && (connection->transmit_end != NULL) && (connection->transmit_direct_message != NULL));
      connection->transmit_begin(connection->arg);
      connection->transmit_direct_message(connection->arg, (uint8_t const*)greeting, greeting_size, &bytes_available);
      connection->transmit_end(connection->arg);
   }
}

static apx_error_t remote_file_published_notification(apx_clientConnection_t* self, apx_file_t* file)
{
   if ((self != NULL) && (file != NULL))
   {
#if APX_DEBUG_ENABLE
      printf("[CLIENT-CONNECTION] remote_file_published_notification: \"%s\"(%d)\n", apx_file_get_name(file), apx_file_get_apx_file_type(file));
#endif
      if (apx_file_get_apx_file_type(file) == APX_REQUIRE_PORT_DATA_FILE_TYPE)
      {
         return process_new_require_port_data_file(self, file);
      }
      //TODO: Add generic handling of new file types
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t process_new_require_port_data_file(apx_clientConnection_t* self, apx_file_t* file)
{
   assert((self != NULL) && (file != NULL));
   char* base_name = rmf_fileInfo_base_name(apx_file_get_file_info(file));
   if (base_name != NULL)
   {
      assert(self->base.node_manager != NULL);
      apx_nodeInstance_t* node_instance = apx_nodeManager_find(self->base.node_manager, base_name);
      if (node_instance != NULL)
      {
         free(base_name);
         return apx_nodeInstance_remote_file_published_notification(node_instance, file);
      }
      else
      {
#if APX_DEBUG_ENABLE
         printf("Node not found: \"%s\"\n", base_name);
#endif
        free(base_name);
      }
   }
   else
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}


static apx_error_t remote_file_write_notification(apx_clientConnection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   if ( (self != NULL) && (file != NULL) )
   {
      return apx_file_write_notify(file, offset, data, (uint32_t)size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static uint8_t const* parse_message(apx_clientConnection_t* self, uint8_t const* begin, uint8_t const* end, apx_error_t* error_code)
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
         printf("[CLIENT-CONNECTION]: Received message: (%d+%d) bytes\n", (int)header_size, (int)msg_size);
#endif
         if (msg_end <= end)
         {
            if (self->is_greeting_accepted)
            {
               *error_code = apx_connectionBase_message_received(&self->base, msg_data, msg_size);
            }
            else
            {
               if (is_greeting_accepted(msg_data, msg_size, error_code))
               {
                  apx_clientConnection_greeting_header_accepted_notification(self);
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

static bool is_greeting_accepted(uint8_t const* msg_data, apx_size_t msg_size, apx_error_t* error_code)
{
   uint32_t address;
   bool more_bit = false;
   uint8_t const* const msg_end = msg_data + msg_size;
   apx_size_t header_size = rmf_address_decode(msg_data, msg_data + msg_size, &address, &more_bit);
   if (address == RMF_CMD_AREA_START_ADDRESS)
   {
      uint32_t cmd_type = 0u;
      apx_size_t decode_size = rmf_decode_cmd_type(msg_data + header_size, msg_end, &cmd_type);
      if ((decode_size == RMF_CMD_TYPE_SIZE) && (cmd_type ==RMF_CMD_ACK_MSG))
      {
         return true;
      }
   }
   else
   {
      *error_code = APX_INVALID_MSG_ERROR;
   }
   return false;
}

