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
#include "adt_ary.h"
#include "apx/error.h"
#include "apx/types.h"
#include "osmacro.h"
#include <stdbool.h>
#include <stdint.h>

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
#define APX_LOG_EVENT_EVENTS_DROPPED 0x00FEu
#define APX_LOG_EVENT_STREAM_END 0x00FFu

#define APX_LOG_FLAG_SNAPSHOT 0x0001u
#define APX_LOG_FLAG_INITIAL_VALUE 0x0002u

#define APX_LOG_HEADER_SIZE 28u
#define APX_LOG_DEFAULT_QUEUE_CAPACITY 1024u

struct apx_monitor_session_tag;
struct rmf_file_info_tag;

typedef struct apx_log_event_entry_tag
{
  uint16_t event_type;
  uint16_t event_flags;
  uint64_t sequence;
  uint64_t timestamp_ns;
  uint32_t connection_id;
  uint32_t object_id;
  uint8_t *payload;
  uint32_t payload_size;
} apx_log_event_entry_t;

typedef struct apx_log_file_entry_tag
{
  uint32_t connection_id;
  uint32_t address;
  uint32_t file_id;
} apx_log_file_entry_t;

typedef struct apx_log_streamer_tag
{
  struct apx_monitor_session_tag *session;
  bool is_active;
  uint64_t sequence;
  uint8_t stream_id[16];
  uint64_t stream_start_unix_ns;
  uint64_t monotonic_origin_ns;

  // Queue and drop accounting
  apx_log_event_entry_t *queue;
  uint32_t queue_capacity;
  uint32_t queue_count;
  uint32_t write_idx;
  uint32_t read_idx;
  uint64_t drop_first_sequence;
  uint64_t drop_count;
  MUTEX_T lock;

  // Scoped ID mappings
  uint32_t next_file_id;
  uint32_t next_node_id;
  adt_ary_t file_entries; // Elements of apx_log_file_entry_t*
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

// Producer enqueue methods (called from fast-path tap)
apx_error_t apx_log_streamer_enqueue_connection_create(apx_log_streamer_t *self, uint32_t connection_id,
  uint8_t connection_type, uint8_t connection_state, const char *tag, const char *endpoint);
apx_error_t apx_log_streamer_enqueue_connection_close(
  apx_log_streamer_t *self, uint32_t connection_id, uint32_t reason);
apx_error_t apx_log_streamer_enqueue_node_attach(
  apx_log_streamer_t *self, uint32_t connection_id, uint32_t node_id, const char *node_name);
apx_error_t apx_log_streamer_enqueue_node_detach(apx_log_streamer_t *self, uint32_t connection_id, uint32_t node_id);
apx_error_t apx_log_streamer_enqueue_file_publish(
  apx_log_streamer_t *self, uint32_t connection_id, const struct rmf_file_info_tag *file_info);
apx_error_t apx_log_streamer_enqueue_file_revoke(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address);
apx_error_t apx_log_streamer_enqueue_file_open(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address);
apx_error_t apx_log_streamer_enqueue_file_close(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address);
apx_error_t apx_log_streamer_enqueue_file_write(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address,
  uint32_t offset, const uint8_t *data, uint32_t size);

// Drain queue into server.log
void apx_log_streamer_drain(apx_log_streamer_t *self);

// Inspection for testing
uint64_t apx_log_streamer_get_sequence(const apx_log_streamer_t *self);
uint64_t apx_log_streamer_get_drop_count(const apx_log_streamer_t *self);
uint32_t apx_log_streamer_get_queue_count(const apx_log_streamer_t *self);
void apx_log_streamer_set_queue_capacity(apx_log_streamer_t *self, uint32_t capacity);

#endif // APX_LOG_STREAMER_H
