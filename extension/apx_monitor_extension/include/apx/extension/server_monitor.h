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
#include "adt_list.h"
#include "apx/error.h"
#include "apx/event_listener.h"
#include "apx/types.h"
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

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
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
  MUTEX_T lock;
} apx_server_monitor_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_server_monitor_create(apx_server_monitor_t *self, struct apx_server_tag *server);
void apx_server_monitor_destroy(apx_server_monitor_t *self);
apx_server_monitor_t *apx_server_monitor_new(struct apx_server_tag *server);
void apx_server_monitor_delete(apx_server_monitor_t *self);
void apx_server_monitor_vdelete(void *arg);

int32_t apx_server_monitor_num_sessions(apx_server_monitor_t *self);
struct apx_monitor_session_tag *apx_server_monitor_create_session(
  apx_server_monitor_t *self, struct apx_server_connection_tag *connection);
void apx_server_monitor_remove_session(apx_server_monitor_t *self, struct apx_monitor_session_tag *session);
struct apx_monitor_session_tag *apx_server_monitor_find_session(
  apx_server_monitor_t *self, struct apx_server_connection_tag *connection);

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

#endif // APX_SERVER_MONITOR_H
