/*****************************************************************************
 * \file      tls_server.c
 * \author    Conny Gustafsson
 * \date      2026-09-27
 * \brief     TLS server for apx_server
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/tls_server.h"
#include "apx/extension/tls_server_connection.h"
#include "apx/server.h"
#include "apx/types.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_tls_server_tcp_accept(void *arg, struct msocket_server_tag *srv, void *sock);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_tls_server_create(apx_tls_server_t *self, struct apx_server_tag *apx_server)
{
  if (self != NULL)
  {
    self->parent = apx_server;
    self->tcp_port = 0u;
    self->is_tcp_server_started = false;
    self->tcp_connection_tag = NULL;
  }
}

void apx_tls_server_destroy(apx_tls_server_t *self)
{
  if (self != NULL)
  {
    apx_tls_server_stop(self);
    if (self->tcp_connection_tag != NULL)
    {
      free(self->tcp_connection_tag);
      self->tcp_connection_tag = NULL;
    }
  }
}

apx_tls_server_t *apx_tls_server_new(struct apx_server_tag *apx_server)
{
  apx_tls_server_t *self = (apx_tls_server_t *)malloc(sizeof(apx_tls_server_t));
  if (self != NULL)
  {
    apx_tls_server_create(self, apx_server);
  }
  return self;
}

void apx_tls_server_delete(apx_tls_server_t *self)
{
  if (self != NULL)
  {
    apx_tls_server_destroy(self);
    free(self);
  }
}

apx_error_t apx_tls_server_start(
  apx_tls_server_t *self, uint16_t tcp_port, const msocket_tls_config_t *tls_config, const char *tag)
{
  if (self == NULL || tls_config == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  msocket_handler_t server_handler;
  self->tcp_port = tcp_port;
  if (tag != NULL && strlen(tag) > 0u)
  {
    self->tcp_connection_tag = STRDUP(tag);
  }
  memset(&server_handler, 0, sizeof(server_handler));
  server_handler.stream_accept = apx_tls_server_tcp_accept;

  msocket_server_create(&self->tcp_server, MSOCKET_ADDR_INET, NULL);
  msocket_server_disable_cleanup(&self->tcp_server);
  msocket_server_set_handler(&self->tcp_server, &server_handler, self);

  msocket_error_t rc = msocket_server_start_tls(&self->tcp_server, self->tcp_port, tls_config);
  if (rc != MSOCKET_NO_ERROR)
  {
    msocket_server_destroy(&self->tcp_server);
    return APX_GENERIC_ERROR;
  }

  self->is_tcp_server_started = true;
  printf("Listening on TLS TCP port %d\n", (int)self->tcp_port);
  return APX_NO_ERROR;
}

void apx_tls_server_stop(apx_tls_server_t *self)
{
  if (self != NULL && self->is_tcp_server_started)
  {
    msocket_server_destroy(&self->tcp_server);
    self->is_tcp_server_started = false;
  }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_tls_server_tcp_accept(void *arg, struct msocket_server_tag *srv, void *sock)
{
  apx_tls_server_t *self = (apx_tls_server_t *)arg;
  (void)srv;
  if (self != NULL && sock != NULL)
  {
    if (self->parent != NULL)
    {
      apx_server_log_write(self->parent, APX_LOG_LEVEL_DEBUG, "TLS_SERVER", "New incoming connection");
    }
    apx_tls_server_connection_t *new_connection = apx_tls_server_connection_new((msocket_t *)sock);
    if (new_connection != NULL)
    {
      if (self->tcp_connection_tag != NULL)
      {
        apx_tls_server_connection_set_tag(new_connection, self->tcp_connection_tag);
      }
      apx_server_accept_connection(self->parent, (apx_server_connection_t *)new_connection);
    }
    else
    {
      msocket_delete((msocket_t *)sock);
    }
  }
}
