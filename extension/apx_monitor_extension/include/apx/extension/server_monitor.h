/*****************************************************************************
 * \file      server_monitor.h
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Server monitor extension core
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_SERVER_MONITOR_H
#define APX_SERVER_MONITOR_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_hash.h"
#include "adt_list.h"
#include "apx/error.h"
#include "apx/event_listener.h"
#include "apx/types.h"
#include "dtl_type.h"
#include "msocket_server.h"
#include "msocket_tls.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "osmacro.h"

#ifdef UNIT_TEST
# include "testsocket.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_MONITOR_SERVER_LABEL "MONITOR"
#define APX_MONITOR_DEFAULT_PORT ((uint16_t)5080u)

struct apx_server_tag;
struct apx_server_connection_tag;
struct apx_monitor_session_tag;

typedef struct apx_server_monitor_tag
{
  struct apx_server_tag *server;
  void *server_listener_handle;
  adt_list_t sessions; // Strong references to apx_monitor_session_t
  int32_t num_active_log_streams;
  int32_t num_active_port_watches;
  uint32_t next_node_id;
  adt_hash_t node_id_map; // maps node_name -> uint32_t (allocated)
  MUTEX_T lock;

  // Transport configuration
  bool is_enabled;
  uint16_t tcp_port;
  char *bind_address;
  char *socket_file;
  bool is_tcp_started;
  bool is_unix_started;
  msocket_server_t tcp_server;
  msocket_server_t unix_server;
  msocket_tls_config_t tls_config;
  bool tls_enabled;
} apx_server_monitor_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_server_monitor_create(apx_server_monitor_t *self, struct apx_server_tag *server);
void apx_server_monitor_destroy(apx_server_monitor_t *self);
apx_server_monitor_t *apx_server_monitor_new(struct apx_server_tag *server);
void apx_server_monitor_delete(apx_server_monitor_t *self);
void apx_server_monitor_vdelete(void *arg);

apx_error_t apx_server_monitor_configure(apx_server_monitor_t *self, dtl_hv_t *config);
apx_error_t apx_server_monitor_start(apx_server_monitor_t *self);
void apx_server_monitor_stop(apx_server_monitor_t *self);

int32_t apx_server_monitor_num_sessions(apx_server_monitor_t *self);
struct apx_monitor_session_tag *apx_server_monitor_create_session(
  apx_server_monitor_t *self, struct apx_server_connection_tag *connection);
void apx_server_monitor_remove_session(apx_server_monitor_t *self, struct apx_monitor_session_tag *session);
struct apx_monitor_session_tag *apx_server_monitor_find_session(
  apx_server_monitor_t *self, struct apx_server_connection_tag *connection);

void apx_server_monitor_on_log_stream_started(apx_server_monitor_t *self, struct apx_monitor_session_tag *session);
void apx_server_monitor_on_log_stream_stopped(apx_server_monitor_t *self, struct apx_monitor_session_tag *session);

uint32_t apx_server_monitor_get_or_create_node_id(apx_server_monitor_t *self, const char *node_name);
uint32_t apx_server_monitor_get_node_id(apx_server_monitor_t *self, const char *node_name);
void apx_server_monitor_increment_port_watches(apx_server_monitor_t *self);
void apx_server_monitor_decrement_port_watches(apx_server_monitor_t *self);

// Gated fast-path hooks registered on apx_server_t
void apx_server_monitor_on_new_connection(void *arg, struct apx_server_connection_tag *connection);
void apx_server_monitor_on_connection_closed(void *arg, struct apx_server_connection_tag *connection);
void apx_server_monitor_on_protocol_header_accepted(void *arg, struct apx_connection_base_tag *connection);
void apx_server_monitor_on_file_published(
  void *arg, struct apx_connection_base_tag *connection, const struct rmf_file_info_tag *file_info);
void apx_server_monitor_on_file_revoked(
  void *arg, struct apx_connection_base_tag *connection, const struct rmf_file_info_tag *file_info);
void apx_server_monitor_on_file_opened(
  void *arg, struct apx_connection_base_tag *connection, const struct rmf_file_info_tag *file_info);
void apx_server_monitor_on_file_closed(
  void *arg, struct apx_connection_base_tag *connection, const struct rmf_file_info_tag *file_info);
void apx_server_monitor_on_file_written(void *arg, struct apx_connection_base_tag *connection,
  struct apx_file_tag *file, uint32_t offset, const uint8_t *data, apx_size_t size);

#ifdef UNIT_TEST
void apx_server_monitor_accept_testsocket(apx_server_monitor_t *self, testsocket_t *sock);
#endif

#if defined(__GNUC__) || defined(__clang__)
void apx_server_monitor_log(struct apx_server_tag *server, apx_log_level_t level, const char *format, ...)
  __attribute__((format(printf, 3, 4)));
#else
void apx_server_monitor_log(struct apx_server_tag *server, apx_log_level_t level, const char *format, ...);
#endif

#endif // APX_SERVER_MONITOR_H
