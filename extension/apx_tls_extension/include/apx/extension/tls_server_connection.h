/*****************************************************************************
 * \file      tls_server_connection.h
 * \author    Conny Gustafsson
 * \date      2026-09-27
 * \brief     TLS server connection class
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_TLS_SERVER_CONNECTION_H
#define APX_TLS_SERVER_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "apx/server_connection.h"
#include "msocket.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct msocket_tag;

typedef struct apx_tls_server_connection_tag
{
  apx_server_connection_t base;
  apx_node_manager_t node_manager;
  adt_bytearray_t send_buffer;
  apx_size_t default_buffer_size;
  apx_size_t pending_bytes;
  struct msocket_tag *socket_object;
  MUTEX_T lock;
} apx_tls_server_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_tls_server_connection_create(apx_tls_server_connection_t *self, msocket_t *socket_object);
void apx_tls_server_connection_destroy(apx_tls_server_connection_t *self);
void apx_tls_server_connection_vdestroy(void *arg);
apx_tls_server_connection_t *apx_tls_server_connection_new(msocket_t *socket_object);
void apx_tls_server_connection_delete(apx_tls_server_connection_t *self);
void apx_tls_server_connection_vdelete(void *arg);
void apx_tls_server_connection_vstart(void *arg);
void apx_tls_server_connection_vclose(void *arg);
void apx_tls_server_connection_set_tag(apx_tls_server_connection_t *self, const char *tag);

// ConnectionInterface API
int32_t apx_tls_server_connection_vtransmit_max_bytes_avaiable(void *arg);
int32_t apx_tls_server_connection_vtransmit_current_bytes_avaiable(void *arg);
void apx_tls_server_connection_vtransmit_begin(void *arg);
void apx_tls_server_connection_vtransmit_end(void *arg);
apx_error_t apx_tls_server_connection_vtransmit_data_message(void *arg, uint32_t write_address, bool more_bit,
  const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available);
apx_error_t apx_tls_server_connection_vtransmit_direct_message(
  void *arg, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available);

#endif // APX_TLS_SERVER_CONNECTION_H
