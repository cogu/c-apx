/*****************************************************************************
 * \file      log_streamer.h
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Event log streaming producer (server.log / apxlog)
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_LOG_STREAMER_H
#define APX_LOG_STREAMER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "apx/types.h"
#include <stdbool.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_monitor_session_tag;

typedef struct apx_log_streamer_tag
{
  struct apx_monitor_session_tag *session;
  bool is_active;
  uint64_t sequence;
  uint8_t stream_id[16];
  uint64_t stream_start_unix_ns;
} apx_log_streamer_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_log_streamer_create(apx_log_streamer_t *self, struct apx_monitor_session_tag *session);
void apx_log_streamer_destroy(apx_log_streamer_t *self);
apx_log_streamer_t *apx_log_streamer_new(struct apx_monitor_session_tag *session);
void apx_log_streamer_delete(apx_log_streamer_t *self);
void apx_log_streamer_vdelete(void *arg);

apx_error_t apx_log_streamer_start(apx_log_streamer_t *self);
void apx_log_streamer_stop(apx_log_streamer_t *self);
bool apx_log_streamer_is_active(const apx_log_streamer_t *self);

#endif // APX_LOG_STREAMER_H
