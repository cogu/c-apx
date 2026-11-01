/*****************************************************************************
 * \file      json_server_connection.c
 * \author    Conny Gustafsson
 * \date      2020-03-07
 * \brief     Connection in the json server
 *
 * Copyright (c) 2020-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "json_server_connection.h"
#include "apx/error.h"
#include "apx_connection.h"
#include "bstr.h"
#include "dtl_json.h"
#include "json_server.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void json_server_connection_disconnected(void *arg, void *socket);
static msocket_error_t json_server_connection_data(void *arg, void *socket, const uint8_t *data_buf,
  const uint32_t data_len, uint32_t *consumed_bytes, uint32_t *msg_size_hint);
static void json_server_connection_process_message(
  json_server_connection_t *self, const uint8_t *p_begin, const uint8_t *p_end);
static void json_server_connection_process_hash_value(json_server_connection_t *self, dtl_hv_t *hv);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void json_server_connection_create(
  json_server_connection_t *self, msocket_t *msocket, struct apx_connection_tag *apx_connection)
{
  if ((self != NULL) && (msocket != NULL) && (apx_connection != NULL))
  {
    msocket_handler_t handler;
    self->msocket = msocket;
    self->apx_connection = apx_connection;
    memset(&handler, 0, sizeof(msocket_handler_t));
    handler.tcp_data = json_server_connection_data;
    handler.tcp_disconnected = json_server_connection_disconnected;
    msocket_sethandler(self->msocket, &handler, (void *)self);
  }
}

void json_server_connection_destroy(json_server_connection_t *self)
{
  if (self != NULL)
  {
    msocket_delete(self->msocket);
  }
}

json_server_connection_t *json_server_connection_new(msocket_t *msocket, struct apx_connection_tag *apx_connection)
{
  json_server_connection_t *self = (json_server_connection_t *)malloc(sizeof(json_server_connection_t));
  if (self != NULL)
  {
    json_server_connection_create(self, msocket, apx_connection);
  }
  return self;
}

void json_server_connection_delete(json_server_connection_t *self)
{
  if (self != NULL)
  {
    json_server_connection_destroy(self);
    free(self);
  }
}

void json_server_connection_vdelete(void *arg) { json_server_connection_delete((json_server_connection_t *)arg); }

void json_server_connection_start(json_server_connection_t *self)
{
  if (self != NULL)
  {
    msocket_start_io(self->msocket);
  }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void json_server_connection_disconnected(void *arg, void *socket)
{
  (void)socket;
  json_server_connection_t *self = (json_server_connection_t *)arg;
  if (self != NULL)
  {
    json_server_cleanup_connection(self);
  }
}

static msocket_error_t json_server_connection_data(void *arg, void *socket, const uint8_t *data_buf,
  const uint32_t data_len, uint32_t *consumed_bytes, uint32_t *msg_size_hint)
{
  (void)socket;
  json_server_connection_t *self = (json_server_connection_t *)arg;
  if ((self == NULL) || (data_buf == NULL) || (consumed_bytes == NULL))
  {
    return MSOCKET_INVALID_ARGUMENT_ERROR;
  }
  *consumed_bytes = 0u;
  if (msg_size_hint != NULL)
  {
    *msg_size_hint = 0u;
  }
  if (data_len == 0u)
  {
    return MSOCKET_NO_ERROR;
  }

  const uint8_t *data_end = data_buf + data_len;
  const uint8_t *line_end = bstr_find_line_feed(data_buf, data_end);
  if (line_end < data_end)
  {
    const uint8_t *p_begin = NULL;
    const uint8_t *p_end = NULL;
    bstr_strip(data_buf, line_end, &p_begin, &p_end);
    if (p_end > p_begin)
    {
      json_server_connection_process_message(self, p_begin, p_end);
    }
    *consumed_bytes = (uint32_t)((line_end + 1) - data_buf);
    return MSOCKET_NO_ERROR;
  }

  return MSOCKET_NO_ERROR;
}

static void json_server_connection_process_message(
  json_server_connection_t *self, const uint8_t *p_begin, const uint8_t *p_end)
{
  dtl_dv_t *dv = dtl_json_load_bstr(p_begin, p_end);
  if (dv != NULL)
  {
    if (dtl_dv_type(dv) == DTL_DV_HASH)
    {
      json_server_connection_process_hash_value(self, (dtl_hv_t *)dv);
    }
    dtl_dv_dec_ref(dv);
  }
}

static void json_server_connection_process_hash_value(json_server_connection_t *self, dtl_hv_t *hv)
{
  const char *key;
  dtl_dv_t *dv;
  assert(self != NULL);
  assert(hv != NULL);
  dtl_hv_iter_init(hv);
  dv = dtl_hv_iter_next_cstr(hv, &key);
  while (dv != NULL)
  {
    apx_error_t result;
    if (self->apx_connection != NULL)
    {
      result = apx_connection_write_provide_port_data(self->apx_connection, key, dv);
    }
    else
    {
      result = APX_NULL_PTR_ERROR;
    }
    if (result != APX_NO_ERROR)
    {
      printf("Write failed on port '%s': %s\n", key, apx_strerror(result));
    }
    dv = dtl_hv_iter_next_cstr(hv, &key);
  }
}
