/*****************************************************************************
 * \file      tls_server_connection.c
 * \author    Conny Gustafsson
 * \date      2026-09-27
 * \brief     TLS server connection class
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/tls_server_connection.h"
#include "apx/file_manager.h"
#include "apx/numheader.h"
#include "apx/remotefile.h"
#include "apx/server.h"
#include "bstr.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define SEND_BUFFER_GROW_SIZE 4096

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void register_msocket_handler(apx_tls_server_connection_t *self, msocket_t *socket_object);
static void create_connection_interface_vtable(
  apx_tls_server_connection_t *self, apx_connection_interface_t *interface);

// msocket API callbacks
static void socket_disconnected_notification(void *arg, void *socket);
static msocket_error_t socket_data_notification(void *arg, void *socket, const uint8_t *data, const uint32_t num_bytes,
  uint32_t *consumed_bytes, uint32_t *msg_size_hint);

// APX BaseConnection API
static void connection_close(apx_tls_server_connection_t *self);
static void connection_start(apx_tls_server_connection_t *self);

// ConnectionInterface API
static int32_t connection_transmit_max_bytes_available(apx_tls_server_connection_t *self);
static int32_t connection_transmit_current_bytes_available(apx_tls_server_connection_t *self);
static void connection_transmit_begin(apx_tls_server_connection_t *self);
static void connection_transmit_end(apx_tls_server_connection_t *self);
static apx_error_t connection_transmit_data_message(apx_tls_server_connection_t *self, uint32_t write_address,
  bool more_bit, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available);
static apx_error_t connection_transmit_direct_message(
  apx_tls_server_connection_t *self, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available);
static void connection_send_packet(apx_tls_server_connection_t *self);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_tls_server_connection_create(apx_tls_server_connection_t *self, msocket_t *socket_object)
{
  if (self != NULL && socket_object != NULL)
  {
    apx_connection_base_vtable_t base_connection_vtable;
    apx_connection_interface_t connection_interface;
    MUTEX_INIT(self->lock);
    self->default_buffer_size = SEND_BUFFER_GROW_SIZE;
    self->pending_bytes = 0u;
    apx_connection_base_vtable_create(&base_connection_vtable, apx_tls_server_connection_vdestroy,
      apx_tls_server_connection_vstart, apx_tls_server_connection_vclose);
    create_connection_interface_vtable(self, &connection_interface);
    apx_error_t retval = apx_server_connection_create(&self->base, &base_connection_vtable, &connection_interface);
    if (retval == APX_NO_ERROR)
    {
      adt_bytearray_create(&self->send_buffer);
      register_msocket_handler(self, socket_object);
      apx_node_manager_create(&self->node_manager, APX_SERVER_MODE);
      apx_server_connection_attach_node_manager(&self->base, &self->node_manager);
    }
    return retval;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

void apx_tls_server_connection_destroy(apx_tls_server_connection_t *self)
{
  if (self != NULL)
  {
    apx_server_connection_destroy(&self->base);
    adt_bytearray_destroy(&self->send_buffer);
    apx_node_manager_destroy(&self->node_manager);
    msocket_delete(self->socket_object);
    MUTEX_DESTROY(self->lock);
  }
}

void apx_tls_server_connection_vdestroy(void *arg)
{
  apx_tls_server_connection_destroy((apx_tls_server_connection_t *)arg);
}

apx_tls_server_connection_t *apx_tls_server_connection_new(msocket_t *socket_object)
{
  if (socket_object != NULL)
  {
    apx_tls_server_connection_t *self = (apx_tls_server_connection_t *)malloc(sizeof(apx_tls_server_connection_t));
    if (self != NULL)
    {
      apx_error_t result = apx_tls_server_connection_create(self, socket_object);
      if (result != APX_NO_ERROR)
      {
        free(self);
        self = NULL;
      }
    }
    return self;
  }
  return NULL;
}

void apx_tls_server_connection_delete(apx_tls_server_connection_t *self)
{
  if (self != NULL)
  {
    apx_tls_server_connection_destroy(self);
    free(self);
  }
}

void apx_tls_server_connection_vdelete(void *arg)
{
  apx_tls_server_connection_delete((apx_tls_server_connection_t *)arg);
}

void apx_tls_server_connection_vstart(void *arg) { connection_start((apx_tls_server_connection_t *)arg); }

void apx_tls_server_connection_vclose(void *arg) { connection_close((apx_tls_server_connection_t *)arg); }

void apx_tls_server_connection_set_tag(apx_tls_server_connection_t *self, const char *tag)
{
  if (self != NULL && tag != NULL)
  {
    apx_server_connection_set_tag(&self->base, tag);
  }
}

int32_t apx_tls_server_connection_vtransmit_max_bytes_avaiable(void *arg)
{
  return connection_transmit_max_bytes_available((apx_tls_server_connection_t *)arg);
}

int32_t apx_tls_server_connection_vtransmit_current_bytes_avaiable(void *arg)
{
  return connection_transmit_current_bytes_available((apx_tls_server_connection_t *)arg);
}

void apx_tls_server_connection_vtransmit_begin(void *arg)
{
  connection_transmit_begin((apx_tls_server_connection_t *)arg);
}

void apx_tls_server_connection_vtransmit_end(void *arg) { connection_transmit_end((apx_tls_server_connection_t *)arg); }

apx_error_t apx_tls_server_connection_vtransmit_data_message(
  void *arg, uint32_t write_address, bool more_bit, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available)
{
  return connection_transmit_data_message(
    (apx_tls_server_connection_t *)arg, write_address, more_bit, msg_data, msg_size, bytes_available);
}

apx_error_t apx_tls_server_connection_vtransmit_direct_message(
  void *arg, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available)
{
  return connection_transmit_direct_message((apx_tls_server_connection_t *)arg, msg_data, msg_size, bytes_available);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void register_msocket_handler(apx_tls_server_connection_t *self, msocket_t *socket_object)
{
  msocket_handler_t handler_table;
  memset(&handler_table, 0, sizeof(handler_table));
  handler_table.stream_data = socket_data_notification;
  handler_table.stream_disconnected = socket_disconnected_notification;
  self->socket_object = socket_object;
  msocket_set_handler(self->socket_object, &handler_table, self);
}

static void create_connection_interface_vtable(apx_tls_server_connection_t *self, apx_connection_interface_t *interface)
{
  memset(interface, 0, sizeof(apx_connection_interface_t));
  interface->arg = (void *)self;
  interface->transmit_max_buffer_size = apx_tls_server_connection_vtransmit_max_bytes_avaiable;
  interface->transmit_current_bytes_avaiable = apx_tls_server_connection_vtransmit_current_bytes_avaiable;
  interface->transmit_begin = apx_tls_server_connection_vtransmit_begin;
  interface->transmit_end = apx_tls_server_connection_vtransmit_end;
  interface->transmit_data_message = apx_tls_server_connection_vtransmit_data_message;
  interface->transmit_direct_message = apx_tls_server_connection_vtransmit_direct_message;
}

static void socket_disconnected_notification(void *arg, void *socket)
{
  (void)socket;
  apx_tls_server_connection_t *self = (apx_tls_server_connection_t *)arg;
  if (self != NULL)
  {
    assert(self->base.parent != NULL);
    apx_server_log_write(self->base.parent, APX_LOG_LEVEL_INFO, "TLS_SERVER", "[%u] Client disconnected",
      apx_server_connection_get_connection_id(&self->base));
    apx_server_detach_connection(self->base.parent, &self->base);
  }
}

static msocket_error_t socket_data_notification(void *arg, void *socket, const uint8_t *data, const uint32_t num_bytes,
  uint32_t *consumed_bytes, uint32_t *msg_size_hint)
{
  (void)socket;
  apx_tls_server_connection_t *self = (apx_tls_server_connection_t *)arg;
  int retval = apx_server_connection_on_data_received(&self->base, data, num_bytes, consumed_bytes, msg_size_hint);
  return (retval == 0) ? MSOCKET_NO_ERROR : MSOCKET_GENERIC_ERROR;
}

static void connection_close(apx_tls_server_connection_t *self)
{
  if (self != NULL && self->socket_object != NULL)
  {
    msocket_close(self->socket_object);
  }
}

static void connection_start(apx_tls_server_connection_t *self)
{
  assert(self->socket_object != NULL);
  apx_server_connection_start(&self->base);
  msocket_start_io(self->socket_object);
}

static int32_t connection_transmit_max_bytes_available(apx_tls_server_connection_t *self)
{
  if (self != NULL)
  {
    return (int32_t)self->default_buffer_size;
  }
  return -1;
}

static int32_t connection_transmit_current_bytes_available(apx_tls_server_connection_t *self)
{
  if (self != NULL)
  {
    return (int32_t)(adt_bytearray_length(&self->send_buffer) - self->pending_bytes);
  }
  return -1;
}

static void connection_transmit_begin(apx_tls_server_connection_t *self)
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

static void connection_transmit_end(apx_tls_server_connection_t *self)
{
  if (self != NULL)
  {
    connection_send_packet(self);
    MUTEX_UNLOCK(self->lock);
  }
}

static apx_error_t connection_transmit_data_message(apx_tls_server_connection_t *self, uint32_t write_address,
  bool more_bit, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available)
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
    apx_size_t const header2_size =
      rmf_address_encode(header + header1_size, sizeof(header) - header1_size, write_address, more_bit);
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

static apx_error_t connection_transmit_direct_message(
  apx_tls_server_connection_t *self, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available)
{
  if (self != NULL)
  {
    uint8_t header[NUMHEADER32_LONG_SIZE];
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


static void connection_send_packet(apx_tls_server_connection_t *self)
{
  if (self->pending_bytes > 0u)
  {
    msocket_error_t result =
      msocket_send(self->socket_object, adt_bytearray_data(&self->send_buffer), (uint32_t)self->pending_bytes);
    if (result != MSOCKET_NO_ERROR)
    {
      fprintf(stderr, "[TLS-SERVER-CONNECTION] msocket_send failed with %d\n", (int)result);
    }
    self->pending_bytes = 0u;
    adt_bytearray_clear(&self->send_buffer);
  }
}
