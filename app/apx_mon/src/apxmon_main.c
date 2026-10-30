/*****************************************************************************
 * \file      apxmon_main.c
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     apxmon CLI diagnostic application
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "adt_ary.h"
#include "adt_str.h"
#include "apx_diag_client.h"
#include "apx_version.h"
#include "argparse.h"
#include "dtl_json.h"
#include "dtl_type.h"
#include "msocket.h"
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define APP_NAME "apxmon"

#ifdef _WIN32
static const char *m_connect_address_default = "127.0.0.1:5000";
#else
static const char *m_connect_address_default = "/tmp/apx.socket";
#endif

static bool m_json_output = false;
static bool m_use_tls = false;
static adt_str_t *m_connect_address = NULL;
static uint16_t m_connect_port = 5000;
static msocket_endpoint_type_t m_connect_resource_type = MSOCKET_ENDPOINT_UNKNOWN;
static adt_str_t *m_ca_cert = NULL;
static adt_str_t *m_client_cert = NULL;
static adt_str_t *m_client_key = NULL;
static volatile sig_atomic_t m_running = 1;
static apx_diag_client_t *m_client = NULL;
static uint32_t m_active_subscription_id = 0u;

static void sigint_handler(int sig)
{
  (void)sig;
  m_running = 0;
}

static void print_usage(const char *prog_name)
{
  printf("Usage: %s [OPTIONS] COMMAND [ARGS...]\n\n", prog_name);
  printf("Options:\n");
  printf("  -c, --connect <ADDR>      Diagnostic endpoint (default: %s)\n", m_connect_address_default);
  printf("  --tls                     Use TLS transport\n");
  printf("  --ca-cert <FILE>          Server trust anchor\n");
  printf("  --client-cert <FILE>      Client certificate\n");
  printf("  --client-key <FILE>       Client private key\n");
  printf("  --json                    Emit JSON or NDJSON output\n");
  printf("  -h, --help                Show this help message\n");
  printf("  -v, --version             Show version\n\n");
  printf("Commands:\n");
  printf("  info                      Show APX server summary info\n");
  printf("  clients                   List connected clients\n");
  printf("  nodes                     List registered nodes\n");
  printf("  routing [--unconnected]   List port routing or unconnected ports\n");
  printf("  get-def <NODE>            Fetch node APX definition text\n");
  printf("  watch <NODE:PORT...>      Live signal watch notifications\n");
  printf("  top                       Interactive server dashboard\n");
}

static void print_version(void) { printf("%s %s\n", APP_NAME, APX_VERSION_STRING); }

static void on_watch_notification(void *arg, apx_diag_client_notification_t const *notif)
{
  (void)arg;
  if (notif == NULL)
    return;

  if (m_json_output)
  {
    dtl_hv_t *hv = dtl_hv_new();
    dtl_hv_set_cstr(hv, "subscription_id", (dtl_dv_t *)dtl_sv_make_u32(notif->subscription_id), false);
    dtl_hv_set_cstr(hv, "event_type", (dtl_dv_t *)dtl_sv_make_u32(notif->event_type), false);
    dtl_hv_set_cstr(hv, "sequence", (dtl_dv_t *)dtl_sv_make_u64(notif->sequence), false);
    dtl_hv_set_cstr(hv, "timestamp_ns", (dtl_dv_t *)dtl_sv_make_u64(notif->timestamp_ns), false);

    if (notif->event_type == 0x0001u) // PORT_VALUE
    {
      dtl_hv_set_cstr(hv, "type", (dtl_dv_t *)dtl_sv_make_cstr("PORT_VALUE"), false);
      dtl_hv_set_cstr(hv, "node_id", (dtl_dv_t *)dtl_sv_make_u32(notif->node_id), false);
      dtl_hv_set_cstr(hv, "port_id", (dtl_dv_t *)dtl_sv_make_u32(notif->port_id), false);
      char hex[512] = {0};
      for (uint32_t i = 0; i < notif->value_size && (i * 2 + 2) < sizeof(hex); ++i)
      {
        sprintf(hex + i * 2, "%02X", notif->value[i]);
      }
      dtl_hv_set_cstr(hv, "value_hex", (dtl_dv_t *)dtl_sv_make_cstr(hex), false);
    }
    else if (notif->event_type == 0x0002u) // SUBSCRIPTION_INVALIDATED
    {
      dtl_hv_set_cstr(hv, "type", (dtl_dv_t *)dtl_sv_make_cstr("SUBSCRIPTION_INVALIDATED"), false);
      dtl_hv_set_cstr(hv, "node_id", (dtl_dv_t *)dtl_sv_make_u32(notif->node_id), false);
      dtl_hv_set_cstr(hv, "port_id", (dtl_dv_t *)dtl_sv_make_u32(notif->port_id), false);
      dtl_hv_set_cstr(hv, "reason", (dtl_dv_t *)dtl_sv_make_u32(notif->reason), false);
    }
    else if (notif->event_type == 0x00FEu) // NOTIFICATIONS_DROPPED
    {
      dtl_hv_set_cstr(hv, "type", (dtl_dv_t *)dtl_sv_make_cstr("NOTIFICATIONS_DROPPED"), false);
      dtl_hv_set_cstr(hv, "first_sequence", (dtl_dv_t *)dtl_sv_make_u64(notif->first_sequence), false);
      dtl_hv_set_cstr(hv, "dropped_count", (dtl_dv_t *)dtl_sv_make_u64(notif->dropped_count), false);
    }

    dtl_json_dump((dtl_dv_t *)hv, stdout, 0, false);
    printf("\n");
    fflush(stdout);
    dtl_hv_delete(hv);
  }
  else
  {
    if (notif->event_type == 0x0001u)
    {
      printf("[%" PRIu64 " ns] Node %u, Port %u: (len=%u) 0x", notif->timestamp_ns, notif->node_id, notif->port_id,
        notif->value_size);
      for (uint32_t i = 0; i < notif->value_size; ++i)
      {
        printf("%02X", notif->value[i]);
      }
      printf("\n");
    }
    else if (notif->event_type == 0x0002u)
    {
      printf("[%" PRIu64 " ns] SUBSCRIPTION INVALIDATED: Node %u, Port %u (reason=%u)\n", notif->timestamp_ns,
        notif->node_id, notif->port_id, notif->reason);
    }
    else if (notif->event_type == 0x00FEu)
    {
      printf("[%" PRIu64 " ns] WARNING: NOTIFICATIONS DROPPED (first=%" PRIu64 ", count=%" PRIu64 ")\n",
        notif->timestamp_ns, notif->first_sequence, notif->dropped_count);
    }
    fflush(stdout);
  }
}

static apx_error_t connect_client(apx_diag_client_t *client)
{
  const char *addr_str = (m_connect_address != NULL) ? adt_str_cstr(m_connect_address) : m_connect_address_default;

#if defined(MSOCKET_ENABLE_TLS)
  if (m_use_tls)
  {
    msocket_tls_config_t tls_config;
    msocket_tls_config_create(&tls_config);
    if (m_ca_cert != NULL)
    {
      msocket_tls_config_set_ca_cert(&tls_config, adt_str_cstr(m_ca_cert));
    }
    if (m_client_cert != NULL && m_client_key != NULL)
    {
      msocket_tls_config_set_client_cert(&tls_config, adt_str_cstr(m_client_cert), adt_str_cstr(m_client_key));
    }
    apx_error_t err = apx_diag_client_connect_tls(client, addr_str, m_connect_port, &tls_config);
    msocket_tls_config_destroy(&tls_config);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    return apx_diag_client_wait_ready(client, 5000u);
  }
#endif

  switch (m_connect_resource_type)
  {
  case MSOCKET_ENDPOINT_IPV4:
  case MSOCKET_ENDPOINT_NAME: {
    apx_error_t err = apx_diag_client_connect_tcp(client, addr_str, m_connect_port);
    if (err != APX_NO_ERROR)
      return err;
    return apx_diag_client_wait_ready(client, 5000u);
  }
  case MSOCKET_ENDPOINT_FILE:
  default: {
    apx_error_t err = apx_diag_client_connect_unix(client, addr_str);
    if (err != APX_NO_ERROR)
      return err;
    return apx_diag_client_wait_ready(client, 5000u);
  }
  }
}

static int cmd_info(apx_diag_client_t *client)
{
  apx_diag_server_info_t info;
  memset(&info, 0, sizeof(info));
  apx_error_t err = apx_diag_client_get_server_info(client, &info, 5000u);
  if (err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to query server info (error=%d)\n", err);
    return 1;
  }

  uint64_t const uptime_sec = info.uptime_ns / 1000000000ULL;
  uint64_t const uptime_ms = info.uptime_ns / 1000000ULL;
  const char *version_str = (info.server_version != NULL) ? info.server_version : "unknown";

  if (m_json_output)
  {
    dtl_hv_t *hv = dtl_hv_new();
    dtl_hv_set_cstr(hv, "server_version", (dtl_dv_t *)dtl_sv_make_cstr(version_str), false);
    dtl_hv_set_cstr(hv, "protocol_minor", (dtl_dv_t *)dtl_sv_make_u32((uint32_t)info.protocol_minor), false);
    dtl_hv_set_cstr(hv, "server_start_unix_ns", (dtl_dv_t *)dtl_sv_make_u64(info.server_start_unix_ns), false);
    dtl_hv_set_cstr(hv, "uptime_ns", (dtl_dv_t *)dtl_sv_make_u64(info.uptime_ns), false);
    dtl_hv_set_cstr(hv, "uptime_ms", (dtl_dv_t *)dtl_sv_make_u64(uptime_ms), false);
    dtl_hv_set_cstr(hv, "active_connections", (dtl_dv_t *)dtl_sv_make_u32(info.connection_count), false);
    dtl_hv_set_cstr(hv, "active_nodes", (dtl_dv_t *)dtl_sv_make_u32(info.node_count), false);
    dtl_hv_set_cstr(hv, "total_ports", (dtl_dv_t *)dtl_sv_make_u32(info.route_count), false);

    dtl_json_dump((dtl_dv_t *)hv, stdout, 2, false);
    printf("\n");
    dtl_hv_delete(hv);
  }
  else
  {
    uint64_t const h = uptime_sec / 3600u;
    uint64_t const m = (uptime_sec % 3600u) / 60u;
    uint64_t const sec = uptime_sec % 60u;

    printf("Server Version:     %s\n", version_str);
    printf("Protocol Minor:     %u\n", (unsigned int)info.protocol_minor);
    printf("Uptime:             %" PRIu64 "h %" PRIu64 "m %" PRIu64 "s (%" PRIu64 " ms)\n", h, m, sec, uptime_ms);
    printf("Active Connections: %u\n", info.connection_count);
    printf("Active Nodes:       %u\n", info.node_count);
    printf("Active Routes:      %u\n", info.route_count);
  }
  apx_diag_server_info_destroy(&info);
  return 0;
}

static int cmd_clients(apx_diag_client_t *client)
{
  adt_ary_t list;
  apx_error_t err = apx_diag_client_list_connections(client, &list, 5000u);
  if (err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to list connections (error=%d)\n", err);
    return 1;
  }

  int32_t const len = adt_ary_length(&list);

  if (m_json_output)
  {
    dtl_av_t *av = dtl_av_new();
    for (int32_t i = 0; i < len; ++i)
    {
      apx_diag_client_conn_item_t *item = (apx_diag_client_conn_item_t *)adt_ary_value(&list, i);
      if (item != NULL)
      {
        dtl_hv_t *hv = dtl_hv_new();
        dtl_hv_set_cstr(hv, "connection_id", (dtl_dv_t *)dtl_sv_make_u32(item->connection_id), false);
        dtl_hv_set_cstr(hv, "connection_type", (dtl_dv_t *)dtl_sv_make_u32(item->connection_type), false);
        dtl_hv_set_cstr(hv, "connection_state", (dtl_dv_t *)dtl_sv_make_u32(item->connection_state), false);
        dtl_hv_set_cstr(hv, "connected_at_ns", (dtl_dv_t *)dtl_sv_make_u64(item->connected_at_ns), false);
        dtl_hv_set_cstr(hv, "tag", (dtl_dv_t *)dtl_sv_make_cstr(item->tag ? item->tag : ""), false);
        dtl_hv_set_cstr(hv, "endpoint", (dtl_dv_t *)dtl_sv_make_cstr(item->endpoint ? item->endpoint : ""), false);
        dtl_av_push(av, (dtl_dv_t *)hv, false);
      }
    }
    dtl_json_dump((dtl_dv_t *)av, stdout, 2, false);
    printf("\n");
    dtl_av_delete(av);
  }
  else
  {
    printf("%-5s %-10s %-12s %-20s %s\n", "ID", "TYPE", "STATE", "TAG", "ENDPOINT");
    printf("----------------------------------------------------------------------\n");
    for (int32_t i = 0; i < len; ++i)
    {
      apx_diag_client_conn_item_t *item = (apx_diag_client_conn_item_t *)adt_ary_value(&list, i);
      if (item != NULL)
      {
        const char *tname = (item->connection_type == 1) ? "MONITOR" : "CLIENT";
        const char *sname = (item->connection_state == 2) ? "CONNECTED" : "DISCONNECTED";
        printf("%-5u %-10s %-12s %-20s %s\n", item->connection_id, tname, sname, item->tag ? item->tag : "",
          item->endpoint ? item->endpoint : "");
      }
    }
  }

  adt_ary_destroy(&list);
  return 0;
}

static int cmd_nodes(apx_diag_client_t *client)
{
  adt_ary_t list;
  apx_error_t err = apx_diag_client_list_nodes(client, &list, 5000u);
  if (err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to list nodes (error=%d)\n", err);
    return 1;
  }

  int32_t const len = adt_ary_length(&list);

  if (m_json_output)
  {
    dtl_av_t *av = dtl_av_new();
    for (int32_t i = 0; i < len; ++i)
    {
      apx_diag_client_node_item_t *item = (apx_diag_client_node_item_t *)adt_ary_value(&list, i);
      if (item != NULL)
      {
        dtl_hv_t *hv = dtl_hv_new();
        dtl_hv_set_cstr(hv, "node_id", (dtl_dv_t *)dtl_sv_make_u32(item->node_id), false);
        dtl_hv_set_cstr(hv, "connection_id", (dtl_dv_t *)dtl_sv_make_u32(item->connection_id), false);
        dtl_hv_set_cstr(hv, "provide_port_count", (dtl_dv_t *)dtl_sv_make_u32(item->provide_port_count), false);
        dtl_hv_set_cstr(hv, "require_port_count", (dtl_dv_t *)dtl_sv_make_u32(item->require_port_count), false);
        dtl_hv_set_cstr(hv, "node_name", (dtl_dv_t *)dtl_sv_make_cstr(item->node_name ? item->node_name : ""), false);
        dtl_av_push(av, (dtl_dv_t *)hv, false);
      }
    }
    dtl_json_dump((dtl_dv_t *)av, stdout, 2, false);
    printf("\n");
    dtl_av_delete(av);
  }
  else
  {
    printf("%-5s %-8s %-14s %-14s %s\n", "ID", "CONN_ID", "PROVIDE_PORTS", "REQUIRE_PORTS", "NODE_NAME");
    printf("----------------------------------------------------------------------\n");
    for (int32_t i = 0; i < len; ++i)
    {
      apx_diag_client_node_item_t *item = (apx_diag_client_node_item_t *)adt_ary_value(&list, i);
      if (item != NULL)
      {
        printf("%-5u %-8u %-14u %-14u %s\n", item->node_id, item->connection_id, item->provide_port_count,
          item->require_port_count, item->node_name ? item->node_name : "");
      }
    }
  }

  adt_ary_destroy(&list);
  return 0;
}

static apx_diag_client_node_item_t const *find_node(adt_ary_t const *nodes, uint32_t node_id)
{
  int32_t const len = adt_ary_length(nodes);
  for (int32_t i = 0; i < len; ++i)
  {
    apx_diag_client_node_item_t const *node = (apx_diag_client_node_item_t const *)adt_ary_value(nodes, i);
    if (node != NULL && node->node_id == node_id && node->node_name != NULL)
    {
      return node;
    }
  }
  return NULL;
}

static int cmd_routing(apx_diag_client_t *client, bool unconnected)
{
  if (unconnected)
  {
    adt_ary_t list;
    apx_error_t err = apx_diag_client_list_unconnected_ports(client, &list, 5000u);
    if (err != APX_NO_ERROR)
    {
      fprintf(stderr, "Error: Failed to list unconnected ports (error=%d)\n", err);
      return 1;
    }

    int32_t const len = adt_ary_length(&list);

    if (m_json_output)
    {
      dtl_av_t *av = dtl_av_new();
      for (int32_t i = 0; i < len; ++i)
      {
        apx_diag_client_unconnected_port_item_t *item =
          (apx_diag_client_unconnected_port_item_t *)adt_ary_value(&list, i);
        if (item != NULL)
        {
          dtl_hv_t *hv = dtl_hv_new();
          dtl_hv_set_cstr(hv, "node_id", (dtl_dv_t *)dtl_sv_make_u32(item->node_id), false);
          dtl_hv_set_cstr(hv, "port_id", (dtl_dv_t *)dtl_sv_make_u32(item->port_id), false);
          dtl_hv_set_cstr(
            hv, "direction", (dtl_dv_t *)dtl_sv_make_cstr(item->direction == 0 ? "REQUIRE" : "PROVIDE"), false);
          dtl_hv_set_cstr(
            hv, "reason", (dtl_dv_t *)dtl_sv_make_cstr(item->reason == 0 ? "NO_PROVIDER" : "NO_CONSUMER"), false);
          dtl_hv_set_cstr(hv, "port_name", (dtl_dv_t *)dtl_sv_make_cstr(item->port_name ? item->port_name : ""), false);
          dtl_av_push(av, (dtl_dv_t *)hv, false);
        }
      }
      dtl_json_dump((dtl_dv_t *)av, stdout, 2, false);
      printf("\n");
      dtl_av_delete(av);
    }
    else
    {
      adt_ary_t nodes;
      err = apx_diag_client_list_nodes(client, &nodes, 5000u);
      if (err != APX_NO_ERROR)
      {
        fprintf(stderr, "Error: Failed to list nodes (error=%d)\n", err);
        adt_ary_destroy(&nodes);
        adt_ary_destroy(&list);
        return 1;
      }

      int node_width = 24;
      for (int32_t i = 0; i < len; ++i)
      {
        apx_diag_client_unconnected_port_item_t *item =
          (apx_diag_client_unconnected_port_item_t *)adt_ary_value(&list, i);
        if (item != NULL)
        {
          apx_diag_client_node_item_t const *node = find_node(&nodes, item->node_id);
          if (node == NULL)
          {
            fprintf(stderr, "Error: Cannot resolve node ID %u; server state may have changed. Retry the command.\n",
              item->node_id);
            adt_ary_destroy(&nodes);
            adt_ary_destroy(&list);
            return 1;
          }
          int const name_width = snprintf(NULL, 0, "%u:%s", node->connection_id, node->node_name);
          if (name_width > node_width)
          {
            node_width = name_width;
          }
        }
      }

      printf("%-*s %-10s %-14s %s\n", node_width, "NODE_NAME", "DIRECTION", "REASON", "PORT_NAME");
      printf("----------------------------------------------------------------------\n");
      for (int32_t i = 0; i < len; ++i)
      {
        apx_diag_client_unconnected_port_item_t *item =
          (apx_diag_client_unconnected_port_item_t *)adt_ary_value(&list, i);
        if (item != NULL)
        {
          const char *dir = (item->direction == 0) ? "REQUIRE" : "PROVIDE";
          const char *rsn = (item->reason == 0) ? "NO_PROVIDER" : "NO_CONSUMER";
          apx_diag_client_node_item_t const *node = find_node(&nodes, item->node_id);
          int const prefix_width = snprintf(NULL, 0, "%u:", node->connection_id);
          printf("%u:%-*s %-10s %-14s %s\n", node->connection_id, node_width - prefix_width, node->node_name, dir, rsn,
            item->port_name ? item->port_name : "");
        }
      }
      adt_ary_destroy(&nodes);
    }
    adt_ary_destroy(&list);
  }
  else
  {
    adt_ary_t list;
    apx_error_t err = apx_diag_client_list_routes(client, &list, 5000u);
    if (err != APX_NO_ERROR)
    {
      fprintf(stderr, "Error: Failed to list routes (error=%d)\n", err);
      return 1;
    }

    int32_t const len = adt_ary_length(&list);

    if (m_json_output)
    {
      dtl_av_t *av = dtl_av_new();
      for (int32_t i = 0; i < len; ++i)
      {
        apx_diag_client_route_item_t *item = (apx_diag_client_route_item_t *)adt_ary_value(&list, i);
        if (item != NULL)
        {
          dtl_hv_t *hv = dtl_hv_new();
          dtl_hv_set_cstr(hv, "port_name", (dtl_dv_t *)dtl_sv_make_cstr(item->port_name ? item->port_name : ""), false);
          dtl_hv_set_cstr(hv, "provide_node_id", (dtl_dv_t *)dtl_sv_make_u32(item->provide_node_id), false);
          dtl_hv_set_cstr(hv, "provide_port_id", (dtl_dv_t *)dtl_sv_make_u32(item->provide_port_id), false);
          dtl_hv_set_cstr(hv, "require_node_id", (dtl_dv_t *)dtl_sv_make_u32(item->require_node_id), false);
          dtl_hv_set_cstr(hv, "require_port_id", (dtl_dv_t *)dtl_sv_make_u32(item->require_port_id), false);
          dtl_av_push(av, (dtl_dv_t *)hv, false);
        }
      }
      dtl_json_dump((dtl_dv_t *)av, stdout, 2, false);
      printf("\n");
      dtl_av_delete(av);
    }
    else
    {
      adt_ary_t nodes;
      err = apx_diag_client_list_nodes(client, &nodes, 5000u);
      if (err != APX_NO_ERROR)
      {
        fprintf(stderr, "Error: Failed to list nodes (error=%d)\n", err);
        adt_ary_destroy(&nodes);
        adt_ary_destroy(&list);
        return 1;
      }

      int port_width = 24;
      int provider_width = 20;
      for (int32_t i = 0; i < len; ++i)
      {
        apx_diag_client_route_item_t *item = (apx_diag_client_route_item_t *)adt_ary_value(&list, i);
        if (item != NULL)
        {
          apx_diag_client_node_item_t const *provider = find_node(&nodes, item->provide_node_id);
          apx_diag_client_node_item_t const *requirer = find_node(&nodes, item->require_node_id);
          if (provider == NULL || requirer == NULL)
          {
            uint32_t const node_id = (provider == NULL) ? item->provide_node_id : item->require_node_id;
            fprintf(
              stderr, "Error: Cannot resolve node ID %u; server state may have changed. Retry the command.\n", node_id);
            adt_ary_destroy(&nodes);
            adt_ary_destroy(&list);
            return 1;
          }
          int const name_width = item->port_name ? (int)strlen(item->port_name) : 0;
          int const node_width = snprintf(NULL, 0, "%u:%s", provider->connection_id, provider->node_name);
          if (name_width > port_width)
          {
            port_width = name_width;
          }
          if (node_width > provider_width)
          {
            provider_width = node_width;
          }
        }
      }

      printf("%-*s %-*s %s\n", port_width, "PORT_NAME", provider_width, "PROVIDER_NODE", "REQUIRER_NODE");
      printf("----------------------------------------------------------------------\n");
      for (int32_t i = 0; i < len; ++i)
      {
        apx_diag_client_route_item_t *item = (apx_diag_client_route_item_t *)adt_ary_value(&list, i);
        if (item != NULL)
        {
          apx_diag_client_node_item_t const *provider = find_node(&nodes, item->provide_node_id);
          apx_diag_client_node_item_t const *requirer = find_node(&nodes, item->require_node_id);
          int const prefix_width = snprintf(NULL, 0, "%u:", provider->connection_id);
          printf("%-*s %u:%-*s %u:%s\n", port_width, item->port_name ? item->port_name : "", provider->connection_id,
            provider_width - prefix_width, provider->node_name, requirer->connection_id, requirer->node_name);
        }
      }
      adt_ary_destroy(&nodes);
    }
    adt_ary_destroy(&list);
  }
  return 0;
}

static char *json_escape_string(const char *input)
{
  if (input == NULL)
  {
    return NULL;
  }
  size_t len = strlen(input);
  char *out = (char *)malloc(len * 6 + 1);
  if (out == NULL)
  {
    return NULL;
  }
  char *p = out;
  for (size_t i = 0; i < len; ++i)
  {
    char c = input[i];
    switch (c)
    {
    case '"':
      *p++ = '\\';
      *p++ = '"';
      break;
    case '\\':
      *p++ = '\\';
      *p++ = '\\';
      break;
    case '\b':
      *p++ = '\\';
      *p++ = 'b';
      break;
    case '\f':
      *p++ = '\\';
      *p++ = 'f';
      break;
    case '\n':
      *p++ = '\\';
      *p++ = 'n';
      break;
    case '\r':
      *p++ = '\\';
      *p++ = 'r';
      break;
    case '\t':
      *p++ = '\\';
      *p++ = 't';
      break;
    default:
      if ((unsigned char)c < 0x20)
      {
        p += sprintf(p, "\\u%04x", (unsigned char)c);
      }
      else
      {
        *p++ = c;
      }
      break;
    }
  }
  *p = '\0';
  return out;
}

static int cmd_get_def(apx_diag_client_t *client, const char *node_name)
{
  char *def_text = NULL;
  apx_error_t err = apx_diag_client_get_node_definition(client, node_name, &def_text, 5000u);
  if (err != APX_NO_ERROR || def_text == NULL)
  {
    fprintf(stderr, "Error: Failed to get definition for node '%s' (error=%d)\n", node_name, err);
    return 1;
  }

  if (m_json_output)
  {
    char *escaped = json_escape_string(def_text);
    printf("{\n  \"node_name\": \"%s\",\n  \"definition\": \"%s\"\n}\n", node_name, escaped != NULL ? escaped : "");
    if (escaped != NULL)
    {
      free(escaped);
    }
  }
  else
  {
    fputs(def_text, stdout);
    if (def_text[strlen(def_text) - 1] != '\n')
    {
      printf("\n");
    }
  }

  free(def_text);
  return 0;
}

static int cmd_watch(apx_diag_client_t *client, const char **selectors, int num_selectors)
{
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);

  apx_diag_client_set_notification_callback(client, on_watch_notification, NULL);

  uint32_t sub_id = 0u;
  adt_ary_t resolutions;
  apx_error_t err = apx_diag_client_subscribe_ports(
    client, 0x0001u /* SEND_INITIAL */, selectors, (uint16_t)num_selectors, &sub_id, &resolutions, 5000u);

  if (err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to subscribe to ports (error=%d)\n", err);
    return 1;
  }

  m_active_subscription_id = sub_id;

  if (!m_json_output)
  {
    printf("Subscribed (id=%u) to %d selector(s). Press Ctrl+C to stop.\n", sub_id, num_selectors);
    int32_t res_count = adt_ary_length(&resolutions);
    for (int32_t i = 0; i < res_count; ++i)
    {
      apx_diag_client_resolution_t *r = (apx_diag_client_resolution_t *)adt_ary_value(&resolutions, i);
      if (r != NULL)
      {
        printf("  Selector #%u -> Node %u, Port %u (size=%u, status=%u)\n", r->selector_index, r->node_id, r->port_id,
          r->value_size, r->status);
      }
    }
  }

  adt_ary_destroy(&resolutions);

  while (m_running)
  {
    usleep(100000);
  }

  if (!m_json_output)
  {
    printf("\nUnsubscribing...\n");
  }
  apx_diag_client_unsubscribe(client, sub_id, 2000u);
  return 0;
}

static int cmd_top(apx_diag_client_t *client)
{
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);

  while (m_running)
  {
    if (!m_json_output)
    {
      // Clear ANSI screen
      printf("\033[H\033[J");
    }
    cmd_info(client);
    if (!m_json_output)
    {
      printf("\n--- Nodes ---\n");
    }
    cmd_nodes(client);

    for (int i = 0; i < 10 && m_running; ++i)
    {
      usleep(100000);
    }
  }
  return 0;
}

static argparse_result_t parse_arg(const char *short_name, const char *long_name, const char *value)
{
  if (short_name != NULL)
  {
    if (strcmp(short_name, "h") == 0)
    {
      print_usage(APP_NAME);
      exit(0);
    }
    else if (strcmp(short_name, "v") == 0)
    {
      print_version();
      exit(0);
    }
    else if (strcmp(short_name, "c") == 0)
    {
      if (m_connect_address != NULL)
        adt_str_delete(m_connect_address);
      m_connect_resource_type = msocket_parse_endpoint(value, &m_connect_address, &m_connect_port);
      if (m_connect_resource_type == MSOCKET_ENDPOINT_UNKNOWN || m_connect_resource_type == MSOCKET_ENDPOINT_ERROR)
      {
        return ARGPARSE_VALUE_ERROR;
      }
    }
  }
  if (long_name != NULL)
  {
    if (strcmp(long_name, "help") == 0)
    {
      print_usage(APP_NAME);
      exit(0);
    }
    else if (strcmp(long_name, "version") == 0)
    {
      print_version();
      exit(0);
    }
    else if (strcmp(long_name, "connect") == 0)
    {
      if (m_connect_address != NULL)
        adt_str_delete(m_connect_address);
      m_connect_resource_type = msocket_parse_endpoint(value, &m_connect_address, &m_connect_port);
      if (m_connect_resource_type == MSOCKET_ENDPOINT_UNKNOWN || m_connect_resource_type == MSOCKET_ENDPOINT_ERROR)
      {
        return ARGPARSE_VALUE_ERROR;
      }
    }
    else if (strcmp(long_name, "tls") == 0)
    {
      m_use_tls = true;
    }
    else if (strcmp(long_name, "json") == 0)
    {
      m_json_output = true;
    }
    else if (strcmp(long_name, "ca-cert") == 0)
    {
      if (m_ca_cert != NULL)
        adt_str_delete(m_ca_cert);
      m_ca_cert = adt_str_new_cstr(value);
    }
    else if (strcmp(long_name, "client-cert") == 0)
    {
      if (m_client_cert != NULL)
        adt_str_delete(m_client_cert);
      m_client_cert = adt_str_new_cstr(value);
    }
    else if (strcmp(long_name, "client-key") == 0)
    {
      if (m_client_key != NULL)
        adt_str_delete(m_client_key);
      m_client_key = adt_str_new_cstr(value);
    }
  }
  return ARGPARSE_SUCCESS;
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    print_usage(APP_NAME);
    return 1;
  }

  // Parse options preceding the command
  int cmd_idx = 1;
  while (cmd_idx < argc && argv[cmd_idx][0] == '-')
  {
    if (strcmp(argv[cmd_idx], "-h") == 0 || strcmp(argv[cmd_idx], "--help") == 0)
    {
      print_usage(APP_NAME);
      return 0;
    }
    if (strcmp(argv[cmd_idx], "-v") == 0 || strcmp(argv[cmd_idx], "--version") == 0)
    {
      print_version();
      return 0;
    }
    if (strcmp(argv[cmd_idx], "--json") == 0)
    {
      m_json_output = true;
      cmd_idx++;
      continue;
    }
    if (strcmp(argv[cmd_idx], "--tls") == 0)
    {
      m_use_tls = true;
      cmd_idx++;
      continue;
    }
    if ((strcmp(argv[cmd_idx], "-c") == 0 || strcmp(argv[cmd_idx], "--connect") == 0) && (cmd_idx + 1 < argc))
    {
      parse_arg(NULL, "connect", argv[cmd_idx + 1]);
      cmd_idx += 2;
      continue;
    }
    if (strcmp(argv[cmd_idx], "--ca-cert") == 0 && (cmd_idx + 1 < argc))
    {
      parse_arg(NULL, "ca-cert", argv[cmd_idx + 1]);
      cmd_idx += 2;
      continue;
    }
    if (strcmp(argv[cmd_idx], "--client-cert") == 0 && (cmd_idx + 1 < argc))
    {
      parse_arg(NULL, "client-cert", argv[cmd_idx + 1]);
      cmd_idx += 2;
      continue;
    }
    if (strcmp(argv[cmd_idx], "--client-key") == 0 && (cmd_idx + 1 < argc))
    {
      parse_arg(NULL, "client-key", argv[cmd_idx + 1]);
      cmd_idx += 2;
      continue;
    }
    fprintf(stderr, "Unknown option: %s\n", argv[cmd_idx]);
    print_usage(APP_NAME);
    return 1;
  }

  if (cmd_idx >= argc)
  {
    print_usage(APP_NAME);
    return 1;
  }

  const char *command = argv[cmd_idx];

  // Default endpoint if unspecified
  if (m_connect_resource_type == MSOCKET_ENDPOINT_UNKNOWN)
  {
    uint16_t dummy_port = 0;
    const char *default_addr = m_use_tls ? "127.0.0.1" : m_connect_address_default;
    m_connect_resource_type = msocket_parse_endpoint(default_addr, &m_connect_address, &dummy_port);
    if (m_use_tls)
    {
      m_connect_port = 5020;
    }
  }

  m_client = apx_diag_client_new();
  if (m_client == NULL)
  {
    fprintf(stderr, "Error: Failed to create diagnostic client\n");
    return 1;
  }

  apx_error_t conn_err = connect_client(m_client);
  if (conn_err != APX_NO_ERROR)
  {
    fprintf(stderr, "Error: Failed to connect to APX server (error=%d)\n", conn_err);
    apx_diag_client_delete(m_client);
    return 1;
  }

  int exit_code = 0;
  if (strcmp(command, "info") == 0)
  {
    exit_code = cmd_info(m_client);
  }
  else if (strcmp(command, "clients") == 0)
  {
    exit_code = cmd_clients(m_client);
  }
  else if (strcmp(command, "nodes") == 0)
  {
    exit_code = cmd_nodes(m_client);
  }
  else if (strcmp(command, "routing") == 0)
  {
    bool unconnected = false;
    for (int i = cmd_idx + 1; i < argc; ++i)
    {
      if (strcmp(argv[i], "--unconnected") == 0)
      {
        unconnected = true;
      }
    }
    exit_code = cmd_routing(m_client, unconnected);
  }
  else if (strcmp(command, "get-def") == 0)
  {
    if (cmd_idx + 1 >= argc)
    {
      fprintf(stderr, "Error: 'get-def' requires a node name argument\n");
      exit_code = 1;
    }
    else
    {
      exit_code = cmd_get_def(m_client, argv[cmd_idx + 1]);
    }
  }
  else if (strcmp(command, "watch") == 0)
  {
    if (cmd_idx + 1 >= argc)
    {
      fprintf(stderr, "Error: 'watch' requires at least one port selector argument\n");
      exit_code = 1;
    }
    else
    {
      const char **selectors = (const char **)&argv[cmd_idx + 1];
      int num_selectors = argc - (cmd_idx + 1);
      exit_code = cmd_watch(m_client, selectors, num_selectors);
    }
  }
  else if (strcmp(command, "top") == 0)
  {
    exit_code = cmd_top(m_client);
  }
  else
  {
    fprintf(stderr, "Error: Unknown command '%s'\n", command);
    print_usage(APP_NAME);
    exit_code = 1;
  }

  apx_diag_client_delete(m_client);
  if (m_connect_address != NULL)
    adt_str_delete(m_connect_address);
  if (m_ca_cert != NULL)
    adt_str_delete(m_ca_cert);
  if (m_client_cert != NULL)
    adt_str_delete(m_client_cert);
  if (m_client_key != NULL)
    adt_str_delete(m_client_key);

  return exit_code;
}
