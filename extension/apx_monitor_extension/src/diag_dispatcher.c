/*****************************************************************************
 * \file      diag_dispatcher.c
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Interactive diagnostics query dispatcher (apxmon)
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/diag_dispatcher.h"
#include "apx/connection_base.h"
#include "apx/extension/monitor_session.h"
#include "apx/extension/server_monitor.h"
#include "apx/file.h"
#include "apx/file_manager.h"
#include "apx/node_data.h"
#include "apx/node_instance.h"
#include "apx/node_manager.h"
#include "apx/port_instance.h"
#include "apx/port_signature_map.h"
#include "apx/port_signature_map_entry.h"
#include "apx/server.h"
#include "apx/server_connection.h"
#include "apx/util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_VERSION_STRING "0.3.15a1"

typedef struct apx_diag_conn_item_tag
{
  uint32_t connection_id;
  uint8_t connection_type;
  uint8_t connection_state;
  uint64_t connected_at_ns;
  char *tag;
  char *endpoint;
} apx_diag_conn_item_t;

typedef struct apx_diag_node_item_tag
{
  uint32_t node_id;
  uint32_t connection_id;
  uint32_t provide_port_count;
  uint32_t require_port_count;
  char *node_name;
} apx_diag_node_item_t;

typedef struct apx_diag_route_item_tag
{
  uint32_t provide_node_id;
  uint32_t provide_port_id;
  uint32_t require_node_id;
  uint32_t require_port_id;
  char *port_name;
} apx_diag_route_item_t;

typedef struct apx_diag_unconnected_port_item_tag
{
  uint32_t node_id;
  uint32_t port_id;
  uint8_t direction;
  uint8_t reason;
  char *port_name;
} apx_diag_unconnected_port_item_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void pack_le_u16(uint8_t *dst, uint16_t val);
static void pack_le_u32(uint8_t *dst, uint32_t val);
static void pack_le_u64(uint8_t *dst, uint64_t val);
static uint16_t unpack_le_u16(const uint8_t *src);
static uint32_t unpack_le_u32(const uint8_t *src);
static uint64_t unpack_le_u64(const uint8_t *src);

static uint32_t encode_string16(uint8_t *dst, const char *str);
static bool decode_string16(const uint8_t *src, uint32_t available, char **out_str, uint32_t *bytes_read);

static apx_diag_port_watch_t *apx_diag_port_watch_new(uint32_t node_id, uint32_t port_id, uint8_t port_type,
  uint32_t data_offset, uint32_t value_size, const char *node_name, const char *port_name);
static void apx_diag_port_watch_delete(apx_diag_port_watch_t *self);
static void apx_diag_port_watch_vdelete(void *arg);

static apx_diag_subscription_t *apx_diag_subscription_new(uint32_t subscription_id);
static void apx_diag_subscription_delete(apx_diag_subscription_t *self);
static void apx_diag_subscription_vdelete(void *arg);

static apx_diag_snapshot_t *apx_diag_snapshot_new(uint64_t snapshot_id, uint16_t opcode);
static void apx_diag_snapshot_delete(apx_diag_snapshot_t *self);
static void apx_diag_snapshot_vdelete(void *arg);

static apx_diag_temp_file_t *apx_diag_temp_file_new(
  uint32_t request_id, uint32_t address, const char *file_name, const uint8_t *data, uint32_t size);
static void apx_diag_temp_file_delete(apx_diag_temp_file_t *self);
static void apx_diag_temp_file_vdelete(void *arg);

static apx_error_t send_response(apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t opcode, uint16_t flags,
  uint32_t status, const uint8_t *payload, uint32_t payload_size);
static apx_error_t send_notification(apx_diag_dispatcher_t *self, uint32_t subscription_id, uint16_t event_type,
  uint16_t flags, const uint8_t *payload, uint32_t payload_size);

static void handle_get_server_info(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size);
static void handle_list_connections(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size);
static void handle_list_nodes(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size);
static void handle_list_routes(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size);
static void handle_list_unconnected_ports(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size);
static void handle_get_node_definition(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size);
static void handle_node_files(apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t opcode, uint16_t flags,
  const uint8_t *payload, uint32_t payload_size);
static apx_error_t resolve_node(apx_server_t *server, char const *selector, apx_node_instance_t **out_node);
static apx_error_t publish_temp_file(apx_diag_dispatcher_t *self, uint32_t request_id, char const *name,
  uint8_t const *data, uint32_t size, uint32_t *address);
static void handle_subscribe_ports(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size);
static void handle_unsubscribe(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size);

static apx_node_instance_t *find_node_in_server(apx_server_t *server, const char *node_name);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_diag_dispatcher_create(apx_diag_dispatcher_t *self, apx_monitor_session_t *session)
{
  if (self != NULL)
  {
    self->session = session;
    self->next_snapshot_id = 0ULL;
    self->next_subscription_id = 0u;
    self->notify_sequence = 0ULL;
    adt_ary_create(&self->snapshots, apx_diag_snapshot_vdelete);
    adt_ary_create(&self->subscriptions, apx_diag_subscription_vdelete);
    adt_ary_create(&self->temp_files, apx_diag_temp_file_vdelete);
    MUTEX_INIT(self->lock);
  }
}

void apx_diag_dispatcher_destroy(apx_diag_dispatcher_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    apx_server_monitor_t *monitor =
      self->session != NULL ? apx_monitor_session_get_server_monitor(self->session) : NULL;

    // Decrement port watches for active subscriptions
    if (monitor != NULL)
    {
      int32_t num_subs = adt_ary_length(&self->subscriptions);
      for (int32_t i = 0; i < num_subs; ++i)
      {
        apx_server_monitor_decrement_port_watches(monitor);
      }
    }
    adt_ary_destroy(&self->subscriptions);
    adt_ary_destroy(&self->snapshots);

    adt_ary_destroy(&self->temp_files);
    MUTEX_UNLOCK(self->lock);

    MUTEX_DESTROY(self->lock);
    self->session = NULL;
  }
}

apx_diag_dispatcher_t *apx_diag_dispatcher_new(apx_monitor_session_t *session)
{
  apx_diag_dispatcher_t *self = (apx_diag_dispatcher_t *)malloc(sizeof(apx_diag_dispatcher_t));
  if (self != NULL)
  {
    apx_diag_dispatcher_create(self, session);
  }
  return self;
}

void apx_diag_dispatcher_delete(apx_diag_dispatcher_t *self)
{
  if (self != NULL)
  {
    apx_diag_dispatcher_destroy(self);
    free(self);
  }
}

void apx_diag_dispatcher_vdelete(void *arg) { apx_diag_dispatcher_delete((apx_diag_dispatcher_t *)arg); }

void apx_diag_dispatcher_on_request_written(
  apx_diag_dispatcher_t *self, uint32_t offset, const uint8_t *data, apx_size_t size)
{
  (void)offset;
  if (self == NULL || data == NULL || size < APX_DIAG_HEADER_SIZE)
  {
    return;
  }

  uint32_t const request_id = unpack_le_u32(data + 0);
  uint16_t const opcode = unpack_le_u16(data + 4);
  uint16_t const flags = unpack_le_u16(data + 6);
  const uint8_t *payload = data + APX_DIAG_HEADER_SIZE;
  uint32_t const payload_size = (uint32_t)(size - APX_DIAG_HEADER_SIZE);

  if (request_id == 0u)
  {
    // Section 6: Request ID zero is rejected without guessing correlation ID
    return;
  }

  switch (opcode)
  {
  case APX_DIAG_CMD_GET_SERVER_INFO:
    handle_get_server_info(self, request_id, flags, payload, payload_size);
    break;
  case APX_DIAG_CMD_LIST_CONNECTIONS:
    handle_list_connections(self, request_id, flags, payload, payload_size);
    break;
  case APX_DIAG_CMD_LIST_NODES:
    handle_list_nodes(self, request_id, flags, payload, payload_size);
    break;
  case APX_DIAG_CMD_LIST_ROUTES:
    handle_list_routes(self, request_id, flags, payload, payload_size);
    break;
  case APX_DIAG_CMD_LIST_UNCONNECTED_PORTS:
    handle_list_unconnected_ports(self, request_id, flags, payload, payload_size);
    break;
  case APX_DIAG_CMD_GET_NODE_DEFINITION:
    handle_get_node_definition(self, request_id, flags, payload, payload_size);
    break;
  case APX_DIAG_CMD_SNAPSHOT_NODE_DATA:
    handle_node_files(self, request_id, opcode, flags, payload, payload_size);
    break;
  case APX_DIAG_CMD_SUBSCRIBE_PORTS:
    handle_subscribe_ports(self, request_id, flags, payload, payload_size);
    break;
  case APX_DIAG_CMD_UNSUBSCRIBE:
    handle_unsubscribe(self, request_id, flags, payload, payload_size);
    break;
  default:
    send_response(self, request_id, opcode, 0u, (uint32_t)APX_UNSUPPORTED_ERROR, NULL, 0u);
    break;
  }
}

void apx_diag_dispatcher_on_file_opened(apx_diag_dispatcher_t *self, apx_file_t *file)
{
  if (self == NULL || file == NULL || self->session == NULL)
  {
    return;
  }
  const char *fname = rmf_file_info_name(&file->file_info);
  if (fname == NULL)
  {
    return;
  }

  uint32_t send_addr = 0u;
  uint8_t *copy = NULL;
  uint32_t copy_size = 0u;

  MUTEX_LOCK(self->lock);
  int32_t const num_temp = adt_ary_length(&self->temp_files);
  for (int32_t i = 0; i < num_temp; ++i)
  {
    apx_diag_temp_file_t *tf = (apx_diag_temp_file_t *)adt_ary_value(&self->temp_files, i);
    if (tf != NULL && strcmp(tf->file_name, fname) == 0)
    {
      if (tf->data != NULL && tf->size > 0u)
      {
        copy = (uint8_t *)malloc(tf->size);
        if (copy != NULL)
        {
          memcpy(copy, tf->data, tf->size);
          copy_size = tf->size;
          send_addr = tf->address;
        }
      }
      break;
    }
  }
  MUTEX_UNLOCK(self->lock);

  if (copy != NULL)
  {
    apx_file_manager_t *fm = apx_monitor_session_get_file_manager(self->session);
    if (fm != NULL)
    {
      (void)apx_file_manager_send_local_data(fm, send_addr, copy, copy_size);
    }
    else
    {
      free(copy);
    }
  }
}

void apx_diag_dispatcher_on_file_closed(apx_diag_dispatcher_t *self, apx_file_t *file)
{
  if (self == NULL || file == NULL || self->session == NULL)
  {
    return;
  }
  const char *fname = rmf_file_info_name(&file->file_info);
  if (fname == NULL)
  {
    return;
  }

  uint32_t revoke_addr = 0u;
  bool found = false;

  MUTEX_LOCK(self->lock);
  int32_t const num_temp = adt_ary_length(&self->temp_files);
  for (int32_t i = 0; i < num_temp; ++i)
  {
    apx_diag_temp_file_t *tf = (apx_diag_temp_file_t *)adt_ary_value(&self->temp_files, i);
    if (tf != NULL && strcmp(tf->file_name, fname) == 0)
    {
      revoke_addr = tf->address;
      found = true;
      adt_ary_splice(&self->temp_files, i, 1);
      break;
    }
  }
  MUTEX_UNLOCK(self->lock);

  if (found)
  {
    apx_file_manager_t *fm = apx_monitor_session_get_file_manager(self->session);
    if (fm != NULL)
    {
      apx_file_manager_revoke_local_file(fm, revoke_addr);
    }
  }
}

void apx_diag_dispatcher_notify_port_change(
  apx_diag_dispatcher_t *self, const char *node_name, uint32_t offset, const uint8_t *data, uint32_t size)
{
  if (self == NULL || node_name == NULL || data == NULL || size == 0u)
  {
    return;
  }

  MUTEX_LOCK(self->lock);
  int32_t const num_subs = adt_ary_length(&self->subscriptions);
  for (int32_t s = 0; s < num_subs; ++s)
  {
    apx_diag_subscription_t *sub = (apx_diag_subscription_t *)adt_ary_value(&self->subscriptions, s);
    if (sub == NULL)
    {
      continue;
    }
    int32_t const num_watches = adt_ary_length(&sub->watches);
    for (int32_t w = 0; w < num_watches; ++w)
    {
      apx_diag_port_watch_t *watch = (apx_diag_port_watch_t *)adt_ary_value(&sub->watches, w);
      if (watch != NULL && strcmp(watch->node_name, node_name) == 0)
      {
        // Check if write range overlaps with port data offset
        uint32_t const port_start = watch->data_offset;
        uint32_t const port_end = watch->data_offset + watch->value_size;
        uint32_t const write_start = offset;
        uint32_t const write_end = offset + size;

        if (write_start <= port_start && write_end >= port_end)
        {
          // Write fully contains port value
          uint32_t const diff = port_start - write_start;
          uint32_t const payload_size = 8u + watch->value_size;
          uint8_t *payload = (uint8_t *)malloc(payload_size);
          if (payload != NULL)
          {
            pack_le_u32(payload + 0, watch->node_id);
            pack_le_u32(payload + 4, watch->port_id);
            memcpy(payload + 8, data + diff, watch->value_size);
            send_notification(self, sub->subscription_id, APX_DIAG_NOTIF_PORT_VALUE, 0u, payload, payload_size);
            free(payload);
          }
        }
      }
    }
  }
  MUTEX_UNLOCK(self->lock);
}

void apx_diag_dispatcher_notify_connection_closed(apx_diag_dispatcher_t *self, uint32_t connection_id)
{
  (void)connection_id;
  if (self == NULL)
  {
    return;
  }
  // Optional subscription invalidation can be performed here when nodes detach
}

int32_t apx_diag_dispatcher_num_subscriptions(apx_diag_dispatcher_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    int32_t const count = adt_ary_length(&self->subscriptions);
    MUTEX_UNLOCK(self->lock);
    return count;
  }
  return 0;
}

int32_t apx_diag_dispatcher_num_snapshots(apx_diag_dispatcher_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    int32_t const count = adt_ary_length(&self->snapshots);
    MUTEX_UNLOCK(self->lock);
    return count;
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
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
  return (((uint32_t)src[0]) | (((uint32_t)src[1]) << 8) | (((uint32_t)src[2]) << 16) | (((uint32_t)src[3]) << 24));
}

static uint64_t unpack_le_u64(const uint8_t *src)
{
  uint64_t low = unpack_le_u32(src + 0);
  uint64_t high = unpack_le_u32(src + 4);
  return low | (high << 32);
}

static uint32_t encode_string16(uint8_t *dst, const char *str)
{
  if (str == NULL)
  {
    pack_le_u16(dst, 0u);
    return 2u;
  }
  size_t len = strlen(str);
  if (len > 0xFFFFu)
  {
    len = 0xFFFFu;
  }
  pack_le_u16(dst, (uint16_t)len);
  if (len > 0u)
  {
    memcpy(dst + 2, str, len); // NOLINT(bugprone-not-null-terminated-result)
  }
  return 2u + (uint32_t)len;
}

static bool decode_string16(const uint8_t *src, uint32_t available, char **out_str, uint32_t *bytes_read)
{
  if (available < 2u)
  {
    return false;
  }
  uint16_t const len = unpack_le_u16(src);
  if (2u + (uint32_t)len > available)
  {
    return false;
  }
  char *str = (char *)malloc(len + 1u);
  if (str == NULL)
  {
    return false;
  }
  if (len > 0u)
  {
    memcpy(str, src + 2, len);
  }
  str[len] = '\0';
  *out_str = str;
  if (bytes_read != NULL)
  {
    *bytes_read = 2u + (uint32_t)len;
  }
  return true;
}

static apx_diag_port_watch_t *apx_diag_port_watch_new(uint32_t node_id, uint32_t port_id, uint8_t port_type,
  uint32_t data_offset, uint32_t value_size, const char *node_name, const char *port_name)
{
  apx_diag_port_watch_t *self = (apx_diag_port_watch_t *)malloc(sizeof(apx_diag_port_watch_t));
  if (self != NULL)
  {
    self->node_id = node_id;
    self->port_id = port_id;
    self->port_type = port_type;
    self->data_offset = data_offset;
    self->value_size = value_size;
    self->node_name = node_name != NULL ? strdup(node_name) : NULL;
    self->port_name = port_name != NULL ? strdup(port_name) : NULL;
  }
  return self;
}

static void apx_diag_port_watch_delete(apx_diag_port_watch_t *self)
{
  if (self != NULL)
  {
    if (self->node_name != NULL)
    {
      free(self->node_name);
    }
    if (self->port_name != NULL)
    {
      free(self->port_name);
    }
    free(self);
  }
}

static void apx_diag_port_watch_vdelete(void *arg) { apx_diag_port_watch_delete((apx_diag_port_watch_t *)arg); }

static apx_diag_subscription_t *apx_diag_subscription_new(uint32_t subscription_id)
{
  apx_diag_subscription_t *self = (apx_diag_subscription_t *)malloc(sizeof(apx_diag_subscription_t));
  if (self != NULL)
  {
    self->subscription_id = subscription_id;
    adt_ary_create(&self->watches, apx_diag_port_watch_vdelete);
  }
  return self;
}

static void apx_diag_subscription_delete(apx_diag_subscription_t *self)
{
  if (self != NULL)
  {
    adt_ary_destroy(&self->watches);
    free(self);
  }
}

static void apx_diag_subscription_vdelete(void *arg) { apx_diag_subscription_delete((apx_diag_subscription_t *)arg); }

static void free_item_by_opcode(uint16_t opcode, void *item)
{
  if (item == NULL)
  {
    return;
  }
  switch (opcode)
  {
  case APX_DIAG_CMD_LIST_CONNECTIONS: {
    apx_diag_conn_item_t *ci = (apx_diag_conn_item_t *)item;
    if (ci->tag != NULL)
    {
      free(ci->tag);
    }
    if (ci->endpoint != NULL)
    {
      free(ci->endpoint);
    }
    free(ci);
    break;
  }
  case APX_DIAG_CMD_LIST_NODES: {
    apx_diag_node_item_t *ni = (apx_diag_node_item_t *)item;
    if (ni->node_name != NULL)
    {
      free(ni->node_name);
    }
    free(ni);
    break;
  }
  case APX_DIAG_CMD_LIST_ROUTES: {
    apx_diag_route_item_t *ri = (apx_diag_route_item_t *)item;
    if (ri->port_name != NULL)
    {
      free(ri->port_name);
    }
    free(ri);
    break;
  }
  case APX_DIAG_CMD_LIST_UNCONNECTED_PORTS: {
    apx_diag_unconnected_port_item_t *ui = (apx_diag_unconnected_port_item_t *)item;
    if (ui->port_name != NULL)
    {
      free(ui->port_name);
    }
    free(ui);
    break;
  }
  default:
    free(item);
    break;
  }
}

static apx_diag_snapshot_t *apx_diag_snapshot_new(uint64_t snapshot_id, uint16_t opcode)
{
  apx_diag_snapshot_t *self = (apx_diag_snapshot_t *)malloc(sizeof(apx_diag_snapshot_t));
  if (self != NULL)
  {
    self->snapshot_id = snapshot_id;
    self->opcode = opcode;
    self->created_at_ns = apx_get_monotonic_time_ns();
    adt_ary_create(&self->items, NULL);
  }
  return self;
}

static void apx_diag_snapshot_delete(apx_diag_snapshot_t *self)
{
  if (self != NULL)
  {
    int32_t const len = adt_ary_length(&self->items);
    for (int32_t i = 0; i < len; ++i)
    {
      void *item = adt_ary_value(&self->items, i);
      free_item_by_opcode(self->opcode, item);
    }
    adt_ary_destroy(&self->items);
    free(self);
  }
}

static void apx_diag_snapshot_vdelete(void *arg) { apx_diag_snapshot_delete((apx_diag_snapshot_t *)arg); }

static apx_diag_temp_file_t *apx_diag_temp_file_new(
  uint32_t request_id, uint32_t address, const char *file_name, const uint8_t *data, uint32_t size)
{
  apx_diag_temp_file_t *self = (apx_diag_temp_file_t *)malloc(sizeof(apx_diag_temp_file_t));
  if (self != NULL)
  {
    self->request_id = request_id;
    self->address = address;
    self->file_name = file_name != NULL ? strdup(file_name) : NULL;
    self->size = size;
    self->data = (uint8_t *)malloc(size);
    if (self->file_name == NULL || (size > 0u && self->data == NULL))
    {
      apx_diag_temp_file_delete(self);
      return NULL;
    }
    if (size > 0u)
    {
      memcpy(self->data, data, size);
    }
  }
  return self;
}

static void apx_diag_temp_file_delete(apx_diag_temp_file_t *self)
{
  if (self != NULL)
  {
    if (self->file_name != NULL)
    {
      free(self->file_name);
    }
    if (self->data != NULL)
    {
      free(self->data);
    }
    free(self);
  }
}

static void apx_diag_temp_file_vdelete(void *arg) { apx_diag_temp_file_delete((apx_diag_temp_file_t *)arg); }

static apx_error_t send_response(apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t opcode, uint16_t flags,
  uint32_t status, const uint8_t *payload, uint32_t payload_size)
{
  if (self == NULL || self->session == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!self->session->is_diag_response_open)
  {
    return APX_FILE_NOT_OPEN_ERROR;
  }
  apx_file_manager_t *fm = apx_monitor_session_get_file_manager(self->session);
  if (fm == NULL)
  {
    return APX_NOT_FOUND_ERROR;
  }
  apx_file_t *file = apx_file_manager_find_local_file_by_name(fm, APX_DIAG_RESPONSE_FILE_NAME);
  if (file == NULL)
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  uint32_t const address = apx_file_get_address_without_flags(file);
  uint32_t const total_size = APX_DIAG_RESPONSE_HEADER_SIZE + payload_size;
  uint8_t *msg = (uint8_t *)malloc(total_size);
  if (msg == NULL)
  {
    return APX_MEM_ERROR;
  }

  pack_le_u32(msg + 0, request_id);
  pack_le_u16(msg + 4, opcode);
  pack_le_u16(msg + 6, flags);
  pack_le_u32(msg + 8, status);
  if (payload_size > 0u && payload != NULL)
  {
    memcpy(msg + APX_DIAG_RESPONSE_HEADER_SIZE, payload, payload_size);
  }

  apx_error_t const err = apx_file_manager_send_local_data(fm, address, msg, total_size);
  return err;
}

static apx_error_t send_notification(apx_diag_dispatcher_t *self, uint32_t subscription_id, uint16_t event_type,
  uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  if (self == NULL || self->session == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!self->session->is_diag_notify_open)
  {
    return APX_FILE_NOT_OPEN_ERROR;
  }
  apx_file_manager_t *fm = apx_monitor_session_get_file_manager(self->session);
  if (fm == NULL)
  {
    return APX_NOT_FOUND_ERROR;
  }
  apx_file_t *file = apx_file_manager_find_local_file_by_name(fm, APX_DIAG_NOTIFY_FILE_NAME);
  if (file == NULL)
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
  if (monitor == NULL || monitor->server == NULL)
  {
    return APX_INVALID_STATE_ERROR;
  }
  uint64_t start_unix_ns;
  uint64_t uptime_ns;
  apx_error_t const time_result = apx_server_get_time_info(monitor->server, &start_unix_ns, &uptime_ns);
  if (time_result != APX_NO_ERROR)
  {
    return time_result;
  }
  uint32_t const address = apx_file_get_address_without_flags(file);
  uint32_t const total_size = APX_DIAG_NOTIFY_HEADER_SIZE + payload_size;
  uint8_t *msg = (uint8_t *)malloc(total_size);
  if (msg == NULL)
  {
    return APX_MEM_ERROR;
  }

  uint64_t const seq = self->notify_sequence++;
  pack_le_u32(msg + 0, subscription_id);
  pack_le_u16(msg + 4, event_type);
  pack_le_u16(msg + 6, flags);
  pack_le_u64(msg + 8, seq);
  pack_le_u64(msg + 16, uptime_ns);
  if (payload_size > 0u && payload != NULL)
  {
    memcpy(msg + APX_DIAG_NOTIFY_HEADER_SIZE, payload, payload_size);
  }

  apx_error_t const err = apx_file_manager_send_local_data(fm, address, msg, total_size);
  return err;
}

static void handle_get_server_info(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  (void)payload;
  (void)payload_size;
  if (flags != 0u)
  {
    send_response(self, request_id, APX_DIAG_CMD_GET_SERVER_INFO, 0u, (uint32_t)APX_INVALID_ARGUMENT_ERROR, NULL, 0u);
    return;
  }

  apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
  if (monitor == NULL || monitor->server == NULL)
  {
    send_response(self, request_id, APX_DIAG_CMD_GET_SERVER_INFO, 0u, (uint32_t)APX_INVALID_STATE_ERROR, NULL, 0u);
    return;
  }
  apx_server_t *server = monitor->server;
  uint64_t start_unix_ns;
  uint64_t uptime_ns;
  apx_error_t const time_result = apx_server_get_time_info(server, &start_unix_ns, &uptime_ns);
  if (time_result != APX_NO_ERROR)
  {
    send_response(self, request_id, APX_DIAG_CMD_GET_SERVER_INFO, 0u, (uint32_t)time_result, NULL, 0u);
    return;
  }

  uint32_t conn_count = 0u;
  uint32_t node_count = 0u;
  uint32_t route_count = 0u;

  MUTEX_LOCK(server->global_lock);
  adt_list_t const *connections = &server->connection_manager.active_connections;
  adt_list_elem_t *iter = adt_list_iter_first(connections);
  while (iter != NULL)
  {
    apx_server_connection_t *conn = (apx_server_connection_t *)iter->pItem;
    iter = adt_list_iter_next(iter);
    if (conn != NULL)
    {
      conn_count++;
      apx_node_manager_t *node_mgr = apx_server_connection_get_node_manager(conn);
      if (node_mgr != NULL)
      {
        node_count += (uint32_t)apx_node_manager_length(node_mgr);
      }
    }
  }

  // Count active provider-to-requirer routes from port_signature_map
  adt_hash_t *map = &server->port_signature_map.internal_map;
  adt_hash_iter_init(map);
  const char *key = NULL;
  void **pval = NULL;
  while ((pval = adt_hash_iter_next(map, &key)) != NULL)
  {
    apx_port_signature_map_entry_t *entry = (apx_port_signature_map_entry_t *)*pval;
    if (entry != NULL && entry->preferred_provider != NULL)
    {
      route_count += (uint32_t)adt_list_length(&entry->require_ports);
    }
  }
  MUTEX_UNLOCK(server->global_lock);

  const char *ver_str = APX_VERSION_STRING;
  uint32_t const ver_len = (uint32_t)strlen(ver_str);
  uint32_t const resp_size = 2u + 2u + 8u + 8u + 4u + 4u + 4u + 2u + ver_len;

  uint8_t *resp = (uint8_t *)malloc(resp_size);
  if (resp == NULL)
  {
    send_response(self, request_id, APX_DIAG_CMD_GET_SERVER_INFO, 0u, (uint32_t)APX_MEM_ERROR, NULL, 0u);
    return;
  }

  pack_le_u16(resp + 0, 0u); // protocol_minor
  pack_le_u16(resp + 2, 0u); // reserved
  pack_le_u64(resp + 4, start_unix_ns); // server_start_unix_ns
  pack_le_u64(resp + 12, uptime_ns); // uptime_ns
  pack_le_u32(resp + 20, conn_count);
  pack_le_u32(resp + 24, node_count);
  pack_le_u32(resp + 28, route_count);
  encode_string16(resp + 32, ver_str);

  send_response(self, request_id, APX_DIAG_CMD_GET_SERVER_INFO, 0u, (uint32_t)APX_NO_ERROR, resp, resp_size);
  free(resp);
}

static void handle_list_connections(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  (void)flags;
  if (payload_size < 16u)
  {
    send_response(self, request_id, APX_DIAG_CMD_LIST_CONNECTIONS, 0u, (uint32_t)APX_INVALID_MSG_ERROR, NULL, 0u);
    return;
  }

  uint64_t snapshot_id = unpack_le_u64(payload + 0);
  uint32_t const cursor = unpack_le_u32(payload + 8);
  uint16_t const max_items = unpack_le_u16(payload + 12);

  apx_diag_snapshot_t *snapshot = NULL;
  MUTEX_LOCK(self->lock);
  if (snapshot_id == 0ULL)
  {
    apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
    if (monitor == NULL || monitor->server == NULL)
    {
      MUTEX_UNLOCK(self->lock);
      send_response(self, request_id, APX_DIAG_CMD_LIST_CONNECTIONS, 0u, (uint32_t)APX_INVALID_STATE_ERROR, NULL, 0u);
      return;
    }
    apx_server_t *server = monitor->server;

    snapshot = apx_diag_snapshot_new(++self->next_snapshot_id, APX_DIAG_CMD_LIST_CONNECTIONS);
    if (snapshot != NULL)
    {
      MUTEX_LOCK(server->global_lock);
      adt_list_t const *connections = &server->connection_manager.active_connections;
      adt_list_elem_t *iter = adt_list_iter_first(connections);
      while (iter != NULL)
      {
        apx_server_connection_t *conn = (apx_server_connection_t *)iter->pItem;
        iter = adt_list_iter_next(iter);
        if (conn != NULL)
        {
          apx_diag_conn_item_t *item = (apx_diag_conn_item_t *)malloc(sizeof(apx_diag_conn_item_t));
          if (item != NULL)
          {
            item->connection_id = apx_server_connection_get_connection_id(conn);
            item->connection_type = (uint8_t)apx_server_connection_get_connection_type(conn);
            item->connection_state = (uint8_t)conn->connection_state;
            item->connected_at_ns = 0ULL;
            item->tag = conn->tag != NULL ? strdup(adt_str_cstr(conn->tag)) : strdup("");
            item->endpoint = strdup("");
            adt_ary_push(&snapshot->items, item);
          }
        }
      }
      MUTEX_UNLOCK(server->global_lock);
      adt_ary_push(&self->snapshots, snapshot);
      snapshot_id = snapshot->snapshot_id;
    }
  }
  else
  {
    int32_t const num_snaps = adt_ary_length(&self->snapshots);
    for (int32_t i = 0; i < num_snaps; ++i)
    {
      apx_diag_snapshot_t *s = (apx_diag_snapshot_t *)adt_ary_value(&self->snapshots, i);
      if (s != NULL && s->snapshot_id == snapshot_id && s->opcode == APX_DIAG_CMD_LIST_CONNECTIONS)
      {
        snapshot = s;
        break;
      }
    }
  }

  if (snapshot == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    send_response(self, request_id, APX_DIAG_CMD_LIST_CONNECTIONS, 0u, (uint32_t)APX_NOT_FOUND_ERROR, NULL, 0u);
    return;
  }

  uint32_t const total_items = (uint32_t)adt_ary_length(&snapshot->items);
  uint32_t count = 0u;
  uint32_t next_cursor = APX_DIAG_CURSOR_END;

  if (cursor < total_items)
  {
    uint32_t const remaining = total_items - cursor;
    count = (max_items > 0u && max_items < remaining) ? (uint32_t)max_items : remaining;
    if (cursor + count < total_items)
    {
      next_cursor = cursor + count;
    }
  }

  // Calculate payload size
  uint32_t payload_len = 16u; // snapshot response header
  for (uint32_t i = 0; i < count; ++i)
  {
    apx_diag_conn_item_t const *item =
      (apx_diag_conn_item_t const *)adt_ary_value(&snapshot->items, (int32_t)(cursor + i));
    if (item != NULL)
    {
      payload_len += 4u + 1u + 1u + 2u + 8u + 2u + (uint32_t)strlen(item->tag) + 2u + (uint32_t)strlen(item->endpoint);
    }
  }

  uint8_t *resp = (uint8_t *)malloc(payload_len);
  if (resp == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    send_response(self, request_id, APX_DIAG_CMD_LIST_CONNECTIONS, 0u, (uint32_t)APX_MEM_ERROR, NULL, 0u);
    return;
  }

  pack_le_u64(resp + 0, snapshot_id);
  pack_le_u32(resp + 8, next_cursor);
  pack_le_u16(resp + 12, (uint16_t)count);
  pack_le_u16(resp + 14, 0u);

  uint32_t off = 16u;
  for (uint32_t i = 0; i < count; ++i)
  {
    apx_diag_conn_item_t const *item =
      (apx_diag_conn_item_t const *)adt_ary_value(&snapshot->items, (int32_t)(cursor + i));
    if (item != NULL)
    {
      uint32_t const tag_len = (uint32_t)strlen(item->tag);
      uint32_t const ep_len = (uint32_t)strlen(item->endpoint);
      uint32_t const item_len = 16u + 2u + tag_len + 2u + ep_len;
      if (item_len >= 20u && payload_len >= off && (payload_len - off) >= item_len)
      {
        pack_le_u32(resp + off + 0, item->connection_id);
        resp[off + 4] = item->connection_type;
        resp[off + 5] = item->connection_state;
        pack_le_u16(resp + off + 6, 0u);
        pack_le_u64(resp + off + 8, item->connected_at_ns);
        off += 16u;
        off += encode_string16(resp + off, item->tag);
        off += encode_string16(resp + off, item->endpoint);
      }
    }
  }

  MUTEX_UNLOCK(self->lock);

  send_response(self, request_id, APX_DIAG_CMD_LIST_CONNECTIONS, 0u, (uint32_t)APX_NO_ERROR, resp, payload_len);
  free(resp);
}

static void handle_list_nodes(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  (void)flags;
  if (payload_size < 16u)
  {
    send_response(self, request_id, APX_DIAG_CMD_LIST_NODES, 0u, (uint32_t)APX_INVALID_MSG_ERROR, NULL, 0u);
    return;
  }

  uint64_t snapshot_id = unpack_le_u64(payload + 0);
  uint32_t const cursor = unpack_le_u32(payload + 8);
  uint16_t const max_items = unpack_le_u16(payload + 12);

  apx_diag_snapshot_t *snapshot = NULL;
  MUTEX_LOCK(self->lock);
  if (snapshot_id == 0ULL)
  {
    apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
    if (monitor == NULL || monitor->server == NULL)
    {
      MUTEX_UNLOCK(self->lock);
      send_response(self, request_id, APX_DIAG_CMD_LIST_NODES, 0u, (uint32_t)APX_INVALID_STATE_ERROR, NULL, 0u);
      return;
    }
    apx_server_t *server = monitor->server;

    snapshot = apx_diag_snapshot_new(++self->next_snapshot_id, APX_DIAG_CMD_LIST_NODES);
    if (snapshot != NULL)
    {
      MUTEX_LOCK(server->global_lock);
      adt_list_t const *connections = &server->connection_manager.active_connections;
      adt_list_elem_t *iter = adt_list_iter_first(connections);
      while (iter != NULL)
      {
        apx_server_connection_t *conn = (apx_server_connection_t *)iter->pItem;
        iter = adt_list_iter_next(iter);
        if (conn != NULL && apx_server_connection_get_connection_type(conn) != APX_CONNECTION_TYPE_MONITOR)
        {
          uint32_t conn_id = apx_server_connection_get_connection_id(conn);
          apx_node_manager_t *node_mgr = apx_server_connection_get_node_manager(conn);
          if (node_mgr != NULL)
          {
            adt_ary_t *nodes = apx_node_manager_get_nodes(node_mgr);
            if (nodes != NULL)
            {
              int32_t num_nodes = adt_ary_length(nodes);
              for (int32_t n = 0; n < num_nodes; ++n)
              {
                apx_node_instance_t *node = (apx_node_instance_t *)adt_ary_value(nodes, n);
                if (node != NULL)
                {
                  const char *nname = apx_node_instance_get_name(node);
                  apx_diag_node_item_t *item = (apx_diag_node_item_t *)malloc(sizeof(apx_diag_node_item_t));
                  if (item != NULL)
                  {
                    item->node_id = apx_server_monitor_get_or_create_node_id(monitor, conn_id, nname);
                    item->connection_id = conn_id;
                    item->provide_port_count = (uint32_t)apx_node_instance_get_num_provide_ports(node);
                    item->require_port_count = (uint32_t)apx_node_instance_get_num_require_ports(node);
                    item->node_name = nname != NULL ? strdup(nname) : strdup("");
                    adt_ary_push(&snapshot->items, item);
                  }
                }
              }
              adt_ary_delete(nodes);
            }
          }
        }
      }
      MUTEX_UNLOCK(server->global_lock);
      adt_ary_push(&self->snapshots, snapshot);
      snapshot_id = snapshot->snapshot_id;
    }
  }
  else
  {
    int32_t const num_snaps = adt_ary_length(&self->snapshots);
    for (int32_t i = 0; i < num_snaps; ++i)
    {
      apx_diag_snapshot_t *s = (apx_diag_snapshot_t *)adt_ary_value(&self->snapshots, i);
      if (s != NULL && s->snapshot_id == snapshot_id && s->opcode == APX_DIAG_CMD_LIST_NODES)
      {
        snapshot = s;
        break;
      }
    }
  }

  if (snapshot == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    send_response(self, request_id, APX_DIAG_CMD_LIST_NODES, 0u, (uint32_t)APX_NOT_FOUND_ERROR, NULL, 0u);
    return;
  }

  uint32_t const total_items = (uint32_t)adt_ary_length(&snapshot->items);
  uint32_t count = 0u;
  uint32_t next_cursor = APX_DIAG_CURSOR_END;

  if (cursor < total_items)
  {
    uint32_t const remaining = total_items - cursor;
    count = (max_items > 0u && max_items < remaining) ? (uint32_t)max_items : remaining;
    if (cursor + count < total_items)
    {
      next_cursor = cursor + count;
    }
  }

  uint32_t payload_len = 16u;
  for (uint32_t i = 0; i < count; ++i)
  {
    apx_diag_node_item_t const *item =
      (apx_diag_node_item_t const *)adt_ary_value(&snapshot->items, (int32_t)(cursor + i));
    if (item != NULL)
    {
      payload_len += 4u + 4u + 4u + 4u + 2u + (uint32_t)strlen(item->node_name);
    }
  }

  uint8_t *resp = (uint8_t *)malloc(payload_len);
  if (resp == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    send_response(self, request_id, APX_DIAG_CMD_LIST_NODES, 0u, (uint32_t)APX_MEM_ERROR, NULL, 0u);
    return;
  }

  pack_le_u64(resp + 0, snapshot_id);
  pack_le_u32(resp + 8, next_cursor);
  pack_le_u16(resp + 12, (uint16_t)count);
  pack_le_u16(resp + 14, 0u);

  uint32_t off = 16u;
  for (uint32_t i = 0; i < count; ++i)
  {
    apx_diag_node_item_t const *item =
      (apx_diag_node_item_t const *)adt_ary_value(&snapshot->items, (int32_t)(cursor + i));
    if (item != NULL)
    {
      uint32_t const name_len = (uint32_t)strlen(item->node_name);
      uint32_t const item_len = 16u + 2u + name_len;
      if (item_len >= 18u && payload_len >= off && (payload_len - off) >= item_len)
      {
        pack_le_u32(resp + off + 0, item->node_id);
        pack_le_u32(resp + off + 4, item->connection_id);
        pack_le_u32(resp + off + 8, item->provide_port_count);
        pack_le_u32(resp + off + 12, item->require_port_count);
        off += 16u;
        off += encode_string16(resp + off, item->node_name);
      }
    }
  }

  MUTEX_UNLOCK(self->lock);

  send_response(self, request_id, APX_DIAG_CMD_LIST_NODES, 0u, (uint32_t)APX_NO_ERROR, resp, payload_len);
  free(resp);
}

static void handle_list_routes(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  (void)flags;
  if (payload_size < 16u)
  {
    send_response(self, request_id, APX_DIAG_CMD_LIST_ROUTES, 0u, (uint32_t)APX_INVALID_MSG_ERROR, NULL, 0u);
    return;
  }

  uint64_t snapshot_id = unpack_le_u64(payload + 0);
  uint32_t const cursor = unpack_le_u32(payload + 8);
  uint16_t const max_items = unpack_le_u16(payload + 12);

  apx_diag_snapshot_t *snapshot = NULL;
  MUTEX_LOCK(self->lock);
  if (snapshot_id == 0ULL)
  {
    apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
    if (monitor == NULL || monitor->server == NULL)
    {
      MUTEX_UNLOCK(self->lock);
      send_response(self, request_id, APX_DIAG_CMD_LIST_ROUTES, 0u, (uint32_t)APX_INVALID_STATE_ERROR, NULL, 0u);
      return;
    }
    apx_server_t *server = monitor->server;

    snapshot = apx_diag_snapshot_new(++self->next_snapshot_id, APX_DIAG_CMD_LIST_ROUTES);
    if (snapshot != NULL)
    {
      MUTEX_LOCK(server->global_lock);
      adt_hash_t *map = &server->port_signature_map.internal_map;
      adt_hash_iter_init(map);
      const char *key = NULL;
      void **pval = NULL;
      while ((pval = adt_hash_iter_next(map, &key)) != NULL)
      {
        apx_port_signature_map_entry_t *entry = (apx_port_signature_map_entry_t *)*pval;
        if (entry != NULL && entry->preferred_provider != NULL)
        {
          apx_port_instance_t *provider = entry->preferred_provider;
          apx_node_instance_t *pnode = apx_port_instance_parent(provider);
          const char *pname = pnode != NULL ? apx_node_instance_get_name(pnode) : "";
          uint32_t pnode_id =
            apx_server_monitor_get_or_create_node_id(monitor, apx_node_instance_get_connection_id(pnode), pname);

          adt_list_elem_t *riter = adt_list_iter_first(&entry->require_ports);
          while (riter != NULL)
          {
            apx_port_instance_t *requester = (apx_port_instance_t *)riter->pItem;
            riter = adt_list_iter_next(riter);
            if (requester != NULL)
            {
              apx_node_instance_t *rnode = apx_port_instance_parent(requester);
              const char *rname = rnode != NULL ? apx_node_instance_get_name(rnode) : "";
              uint32_t rnode_id =
                apx_server_monitor_get_or_create_node_id(monitor, apx_node_instance_get_connection_id(rnode), rname);

              apx_diag_route_item_t *item = (apx_diag_route_item_t *)malloc(sizeof(apx_diag_route_item_t));
              if (item != NULL)
              {
                item->provide_node_id = pnode_id;
                item->provide_port_id = provider->port_id;
                item->require_node_id = rnode_id;
                item->require_port_id = requester->port_id;
                item->port_name = provider->name != NULL ? strdup(provider->name) : strdup("");
                adt_ary_push(&snapshot->items, item);
              }
            }
          }
        }
      }
      MUTEX_UNLOCK(server->global_lock);
      adt_ary_push(&self->snapshots, snapshot);
      snapshot_id = snapshot->snapshot_id;
    }
  }
  else
  {
    int32_t const num_snaps = adt_ary_length(&self->snapshots);
    for (int32_t i = 0; i < num_snaps; ++i)
    {
      apx_diag_snapshot_t *s = (apx_diag_snapshot_t *)adt_ary_value(&self->snapshots, i);
      if (s != NULL && s->snapshot_id == snapshot_id && s->opcode == APX_DIAG_CMD_LIST_ROUTES)
      {
        snapshot = s;
        break;
      }
    }
  }

  if (snapshot == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    send_response(self, request_id, APX_DIAG_CMD_LIST_ROUTES, 0u, (uint32_t)APX_NOT_FOUND_ERROR, NULL, 0u);
    return;
  }

  uint32_t const total_items = (uint32_t)adt_ary_length(&snapshot->items);
  uint32_t count = 0u;
  uint32_t next_cursor = APX_DIAG_CURSOR_END;

  if (cursor < total_items)
  {
    uint32_t const remaining = total_items - cursor;
    count = (max_items > 0u && max_items < remaining) ? (uint32_t)max_items : remaining;
    if (cursor + count < total_items)
    {
      next_cursor = cursor + count;
    }
  }

  uint32_t payload_len = 16u;
  for (uint32_t i = 0; i < count; ++i)
  {
    apx_diag_route_item_t const *item =
      (apx_diag_route_item_t const *)adt_ary_value(&snapshot->items, (int32_t)(cursor + i));
    if (item != NULL)
    {
      payload_len += 4u + 4u + 4u + 4u + 2u + (uint32_t)strlen(item->port_name);
    }
  }

  uint8_t *resp = (uint8_t *)malloc(payload_len);
  if (resp == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    send_response(self, request_id, APX_DIAG_CMD_LIST_ROUTES, 0u, (uint32_t)APX_MEM_ERROR, NULL, 0u);
    return;
  }

  pack_le_u64(resp + 0, snapshot_id);
  pack_le_u32(resp + 8, next_cursor);
  pack_le_u16(resp + 12, (uint16_t)count);
  pack_le_u16(resp + 14, 0u);

  uint32_t off = 16u;
  for (uint32_t i = 0; i < count; ++i)
  {
    apx_diag_route_item_t const *item =
      (apx_diag_route_item_t const *)adt_ary_value(&snapshot->items, (int32_t)(cursor + i));
    if (item != NULL)
    {
      uint32_t const name_len = (uint32_t)strlen(item->port_name);
      uint32_t const item_len = 16u + 2u + name_len;
      if (item_len >= 18u && payload_len >= off && (payload_len - off) >= item_len)
      {
        pack_le_u32(resp + off + 0, item->provide_node_id);
        pack_le_u32(resp + off + 4, item->provide_port_id);
        pack_le_u32(resp + off + 8, item->require_node_id);
        pack_le_u32(resp + off + 12, item->require_port_id);
        off += 16u;
        off += encode_string16(resp + off, item->port_name);
      }
    }
  }

  MUTEX_UNLOCK(self->lock);

  send_response(self, request_id, APX_DIAG_CMD_LIST_ROUTES, 0u, (uint32_t)APX_NO_ERROR, resp, payload_len);
  free(resp);
}

static void handle_list_unconnected_ports(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  (void)flags;
  if (payload_size < 16u)
  {
    send_response(self, request_id, APX_DIAG_CMD_LIST_UNCONNECTED_PORTS, 0u, (uint32_t)APX_INVALID_MSG_ERROR, NULL, 0u);
    return;
  }

  uint64_t snapshot_id = unpack_le_u64(payload + 0);
  uint32_t const cursor = unpack_le_u32(payload + 8);
  uint16_t const max_items = unpack_le_u16(payload + 12);

  apx_diag_snapshot_t *snapshot = NULL;
  MUTEX_LOCK(self->lock);
  if (snapshot_id == 0ULL)
  {
    apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
    if (monitor == NULL || monitor->server == NULL)
    {
      MUTEX_UNLOCK(self->lock);
      send_response(
        self, request_id, APX_DIAG_CMD_LIST_UNCONNECTED_PORTS, 0u, (uint32_t)APX_INVALID_STATE_ERROR, NULL, 0u);
      return;
    }
    apx_server_t *server = monitor->server;

    snapshot = apx_diag_snapshot_new(++self->next_snapshot_id, APX_DIAG_CMD_LIST_UNCONNECTED_PORTS);
    if (snapshot != NULL)
    {
      MUTEX_LOCK(server->global_lock);
      adt_hash_t *map = &server->port_signature_map.internal_map;
      adt_hash_iter_init(map);
      const char *key = NULL;
      void **pval = NULL;
      while ((pval = adt_hash_iter_next(map, &key)) != NULL)
      {
        apx_port_signature_map_entry_t *entry = (apx_port_signature_map_entry_t *)*pval;
        if (entry != NULL)
        {
          int32_t num_prov = apx_port_signature_map_entry_get_num_providers(entry);
          int32_t num_req = apx_port_signature_map_entry_get_num_requesters(entry);

          // 1. Require ports without provider (direction = 0, reason = 0)
          if (num_prov == 0 && num_req > 0)
          {
            adt_list_elem_t *riter = adt_list_iter_first(&entry->require_ports);
            while (riter != NULL)
            {
              apx_port_instance_t *req = (apx_port_instance_t *)riter->pItem;
              riter = adt_list_iter_next(riter);
              if (req != NULL)
              {
                apx_node_instance_t *rnode = apx_port_instance_parent(req);
                const char *rname = rnode != NULL ? apx_node_instance_get_name(rnode) : "";
                uint32_t rnode_id =
                  apx_server_monitor_get_or_create_node_id(monitor, apx_node_instance_get_connection_id(rnode), rname);

                apx_diag_unconnected_port_item_t *item =
                  (apx_diag_unconnected_port_item_t *)malloc(sizeof(apx_diag_unconnected_port_item_t));
                if (item != NULL)
                {
                  item->node_id = rnode_id;
                  item->port_id = req->port_id;
                  item->direction = 0u; // require
                  item->reason = 0u;    // no provider
                  item->port_name = req->name != NULL ? strdup(req->name) : strdup("");
                  adt_ary_push(&snapshot->items, item);
                }
              }
            }
          }
          // 2. Provide ports without consumers (direction = 1, reason = 1)
          else if (num_prov > 0 && num_req == 0)
          {
            adt_list_elem_t *piter = adt_list_iter_first(&entry->provide_ports);
            while (piter != NULL)
            {
              apx_port_instance_t *prov = (apx_port_instance_t *)piter->pItem;
              piter = adt_list_iter_next(piter);
              if (prov != NULL)
              {
                apx_node_instance_t *pnode = apx_port_instance_parent(prov);
                const char *pname = pnode != NULL ? apx_node_instance_get_name(pnode) : "";
                uint32_t pnode_id =
                  apx_server_monitor_get_or_create_node_id(monitor, apx_node_instance_get_connection_id(pnode), pname);

                apx_diag_unconnected_port_item_t *item =
                  (apx_diag_unconnected_port_item_t *)malloc(sizeof(apx_diag_unconnected_port_item_t));
                if (item != NULL)
                {
                  item->node_id = pnode_id;
                  item->port_id = prov->port_id;
                  item->direction = 1u; // provide
                  item->reason = 1u;    // no consumer
                  item->port_name = prov->name != NULL ? strdup(prov->name) : strdup("");
                  adt_ary_push(&snapshot->items, item);
                }
              }
            }
          }
        }
      }
      MUTEX_UNLOCK(server->global_lock);
      adt_ary_push(&self->snapshots, snapshot);
      snapshot_id = snapshot->snapshot_id;
    }
  }
  else
  {
    int32_t const num_snaps = adt_ary_length(&self->snapshots);
    for (int32_t i = 0; i < num_snaps; ++i)
    {
      apx_diag_snapshot_t *s = (apx_diag_snapshot_t *)adt_ary_value(&self->snapshots, i);
      if (s != NULL && s->snapshot_id == snapshot_id && s->opcode == APX_DIAG_CMD_LIST_UNCONNECTED_PORTS)
      {
        snapshot = s;
        break;
      }
    }
  }

  if (snapshot == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    send_response(self, request_id, APX_DIAG_CMD_LIST_UNCONNECTED_PORTS, 0u, (uint32_t)APX_NOT_FOUND_ERROR, NULL, 0u);
    return;
  }

  uint32_t const total_items = (uint32_t)adt_ary_length(&snapshot->items);
  uint32_t count = 0u;
  uint32_t next_cursor = APX_DIAG_CURSOR_END;

  if (cursor < total_items)
  {
    uint32_t const remaining = total_items - cursor;
    count = (max_items > 0u && max_items < remaining) ? (uint32_t)max_items : remaining;
    if (cursor + count < total_items)
    {
      next_cursor = cursor + count;
    }
  }

  uint32_t payload_len = 16u;
  for (uint32_t i = 0; i < count; ++i)
  {
    apx_diag_unconnected_port_item_t const *item =
      (apx_diag_unconnected_port_item_t const *)adt_ary_value(&snapshot->items, (int32_t)(cursor + i));
    if (item != NULL)
    {
      payload_len += 4u + 4u + 1u + 1u + 2u + 2u + (uint32_t)strlen(item->port_name);
    }
  }

  uint8_t *resp = (uint8_t *)malloc(payload_len);
  if (resp == NULL)
  {
    MUTEX_UNLOCK(self->lock);
    send_response(self, request_id, APX_DIAG_CMD_LIST_UNCONNECTED_PORTS, 0u, (uint32_t)APX_MEM_ERROR, NULL, 0u);
    return;
  }

  pack_le_u64(resp + 0, snapshot_id);
  pack_le_u32(resp + 8, next_cursor);
  pack_le_u16(resp + 12, (uint16_t)count);
  pack_le_u16(resp + 14, 0u);

  uint32_t off = 16u;
  for (uint32_t i = 0; i < count; ++i)
  {
    apx_diag_unconnected_port_item_t const *item =
      (apx_diag_unconnected_port_item_t const *)adt_ary_value(&snapshot->items, (int32_t)(cursor + i));
    if (item != NULL)
    {
      uint32_t const name_len = (uint32_t)strlen(item->port_name);
      uint32_t const item_len = 12u + 2u + name_len;
      if (item_len >= 14u && payload_len >= off && (payload_len - off) >= item_len)
      {
        pack_le_u32(resp + off + 0, item->node_id);
        pack_le_u32(resp + off + 4, item->port_id);
        resp[off + 8] = item->direction;
        resp[off + 9] = item->reason;
        pack_le_u16(resp + off + 10, 0u);
        off += 12u;
        off += encode_string16(resp + off, item->port_name);
      }
    }
  }

  MUTEX_UNLOCK(self->lock);

  send_response(self, request_id, APX_DIAG_CMD_LIST_UNCONNECTED_PORTS, 0u, (uint32_t)APX_NO_ERROR, resp, payload_len);
  free(resp);
}

static void handle_get_node_definition(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  handle_node_files(self, request_id, APX_DIAG_CMD_GET_NODE_DEFINITION, flags, payload, payload_size);
}

static apx_error_t publish_temp_file(apx_diag_dispatcher_t *self, uint32_t request_id, char const *name,
  uint8_t const *data, uint32_t size, uint32_t *address)
{
  apx_file_manager_t *fm = apx_monitor_session_get_file_manager(self->session);
  if (fm == NULL)
  {
    return APX_INVALID_STATE_ERROR;
  }
  apx_diag_temp_file_t *tf = apx_diag_temp_file_new(request_id, RMF_INVALID_ADDRESS, name, data, size);
  if (tf == NULL)
  {
    return APX_MEM_ERROR;
  }
  rmf_file_info_t file_info;
  apx_error_t result =
    rmf_file_info_create(&file_info, RMF_INVALID_ADDRESS, size, name, RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
  if (result != APX_NO_ERROR)
  {
    rmf_file_info_destroy(&file_info);
    apx_diag_temp_file_delete(tf);
    return result;
  }
  apx_file_t *local_file = apx_file_manager_create_local_file(fm, &file_info);
  rmf_file_info_destroy(&file_info);
  if (local_file == NULL)
  {
    apx_diag_temp_file_delete(tf);
    return APX_MEM_ERROR;
  }
  tf->address = apx_file_get_address_without_flags(local_file);
  uint32_t const file_address = tf->address;
  MUTEX_LOCK(self->lock);
  if (adt_ary_push(&self->temp_files, tf) != ADT_NO_ERROR)
  {
    result = APX_MEM_ERROR;
  }
  MUTEX_UNLOCK(self->lock);
  if (result != APX_NO_ERROR)
  {
    apx_diag_temp_file_delete(tf);
  }
  else
  {
    apx_monitor_session_attach_file_handler(self->session, local_file);
    result = apx_file_manager_publish_local_file(fm, &local_file->file_info);
  }
  if (result != APX_NO_ERROR)
  {
    apx_file_manager_revoke_local_file(fm, file_address);
    return result;
  }
  *address = file_address;
  return APX_NO_ERROR;
}

static void handle_node_files(apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t opcode, uint16_t flags,
  const uint8_t *payload, uint32_t payload_size)
{
  char *selector = NULL;
  uint32_t bytes_read = 0u;
  if (flags != 0u)
  {
    send_response(self, request_id, opcode, 0u, (uint32_t)APX_INVALID_ARGUMENT_ERROR, NULL, 0u);
    return;
  }
  if (!decode_string16(payload, payload_size, &selector, &bytes_read))
  {
    send_response(self, request_id, opcode, 0u, (uint32_t)APX_INVALID_MSG_ERROR, NULL, 0u);
    return;
  }
  if (bytes_read != payload_size || strlen(selector) != bytes_read - 2u)
  {
    free(selector);
    send_response(self, request_id, opcode, 0u, (uint32_t)APX_INVALID_MSG_ERROR, NULL, 0u);
    return;
  }
  apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
  apx_error_t result = APX_INVALID_STATE_ERROR;
  uint8_t *out_data = NULL;
  uint8_t *in_data = NULL;
  uint32_t out_size = 0u;
  uint32_t in_size = 0u;
  if (monitor != NULL && monitor->server != NULL)
  {
    apx_server_t *server = monitor->server;
    MUTEX_LOCK(server->global_lock);
    apx_node_instance_t *node = NULL;
    result = resolve_node(server, selector, &node);
    if (result == APX_NO_ERROR)
    {
      if (opcode == APX_DIAG_CMD_GET_NODE_DEFINITION)
      {
        uint8_t const *definition = apx_node_instance_get_definition_data(node);
        out_size = apx_node_instance_get_definition_size(node);
        if (definition == NULL || out_size == 0u)
        {
          result = APX_NOT_FOUND_ERROR;
        }
        else
        {
          out_data = apx_node_data_take_definition_data_snapshot(apx_node_instance_get_node_data(node));
          if (out_data == NULL)
          {
            result = APX_MEM_ERROR;
          }
        }
      }
      else
      {
        apx_node_data_t *nd = apx_node_instance_get_node_data(node);
        if (nd == NULL)
        {
          result = APX_INVALID_STATE_ERROR;
        }
        else
        {
          out_size = apx_node_data_provide_port_data_size(nd);
          in_size = apx_node_data_require_port_data_size(nd);
          result = apx_node_data_take_port_data_snapshot(nd, &out_data, &in_data);
        }
      }
    }
    MUTEX_UNLOCK(server->global_lock);
  }
  free(selector);
  uint32_t addresses[2] = {RMF_INVALID_ADDRESS, RMF_INVALID_ADDRESS};
  char name[64];
  if (result == APX_NO_ERROR && out_size > 0u)
  {
    if (opcode == APX_DIAG_CMD_GET_NODE_DEFINITION)
    {
      snprintf(name, sizeof(name), "apx.diag.v1.def.%u", request_id);
    }
    else
    {
      snprintf(name, sizeof(name), "apx.diag.v1.data.%u.p", request_id);
    }
    result = publish_temp_file(self, request_id, name, out_data, out_size, &addresses[0]);
  }
  if (result == APX_NO_ERROR && in_size > 0u)
  {
    snprintf(name, sizeof(name), "apx.diag.v1.data.%u.r", request_id);
    result = publish_temp_file(self, request_id, name, in_data, in_size, &addresses[1]);
  }
  free(out_data);
  free(in_data);
  if (result != APX_NO_ERROR)
  {
    apx_file_manager_t *fm = apx_monitor_session_get_file_manager(self->session);
    for (size_t i = 0; i < 2u; ++i)
    {
      if (fm != NULL && addresses[i] != RMF_INVALID_ADDRESS)
      {
        apx_file_manager_revoke_local_file(fm, addresses[i]);
      }
    }
    send_response(self, request_id, opcode, 0u, (uint32_t)result, NULL, 0u);
    return;
  }
  uint8_t response[8];
  pack_le_u32(response, addresses[0]);
  pack_le_u32(response + 4, addresses[1]);
  send_response(self, request_id, opcode, 0u, 0u, response, opcode == APX_DIAG_CMD_GET_NODE_DEFINITION ? 4u : 8u);
}

static void handle_subscribe_ports(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  if (payload_size < 4u)
  {
    send_response(self, request_id, APX_DIAG_CMD_SUBSCRIBE_PORTS, 0u, (uint32_t)APX_INVALID_MSG_ERROR, NULL, 0u);
    return;
  }

  uint16_t const selector_count = unpack_le_u16(payload + 0);
  if (selector_count == 0u || selector_count > APX_DIAG_MAX_SELECTORS)
  {
    send_response(self, request_id, APX_DIAG_CMD_SUBSCRIBE_PORTS, 0u, (uint32_t)APX_INVALID_ARGUMENT_ERROR, NULL, 0u);
    return;
  }

  apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
  if (monitor == NULL || monitor->server == NULL)
  {
    send_response(self, request_id, APX_DIAG_CMD_SUBSCRIBE_PORTS, 0u, (uint32_t)APX_INVALID_STATE_ERROR, NULL, 0u);
    return;
  }
  apx_server_t *server = monitor->server;

  MUTEX_LOCK(self->lock);
  uint32_t const subscription_id = ++self->next_subscription_id;
  MUTEX_UNLOCK(self->lock);

  apx_diag_subscription_t *sub = apx_diag_subscription_new(subscription_id);
  if (sub == NULL)
  {
    send_response(self, request_id, APX_DIAG_CMD_SUBSCRIBE_PORTS, 0u, (uint32_t)APX_MEM_ERROR, NULL, 0u);
    return;
  }

  uint32_t const resp_len = 8u + ((uint32_t)selector_count * 20u);
  uint8_t *resp = (uint8_t *)malloc(resp_len);
  if (resp == NULL)
  {
    apx_diag_subscription_delete(sub);
    send_response(self, request_id, APX_DIAG_CMD_SUBSCRIBE_PORTS, 0u, (uint32_t)APX_MEM_ERROR, NULL, 0u);
    return;
  }

  pack_le_u32(resp + 0, subscription_id);
  pack_le_u16(resp + 4, selector_count);
  pack_le_u16(resp + 6, 0u);

  uint32_t off = 4u;
  uint32_t resp_off = 8u;
  uint32_t resolved_count = 0u;

  MUTEX_LOCK(server->global_lock);
  for (uint16_t i = 0u; i < selector_count; ++i)
  {
    char *node_name = NULL;
    char *port_name = NULL;
    uint32_t bytes_n = 0u;
    uint32_t bytes_p = 0u;

    if (!decode_string16(payload + off, payload_size - off, &node_name, &bytes_n))
    {
      break;
    }
    off += bytes_n;

    if (!decode_string16(payload + off, payload_size - off, &port_name, &bytes_p))
    {
      free(node_name);
      break;
    }
    off += bytes_p;

    uint32_t status = (uint32_t)APX_NOT_FOUND_ERROR;
    uint32_t node_id = 0u;
    uint32_t port_id = 0u;
    uint32_t val_size = 0u;

    apx_node_instance_t *node = find_node_in_server(server, node_name);
    if (node != NULL)
    {
      apx_port_instance_t *port = apx_node_instance_find_port_by_name(node, port_name);
      if (port != NULL)
      {
        node_id =
          apx_server_monitor_get_or_create_node_id(monitor, apx_node_instance_get_connection_id(node), node_name);
        port_id = port->port_id;
        val_size = port->data_size;
        status = (uint32_t)APX_NO_ERROR;
        resolved_count++;

        apx_diag_port_watch_t *watch = apx_diag_port_watch_new(
          node_id, port_id, (uint8_t)port->port_type, port->data_offset, port->data_size, node_name, port_name);
        if (watch != NULL)
        {
          adt_ary_push(&sub->watches, watch);
        }
      }
    }

    pack_le_u16(resp + resp_off + 0, i);
    pack_le_u16(resp + resp_off + 2, 0u);
    pack_le_u32(resp + resp_off + 4, status);
    pack_le_u32(resp + resp_off + 8, node_id);
    pack_le_u32(resp + resp_off + 12, port_id);
    pack_le_u32(resp + resp_off + 16, val_size);
    resp_off += 20u;

    free(node_name);
    free(port_name);
  }
  MUTEX_UNLOCK(server->global_lock);

  if (resolved_count > 0u)
  {
    MUTEX_LOCK(self->lock);
    adt_ary_push(&self->subscriptions, sub);
    MUTEX_UNLOCK(self->lock);
    apx_server_monitor_increment_port_watches(monitor);

    // Initial values if SEND_INITIAL flag set
    if (flags & APX_DIAG_FLAG_SEND_INITIAL)
    {
      int32_t const num_watches = adt_ary_length(&sub->watches);
      for (int32_t w = 0; w < num_watches; ++w)
      {
        apx_diag_port_watch_t const *watch = (apx_diag_port_watch_t const *)adt_ary_value(&sub->watches, w);
        if (watch != NULL && watch->value_size > 0u)
        {
          MUTEX_LOCK(server->global_lock);
          apx_node_instance_t *node = find_node_in_server(server, watch->node_name);
          if (node != NULL)
          {
            apx_node_data_t *nd = apx_node_instance_get_node_data(node);
            if (nd != NULL)
            {
              uint32_t const psize = 8u + watch->value_size;
              uint8_t *pbuf = (uint8_t *)malloc(psize);
              if (pbuf != NULL)
              {
                pack_le_u32(pbuf + 0, watch->node_id);
                pack_le_u32(pbuf + 4, watch->port_id);
                apx_error_t r_err;
                if (watch->port_type == APX_REQUIRE_PORT)
                {
                  r_err = apx_node_data_read_require_port_data(nd, watch->data_offset, pbuf + 8, watch->value_size);
                }
                else
                {
                  r_err = apx_node_data_read_provide_port_data(nd, watch->data_offset, pbuf + 8, watch->value_size);
                }
                if (r_err == APX_NO_ERROR)
                {
                  send_notification(
                    self, sub->subscription_id, APX_DIAG_NOTIF_PORT_VALUE, APX_DIAG_FLAG_INITIAL_VALUE, pbuf, psize);
                }
                free(pbuf);
              }
            }
          }
          MUTEX_UNLOCK(server->global_lock);
        }
      }
    }

    send_response(self, request_id, APX_DIAG_CMD_SUBSCRIBE_PORTS, 0u, (uint32_t)APX_NO_ERROR, resp, resp_len);
  }
  else
  {
    apx_diag_subscription_delete(sub);
    send_response(self, request_id, APX_DIAG_CMD_SUBSCRIBE_PORTS, 0u, (uint32_t)APX_NOT_FOUND_ERROR, NULL, 0u);
  }

  free(resp);
}

static void handle_unsubscribe(
  apx_diag_dispatcher_t *self, uint32_t request_id, uint16_t flags, const uint8_t *payload, uint32_t payload_size)
{
  (void)flags;
  if (payload_size < 4u)
  {
    send_response(self, request_id, APX_DIAG_CMD_UNSUBSCRIBE, 0u, (uint32_t)APX_INVALID_MSG_ERROR, NULL, 0u);
    return;
  }

  uint32_t const subscription_id = unpack_le_u32(payload + 0);
  apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);

  MUTEX_LOCK(self->lock);
  int32_t const num_subs = adt_ary_length(&self->subscriptions);
  bool found = false;
  for (int32_t i = 0; i < num_subs; ++i)
  {
    apx_diag_subscription_t *sub = (apx_diag_subscription_t *)adt_ary_value(&self->subscriptions, i);
    if (sub != NULL && sub->subscription_id == subscription_id)
    {
      if (monitor != NULL)
      {
        apx_server_monitor_decrement_port_watches(monitor);
      }
      adt_ary_splice(&self->subscriptions, i, 1);
      found = true;
      break;
    }
  }
  MUTEX_UNLOCK(self->lock);

  if (found)
  {
    send_response(self, request_id, APX_DIAG_CMD_UNSUBSCRIBE, 0u, (uint32_t)APX_NO_ERROR, NULL, 0u);
  }
  else
  {
    send_response(self, request_id, APX_DIAG_CMD_UNSUBSCRIBE, 0u, (uint32_t)APX_NOT_FOUND_ERROR, NULL, 0u);
  }
}

static apx_error_t resolve_node(apx_server_t *server, char const *selector, apx_node_instance_t **out_node)
{
  *out_node = NULL;
  char const *node_name = selector;
  char const *separator = strchr(selector, ':');
  uint32_t connection_id = 0u;
  if (separator != NULL)
  {
    if (separator == selector || separator[1] == '\0')
    {
      return APX_INVALID_NAME_ERROR;
    }
    for (char const *p = selector; p < separator; ++p)
    {
      if (*p < '0' || *p > '9' || connection_id > (UINT32_MAX - (uint32_t)(*p - '0')) / 10u)
      {
        return APX_INVALID_NAME_ERROR;
      }
      connection_id = connection_id * 10u + (uint32_t)(*p - '0');
    }
    node_name = separator + 1;
  }
  if (*node_name == '\0' || strchr(node_name, ':') != NULL)
  {
    return APX_INVALID_NAME_ERROR;
  }
  adt_list_elem_t *iter = adt_list_iter_first(&server->connection_manager.active_connections);
  while (iter != NULL)
  {
    apx_server_connection_t *connection = (apx_server_connection_t *)iter->pItem;
    iter = adt_list_iter_next(iter);
    if (connection != NULL && apx_server_connection_get_connection_type(connection) != APX_CONNECTION_TYPE_MONITOR &&
      (separator == NULL || apx_server_connection_get_connection_id(connection) == connection_id))
    {
      apx_node_manager_t *manager = apx_server_connection_get_node_manager(connection);
      apx_node_instance_t *node = manager != NULL ? apx_node_manager_find(manager, node_name) : NULL;
      if (node != NULL)
      {
        if (*out_node != NULL)
        {
          *out_node = NULL;
          return APX_AMBIGUOUS_NODE_ERROR;
        }
        *out_node = node;
      }
    }
  }
  return *out_node != NULL ? APX_NO_ERROR : APX_NOT_FOUND_ERROR;
}

static apx_node_instance_t *find_node_in_server(apx_server_t *server, const char *node_name)
{
  if (server == NULL || node_name == NULL)
  {
    return NULL;
  }
  adt_list_t const *connections = &server->connection_manager.active_connections;
  adt_list_elem_t *iter = adt_list_iter_first(connections);
  while (iter != NULL)
  {
    apx_server_connection_t *conn = (apx_server_connection_t *)iter->pItem;
    iter = adt_list_iter_next(iter);
    if (conn != NULL)
    {
      apx_node_manager_t *node_mgr = apx_server_connection_get_node_manager(conn);
      if (node_mgr != NULL)
      {
        apx_node_instance_t *node = apx_node_manager_find(node_mgr, node_name);
        if (node != NULL)
        {
          return node;
        }
      }
    }
  }
  return NULL;
}
