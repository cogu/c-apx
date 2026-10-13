/*****************************************************************************
 * \file      event.h
 * \author    Conny Gustafsson
 * \date      2018-10-15
 * \brief     Maps all APX event listeners event data into a common data structure
 *
 * Copyright (c) 2018-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_EVENT_H
#define APX_EVENT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_str.h"
#include "apx/types.h"
#include "osmacro.h"
#include "soa.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
// forward declarations
struct apx_connection_base_tag;
struct rmf_file_info_tag;

typedef struct apx_event_tag
{
  apx_event_id_t ev_type;
  uint16_t flags;
  void *callback; // callback function
  uint32_t data1; // generic uint32 value
  uint32_t data2; // generic uint32 value
  void *data3;    // generic void* pointer value
  void *data4;    // generic void* pointer value
  void *data5;    // generic void* pointer value
} apx_event_t;

#define APX_EVENT_SIZE sizeof(apx_event_t)

#define APX_EVENT_FLAG_FILE_MANAGER_EVENT 0x01
#define APX_EVENT_FLAG_REMOTE_ADDRESS 0x02

// APX event IDs
#define APX_EVENT_LOG_WRITE 0
#define APX_EVENT_PROTOCOL_HEADER_ACCEPTED 1
#define APX_EVENT_REMOTE_FILE_PUBLISHED 2
#define APX_EVENT_BARRIER 3

/**
 * Event field usage and ownership:
 *
 * APX_EVENT_LOG_WRITE:
 *   - data1: Log level (0-3, apx_log_level_t)
 *   - data3: Label string (char[16], soa-strong reference: allocated via SOA, freed with soa_free)
 *   - data4: Log message (adt_str_t *, strong reference: freed with adt_str_delete)
 *
 * APX_EVENT_PROTOCOL_HEADER_ACCEPTED:
 *   - data3: Connection object (apx_connection_base_t *, weak reference)
 *
 * APX_EVENT_REMOTE_FILE_PUBLISHED:
 *   - data3: Connection object (apx_connection_base_t *, weak reference)
 *   - data4: Remote file info (rmf_file_info_t *, strong reference: freed with rmf_file_info_delete)
 *
 * APX_EVENT_BARRIER:
 *   - data3: Semaphore (SEMAPHORE_T *, weak reference: signaled via SEMAPHORE_POST)
 */


typedef void(apx_event_handler_func_t)(void *arg, apx_event_t *event);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

void apx_event_pack_log_write(apx_event_t *event, apx_log_level_t level, char *label, adt_str_t *msg);
void apx_event_unpack_log_write(apx_event_t const *event, apx_log_level_t *level, char **label, adt_str_t **msg);
void apx_event_pack_protocol_header_accepted(apx_event_t *event, struct apx_connection_base_tag *connection);
void apx_event_unpack_protocol_header_accepted(apx_event_t const *event, struct apx_connection_base_tag **connection);
void apx_event_pack_remote_file_published(
  apx_event_t *event, struct apx_connection_base_tag *connection, struct rmf_file_info_tag *file_info);
void apx_event_unpack_remote_file_published(
  apx_event_t const *event, struct apx_connection_base_tag **connection, struct rmf_file_info_tag **file_info);
void apx_event_pack_barrier(apx_event_t *event, SEMAPHORE_T *sem);
void apx_event_unpack_barrier(apx_event_t const *event, SEMAPHORE_T **sem);
void apx_event_destroy(apx_event_t *event, soa_t *allocator);

#endif // APX_EVENT_H
