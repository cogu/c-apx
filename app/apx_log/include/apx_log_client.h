/*****************************************************************************
 * \file      apx_log_client.h
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     APX log streaming client library
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_LOG_CLIENT_H
#define APX_LOG_CLIENT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "apx/socket_client_connection.h"
#include "osmacro.h"
#include <stdbool.h>
#include <stdint.h>

#if defined(MSOCKET_ENABLE_TLS)
# include "msocket_tls.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef void(apx_log_client_event_cb_t)(void *arg, const uint8_t *data, uint32_t size);

typedef struct apx_log_client_tag
{
  apx_client_socket_connection_t connection;
  MUTEX_T lock;
  COND_T cond;
  bool is_connected;
  bool is_greeting_accepted;
  bool is_server_log_open;
  uint32_t server_log_address;
  apx_log_client_event_cb_t *event_cb;
  void *event_arg;
} apx_log_client_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

// Lifecycle
apx_error_t apx_log_client_create(apx_log_client_t *self);
void apx_log_client_destroy(apx_log_client_t *self);
apx_log_client_t *apx_log_client_new(void);
void apx_log_client_delete(apx_log_client_t *self);

// Connection
#ifndef _WIN32
apx_error_t apx_log_client_connect_unix(apx_log_client_t *self, const char *socket_path);
#endif
apx_error_t apx_log_client_connect_tcp(apx_log_client_t *self, const char *address, uint16_t port);
#if defined(MSOCKET_ENABLE_TLS)
apx_error_t apx_log_client_connect_tls(
  apx_log_client_t *self, const char *address, uint16_t port, const msocket_tls_config_t *tls_config);
#endif
void apx_log_client_disconnect(apx_log_client_t *self);
apx_error_t apx_log_client_wait_ready(apx_log_client_t *self, uint32_t timeout_ms);
bool apx_log_client_is_connected(const apx_log_client_t *self);

// Event callback setter
void apx_log_client_set_event_callback(apx_log_client_t *self, apx_log_client_event_cb_t *cb, void *arg);

#endif // APX_LOG_CLIENT_H
