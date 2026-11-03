/*****************************************************************************
 * \file      apx_log_event.h
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     Event decoder and formatter for .apxlog records
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_LOG_EVENT_H
#define APX_LOG_EVENT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_LOG_EVENT_STREAM_BEGIN 0x0001u
#define APX_LOG_EVENT_SNAPSHOT_END 0x0002u
#define APX_LOG_EVENT_CONNECTION_CREATE 0x0010u
#define APX_LOG_EVENT_CONNECTION_CLOSE 0x0011u
#define APX_LOG_EVENT_NODE_ATTACH 0x0020u
#define APX_LOG_EVENT_NODE_DETACH 0x0021u
#define APX_LOG_EVENT_FILE_PUBLISH 0x0030u
#define APX_LOG_EVENT_FILE_REVOKE 0x0031u
#define APX_LOG_EVENT_FILE_OPEN 0x0032u
#define APX_LOG_EVENT_FILE_CLOSE 0x0033u
#define APX_LOG_EVENT_FILE_WRITE 0x0034u
#define APX_LOG_EVENT_PROVIDER_CHANGE 0x0050u
#define APX_LOG_EVENT_EVENTS_DROPPED 0x00FEu
#define APX_LOG_EVENT_STREAM_END 0x00FFu

#define APX_LOG_FLAG_SNAPSHOT 0x0001u
#define APX_LOG_FLAG_INITIAL_VALUE 0x0002u

#define APX_LOG_EVENT_HEADER_SIZE 28u

typedef struct apx_log_event_tag
{
  uint16_t event_type;
  uint16_t event_flags;
  uint64_t sequence;
  uint64_t timestamp_ns;
  uint32_t connection_id;
  uint32_t object_id;
  const uint8_t *payload;
  uint32_t payload_size;
} apx_log_event_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_log_event_parse(apx_log_event_t *self, const uint8_t *buf, uint32_t size);
const char *apx_log_event_type_str(uint16_t event_type);

void apx_log_event_print_text(const apx_log_event_t *self, FILE *out);
void apx_log_event_print_json(const apx_log_event_t *self, FILE *out);

#endif // APX_LOG_EVENT_H
