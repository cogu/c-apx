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
#include "apx/connection_base.h"
#include "apx/extension/monitor_session.h"
#include "apx/extension/server_monitor.h"
#include "apx/file.h"
#include "apx/file_manager.h"
#include "apx/file_map.h"
#include "apx/node_data.h"
#include "apx/node_instance.h"
#include "apx/node_manager.h"
#include "apx/server.h"
#include "apx/server_connection.h"
#include "apx_version.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
#endif
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define CHUNK_MAX_SIZE (64u * 1024u)

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static inline void pack_le_u16(uint8_t *p, uint16_t v);
static inline void pack_le_u32(uint8_t *p, uint32_t v);
static inline void pack_le_u64(uint8_t *p, uint64_t v);
static uint32_t encode_string16(uint8_t *dst, const char *str);
static uint64_t get_unix_time_ns(void);
static uint64_t get_monotonic_time_ns(void);
static void generate_stream_id(uint8_t stream_id[16]);
static apx_error_t emit_event_direct(apx_log_streamer_t *self, uint16_t event_type, uint16_t event_flags,
  uint64_t sequence, uint64_t timestamp_ns, uint32_t connection_id, uint32_t object_id, const uint8_t *payload,
  uint32_t payload_size);
static void enqueue_event_entry(apx_log_streamer_t *self, uint16_t event_type, uint16_t event_flags,
  uint32_t connection_id, uint32_t object_id, uint8_t *payload, uint32_t payload_size);
static uint32_t find_or_register_file_id(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address);
static void remove_file_id(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address);
static void remove_connection_files(apx_log_streamer_t *self, uint32_t connection_id);
static void generate_snapshot(apx_log_streamer_t *self);

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
    self->monotonic_origin_ns = 0;

    self->queue_capacity = APX_LOG_DEFAULT_QUEUE_CAPACITY;
    self->queue = (apx_log_event_entry_t *)calloc(self->queue_capacity, sizeof(apx_log_event_entry_t));
    self->queue_count = 0;
    self->write_idx = 0;
    self->read_idx = 0;
    self->drop_first_sequence = 0;
    self->drop_count = 0;
    MUTEX_INIT(self->lock);

    self->next_file_id = 0;
    self->next_node_id = 0;
    adt_ary_create(&self->file_entries, free);
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
    MUTEX_LOCK(self->lock);
    for (uint32_t i = 0; i < self->queue_count; ++i)
    {
      uint32_t idx = (self->read_idx + i) % self->queue_capacity;
      if (self->queue[idx].payload != NULL)
      {
        free(self->queue[idx].payload);
        self->queue[idx].payload = NULL;
      }
    }
    if (self->queue != NULL)
    {
      free(self->queue);
      self->queue = NULL;
    }
    self->queue_count = 0;
    MUTEX_UNLOCK(self->lock);

    adt_ary_destroy(&self->file_entries);
    MUTEX_DESTROY(self->lock);
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
  if (self == NULL || self->session == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!self->is_active)
  {
    MUTEX_LOCK(self->lock);
    self->is_active = true;
    self->sequence = 0;
    self->drop_count = 0;
    self->drop_first_sequence = 0;
    self->next_file_id = 0;
    self->next_node_id = 0;
    generate_stream_id(self->stream_id);
    self->stream_start_unix_ns = get_unix_time_ns();
    self->monotonic_origin_ns = get_monotonic_time_ns();
    MUTEX_UNLOCK(self->lock);

    // Notify server monitor that this session is now actively recording
    apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
    if (monitor != NULL)
    {
      apx_server_monitor_on_log_stream_started(monitor, self->session);
    }

    // 1. Emit STREAM_BEGIN (sequence 0, timestamp 0)
    uint32_t version_len = (uint32_t)strlen(APX_VERSION_STRING);
    uint32_t begin_payload_size = 2u + 2u + 2u + 2u + 8u + 16u + 2u + version_len;
    uint8_t *begin_payload = (uint8_t *)malloc(begin_payload_size);
    if (begin_payload != NULL)
    {
      pack_le_u16(begin_payload + 0, 1u); // format_major = 1
      pack_le_u16(begin_payload + 2, 0u); // format_minor = 0
      pack_le_u16(begin_payload + 4, 1u); // clock_id = 1 (monotonic)
      pack_le_u16(begin_payload + 6, 0u); // reserved = 0
      pack_le_u64(begin_payload + 8, self->stream_start_unix_ns);
      memcpy(begin_payload + 16, self->stream_id, 16);
      encode_string16(begin_payload + 32, APX_VERSION_STRING);

      emit_event_direct(self, APX_LOG_EVENT_STREAM_BEGIN, 0u, 0ULL, 0ULL, 0u, 0u, begin_payload, begin_payload_size);
      free(begin_payload);
    }

    // 2. Generate and emit topology & file state snapshot
    generate_snapshot(self);

    // 3. Emit SNAPSHOT_END
    uint64_t snap_end_seq;
    MUTEX_LOCK(self->lock);
    snap_end_seq = ++self->sequence;
    MUTEX_UNLOCK(self->lock);

    uint64_t now_ns = get_monotonic_time_ns();
    uint64_t rel_ns = now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;
    emit_event_direct(self, APX_LOG_EVENT_SNAPSHOT_END, 0u, snap_end_seq, rel_ns, 0u, 0u, NULL, 0u);

    // 4. Drain any live events queued during snapshot generation
    apx_log_streamer_drain(self);
  }
  return APX_NO_ERROR;
}

void apx_log_streamer_stop(apx_log_streamer_t *self)
{
  if ((self != NULL) && self->is_active)
  {
    // Emit STREAM_END before stopping
    uint64_t end_seq;
    MUTEX_LOCK(self->lock);
    end_seq = ++self->sequence;
    MUTEX_UNLOCK(self->lock);

    uint8_t payload[4];
    pack_le_u32(payload, (uint32_t)APX_NO_ERROR);
    uint64_t now_ns = get_monotonic_time_ns();
    uint64_t rel_ns = now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;
    emit_event_direct(self, APX_LOG_EVENT_STREAM_END, 0u, end_seq, rel_ns, 0u, 0u, payload, sizeof(payload));

    MUTEX_LOCK(self->lock);
    self->is_active = false;
    MUTEX_UNLOCK(self->lock);

    apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
    if (monitor != NULL)
    {
      apx_server_monitor_on_log_stream_stopped(monitor, self->session);
    }
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

apx_error_t apx_log_streamer_enqueue_connection_create(apx_log_streamer_t *self, uint32_t connection_id,
  uint8_t connection_type, uint8_t connection_state, const char *tag, const char *endpoint)
{
  if (self == NULL || !self->is_active)
  {
    return APX_NO_ERROR;
  }
  uint32_t tag_len = tag != NULL ? (uint32_t)strlen(tag) : 0u;
  uint32_t endpoint_len = endpoint != NULL ? (uint32_t)strlen(endpoint) : 0u;
  uint32_t payload_size = 1u + 1u + 2u + 2u + tag_len + 2u + endpoint_len;

  uint8_t *payload = (uint8_t *)malloc(payload_size);
  if (payload == NULL)
  {
    return APX_MEM_ERROR;
  }
  payload[0] = connection_type;
  payload[1] = connection_state;
  pack_le_u16(payload + 2, 0u); // reserved
  uint32_t offset = 4u;
  offset += encode_string16(payload + offset, tag);
  encode_string16(payload + offset, endpoint);

  enqueue_event_entry(self, APX_LOG_EVENT_CONNECTION_CREATE, 0u, connection_id, 0u, payload, payload_size);
  return APX_NO_ERROR;
}

apx_error_t apx_log_streamer_enqueue_connection_close(apx_log_streamer_t *self, uint32_t connection_id, uint32_t reason)
{
  if (self == NULL || !self->is_active)
  {
    return APX_NO_ERROR;
  }
  uint8_t *payload = (uint8_t *)malloc(4u);
  if (payload == NULL)
  {
    return APX_MEM_ERROR;
  }
  pack_le_u32(payload, reason);

  remove_connection_files(self, connection_id);
  enqueue_event_entry(self, APX_LOG_EVENT_CONNECTION_CLOSE, 0u, connection_id, 0u, payload, 4u);
  return APX_NO_ERROR;
}

apx_error_t apx_log_streamer_enqueue_node_attach(
  apx_log_streamer_t *self, uint32_t connection_id, uint32_t node_id, const char *node_name)
{
  if (self == NULL || !self->is_active)
  {
    return APX_NO_ERROR;
  }
  uint32_t name_len = node_name != NULL ? (uint32_t)strlen(node_name) : 0u;
  uint32_t payload_size = 2u + name_len;
  uint8_t *payload = (uint8_t *)malloc(payload_size);
  if (payload == NULL)
  {
    return APX_MEM_ERROR;
  }
  encode_string16(payload, node_name);

  enqueue_event_entry(self, APX_LOG_EVENT_NODE_ATTACH, 0u, connection_id, node_id, payload, payload_size);
  return APX_NO_ERROR;
}

apx_error_t apx_log_streamer_enqueue_node_detach(apx_log_streamer_t *self, uint32_t connection_id, uint32_t node_id)
{
  if (self == NULL || !self->is_active)
  {
    return APX_NO_ERROR;
  }
  enqueue_event_entry(self, APX_LOG_EVENT_NODE_DETACH, 0u, connection_id, node_id, NULL, 0u);
  return APX_NO_ERROR;
}

apx_error_t apx_log_streamer_enqueue_file_publish(
  apx_log_streamer_t *self, uint32_t connection_id, const struct rmf_file_info_tag *file_info)
{
  if (self == NULL || !self->is_active || file_info == NULL)
  {
    return APX_NO_ERROR;
  }
  const char *fname = rmf_file_info_name(file_info);
  uint32_t file_id = find_or_register_file_id(self, connection_id, file_info->address);
  uint32_t name_len = fname != NULL ? (uint32_t)strlen(fname) : 0u;
  uint32_t payload_size = 4u + 4u + 2u + 2u + 32u + 2u + name_len;

  uint8_t *payload = (uint8_t *)malloc(payload_size);
  if (payload == NULL)
  {
    return APX_MEM_ERROR;
  }
  pack_le_u32(payload + 0, file_info->address);
  pack_le_u32(payload + 4, file_info->size);
  pack_le_u16(payload + 8, (uint16_t)file_info->rmf_file_type);
  pack_le_u16(payload + 10, (uint16_t)file_info->digest_type);
  memcpy(payload + 12, file_info->digest_data, 32);
  encode_string16(payload + 44, fname);

  enqueue_event_entry(self, APX_LOG_EVENT_FILE_PUBLISH, 0u, connection_id, file_id, payload, payload_size);
  return APX_NO_ERROR;
}

apx_error_t apx_log_streamer_enqueue_file_revoke(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address)
{
  if (self == NULL || !self->is_active)
  {
    return APX_NO_ERROR;
  }
  uint32_t file_id = find_or_register_file_id(self, connection_id, address);
  remove_file_id(self, connection_id, address);
  enqueue_event_entry(self, APX_LOG_EVENT_FILE_REVOKE, 0u, connection_id, file_id, NULL, 0u);
  return APX_NO_ERROR;
}

apx_error_t apx_log_streamer_enqueue_file_open(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address)
{
  if (self == NULL || !self->is_active)
  {
    return APX_NO_ERROR;
  }
  uint32_t file_id = find_or_register_file_id(self, connection_id, address);
  enqueue_event_entry(self, APX_LOG_EVENT_FILE_OPEN, 0u, connection_id, file_id, NULL, 0u);
  return APX_NO_ERROR;
}

apx_error_t apx_log_streamer_enqueue_file_close(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address)
{
  if (self == NULL || !self->is_active)
  {
    return APX_NO_ERROR;
  }
  uint32_t file_id = find_or_register_file_id(self, connection_id, address);
  enqueue_event_entry(self, APX_LOG_EVENT_FILE_CLOSE, 0u, connection_id, file_id, NULL, 0u);
  return APX_NO_ERROR;
}

apx_error_t apx_log_streamer_enqueue_file_write(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address,
  uint32_t offset, const uint8_t *data, uint32_t size)
{
  if (self == NULL || !self->is_active || data == NULL || size == 0u)
  {
    return APX_NO_ERROR;
  }
  uint32_t file_id = find_or_register_file_id(self, connection_id, address);

  // Chunk writes if size exceeds CHUNK_MAX_SIZE
  uint32_t remaining = size;
  uint32_t cur_offset = offset;
  uint32_t data_idx = 0u;

  while (remaining > 0u)
  {
    uint32_t chunk_size = remaining > CHUNK_MAX_SIZE ? CHUNK_MAX_SIZE : remaining;
    uint32_t payload_size = 4u + chunk_size;
    uint8_t *payload = (uint8_t *)malloc(payload_size);
    if (payload == NULL)
    {
      return APX_MEM_ERROR;
    }
    pack_le_u32(payload + 0, cur_offset);
    memcpy(payload + 4, data + data_idx, chunk_size);

    enqueue_event_entry(self, APX_LOG_EVENT_FILE_WRITE, 0u, connection_id, file_id, payload, payload_size);

    cur_offset += chunk_size;
    data_idx += chunk_size;
    remaining -= chunk_size;
  }
  return APX_NO_ERROR;
}

void apx_log_streamer_drain(apx_log_streamer_t *self)
{
  if (self == NULL || !self->is_active || self->session == NULL)
  {
    return;
  }
  apx_file_manager_t *fm = apx_monitor_session_get_file_manager(self->session);
  if (fm == NULL)
  {
    return;
  }
  apx_file_t *log_file = apx_file_manager_find_local_file_by_name(fm, APX_SERVER_LOG_FILE_NAME);
  if (log_file == NULL || !apx_file_is_open(log_file))
  {
    return;
  }
  uint32_t log_address = apx_file_get_address_without_flags(log_file);

  for (;;)
  {
    apx_log_event_entry_t entry;
    bool has_entry = false;

    MUTEX_LOCK(self->lock);
    if (self->queue_count > 0u)
    {
      entry = self->queue[self->read_idx];
      self->queue[self->read_idx].payload = NULL;
      self->read_idx = (self->read_idx + 1u) % self->queue_capacity;
      self->queue_count--;
      has_entry = true;
    }
    MUTEX_UNLOCK(self->lock);

    if (!has_entry)
    {
      break;
    }

    uint32_t total_size = APX_LOG_HEADER_SIZE + entry.payload_size;
    uint8_t *msg = (uint8_t *)malloc(total_size);
    if (msg != NULL)
    {
      pack_le_u16(msg + 0, entry.event_type);
      pack_le_u16(msg + 2, entry.event_flags);
      pack_le_u64(msg + 4, entry.sequence);
      pack_le_u64(msg + 12, entry.timestamp_ns);
      pack_le_u32(msg + 20, entry.connection_id);
      pack_le_u32(msg + 24, entry.object_id);
      if (entry.payload_size > 0u && entry.payload != NULL)
      {
        memcpy(msg + APX_LOG_HEADER_SIZE, entry.payload, entry.payload_size);
      }
      apx_error_t const send_err = apx_file_manager_send_local_data(fm, log_address, msg, total_size);
      (void)send_err;
    }
    if (entry.payload != NULL)
    {
      free(entry.payload);
    }
  }
}

uint64_t apx_log_streamer_get_sequence(const apx_log_streamer_t *self)
{
  if (self != NULL)
  {
    return self->sequence;
  }
  return 0ULL;
}

uint64_t apx_log_streamer_get_drop_count(const apx_log_streamer_t *self)
{
  if (self != NULL)
  {
    return self->drop_count;
  }
  return 0ULL;
}

uint32_t apx_log_streamer_get_queue_count(const apx_log_streamer_t *self)
{
  if (self != NULL)
  {
    return self->queue_count;
  }
  return 0u;
}

void apx_log_streamer_set_queue_capacity(apx_log_streamer_t *self, uint32_t capacity)
{
  if (self != NULL && capacity > 0u)
  {
    MUTEX_LOCK(self->lock);
    for (uint32_t i = 0; i < self->queue_count; ++i)
    {
      uint32_t idx = (self->read_idx + i) % self->queue_capacity;
      if (self->queue[idx].payload != NULL)
      {
        free(self->queue[idx].payload);
      }
    }
    free(self->queue);
    self->queue = (apx_log_event_entry_t *)calloc(capacity, sizeof(apx_log_event_entry_t));
    self->queue_capacity = capacity;
    self->queue_count = 0u;
    self->write_idx = 0u;
    self->read_idx = 0u;
    MUTEX_UNLOCK(self->lock);
  }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static inline void pack_le_u16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8u) & 0xFFu);
}

static inline void pack_le_u32(uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8u) & 0xFFu);
  p[2] = (uint8_t)((v >> 16u) & 0xFFu);
  p[3] = (uint8_t)((v >> 24u) & 0xFFu);
}

static inline void pack_le_u64(uint8_t *p, uint64_t v)
{
  pack_le_u32(p, (uint32_t)(v & 0xFFFFFFFFu));
  pack_le_u32(p + 4, (uint32_t)((v >> 32u) & 0xFFFFFFFFu));
}

static uint32_t encode_string16(uint8_t *dst, const char *str)
{
  if (str == NULL)
  {
    pack_le_u16(dst, 0u);
    return 2u;
  }
  size_t len = strlen(str);
  if (len > 0xFFFFu)
  {
    len = 0xFFFFu;
  }
  pack_le_u16(dst, (uint16_t)len);
  if (len > 0u)
  {
    memcpy(dst + 2, str, len); // NOLINT(bugprone-not-null-terminated-result)
  }
  return 2u + (uint32_t)len;
}

static uint64_t get_unix_time_ns(void)
{
#ifdef _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  ULARGE_INTEGER uli;
  uli.LowPart = ft.dwLowDateTime;
  uli.HighPart = ft.dwHighDateTime;
  if (uli.QuadPart >= 116444736000000000ULL)
  {
    return (uli.QuadPart - 116444736000000000ULL) * 100ULL;
  }
  return 0ULL;
#else
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == 0)
  {
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
  }
  return 0ULL;
#endif
}

static uint64_t get_monotonic_time_ns(void)
{
#ifdef _WIN32
  LARGE_INTEGER freq;
  LARGE_INTEGER count;
  if (QueryPerformanceFrequency(&freq) && (freq.QuadPart > 0) && QueryPerformanceCounter(&count))
  {
    uint64_t const sec = (uint64_t)count.QuadPart / (uint64_t)freq.QuadPart;
    uint64_t const rem = (uint64_t)count.QuadPart % (uint64_t)freq.QuadPart;
    return (sec * 1000000000ULL) + ((rem * 1000000000ULL) / (uint64_t)freq.QuadPart);
  }
  return (uint64_t)GetTickCount64() * 1000000ULL;
#else
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
  {
    return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
  }
  return 0ULL;
#endif
}

static void generate_stream_id(uint8_t stream_id[16])
{
  FILE *f = fopen("/dev/urandom", "rb");
  if (f != NULL)
  {
    size_t read_bytes = fread(stream_id, 1, 16, f);
    fclose(f);
    if (read_bytes == 16u)
    {
      return;
    }
  }
  uint64_t t = get_monotonic_time_ns() ^ get_unix_time_ns();
  for (int i = 0; i < 16; ++i)
  {
    t = (t * 6364136223846793005ULL) + 1ULL;
    stream_id[i] = (uint8_t)(t >> 32);
  }
}

static apx_error_t emit_event_direct(apx_log_streamer_t *self, uint16_t event_type, uint16_t event_flags,
  uint64_t sequence, uint64_t timestamp_ns, uint32_t connection_id, uint32_t object_id, const uint8_t *payload,
  uint32_t payload_size)
{
  if (self == NULL || self->session == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  apx_file_manager_t *fm = apx_monitor_session_get_file_manager(self->session);
  if (fm == NULL)
  {
    return APX_NOT_CONNECTED_ERROR;
  }
  apx_file_t *log_file = apx_file_manager_find_local_file_by_name(fm, APX_SERVER_LOG_FILE_NAME);
  if (log_file == NULL || !apx_file_is_open(log_file))
  {
    return APX_FILE_NOT_OPEN_ERROR;
  }
  uint32_t log_address = apx_file_get_address_without_flags(log_file);

  uint32_t total_size = APX_LOG_HEADER_SIZE + payload_size;
  uint8_t *msg = (uint8_t *)malloc(total_size);
  if (msg == NULL)
  {
    return APX_MEM_ERROR;
  }
  pack_le_u16(msg + 0, event_type);
  pack_le_u16(msg + 2, event_flags);
  pack_le_u64(msg + 4, sequence);
  pack_le_u64(msg + 12, timestamp_ns);
  pack_le_u32(msg + 20, connection_id);
  pack_le_u32(msg + 24, object_id);
  if (payload_size > 0u && payload != NULL)
  {
    memcpy(msg + APX_LOG_HEADER_SIZE, payload, payload_size);
  }
  apx_error_t const send_err = apx_file_manager_send_local_data(fm, log_address, msg, total_size);
  return send_err;
}

static void enqueue_event_entry(apx_log_streamer_t *self, uint16_t event_type, uint16_t event_flags,
  uint32_t connection_id, uint32_t object_id, uint8_t *payload, uint32_t payload_size)
{
  assert(self != NULL);
  MUTEX_LOCK(self->lock);

  // Check if buffer is full
  if (self->queue_count >= self->queue_capacity)
  {
    uint64_t seq = ++self->sequence;
    if (self->drop_count == 0ULL)
    {
      self->drop_first_sequence = seq;
    }
    self->drop_count++;
    if (payload != NULL)
    {
      free(payload);
    }
    MUTEX_UNLOCK(self->lock);
    return;
  }

  // If there was an overflow window and space is now available, emit EVENTS_DROPPED first
  if (self->drop_count > 0ULL)
  {
    if (self->queue_count + 1u >= self->queue_capacity)
    {
      // No room for both EVENTS_DROPPED and the new event, drop this event too
      ++self->sequence;
      self->drop_count++;
      if (payload != NULL)
      {
        free(payload);
      }
      MUTEX_UNLOCK(self->lock);
      return;
    }

    uint64_t drop_seq = ++self->sequence;
    uint64_t now_ns = get_monotonic_time_ns();
    uint64_t rel_ns = now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;
    uint8_t *drop_payload = (uint8_t *)malloc(16u);
    if (drop_payload != NULL)
    {
      pack_le_u64(drop_payload + 0, self->drop_first_sequence);
      pack_le_u64(drop_payload + 8, self->drop_count);

      apx_log_event_entry_t *drop_entry = &self->queue[self->write_idx];
      drop_entry->event_type = APX_LOG_EVENT_EVENTS_DROPPED;
      drop_entry->event_flags = 0u;
      drop_entry->sequence = drop_seq;
      drop_entry->timestamp_ns = rel_ns;
      drop_entry->connection_id = 0u;
      drop_entry->object_id = 0u;
      drop_entry->payload = drop_payload;
      drop_entry->payload_size = 16u;

      self->write_idx = (self->write_idx + 1u) % self->queue_capacity;
      self->queue_count++;
    }
    self->drop_count = 0ULL;
  }

  // Enqueue new event
  uint64_t seq = ++self->sequence;
  uint64_t now_ns = get_monotonic_time_ns();
  uint64_t rel_ns = now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;

  apx_log_event_entry_t *entry = &self->queue[self->write_idx];
  entry->event_type = event_type;
  entry->event_flags = event_flags;
  entry->sequence = seq;
  entry->timestamp_ns = rel_ns;
  entry->connection_id = connection_id;
  entry->object_id = object_id;
  entry->payload = payload;
  entry->payload_size = payload_size;

  self->write_idx = (self->write_idx + 1u) % self->queue_capacity;
  self->queue_count++;

  MUTEX_UNLOCK(self->lock);

  apx_log_streamer_drain(self);
}

static uint32_t find_or_register_file_id(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address)
{
  assert(self != NULL);
  MUTEX_LOCK(self->lock);
  int32_t const len = adt_ary_length(&self->file_entries);
  for (int32_t i = 0; i < len; ++i)
  {
    apx_log_file_entry_t const *entry = (apx_log_file_entry_t const *)adt_ary_value(&self->file_entries, i);
    if ((entry != NULL) && (entry->connection_id == connection_id) && (entry->address == address))
    {
      uint32_t id = entry->file_id;
      MUTEX_UNLOCK(self->lock);
      return id;
    }
  }

  apx_log_file_entry_t *new_entry = (apx_log_file_entry_t *)malloc(sizeof(apx_log_file_entry_t));
  if (new_entry != NULL)
  {
    new_entry->connection_id = connection_id;
    new_entry->address = address;
    new_entry->file_id = ++self->next_file_id;
    adt_ary_push(&self->file_entries, new_entry);
    uint32_t id = new_entry->file_id;
    MUTEX_UNLOCK(self->lock);
    return id;
  }
  MUTEX_UNLOCK(self->lock);
  return 0u;
}

static void remove_file_id(apx_log_streamer_t *self, uint32_t connection_id, uint32_t address)
{
  assert(self != NULL);
  MUTEX_LOCK(self->lock);
  int32_t const len = adt_ary_length(&self->file_entries);
  for (int32_t i = 0; i < len; ++i)
  {
    apx_log_file_entry_t const *entry = (apx_log_file_entry_t const *)adt_ary_value(&self->file_entries, i);
    if ((entry != NULL) && (entry->connection_id == connection_id) && (entry->address == address))
    {
      adt_ary_splice(&self->file_entries, i, 1);
      break;
    }
  }
  MUTEX_UNLOCK(self->lock);
}

static void remove_connection_files(apx_log_streamer_t *self, uint32_t connection_id)
{
  assert(self != NULL);
  MUTEX_LOCK(self->lock);
  int32_t i = 0;
  while (i < adt_ary_length(&self->file_entries))
  {
    apx_log_file_entry_t const *entry = (apx_log_file_entry_t const *)adt_ary_value(&self->file_entries, i);
    if ((entry != NULL) && (entry->connection_id == connection_id))
    {
      adt_ary_splice(&self->file_entries, i, 1);
    }
    else
    {
      ++i;
    }
  }
  MUTEX_UNLOCK(self->lock);
}

static void generate_snapshot(apx_log_streamer_t *self)
{
  assert(self != NULL && self->session != NULL);
  apx_server_monitor_t *monitor = apx_monitor_session_get_server_monitor(self->session);
  if (monitor == NULL || monitor->server == NULL)
  {
    return;
  }
  apx_server_t *server = monitor->server;

  // Iterate over active connections in the server
  adt_list_t const *connections = &server->connection_manager.active_connections;
  adt_list_elem_t *iter = adt_list_iter_first(connections);
  while (iter != NULL)
  {
    apx_server_connection_t *conn = (apx_server_connection_t *)iter->pItem;
    iter = adt_list_iter_next(iter);
    if (conn == NULL || apx_server_connection_get_connection_type(conn) == APX_CONNECTION_TYPE_MONITOR)
    {
      continue;
    }
    uint32_t conn_id = apx_server_connection_get_connection_id(conn);

    // 1. Snapshot CONNECTION_CREATE
    const char *tag_str = conn->tag != NULL ? adt_str_cstr(conn->tag) : "";
    uint32_t tag_len = (uint32_t)strlen(tag_str);
    uint32_t conn_payload_size = 1u + 1u + 2u + 2u + tag_len + 2u;
    uint8_t *conn_payload = (uint8_t *)malloc(conn_payload_size);
    if (conn_payload != NULL)
    {
      conn_payload[0] = (uint8_t)apx_server_connection_get_connection_type(conn);
      conn_payload[1] = (uint8_t)conn->connection_state;
      pack_le_u16(conn_payload + 2, 0u);
      uint32_t off = 4u;
      off += encode_string16(conn_payload + off, tag_str);
      encode_string16(conn_payload + off, "");

      uint64_t seq;
      MUTEX_LOCK(self->lock);
      seq = ++self->sequence;
      MUTEX_UNLOCK(self->lock);
      uint64_t now_ns = get_monotonic_time_ns();
      uint64_t rel_ns = now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;
      emit_event_direct(self, APX_LOG_EVENT_CONNECTION_CREATE, APX_LOG_FLAG_SNAPSHOT, seq, rel_ns, conn_id, 0u,
        conn_payload, conn_payload_size);
      free(conn_payload);
    }

    // 2. Snapshot NODE_ATTACH
    apx_node_manager_t *node_mgr = apx_server_connection_get_node_manager(conn);
    if (node_mgr != NULL)
    {
      adt_ary_t *nodes = apx_node_manager_get_nodes(node_mgr);
      if (nodes != NULL)
      {
        int32_t num_nodes = adt_ary_length(nodes);
        for (int32_t n = 0; n < num_nodes; ++n)
        {
          apx_node_instance_t *node = (apx_node_instance_t *)adt_ary_value(nodes, n);
          if (node != NULL)
          {
            uint32_t node_id = ++self->next_node_id;
            const char *node_name = apx_node_instance_get_name(node);
            uint32_t nlen = node_name != NULL ? (uint32_t)strlen(node_name) : 0u;
            uint32_t node_payload_size = 2u + nlen;
            uint8_t *node_payload = (uint8_t *)malloc(node_payload_size);
            if (node_payload != NULL)
            {
              encode_string16(node_payload, node_name);
              uint64_t seq;
              MUTEX_LOCK(self->lock);
              seq = ++self->sequence;
              MUTEX_UNLOCK(self->lock);
              uint64_t now_ns = get_monotonic_time_ns();
              uint64_t rel_ns = now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;
              emit_event_direct(self, APX_LOG_EVENT_NODE_ATTACH, APX_LOG_FLAG_SNAPSHOT, seq, rel_ns, conn_id, node_id,
                node_payload, node_payload_size);
              free(node_payload);
            }
          }
        }
        adt_ary_delete(nodes);
      }
    }

    // 3. Snapshot FILE_PUBLISH and initial values
    apx_file_manager_t *fm = apx_connection_base_get_file_manager(&conn->base);
    if (fm != NULL)
    {
      adt_list_t const *file_lists[] = {
        apx_file_map_get_list(&fm->shared.local_file_map),
        apx_file_map_get_list(&fm->shared.remote_file_map),
      };
      for (size_t fl = 0; fl < 2; ++fl)
      {
        if (file_lists[fl] == NULL)
        {
          continue;
        }
        adt_list_elem_t *fiter = adt_list_iter_first(file_lists[fl]);
        while (fiter != NULL)
        {
          apx_file_t *file = (apx_file_t *)fiter->pItem;
          fiter = adt_list_iter_next(fiter);
          if (file == NULL)
          {
            continue;
          }
          uint32_t file_id = find_or_register_file_id(self, conn_id, file->file_info.address);
          const char *fname = rmf_file_info_name(&file->file_info);
          uint32_t fname_len = fname != NULL ? (uint32_t)strlen(fname) : 0u;
          uint32_t pub_payload_size = 4u + 4u + 2u + 2u + 32u + 2u + fname_len;
          uint8_t *pub_payload = (uint8_t *)malloc(pub_payload_size);
          if (pub_payload != NULL)
          {
            pack_le_u32(pub_payload + 0, file->file_info.address);
            pack_le_u32(pub_payload + 4, file->file_info.size);
            pack_le_u16(pub_payload + 8, (uint16_t)file->file_info.rmf_file_type);
            pack_le_u16(pub_payload + 10, (uint16_t)file->file_info.digest_type);
            memcpy(pub_payload + 12, file->file_info.digest_data, 32);
            encode_string16(pub_payload + 44, fname);

            uint64_t seq;
            MUTEX_LOCK(self->lock);
            seq = ++self->sequence;
            MUTEX_UNLOCK(self->lock);
            uint64_t now_ns = get_monotonic_time_ns();
            uint64_t rel_ns = now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;
            emit_event_direct(self, APX_LOG_EVENT_FILE_PUBLISH, APX_LOG_FLAG_SNAPSHOT, seq, rel_ns, conn_id, file_id,
              pub_payload, pub_payload_size);
            free(pub_payload);
          }

          // Initial values: definition text or provide port data
          if (fname != NULL && node_mgr != NULL)
          {
            adt_ary_t *nodes = apx_node_manager_get_nodes(node_mgr);
            if (nodes != NULL)
            {
              int32_t num_nodes = adt_ary_length(nodes);
              for (int32_t n = 0; n < num_nodes; ++n)
              {
                apx_node_instance_t *node = (apx_node_instance_t *)adt_ary_value(nodes, n);
                if (node == NULL)
                {
                  continue;
                }
                const char *nname = apx_node_instance_get_name(node);
                if (nname == NULL)
                {
                  continue;
                }
                size_t nname_len = strlen(nname);

                // Check if file is node definition (.apx)
                if ((strncmp(fname, nname, nname_len) == 0) && (strcmp(fname + nname_len, ".apx") == 0))
                {
                  uint8_t const *def_data = apx_node_instance_get_definition_data(node);
                  apx_size_t def_size = apx_node_instance_get_definition_size(node);
                  if (def_data != NULL && def_size > 0u)
                  {
                    uint32_t rem = (uint32_t)def_size;
                    uint32_t cur_off = 0u;
                    while (rem > 0u)
                    {
                      uint32_t csize = rem > CHUNK_MAX_SIZE ? CHUNK_MAX_SIZE : rem;
                      uint32_t write_payload_size = 4u + csize;
                      uint8_t *write_payload = (uint8_t *)malloc(write_payload_size);
                      if (write_payload != NULL)
                      {
                        pack_le_u32(write_payload + 0, cur_off);
                        memcpy(write_payload + 4, def_data + cur_off, csize);
                        uint64_t seq;
                        MUTEX_LOCK(self->lock);
                        seq = ++self->sequence;
                        MUTEX_UNLOCK(self->lock);
                        uint64_t now_ns = get_monotonic_time_ns();
                        uint64_t rel_ns =
                          now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;
                        emit_event_direct(self, APX_LOG_EVENT_FILE_WRITE,
                          APX_LOG_FLAG_SNAPSHOT | APX_LOG_FLAG_INITIAL_VALUE, seq, rel_ns, conn_id, file_id,
                          write_payload, write_payload_size);
                        free(write_payload);
                      }
                      cur_off += csize;
                      rem -= csize;
                    }
                  }
                }
                // Check if file is provide port data (.out)
                else if ((strncmp(fname, nname, nname_len) == 0) && (strcmp(fname + nname_len, ".out") == 0))
                {
                  apx_node_data_t *nd = apx_node_instance_get_node_data(node);
                  if (nd != NULL)
                  {
                    apx_size_t snap_size = apx_node_data_provide_port_data_size(nd);
                    if (snap_size > 0u)
                    {
                      uint8_t *snap = apx_node_data_take_provide_port_data_snapshot(nd);
                      if (snap != NULL)
                      {
                        uint32_t write_payload_size = 4u + (uint32_t)snap_size;
                        uint8_t *write_payload = (uint8_t *)malloc(write_payload_size);
                        if (write_payload != NULL)
                        {
                          pack_le_u32(write_payload + 0, 0u);
                          memcpy(write_payload + 4, snap, snap_size);
                          uint64_t seq;
                          MUTEX_LOCK(self->lock);
                          seq = ++self->sequence;
                          MUTEX_UNLOCK(self->lock);
                          uint64_t now_ns = get_monotonic_time_ns();
                          uint64_t rel_ns =
                            now_ns >= self->monotonic_origin_ns ? (now_ns - self->monotonic_origin_ns) : 0ULL;
                          emit_event_direct(self, APX_LOG_EVENT_FILE_WRITE,
                            APX_LOG_FLAG_SNAPSHOT | APX_LOG_FLAG_INITIAL_VALUE, seq, rel_ns, conn_id, file_id,
                            write_payload, write_payload_size);
                          free(write_payload);
                        }
                        free(snap);
                      }
                    }
                  }
                }
              }
            }
            adt_ary_delete(nodes);
          }
        }
      }
    }
  }
}
