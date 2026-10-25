/*****************************************************************************
 * \file      log_streamer.c
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Event log streaming producer (server.log / apxlog)
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/log_streamer.h"
#include "apx/extension/monitor_session.h"
#include <assert.h>
#include <malloc.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_log_streamer_create(apx_log_streamer_t *self, apx_monitor_session_t *session)
{
  if (self != NULL)
  {
    self->session = session;
    self->is_active = false;
    self->sequence = 0;
    memset(self->stream_id, 0, sizeof(self->stream_id));
    self->stream_start_unix_ns = 0;
  }
}

void apx_log_streamer_destroy(apx_log_streamer_t *self)
{
  if (self != NULL)
  {
    if (self->is_active)
    {
      apx_log_streamer_stop(self);
    }
    self->session = NULL;
  }
}

apx_log_streamer_t *apx_log_streamer_new(apx_monitor_session_t *session)
{
  apx_log_streamer_t *self = (apx_log_streamer_t *)malloc(sizeof(apx_log_streamer_t));
  if (self != NULL)
  {
    apx_log_streamer_create(self, session);
  }
  return self;
}

void apx_log_streamer_delete(apx_log_streamer_t *self)
{
  if (self != NULL)
  {
    apx_log_streamer_destroy(self);
    free(self);
  }
}

void apx_log_streamer_vdelete(void *arg) { apx_log_streamer_delete((apx_log_streamer_t *)arg); }

apx_error_t apx_log_streamer_start(apx_log_streamer_t *self)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!self->is_active)
  {
    self->is_active = true;
    self->sequence = 0;
  }
  return APX_NO_ERROR;
}

void apx_log_streamer_stop(apx_log_streamer_t *self)
{
  if (self != NULL && self->is_active)
  {
    self->is_active = false;
  }
}

bool apx_log_streamer_is_active(const apx_log_streamer_t *self)
{
  if (self != NULL)
  {
    return self->is_active;
  }
  return false;
}
