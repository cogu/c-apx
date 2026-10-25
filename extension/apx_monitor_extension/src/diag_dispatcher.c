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
#include "apx/extension/monitor_session.h"
#include <assert.h>
#include <malloc.h>
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
void apx_diag_dispatcher_create(apx_diag_dispatcher_t *self, apx_monitor_session_t *session)
{
  if (self != NULL)
  {
    self->session = session;
  }
}

void apx_diag_dispatcher_destroy(apx_diag_dispatcher_t *self)
{
  if (self != NULL)
  {
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
  (void)self;
  (void)offset;
  (void)data;
  (void)size;
}
