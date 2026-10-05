/*****************************************************************************
 * \file      tls_server.h
 * \author    Conny Gustafsson
 * \date      2026-09-27
 * \brief     TLS server for apx_server
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_TLS_SERVER_H
#define APX_TLS_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "msocket.h"
#include "msocket_server.h"
#include "msocket_tls.h"
#include <stdbool.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_server_tag;

typedef struct apx_tls_server_tag
{
  uint16_t tcp_port;
  msocket_server_t tcp_server;
  struct apx_server_tag *parent;
  char *tcp_connection_tag;
  bool is_tcp_server_started;
} apx_tls_server_t;

#define APX_TLS_SERVER_LABEL "TLS"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_tls_server_create(apx_tls_server_t *self, struct apx_server_tag *apx_server);
void apx_tls_server_destroy(apx_tls_server_t *self);
apx_tls_server_t *apx_tls_server_new(struct apx_server_tag *apx_server);
void apx_tls_server_delete(apx_tls_server_t *self);

apx_error_t apx_tls_server_start(
  apx_tls_server_t *self, uint16_t tcp_port, const msocket_tls_config_t *tls_config, const char *tag);
void apx_tls_server_stop(apx_tls_server_t *self);

#endif // APX_TLS_SERVER_H
