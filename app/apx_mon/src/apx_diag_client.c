/*****************************************************************************
 * \file      apx_diag_client.c
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     APX diagnostic protocol client implementation
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "apx_diag_client.h"
#include "apx/file.h"
#include "apx/file_manager.h"
#include "apx/remotefile.h"
#include "pack.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define APX_DIAG_REQ_FILE_NAME "apx.diag.v1.request"
#define APX_DIAG_RESP_FILE_NAME "apx.diag.v1.response"
#define APX_DIAG_NOTIF_FILE_NAME "apx.diag.v1.notify"
#define APX_DIAG_DEF_FILE_PREFIX "apx.diag.v1.def."

#define APX_DIAG_STREAM_SIZE ((uint32_t)(64u * 1024u))
#define APX_DIAG_HEADER_SIZE 8u
#define APX_DIAG_RESP_HEADER_SIZE 12u
#define APX_DIAG_NOTIF_HEADER_SIZE 24u
#define APX_DIAG_CURSOR_END 0xFFFFFFFFu

// Forward declarations of private notification handlers
static void on_client_greeting_accepted(void *arg, apx_client_connection_t *connection);
static void on_client_disconnected(void *arg, apx_client_connection_t *connection);
static apx_error_t on_client_file_published(void *arg, apx_file_t *file);
static apx_error_t on_resp_file_written(
  void *arg, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t size);
static apx_error_t on_notif_file_written(
  void *arg, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t size);
static apx_error_t on_req_file_opened(void *arg, apx_file_t *file);
static apx_error_t on_def_file_written(
  void *arg, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t size);

static void pack_le_u16(uint8_t *dst, uint16_t val)
{
  dst[0] = (uint8_t)(val & 0xFFu);
  dst[1] = (uint8_t)((val >> 8) & 0xFFu);
}

static void pack_le_u32(uint8_t *dst, uint32_t val)
{
  dst[0] = (uint8_t)(val & 0xFFu);
  dst[1] = (uint8_t)((val >> 8) & 0xFFu);
  dst[2] = (uint8_t)((val >> 16) & 0xFFu);
  dst[3] = (uint8_t)((val >> 24) & 0xFFu);
}

static void pack_le_u64(uint8_t *dst, uint64_t val)
{
  pack_le_u32(dst + 0, (uint32_t)(val & 0xFFFFFFFFu));
  pack_le_u32(dst + 4, (uint32_t)((val >> 32) & 0xFFFFFFFFu));
}

static uint16_t unpack_le_u16(const uint8_t *src) { return (uint16_t)(((uint16_t)src[0]) | (((uint16_t)src[1]) << 8)); }

static uint32_t unpack_le_u32(const uint8_t *src)
{
  return ((uint32_t)src[0]) | (((uint32_t)src[1]) << 8) | (((uint32_t)src[2]) << 16) | (((uint32_t)src[3]) << 24);
}

static uint64_t unpack_le_u64(const uint8_t *src)
{
  uint64_t const low = (uint64_t)unpack_le_u32(src + 0);
  uint64_t const high = (uint64_t)unpack_le_u32(src + 4);
  return low | (high << 32);
}

static char *decode_string16(const uint8_t *src, uint32_t max_bytes, uint32_t *consumed)
{
  if (max_bytes < 2u)
  {
    *consumed = 0u;
    return NULL;
  }
  uint16_t const len = unpack_le_u16(src);
  if (2u + (uint32_t)len > max_bytes)
  {
    *consumed = 0u;
    return NULL;
  }
  char *str = (char *)malloc(len + 1u);
  if (str != NULL)
  {
    if (len > 0u)
    {
      memcpy(str, src + 2, len);
    }
    str[len] = '\0';
  }
  *consumed = 2u + len;
  return str;
}

static struct timespec compute_deadline(uint32_t timeout_ms)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += timeout_ms / 1000u;
  ts.tv_nsec += (long)(timeout_ms % 1000u) * 1000000L;
  if (ts.tv_nsec >= 1000000000L)
  {
    ts.tv_sec += 1;
    ts.tv_nsec -= 1000000000L;
  }
  return ts;
}

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_diag_client_create(apx_diag_client_t *self)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  memset(self, 0, sizeof(apx_diag_client_t));
  MUTEX_INIT(self->lock);
  pthread_cond_init(&self->cond, NULL);

  self->request_file_address = RMF_INVALID_ADDRESS;
  self->response_file_address = RMF_INVALID_ADDRESS;
  self->notify_file_address = RMF_INVALID_ADDRESS;
  self->def_file_address = RMF_INVALID_ADDRESS;

  apx_error_t err = apx_client_socket_connection_create(&self->connection, NULL, APX_CONNECTION_TYPE_MONITOR);
  if (err != APX_NO_ERROR)
  {
    MUTEX_DESTROY(self->lock);
    pthread_cond_destroy(&self->cond);
    return err;
  }

  apx_client_socket_connection_set_greeting_accepted_notification(&self->connection, on_client_greeting_accepted, self);
  apx_client_socket_connection_set_disconnected_notification(&self->connection, on_client_disconnected, self);
  apx_client_socket_connection_set_file_published_notification(&self->connection, on_client_file_published, self);

  return APX_NO_ERROR;
}

void apx_diag_client_destroy(apx_diag_client_t *self)
{
  if (self != NULL)
  {
    apx_diag_client_disconnect(self);
    apx_client_socket_connection_destroy(&self->connection);

    if (self->response_payload != NULL)
    {
      free(self->response_payload);
      self->response_payload = NULL;
    }
    if (self->def_data != NULL)
    {
      free(self->def_data);
      self->def_data = NULL;
    }

    MUTEX_DESTROY(self->lock);
    pthread_cond_destroy(&self->cond);
  }
}

apx_diag_client_t *apx_diag_client_new(void)
{
  apx_diag_client_t *self = (apx_diag_client_t *)malloc(sizeof(apx_diag_client_t));
  if (self != NULL)
  {
    apx_error_t err = apx_diag_client_create(self);
    if (err != APX_NO_ERROR)
    {
      free(self);
      return NULL;
    }
  }
  return self;
}

void apx_diag_client_delete(apx_diag_client_t *self)
{
  if (self != NULL)
  {
    apx_diag_client_destroy(self);
    free(self);
  }
}

apx_error_t apx_diag_client_connect_unix(apx_diag_client_t *self, const char *socket_path)
{
  if (self == NULL || socket_path == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  return apx_client_socket_connection_connect_unix(&self->connection, socket_path);
}

apx_error_t apx_diag_client_connect_tcp(apx_diag_client_t *self, const char *address, uint16_t port)
{
  if (self == NULL || address == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  return apx_client_socket_connection_connect_tcp(&self->connection, address, port);
}

#if defined(MSOCKET_ENABLE_TLS)
apx_error_t apx_diag_client_connect_tls(
  apx_diag_client_t *self, const char *address, uint16_t port, const msocket_tls_config_t *tls_config)
{
  if (self == NULL || address == NULL || tls_config == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  return apx_client_socket_connection_connect_tls(&self->connection, address, port, tls_config);
}
#endif

void apx_diag_client_disconnect(apx_diag_client_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->is_connected = false;
    self->is_greeting_accepted = false;
    self->is_request_file_open = false;
    self->is_response_file_open = false;
    self->is_notify_file_open = false;
    MUTEX_UNLOCK(self->lock);
    apx_connection_base_close(&self->connection.base.base);
  }
}

apx_error_t apx_diag_client_wait_ready(apx_diag_client_t *self, uint32_t timeout_ms)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  struct timespec deadline = compute_deadline(timeout_ms);
  MUTEX_LOCK(self->lock);
  while (!self->is_request_file_open || !self->is_response_file_open)
  {
    int rc = pthread_cond_timedwait(&self->cond, &self->lock, &deadline);
    if (rc == ETIMEDOUT)
    {
      MUTEX_UNLOCK(self->lock);
      return APX_TIMEOUT_ERROR;
    }
  }
  MUTEX_UNLOCK(self->lock);
  return APX_NO_ERROR;
}

apx_error_t apx_diag_client_send_request(apx_diag_client_t *self, uint16_t opcode, uint16_t flags,
  const uint8_t *payload, uint32_t payload_size, uint32_t *out_status, uint8_t **out_payload,
  uint32_t *out_payload_size, uint32_t timeout_ms)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  apx_file_manager_t *fm = apx_client_connection_get_file_manager(&self->connection.base);
  if (fm == NULL)
  {
    return APX_NULL_PTR_ERROR;
  }

  MUTEX_LOCK(self->lock);
  if (!self->is_request_file_open || !self->is_response_file_open)
  {
    MUTEX_UNLOCK(self->lock);
    return APX_FILE_NOT_OPEN_ERROR;
  }

  uint32_t const req_id = ++self->next_request_id;
  self->active_request_id = req_id;
  self->response_received = false;
  if (self->response_payload != NULL)
  {
    free(self->response_payload);
    self->response_payload = NULL;
    self->response_payload_len = 0u;
  }

  uint32_t const total_len = APX_DIAG_HEADER_SIZE + payload_size;
  uint8_t *req_buf = (uint8_t *)malloc(total_len);
  if (req_buf == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    return APX_MEM_ERROR;
  }

  pack_le_u32(req_buf + 0, req_id);
  pack_le_u16(req_buf + 4, opcode);
  pack_le_u16(req_buf + 6, flags);
  if (payload != NULL && payload_size > 0u)
  {
    memcpy(req_buf + APX_DIAG_HEADER_SIZE, payload, payload_size);
  }

  apx_error_t send_err = apx_file_manager_send_local_data(fm, self->request_file_address, req_buf, total_len);
  if (send_err != APX_NO_ERROR)
  {
    MUTEX_UNLOCK(self->lock);
    return send_err;
  }

  struct timespec deadline = compute_deadline(timeout_ms);
  while (!self->response_received)
  {
    int rc = pthread_cond_timedwait(&self->cond, &self->lock, &deadline);
    if (rc == ETIMEDOUT)
    {
      MUTEX_UNLOCK(self->lock);
      return APX_TIMEOUT_ERROR;
    }
  }

  if (out_status != NULL)
  {
    *out_status = self->response_status;
  }
  if (out_payload != NULL && out_payload_size != NULL)
  {
    if (self->response_payload_len > 0u && self->response_payload != NULL)
    {
      *out_payload = (uint8_t *)malloc(self->response_payload_len);
      if (*out_payload != NULL)
      {
        memcpy(*out_payload, self->response_payload, self->response_payload_len);
        *out_payload_size = self->response_payload_len;
      }
      else
      {
        *out_payload_size = 0u;
      }
    }
    else
    {
      *out_payload = NULL;
      *out_payload_size = 0u;
    }
  }

  MUTEX_UNLOCK(self->lock);
  return APX_NO_ERROR;
}

apx_error_t apx_diag_client_get_server_info(
  apx_diag_client_t *self, apx_diag_server_info_t *out_info, uint32_t timeout_ms)
{
  if (self == NULL || out_info == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  uint32_t status = 0u;
  uint8_t *resp = NULL;
  uint32_t resp_len = 0u;

  apx_error_t err = apx_diag_client_send_request(self, 0x0001u, 0u, NULL, 0u, &status, &resp, &resp_len, timeout_ms);
  if (err != APX_NO_ERROR)
  {
    return err;
  }
  if (status != 0u || resp == NULL || resp_len < 32u)
  {
    if (resp != NULL)
      free(resp);
    return (status != 0u) ? (apx_error_t)status : APX_INVALID_MSG_ERROR;
  }

  out_info->protocol_minor = unpack_le_u16(resp + 0);
  out_info->server_start_unix_ns = unpack_le_u64(resp + 4);
  out_info->uptime_ns = unpack_le_u64(resp + 12);
  out_info->connection_count = unpack_le_u32(resp + 20);
  out_info->node_count = unpack_le_u32(resp + 24);
  out_info->route_count = unpack_le_u32(resp + 28);
  uint32_t consumed = 0u;
  out_info->server_version = decode_string16(resp + 32, resp_len - 32u, &consumed);

  free(resp);
  return APX_NO_ERROR;
}

void apx_diag_server_info_destroy(apx_diag_server_info_t *self)
{
  if (self != NULL && self->server_version != NULL)
  {
    free(self->server_version);
    self->server_version = NULL;
  }
}

apx_error_t apx_diag_client_list_connections(apx_diag_client_t *self, adt_ary_t *out_list, uint32_t timeout_ms)
{
  if (self == NULL || out_list == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  adt_ary_create(out_list, apx_diag_client_conn_item_vdelete);

  uint64_t snapshot_id = 0ULL;
  uint32_t cursor = 0u;

  for (;;)
  {
    uint8_t req_payload[16];
    pack_le_u64(req_payload + 0, snapshot_id);
    pack_le_u32(req_payload + 8, cursor);
    pack_le_u16(req_payload + 12, 0u); // default page
    pack_le_u16(req_payload + 14, 0u);

    uint32_t status = 0u;
    uint8_t *resp = NULL;
    uint32_t resp_len = 0u;

    apx_error_t err =
      apx_diag_client_send_request(self, 0x0002u, 0u, req_payload, 16u, &status, &resp, &resp_len, timeout_ms);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    if (status != 0u || resp == NULL || resp_len < 16u)
    {
      if (resp != NULL)
        free(resp);
      return (status != 0u) ? (apx_error_t)status : APX_INVALID_MSG_ERROR;
    }

    snapshot_id = unpack_le_u64(resp + 0);
    uint32_t const next_cursor = unpack_le_u32(resp + 8);
    uint16_t const count = unpack_le_u16(resp + 12);

    uint32_t off = 16u;
    for (uint16_t i = 0; i < count && off + 16u <= resp_len; ++i)
    {
      apx_diag_client_conn_item_t *item = (apx_diag_client_conn_item_t *)malloc(sizeof(apx_diag_client_conn_item_t));
      if (item != NULL)
      {
        item->connection_id = unpack_le_u32(resp + off + 0);
        item->connection_type = resp[off + 4];
        item->connection_state = resp[off + 5];
        item->connected_at_ns = unpack_le_u64(resp + off + 8);
        off += 16u;

        uint32_t consumed = 0u;
        item->tag = decode_string16(resp + off, resp_len - off, &consumed);
        off += consumed;

        item->endpoint = decode_string16(resp + off, resp_len - off, &consumed);
        off += consumed;

        adt_ary_push(out_list, item);
      }
    }
    free(resp);

    if (next_cursor == APX_DIAG_CURSOR_END || count == 0u)
    {
      break;
    }
    cursor = next_cursor;
  }

  return APX_NO_ERROR;
}

apx_error_t apx_diag_client_list_nodes(apx_diag_client_t *self, adt_ary_t *out_list, uint32_t timeout_ms)
{
  if (self == NULL || out_list == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  adt_ary_create(out_list, apx_diag_client_node_item_vdelete);

  uint64_t snapshot_id = 0ULL;
  uint32_t cursor = 0u;

  for (;;)
  {
    uint8_t req_payload[16];
    pack_le_u64(req_payload + 0, snapshot_id);
    pack_le_u32(req_payload + 8, cursor);
    pack_le_u16(req_payload + 12, 0u);
    pack_le_u16(req_payload + 14, 0u);

    uint32_t status = 0u;
    uint8_t *resp = NULL;
    uint32_t resp_len = 0u;

    apx_error_t err =
      apx_diag_client_send_request(self, 0x0003u, 0u, req_payload, 16u, &status, &resp, &resp_len, timeout_ms);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    if (status != 0u || resp == NULL || resp_len < 16u)
    {
      if (resp != NULL)
        free(resp);
      return (status != 0u) ? (apx_error_t)status : APX_INVALID_MSG_ERROR;
    }

    snapshot_id = unpack_le_u64(resp + 0);
    uint32_t const next_cursor = unpack_le_u32(resp + 8);
    uint16_t const count = unpack_le_u16(resp + 12);

    uint32_t off = 16u;
    for (uint16_t i = 0; i < count && off + 16u <= resp_len; ++i)
    {
      apx_diag_client_node_item_t *item = (apx_diag_client_node_item_t *)malloc(sizeof(apx_diag_client_node_item_t));
      if (item != NULL)
      {
        item->node_id = unpack_le_u32(resp + off + 0);
        item->connection_id = unpack_le_u32(resp + off + 4);
        item->provide_port_count = unpack_le_u32(resp + off + 8);
        item->require_port_count = unpack_le_u32(resp + off + 12);
        off += 16u;

        uint32_t consumed = 0u;
        item->node_name = decode_string16(resp + off, resp_len - off, &consumed);
        off += consumed;

        adt_ary_push(out_list, item);
      }
    }
    free(resp);

    if (next_cursor == APX_DIAG_CURSOR_END || count == 0u)
    {
      break;
    }
    cursor = next_cursor;
  }

  return APX_NO_ERROR;
}

apx_error_t apx_diag_client_list_routes(apx_diag_client_t *self, adt_ary_t *out_list, uint32_t timeout_ms)
{
  if (self == NULL || out_list == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  adt_ary_create(out_list, apx_diag_client_route_item_vdelete);

  uint64_t snapshot_id = 0ULL;
  uint32_t cursor = 0u;

  for (;;)
  {
    uint8_t req_payload[16];
    pack_le_u64(req_payload + 0, snapshot_id);
    pack_le_u32(req_payload + 8, cursor);
    pack_le_u16(req_payload + 12, 0u);
    pack_le_u16(req_payload + 14, 0u);

    uint32_t status = 0u;
    uint8_t *resp = NULL;
    uint32_t resp_len = 0u;

    apx_error_t err =
      apx_diag_client_send_request(self, 0x0004u, 0u, req_payload, 16u, &status, &resp, &resp_len, timeout_ms);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    if (status != 0u || resp == NULL || resp_len < 16u)
    {
      if (resp != NULL)
        free(resp);
      return (status != 0u) ? (apx_error_t)status : APX_INVALID_MSG_ERROR;
    }

    snapshot_id = unpack_le_u64(resp + 0);
    uint32_t const next_cursor = unpack_le_u32(resp + 8);
    uint16_t const count = unpack_le_u16(resp + 12);

    uint32_t off = 16u;
    for (uint16_t i = 0; i < count && off + 16u <= resp_len; ++i)
    {
      apx_diag_client_route_item_t *item = (apx_diag_client_route_item_t *)malloc(sizeof(apx_diag_client_route_item_t));
      if (item != NULL)
      {
        item->provide_node_id = unpack_le_u32(resp + off + 0);
        item->provide_port_id = unpack_le_u32(resp + off + 4);
        item->require_node_id = unpack_le_u32(resp + off + 8);
        item->require_port_id = unpack_le_u32(resp + off + 12);
        off += 16u;

        uint32_t consumed = 0u;
        item->port_name = decode_string16(resp + off, resp_len - off, &consumed);
        off += consumed;

        adt_ary_push(out_list, item);
      }
    }
    free(resp);

    if (next_cursor == APX_DIAG_CURSOR_END || count == 0u)
    {
      break;
    }
    cursor = next_cursor;
  }

  return APX_NO_ERROR;
}

apx_error_t apx_diag_client_list_unconnected_ports(apx_diag_client_t *self, adt_ary_t *out_list, uint32_t timeout_ms)
{
  if (self == NULL || out_list == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  adt_ary_create(out_list, apx_diag_client_unconnected_port_item_vdelete);

  uint64_t snapshot_id = 0ULL;
  uint32_t cursor = 0u;

  for (;;)
  {
    uint8_t req_payload[16];
    pack_le_u64(req_payload + 0, snapshot_id);
    pack_le_u32(req_payload + 8, cursor);
    pack_le_u16(req_payload + 12, 0u);
    pack_le_u16(req_payload + 14, 0u);

    uint32_t status = 0u;
    uint8_t *resp = NULL;
    uint32_t resp_len = 0u;

    apx_error_t err =
      apx_diag_client_send_request(self, 0x0005u, 0u, req_payload, 16u, &status, &resp, &resp_len, timeout_ms);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    if (status != 0u || resp == NULL || resp_len < 16u)
    {
      if (resp != NULL)
        free(resp);
      return (status != 0u) ? (apx_error_t)status : APX_INVALID_MSG_ERROR;
    }

    snapshot_id = unpack_le_u64(resp + 0);
    uint32_t const next_cursor = unpack_le_u32(resp + 8);
    uint16_t const count = unpack_le_u16(resp + 12);

    uint32_t off = 16u;
    for (uint16_t i = 0; i < count && off + 12u <= resp_len; ++i)
    {
      apx_diag_client_unconnected_port_item_t *item =
        (apx_diag_client_unconnected_port_item_t *)malloc(sizeof(apx_diag_client_unconnected_port_item_t));
      if (item != NULL)
      {
        item->node_id = unpack_le_u32(resp + off + 0);
        item->port_id = unpack_le_u32(resp + off + 4);
        item->direction = resp[off + 8];
        item->reason = resp[off + 9];
        off += 12u;

        uint32_t consumed = 0u;
        item->port_name = decode_string16(resp + off, resp_len - off, &consumed);
        off += consumed;

        adt_ary_push(out_list, item);
      }
    }
    free(resp);

    if (next_cursor == APX_DIAG_CURSOR_END || count == 0u)
    {
      break;
    }
    cursor = next_cursor;
  }

  return APX_NO_ERROR;
}

apx_error_t apx_diag_client_get_node_definition(
  apx_diag_client_t *self, const char *node_name, char **out_def_text, uint32_t timeout_ms)
{
  if (self == NULL || node_name == NULL || out_def_text == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  *out_def_text = NULL;

  uint32_t const nlen = (uint32_t)strlen(node_name);
  uint32_t const req_payload_size = 2u + nlen;
  uint8_t *req_payload = (uint8_t *)malloc(req_payload_size);
  if (req_payload == NULL)
  {
    return APX_MEM_ERROR;
  }
  pack_le_u16(req_payload, (uint16_t)nlen);
  memcpy(req_payload + 2, node_name, nlen);

  MUTEX_LOCK(self->lock);
  self->def_received = false;
  self->def_file_address = RMF_INVALID_ADDRESS;
  if (self->def_data != NULL)
  {
    free(self->def_data);
    self->def_data = NULL;
    self->def_data_size = 0u;
  }
  MUTEX_UNLOCK(self->lock);

  uint32_t status = 0u;
  uint8_t *resp = NULL;
  uint32_t resp_len = 0u;

  apx_error_t err = apx_diag_client_send_request(
    self, APX_DIAG_CMD_GET_NODE_DEFINITION, 0u, req_payload, req_payload_size, &status, &resp, &resp_len, timeout_ms);
  free(req_payload);

  if (err != APX_NO_ERROR)
  {
    return err;
  }
  if (status != 0u || resp == NULL || resp_len < 4u)
  {
    if (resp != NULL)
      free(resp);
    return (status != 0u) ? (apx_error_t)status : APX_INVALID_MSG_ERROR;
  }

  uint32_t const file_address = unpack_le_u32(resp);
  free(resp);

  MUTEX_LOCK(self->lock);
  self->def_file_address = file_address;
  MUTEX_UNLOCK(self->lock);

  apx_file_manager_t *fm = apx_client_connection_get_file_manager(&self->connection.base);
  if (fm != NULL)
  {
    // Request server to open the file to transfer definition data
    apx_file_manager_send_open_file_request(fm, file_address);
  }

  // Wait for definition data to be received
  struct timespec deadline = compute_deadline(timeout_ms);
  MUTEX_LOCK(self->lock);
  while (!self->def_received)
  {
    int rc = pthread_cond_timedwait(&self->cond, &self->lock, &deadline);
    if (rc == ETIMEDOUT && !self->def_received)
    {
      MUTEX_UNLOCK(self->lock);
      return APX_TIMEOUT_ERROR;
    }
  }

  if (self->def_data != NULL && self->def_data_size > 0u)
  {
    *out_def_text = (char *)malloc(self->def_data_size + 1u);
    if (*out_def_text != NULL)
    {
      memcpy(*out_def_text, self->def_data, self->def_data_size);
      (*out_def_text)[self->def_data_size] = '\0';
    }
  }

  MUTEX_UNLOCK(self->lock);

  // Close remote definition file to signal server to revoke
  if (fm != NULL)
  {
    apx_file_manager_send_close_file_request(fm, file_address);
  }

  return (*out_def_text != NULL) ? APX_NO_ERROR : APX_MEM_ERROR;
}

apx_error_t apx_diag_client_subscribe_ports(apx_diag_client_t *self, uint32_t flags, const char **selectors,
  uint16_t num_selectors, uint32_t *out_subscription_id, adt_ary_t *out_resolutions, uint32_t timeout_ms)
{
  if (self == NULL || selectors == NULL || num_selectors == 0u)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  apx_file_manager_t *fm = apx_client_connection_get_file_manager(&self->connection.base);
  if (fm == NULL)
  {
    return APX_NULL_PTR_ERROR;
  }

  // Ensure notify stream is opened before subscribing per Section 3.2
  MUTEX_LOCK(self->lock);
  if (!self->is_notify_file_open && self->notify_file_address != 0u)
  {
    apx_file_manager_open_stream(fm, self->notify_file_address, APX_DIAG_STREAM_SIZE, 64u);
    self->is_notify_file_open = true;
  }
  MUTEX_UNLOCK(self->lock);

  // Compute payload size: selector_count(2) + reserved(2) + repeated (node_name:String16, port_name:String16)
  uint32_t payload_size = 4u;
  for (uint16_t i = 0; i < num_selectors; ++i)
  {
    const char *sel = selectors[i];
    const char *sep = strchr(sel, ':');
    if (sep == NULL)
    {
      sep = strchr(sel, '/');
    }
    uint16_t nlen = 0u;
    uint16_t plen = 0u;
    if (sep != NULL)
    {
      nlen = (uint16_t)(sep - sel);
      plen = (uint16_t)strlen(sep + 1);
    }
    else
    {
      plen = (uint16_t)strlen(sel);
    }
    payload_size += (2u + nlen) + (2u + plen);
  }

  uint8_t *payload = (uint8_t *)malloc(payload_size);
  if (payload == NULL)
  {
    return APX_MEM_ERROR;
  }

  pack_le_u16(payload + 0, num_selectors);
  pack_le_u16(payload + 2, 0u); // reserved

  uint32_t off = 4u;
  for (uint16_t i = 0; i < num_selectors; ++i)
  {
    const char *sel = selectors[i];
    const char *sep = strchr(sel, ':');
    if (sep == NULL)
    {
      sep = strchr(sel, '/');
    }
    uint16_t nlen = 0u;
    uint16_t plen = 0u;
    const char *pname = sel;
    if (sep != NULL)
    {
      nlen = (uint16_t)(sep - sel);
      pname = sep + 1;
      plen = (uint16_t)strlen(pname);
    }
    else
    {
      plen = (uint16_t)strlen(sel);
    }

    pack_le_u16(payload + off, nlen);
    off += 2u;
    if (nlen > 0u)
    {
      memcpy(payload + off, sel, nlen);
      off += nlen;
    }

    pack_le_u16(payload + off, plen);
    off += 2u;
    if (plen > 0u)
    {
      memcpy(payload + off, pname, plen);
      off += plen;
    }
  }

  uint32_t status = 0u;
  uint8_t *resp = NULL;
  uint32_t resp_len = 0u;

  apx_error_t err = apx_diag_client_send_request(
    self, APX_DIAG_CMD_SUBSCRIBE_PORTS, (uint16_t)flags, payload, payload_size, &status, &resp, &resp_len, timeout_ms);
  free(payload);

  if (err != APX_NO_ERROR)
  {
    return err;
  }
  if (status != 0u || resp == NULL || resp_len < 8u)
  {
    if (resp != NULL)
      free(resp);
    return (status != 0u) ? (apx_error_t)status : APX_INVALID_MSG_ERROR;
  }

  if (out_subscription_id != NULL)
  {
    *out_subscription_id = unpack_le_u32(resp + 0);
  }

  if (out_resolutions != NULL)
  {
    adt_ary_create(out_resolutions, apx_diag_client_resolution_vdelete);
    uint32_t res_off = 8u;
    while (res_off + 20u <= resp_len)
    {
      apx_diag_client_resolution_t *res = (apx_diag_client_resolution_t *)malloc(sizeof(apx_diag_client_resolution_t));
      if (res != NULL)
      {
        res->selector_index = unpack_le_u16(resp + res_off + 0);
        res->status = unpack_le_u32(resp + res_off + 4);
        res->node_id = unpack_le_u32(resp + res_off + 8);
        res->port_id = unpack_le_u32(resp + res_off + 12);
        res->value_size = unpack_le_u32(resp + res_off + 16);
        adt_ary_push(out_resolutions, res);
      }
      res_off += 20u;
    }
  }

  free(resp);
  return APX_NO_ERROR;
}

apx_error_t apx_diag_client_unsubscribe(apx_diag_client_t *self, uint32_t subscription_id, uint32_t timeout_ms)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  uint8_t payload[4];
  pack_le_u32(payload, subscription_id);

  uint32_t status = 0u;
  uint8_t *resp = NULL;
  uint32_t resp_len = 0u;

  apx_error_t err = apx_diag_client_send_request(
    self, APX_DIAG_CMD_UNSUBSCRIBE, 0u, payload, 4u, &status, &resp, &resp_len, timeout_ms);
  if (resp != NULL)
  {
    free(resp);
  }
  if (err != APX_NO_ERROR)
  {
    return err;
  }
  return (status != 0u) ? (apx_error_t)status : APX_NO_ERROR;
}

void apx_diag_client_set_notification_callback(apx_diag_client_t *self, apx_diag_notification_cb_t *cb, void *arg)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->notification_cb = cb;
    self->notification_arg = arg;
    MUTEX_UNLOCK(self->lock);
  }
}

//////////////////////////////////////////////////////////////////////////////
// ADT DESTRUCTORS
//////////////////////////////////////////////////////////////////////////////

void apx_diag_client_conn_item_delete(apx_diag_client_conn_item_t *self)
{
  if (self != NULL)
  {
    if (self->tag != NULL)
      free(self->tag);
    if (self->endpoint != NULL)
      free(self->endpoint);
    free(self);
  }
}

void apx_diag_client_conn_item_vdelete(void *arg)
{
  apx_diag_client_conn_item_delete((apx_diag_client_conn_item_t *)arg);
}

void apx_diag_client_node_item_delete(apx_diag_client_node_item_t *self)
{
  if (self != NULL)
  {
    if (self->node_name != NULL)
      free(self->node_name);
    free(self);
  }
}

void apx_diag_client_node_item_vdelete(void *arg)
{
  apx_diag_client_node_item_delete((apx_diag_client_node_item_t *)arg);
}

void apx_diag_client_route_item_delete(apx_diag_client_route_item_t *self)
{
  if (self != NULL)
  {
    if (self->port_name != NULL)
      free(self->port_name);
    free(self);
  }
}

void apx_diag_client_route_item_vdelete(void *arg)
{
  apx_diag_client_route_item_delete((apx_diag_client_route_item_t *)arg);
}

void apx_diag_client_unconnected_port_item_delete(apx_diag_client_unconnected_port_item_t *self)
{
  if (self != NULL)
  {
    if (self->port_name != NULL)
      free(self->port_name);
    free(self);
  }
}

void apx_diag_client_unconnected_port_item_vdelete(void *arg)
{
  apx_diag_client_unconnected_port_item_delete((apx_diag_client_unconnected_port_item_t *)arg);
}

void apx_diag_client_resolution_delete(apx_diag_client_resolution_t *self)
{
  if (self != NULL)
  {
    free(self);
  }
}

void apx_diag_client_resolution_vdelete(void *arg)
{
  apx_diag_client_resolution_delete((apx_diag_client_resolution_t *)arg);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE NOTIFICATION HANDLERS
//////////////////////////////////////////////////////////////////////////////

static void on_client_greeting_accepted(void *arg, apx_client_connection_t *connection)
{
  (void)connection;
  apx_diag_client_t *self = (apx_diag_client_t *)arg;
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->is_connected = true;
    self->is_greeting_accepted = true;
    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }
}

static void on_client_disconnected(void *arg, apx_client_connection_t *connection)
{
  (void)connection;
  apx_diag_client_t *self = (apx_diag_client_t *)arg;
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->is_connected = false;
    self->is_greeting_accepted = false;
    self->is_request_file_open = false;
    self->is_response_file_open = false;
    self->is_notify_file_open = false;
    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }
}

static apx_error_t on_client_file_published(void *arg, apx_file_t *file)
{
  apx_diag_client_t *self = (apx_diag_client_t *)arg;
  if (self == NULL || file == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  const char *fname = rmf_file_info_name(&file->file_info);
  if (fname == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  apx_file_manager_t *fm = apx_client_connection_get_file_manager(&self->connection.base);
  if (fm == NULL)
  {
    return APX_NULL_PTR_ERROR;
  }

  uint32_t const clean_addr = apx_file_get_address_without_flags(file);

  if (strcmp(fname, APX_DIAG_RESP_FILE_NAME) == 0)
  {
    MUTEX_LOCK(self->lock);
    self->response_file_address = clean_addr;
    self->is_response_file_open = true;

    // Attach write notification handler to receive responses
    apx_file_notification_handler_t resp_handler;
    memset(&resp_handler, 0, sizeof(resp_handler));
    resp_handler.arg = self;
    resp_handler.write_notify = (apx_file_write_notify_func *)on_resp_file_written;
    apx_file_set_notification_handler(file, &resp_handler);

    // Open stream channel
    apx_file_manager_open_stream(fm, clean_addr, APX_DIAG_STREAM_SIZE, 64u);

    // Publish local request stream file
    if (!self->is_request_file_open && self->request_file_address == RMF_INVALID_ADDRESS)
    {
      rmf_file_info_t req_info;
      apx_error_t err = rmf_file_info_create(&req_info, RMF_INVALID_ADDRESS, APX_DIAG_STREAM_SIZE,
        APX_DIAG_REQ_FILE_NAME, RMF_FILE_TYPE_STREAM, RMF_DIGEST_TYPE_NONE, NULL);
      if (err == APX_NO_ERROR)
      {
        apx_file_t *local_req = apx_file_manager_create_local_file(fm, &req_info);
        if (local_req != NULL)
        {
          apx_file_notification_handler_t req_handler;
          memset(&req_handler, 0, sizeof(req_handler));
          req_handler.arg = self;
          req_handler.open_notify = on_req_file_opened;
          apx_file_set_notification_handler(local_req, &req_handler);

          self->request_file_address = apx_file_get_address_without_flags(local_req);
          apx_file_manager_publish_local_file(fm, &local_req->file_info);
        }
        rmf_file_info_destroy(&req_info);
      }
    }

    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }
  else if (strcmp(fname, APX_DIAG_NOTIF_FILE_NAME) == 0)
  {
    MUTEX_LOCK(self->lock);
    self->notify_file_address = clean_addr;
    self->is_notify_file_open = true;

    apx_file_notification_handler_t notif_handler;
    memset(&notif_handler, 0, sizeof(notif_handler));
    notif_handler.arg = self;
    notif_handler.write_notify = (apx_file_write_notify_func *)on_notif_file_written;
    apx_file_set_notification_handler(file, &notif_handler);

    apx_file_manager_open_stream(fm, clean_addr, APX_DIAG_STREAM_SIZE, 64u);

    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }
  else if (strncmp(fname, APX_DIAG_DEF_FILE_PREFIX, strlen(APX_DIAG_DEF_FILE_PREFIX)) == 0)
  {
    MUTEX_LOCK(self->lock);
    apx_file_notification_handler_t def_handler;
    memset(&def_handler, 0, sizeof(def_handler));
    def_handler.arg = self;
    def_handler.write_notify = (apx_file_write_notify_func *)on_def_file_written;
    apx_file_set_notification_handler(file, &def_handler);
    apx_file_open(file);

    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }

  return APX_NO_ERROR;
}

static apx_error_t on_req_file_opened(void *arg, apx_file_t *file)
{
  (void)file;
  apx_diag_client_t *self = (apx_diag_client_t *)arg;
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->is_request_file_open = true;
    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }
  return APX_NO_ERROR;
}

static apx_error_t on_resp_file_written(
  void *arg, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t size)
{
  (void)file;
  (void)offset;
  apx_diag_client_t *self = (apx_diag_client_t *)arg;
  if (self == NULL || data == NULL || size < APX_DIAG_RESP_HEADER_SIZE)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  uint32_t const req_id = unpack_le_u32(data + 0);
  uint32_t const status = unpack_le_u32(data + 8);
  const uint8_t *payload = data + APX_DIAG_RESP_HEADER_SIZE;
  uint32_t const payload_len = size - APX_DIAG_RESP_HEADER_SIZE;

  MUTEX_LOCK(self->lock);
  if (req_id == self->active_request_id)
  {
    self->response_status = status;
    if (self->response_payload != NULL)
    {
      free(self->response_payload);
      self->response_payload = NULL;
    }
    if (payload_len > 0u)
    {
      self->response_payload = (uint8_t *)malloc(payload_len);
      if (self->response_payload != NULL)
      {
        memcpy(self->response_payload, payload, payload_len);
        self->response_payload_len = payload_len;
      }
    }
    self->response_received = true;
    pthread_cond_broadcast(&self->cond);
  }
  MUTEX_UNLOCK(self->lock);

  return APX_NO_ERROR;
}

static apx_error_t on_notif_file_written(
  void *arg, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t size)
{
  (void)file;
  (void)offset;
  apx_diag_client_t *self = (apx_diag_client_t *)arg;
  if (self == NULL || data == NULL || size < APX_DIAG_NOTIF_HEADER_SIZE)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  apx_diag_client_notification_t notif;
  memset(&notif, 0, sizeof(notif));

  notif.subscription_id = unpack_le_u32(data + 0);
  notif.event_type = unpack_le_u16(data + 4);
  notif.flags = unpack_le_u16(data + 6);
  notif.sequence = unpack_le_u64(data + 8);
  notif.timestamp_ns = unpack_le_u64(data + 16);

  const uint8_t *payload = data + APX_DIAG_NOTIF_HEADER_SIZE;
  uint32_t const payload_len = size - APX_DIAG_NOTIF_HEADER_SIZE;

  if (notif.event_type == 0x0001u) // PORT_VALUE
  {
    if (payload_len >= 8u)
    {
      notif.node_id = unpack_le_u32(payload + 0);
      notif.port_id = unpack_le_u32(payload + 4);
      notif.value = payload + 8;
      notif.value_size = payload_len - 8u;
    }
  }
  else if (notif.event_type == 0x0002u) // SUBSCRIPTION_INVALIDATED
  {
    if (payload_len >= 12u)
    {
      notif.node_id = unpack_le_u32(payload + 0);
      notif.port_id = unpack_le_u32(payload + 4);
      notif.reason = unpack_le_u32(payload + 8);
    }
  }
  else if (notif.event_type == 0x00FEu) // NOTIFICATIONS_DROPPED
  {
    if (payload_len >= 16u)
    {
      notif.first_sequence = unpack_le_u64(payload + 0);
      notif.dropped_count = unpack_le_u64(payload + 8);
    }
  }

  MUTEX_LOCK(self->lock);
  apx_diag_notification_cb_t *cb = self->notification_cb;
  void *cb_arg = self->notification_arg;
  MUTEX_UNLOCK(self->lock);

  if (cb != NULL)
  {
    cb(cb_arg, &notif);
  }

  return APX_NO_ERROR;
}

static apx_error_t on_def_file_written(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t size)
{
  (void)file;
  (void)offset;
  apx_diag_client_t *self = (apx_diag_client_t *)arg;
  if (self == NULL || data == NULL || size == 0u)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  MUTEX_LOCK(self->lock);
  if (self->def_data != NULL)
  {
    free(self->def_data);
  }
  self->def_data = (uint8_t *)malloc(size);
  if (self->def_data != NULL)
  {
    memcpy(self->def_data, data, size);
    self->def_data_size = size;
  }
  self->def_received = true;
  pthread_cond_broadcast(&self->cond);
  MUTEX_UNLOCK(self->lock);

  return APX_NO_ERROR;
}
