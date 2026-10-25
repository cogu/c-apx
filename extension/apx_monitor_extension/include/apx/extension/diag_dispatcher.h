/*****************************************************************************
 * \file      diag_dispatcher.h
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Interactive diagnostics query dispatcher (apxmon)
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_DIAG_DISPATCHER_H
#define APX_DIAG_DISPATCHER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "apx/types.h"
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_monitor_session_tag;

typedef struct apx_diag_dispatcher_tag
{
  struct apx_monitor_session_tag *session;
} apx_diag_dispatcher_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_diag_dispatcher_create(apx_diag_dispatcher_t *self, struct apx_monitor_session_tag *session);
void apx_diag_dispatcher_destroy(apx_diag_dispatcher_t *self);
apx_diag_dispatcher_t *apx_diag_dispatcher_new(struct apx_monitor_session_tag *session);
void apx_diag_dispatcher_delete(apx_diag_dispatcher_t *self);
void apx_diag_dispatcher_vdelete(void *arg);

void apx_diag_dispatcher_on_request_written(
  apx_diag_dispatcher_t *self, uint32_t offset, const uint8_t *data, apx_size_t size);

#endif // APX_DIAG_DISPATCHER_H
