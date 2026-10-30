/*****************************************************************************
 * \file      apx_diag_client.h
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     APX diagnostic protocol client library
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_DIAG_CLIENT_H
#define APX_DIAG_CLIENT_H

#include "adt_ary.h"
#include "adt_str.h"
#include "apx/error.h"
#include "apx/socket_client_connection.h"
#include "apx_diag_client_notification.h"
#include "osmacro.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(MSOCKET_ENABLE_TLS)
# include "msocket_tls.h"
#endif

//////////////////////////////////////////////////////////////////////////////
#define APX_DIAG_CMD_GET_SERVER_INFO ((uint16_t)0x0001u)
#define APX_DIAG_CMD_LIST_CONNECTIONS ((uint16_t)0x0002u)
#define APX_DIAG_CMD_LIST_NODES ((uint16_t)0x0003u)
#define APX_DIAG_CMD_LIST_ROUTES ((uint16_t)0x0004u)
#define APX_DIAG_CMD_LIST_UNCONNECTED_PORTS ((uint16_t)0x0005u)
#define APX_DIAG_CMD_GET_NODE_DEFINITION ((uint16_t)0x0006u)
#define APX_DIAG_CMD_SUBSCRIBE_PORTS ((uint16_t)0x0010u)
#define APX_DIAG_CMD_UNSUBSCRIBE ((uint16_t)0x0011u)

#define APX_DIAG_NOTIF_PORT_VALUE ((uint16_t)0x0001u)
#define APX_DIAG_NOTIF_SUBSCRIPTION_INVALIDATED ((uint16_t)0x0002u)
#define APX_DIAG_NOTIF_NOTIFICATIONS_DROPPED ((uint16_t)0x00FEu)

#define APX_DIAG_FLAG_SEND_INITIAL ((uint16_t)0x0001u)

// Server info structure
typedef struct apx_diag_server_info_tag
{
  uint16_t protocol_minor;
  uint64_t server_start_unix_ns;
  uint64_t uptime_ns;
  uint32_t connection_count;
  uint32_t node_count;
  uint32_t route_count;
  char *server_version;
} apx_diag_server_info_t;

void apx_diag_server_info_destroy(apx_diag_server_info_t *self);

// Connection item
typedef struct apx_diag_client_conn_item_tag
{
  uint32_t connection_id;
  uint8_t connection_type;
  uint8_t connection_state;
  uint64_t connected_at_ns;
  char *tag;
  char *endpoint;
} apx_diag_client_conn_item_t;

// Node item
typedef struct apx_diag_client_node_item_tag
{
  uint32_t node_id;
  uint32_t connection_id;
  uint32_t provide_port_count;
  uint32_t require_port_count;
  char *node_name;
} apx_diag_client_node_item_t;

// Route item
typedef struct apx_diag_client_route_item_tag
{
  uint32_t provide_node_id;
  uint32_t provide_port_id;
  uint32_t require_node_id;
  uint32_t require_port_id;
  char *port_name;
} apx_diag_client_route_item_t;

// Unconnected port item
typedef struct apx_diag_client_unconnected_port_item_tag
{
  uint32_t node_id;
  uint32_t port_id;
  uint8_t direction; // 0 = require, 1 = provide
  uint8_t reason;    // 0 = no provider, 1 = no consumer
  char *port_name;
} apx_diag_client_unconnected_port_item_t;

// Resolution item for subscriptions
typedef struct apx_diag_client_resolution_tag
{
  uint16_t selector_index;
  uint32_t status;
  uint32_t node_id;
  uint32_t port_id;
  uint32_t value_size;
} apx_diag_client_resolution_t;

// Callback notifications borrow their payload and are valid only until the callback returns.
// Retain a notification with apx_diag_client_notification_clone().
typedef void(apx_diag_notification_cb_t)(void *arg, apx_diag_client_notification_t const *notif);

// Client struct
typedef struct apx_diag_client_tag
{
  apx_client_socket_connection_t connection;
  MUTEX_T lock;
  pthread_cond_t cond;
  bool is_connected;
  bool is_greeting_accepted;
  bool is_response_file_open;
  bool is_notify_file_open;
  bool is_request_file_open;
  uint32_t request_file_address;
  uint32_t response_file_address;
  uint32_t notify_file_address;
  uint32_t next_request_id;
  // Outstanding request synchronization
  uint32_t active_request_id;
  bool response_received;
  uint32_t response_status;
  uint8_t *response_payload;
  uint32_t response_payload_len;
  // Node definition temporary file reception
  uint32_t def_req_id;
  uint32_t def_file_address;
  uint8_t *def_data;
  uint32_t def_data_size;
  bool def_received;
  // Notification handling
  apx_diag_notification_cb_t *notification_cb;
  void *notification_arg;
} apx_diag_client_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

// Lifecycle
apx_error_t apx_diag_client_create(apx_diag_client_t *self);
void apx_diag_client_destroy(apx_diag_client_t *self);
apx_diag_client_t *apx_diag_client_new(void);
void apx_diag_client_delete(apx_diag_client_t *self);

// Connection
apx_error_t apx_diag_client_connect_unix(apx_diag_client_t *self, const char *socket_path);
apx_error_t apx_diag_client_connect_tcp(apx_diag_client_t *self, const char *address, uint16_t port);
#if defined(MSOCKET_ENABLE_TLS)
apx_error_t apx_diag_client_connect_tls(
  apx_diag_client_t *self, const char *address, uint16_t port, const msocket_tls_config_t *tls_config);
#endif
void apx_diag_client_disconnect(apx_diag_client_t *self);
apx_error_t apx_diag_client_wait_ready(apx_diag_client_t *self, uint32_t timeout_ms);

// Low-level request/response
apx_error_t apx_diag_client_send_request(apx_diag_client_t *self, uint16_t opcode, uint16_t flags,
  const uint8_t *payload, uint32_t payload_size, uint32_t *out_status, uint8_t **out_payload,
  uint32_t *out_payload_size, uint32_t timeout_ms);

// High-level query commands
apx_error_t apx_diag_client_get_server_info(
  apx_diag_client_t *self, apx_diag_server_info_t *out_info, uint32_t timeout_ms);
apx_error_t apx_diag_client_list_connections(apx_diag_client_t *self, adt_ary_t *out_list, uint32_t timeout_ms);
apx_error_t apx_diag_client_list_nodes(apx_diag_client_t *self, adt_ary_t *out_list, uint32_t timeout_ms);
apx_error_t apx_diag_client_list_routes(apx_diag_client_t *self, adt_ary_t *out_list, uint32_t timeout_ms);
apx_error_t apx_diag_client_list_unconnected_ports(apx_diag_client_t *self, adt_ary_t *out_list, uint32_t timeout_ms);
apx_error_t apx_diag_client_get_node_definition(
  apx_diag_client_t *self, const char *node_name, char **out_def_text, uint32_t timeout_ms);
apx_error_t apx_diag_client_subscribe_ports(apx_diag_client_t *self, uint32_t flags, const char **selectors,
  uint16_t num_selectors, uint32_t *out_subscription_id, adt_ary_t *out_resolutions, uint32_t timeout_ms);
apx_error_t apx_diag_client_unsubscribe(apx_diag_client_t *self, uint32_t subscription_id, uint32_t timeout_ms);

// Notification callback setter
void apx_diag_client_set_notification_callback(apx_diag_client_t *self, apx_diag_notification_cb_t *cb, void *arg);

// Destructors for adt_ary items
void apx_diag_client_conn_item_delete(apx_diag_client_conn_item_t *self);
void apx_diag_client_conn_item_vdelete(void *arg);
void apx_diag_client_node_item_delete(apx_diag_client_node_item_t *self);
void apx_diag_client_node_item_vdelete(void *arg);
void apx_diag_client_route_item_delete(apx_diag_client_route_item_t *self);
void apx_diag_client_route_item_vdelete(void *arg);
void apx_diag_client_unconnected_port_item_delete(apx_diag_client_unconnected_port_item_t *self);
void apx_diag_client_unconnected_port_item_vdelete(void *arg);
void apx_diag_client_resolution_delete(apx_diag_client_resolution_t *self);
void apx_diag_client_resolution_vdelete(void *arg);

#endif // APX_DIAG_CLIENT_H
