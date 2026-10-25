/*****************************************************************************
 * \file      monitor_session.h
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Per-client monitor session
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_MONITOR_SESSION_H
#define APX_MONITOR_SESSION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "apx/types.h"
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

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_SERVER_LOG_FILE_NAME "server.log"
#define APX_DIAG_RESPONSE_FILE_NAME "apx.diag.v1.response"
#define APX_DIAG_NOTIFY_FILE_NAME "apx.diag.v1.notify"
#define APX_DIAG_REQUEST_FILE_NAME "apx.diag.v1.request"

#define APX_MONITOR_DEFAULT_STREAM_FILE_SIZE ((uint32_t)(1024u * 1024u)) // 1 MiB

struct apx_server_monitor_tag;
struct apx_server_connection_tag;
struct apx_log_streamer_tag;
struct apx_diag_dispatcher_tag;
struct apx_file_tag;

typedef struct apx_monitor_session_tag
{
  struct apx_server_monitor_tag *monitor;
  struct apx_server_connection_tag *connection;
  struct apx_log_streamer_tag *log_streamer;
  struct apx_diag_dispatcher_tag *diag_dispatcher;
  bool is_server_log_open;
  bool is_diag_response_open;
  bool is_diag_notify_open;
  MUTEX_T lock;
} apx_monitor_session_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_monitor_session_create(
  apx_monitor_session_t *self, struct apx_server_monitor_tag *monitor, struct apx_server_connection_tag *connection);
void apx_monitor_session_destroy(apx_monitor_session_t *self);
apx_monitor_session_t *apx_monitor_session_new(
  struct apx_server_monitor_tag *monitor, struct apx_server_connection_tag *connection);
void apx_monitor_session_delete(apx_monitor_session_t *self);
void apx_monitor_session_vdelete(void *arg);

apx_error_t apx_monitor_session_publish_files(apx_monitor_session_t *self);
void apx_monitor_session_on_file_opened(apx_monitor_session_t *self, struct apx_file_tag *file);
void apx_monitor_session_on_file_closed(apx_monitor_session_t *self, struct apx_file_tag *file);
void apx_monitor_session_on_file_written(
  apx_monitor_session_t *self, struct apx_file_tag *file, uint32_t offset, const uint8_t *data, apx_size_t size);

struct apx_server_connection_tag *apx_monitor_session_get_connection(apx_monitor_session_t *self);
struct apx_log_streamer_tag *apx_monitor_session_get_log_streamer(apx_monitor_session_t *self);
struct apx_diag_dispatcher_tag *apx_monitor_session_get_diag_dispatcher(apx_monitor_session_t *self);

#endif // APX_MONITOR_SESSION_H
