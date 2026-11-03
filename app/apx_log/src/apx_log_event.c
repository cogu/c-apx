/*****************************************************************************
 * \file      apx_log_event.c
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     Event decoder and formatter implementation for .apxlog records
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "apx_log_event.h"
#include "dtl_json.h"
#include "dtl_type.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static uint16_t unpack_le_u16(const uint8_t *src) { return (uint16_t)(((uint16_t)src[0]) | (((uint16_t)src[1]) << 8)); }

static uint32_t unpack_le_u32(const uint8_t *src)
{
  return ((uint32_t)src[0]) | (((uint32_t)src[1]) << 8) | (((uint32_t)src[2]) << 16) | (((uint32_t)src[3]) << 24);
}

static uint64_t unpack_le_u64(const uint8_t *src)
{
  uint64_t const low = (uint64_t)unpack_le_u32(src + 0);
  uint64_t const high = (uint64_t)unpack_le_u32(src + 4);
  return low | (high << 32);
}

static char *decode_string16(const uint8_t *src, uint32_t max_bytes, uint32_t *consumed)
{
  if (max_bytes < 2u)
  {
    *consumed = 0u;
    return NULL;
  }
  uint16_t const len = unpack_le_u16(src);
  if (2u + (uint32_t)len > max_bytes)
  {
    *consumed = 0u;
    return NULL;
  }
  char *str = (char *)malloc(len + 1u);
  if (str != NULL)
  {
    if (len > 0u)
    {
      memcpy(str, src + 2, len);
    }
    str[len] = '\0';
  }
  *consumed = 2u + len;
  return str;
}

static void format_hex(char *dst, size_t dst_size, const uint8_t *data, uint32_t size, uint32_t max_display)
{
  if (dst_size == 0u)
  {
    return;
  }
  dst[0] = '\0';
  uint32_t show = size < max_display ? size : max_display;
  size_t written = 0;
  for (uint32_t i = 0; i < show; ++i)
  {
    int n = snprintf(dst + written, dst_size - written, i == 0 ? "%02X" : " %02X", data[i]);
    if (n < 0 || (size_t)n >= dst_size - written)
    {
      break;
    }
    written += (size_t)n;
  }
  if (size > max_display && written + 4 < dst_size)
  {
    strcat(dst, " ...");
  }
}

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_log_event_parse(apx_log_event_t *self, const uint8_t *buf, uint32_t size)
{
  if (self == NULL || buf == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (size < APX_LOG_EVENT_HEADER_SIZE)
  {
    return APX_LENGTH_ERROR;
  }

  self->event_type = unpack_le_u16(buf + 0);
  self->event_flags = unpack_le_u16(buf + 2);
  self->sequence = unpack_le_u64(buf + 4);
  self->timestamp_ns = unpack_le_u64(buf + 12);
  self->connection_id = unpack_le_u32(buf + 20);
  self->object_id = unpack_le_u32(buf + 24);
  self->payload_size = size - APX_LOG_EVENT_HEADER_SIZE;
  self->payload = (self->payload_size > 0u) ? (buf + APX_LOG_EVENT_HEADER_SIZE) : NULL;

  return APX_NO_ERROR;
}

const char *apx_log_event_type_str(uint16_t event_type)
{
  switch (event_type)
  {
  case APX_LOG_EVENT_STREAM_BEGIN:
    return "STREAM_BEGIN";
  case APX_LOG_EVENT_SNAPSHOT_END:
    return "SNAPSHOT_END";
  case APX_LOG_EVENT_CONNECTION_CREATE:
    return "CONNECTION_CREATE";
  case APX_LOG_EVENT_CONNECTION_CLOSE:
    return "CONNECTION_CLOSE";
  case APX_LOG_EVENT_NODE_ATTACH:
    return "NODE_ATTACH";
  case APX_LOG_EVENT_NODE_DETACH:
    return "NODE_DETACH";
  case APX_LOG_EVENT_FILE_PUBLISH:
    return "FILE_PUBLISH";
  case APX_LOG_EVENT_FILE_REVOKE:
    return "FILE_REVOKE";
  case APX_LOG_EVENT_FILE_OPEN:
    return "FILE_OPEN";
  case APX_LOG_EVENT_FILE_CLOSE:
    return "FILE_CLOSE";
  case APX_LOG_EVENT_FILE_WRITE:
    return "FILE_WRITE";
  case APX_LOG_EVENT_PROVIDER_CHANGE:
    return "PROVIDER_CHANGE";
  case APX_LOG_EVENT_EVENTS_DROPPED:
    return "EVENTS_DROPPED";
  case APX_LOG_EVENT_STREAM_END:
    return "STREAM_END";
  default:
    return "UNKNOWN";
  }
}

void apx_log_event_print_text(const apx_log_event_t *self, FILE *out)
{
  if (self == NULL || out == NULL)
  {
    return;
  }

  double const secs = (double)self->timestamp_ns / 1000000000.0;
  char flag_str[16] = "";
  if (self->event_flags & APX_LOG_FLAG_SNAPSHOT)
  {
    strcat(flag_str, " [SNAP]");
  }
  if (self->event_flags & APX_LOG_FLAG_INITIAL_VALUE)
  {
    strcat(flag_str, " [INIT]");
  }

  fprintf(
    out, "[+%9.6fs] [seq=%" PRIu64 "]%s %s", secs, self->sequence, flag_str, apx_log_event_type_str(self->event_type));

  switch (self->event_type)
  {
  case APX_LOG_EVENT_STREAM_BEGIN:
    if (self->payload_size >= 32u)
    {
      uint16_t major = unpack_le_u16(self->payload + 0);
      uint16_t minor = unpack_le_u16(self->payload + 2);
      uint64_t wall_ns = unpack_le_u64(self->payload + 8);
      char stream_hex[33];
      for (size_t i = 0; i < 16u; ++i)
      {
        sprintf(stream_hex + (i * 2u), "%02x", self->payload[16 + i]);
      }
      stream_hex[32] = '\0';
      uint32_t consumed = 0;
      char *version = decode_string16(self->payload + 32, self->payload_size - 32, &consumed);
      fprintf(out, " v%u.%u wall=%" PRIu64 " server=%s stream_id=%s", major, minor, wall_ns, version ? version : "",
        stream_hex);
      if (version != NULL)
      {
        free(version);
      }
    }
    break;

  case APX_LOG_EVENT_SNAPSHOT_END:
    break;

  case APX_LOG_EVENT_CONNECTION_CREATE:
    if (self->payload_size >= 4u)
    {
      uint8_t conn_type = self->payload[0];
      uint8_t conn_state = self->payload[1];
      uint32_t offset = 4u;
      uint32_t consumed = 0;
      char *tag = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      offset += consumed;
      char *endpoint = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      fprintf(out, " conn=%u type=%u state=%u tag=\"%s\" endpoint=\"%s\"", self->connection_id, conn_type, conn_state,
        tag ? tag : "", endpoint ? endpoint : "");
      if (tag != NULL)
      {
        free(tag);
      }
      if (endpoint != NULL)
      {
        free(endpoint);
      }
    }
    break;

  case APX_LOG_EVENT_CONNECTION_CLOSE:
    if (self->payload_size >= 4u)
    {
      uint32_t reason = unpack_le_u32(self->payload + 0);
      fprintf(out, " conn=%u reason=%u", self->connection_id, reason);
    }
    break;

  case APX_LOG_EVENT_NODE_ATTACH:
    if (self->payload_size >= 2u)
    {
      uint32_t consumed = 0;
      char *name = decode_string16(self->payload, self->payload_size, &consumed);
      fprintf(out, " conn=%u node_id=%u node=\"%s\"", self->connection_id, self->object_id, name ? name : "");
      if (name != NULL)
      {
        free(name);
      }
    }
    break;

  case APX_LOG_EVENT_NODE_DETACH:
    fprintf(out, " conn=%u node_id=%u", self->connection_id, self->object_id);
    break;

  case APX_LOG_EVENT_FILE_PUBLISH:
    if (self->payload_size >= 44u)
    {
      uint32_t addr = unpack_le_u32(self->payload + 0);
      uint32_t fsize = unpack_le_u32(self->payload + 4);
      uint16_t ftype = unpack_le_u16(self->payload + 8);
      uint32_t consumed = 0;
      char *fname = decode_string16(self->payload + 44, self->payload_size - 44, &consumed);
      fprintf(out, " conn=%u file_id=%u addr=0x%08X size=%u type=%u name=\"%s\"", self->connection_id, self->object_id,
        addr, fsize, ftype, fname ? fname : "");
      if (fname != NULL)
      {
        free(fname);
      }
    }
    break;

  case APX_LOG_EVENT_FILE_REVOKE:
  case APX_LOG_EVENT_FILE_OPEN:
  case APX_LOG_EVENT_FILE_CLOSE:
    fprintf(out, " conn=%u file_id=%u", self->connection_id, self->object_id);
    break;

  case APX_LOG_EVENT_FILE_WRITE:
    if (self->payload_size >= 4u)
    {
      uint32_t offset = unpack_le_u32(self->payload + 0);
      uint32_t data_len = self->payload_size - 4u;
      char hex_buf[64];
      format_hex(hex_buf, sizeof(hex_buf), self->payload + 4, data_len, 16u);
      fprintf(out, " conn=%u file_id=%u offset=%u size=%u [%s]", self->connection_id, self->object_id, offset, data_len,
        hex_buf);
    }
    break;

  case APX_LOG_EVENT_PROVIDER_CHANGE:
    if (self->payload_size >= 12u)
    {
      uint32_t old_conn_id = unpack_le_u32(self->payload + 0);
      uint32_t offset = 12u;
      uint32_t consumed = 0;
      char *old_node = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      offset += consumed;
      char *new_node = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      offset += consumed;
      char *port_name = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      fprintf(out, " %u:%s/%s -> %u:%s/%s", old_conn_id, old_node ? old_node : "", port_name ? port_name : "",
        self->connection_id, new_node ? new_node : "", port_name ? port_name : "");
      if (old_node != NULL)
      {
        free(old_node);
      }
      if (new_node != NULL)
      {
        free(new_node);
      }
      if (port_name != NULL)
      {
        free(port_name);
      }
    }
    break;

  case APX_LOG_EVENT_EVENTS_DROPPED:
    if (self->payload_size >= 16u)
    {
      uint64_t first_seq = unpack_le_u64(self->payload + 0);
      uint64_t dropped = unpack_le_u64(self->payload + 8);
      fprintf(out, " first_seq=%" PRIu64 " dropped=%" PRIu64, first_seq, dropped);
    }
    break;

  case APX_LOG_EVENT_STREAM_END:
    if (self->payload_size >= 4u)
    {
      uint32_t reason = unpack_le_u32(self->payload + 0);
      fprintf(out, " reason=%u", reason);
    }
    break;

  default:
    fprintf(out, " payload_size=%u", self->payload_size);
    break;
  }

  fprintf(out, "\n");
}

void apx_log_event_print_json(const apx_log_event_t *self, FILE *out)
{
  if (self == NULL || out == NULL)
  {
    return;
  }

  dtl_hv_t *hv = dtl_hv_new();
  if (hv == NULL)
  {
    return;
  }

  dtl_hv_set_cstr(hv, "event", (dtl_dv_t *)dtl_sv_make_cstr(apx_log_event_type_str(self->event_type)), false);
  dtl_hv_set_cstr(hv, "seq", (dtl_dv_t *)dtl_sv_make_u64(self->sequence), false);
  dtl_hv_set_cstr(hv, "timestamp_ns", (dtl_dv_t *)dtl_sv_make_u64(self->timestamp_ns), false);

  if (self->event_flags != 0u)
  {
    dtl_av_t *flags_av = dtl_av_new();
    if (self->event_flags & APX_LOG_FLAG_SNAPSHOT)
    {
      dtl_av_push(flags_av, (dtl_dv_t *)dtl_sv_make_cstr("SNAPSHOT"), false);
    }
    if (self->event_flags & APX_LOG_FLAG_INITIAL_VALUE)
    {
      dtl_av_push(flags_av, (dtl_dv_t *)dtl_sv_make_cstr("INITIAL_VALUE"), false);
    }
    dtl_hv_set_cstr(hv, "flags", (dtl_dv_t *)flags_av, false);
  }

  if (self->connection_id != 0u)
  {
    dtl_hv_set_cstr(hv, "connection_id", (dtl_dv_t *)dtl_sv_make_u32(self->connection_id), false);
  }
  if (self->object_id != 0u)
  {
    dtl_hv_set_cstr(hv, "object_id", (dtl_dv_t *)dtl_sv_make_u32(self->object_id), false);
  }

  switch (self->event_type)
  {
  case APX_LOG_EVENT_STREAM_BEGIN:
    if (self->payload_size >= 32u)
    {
      uint16_t major = unpack_le_u16(self->payload + 0);
      uint16_t minor = unpack_le_u16(self->payload + 2);
      char ver[16];
      snprintf(ver, sizeof(ver), "%u.%u", major, minor);
      dtl_hv_set_cstr(hv, "format", (dtl_dv_t *)dtl_sv_make_cstr(ver), false);
      dtl_hv_set_cstr(hv, "wall_unix_ns", (dtl_dv_t *)dtl_sv_make_u64(unpack_le_u64(self->payload + 8)), false);
      char stream_hex[33];
      for (size_t i = 0; i < 16u; ++i)
      {
        sprintf(stream_hex + (i * 2u), "%02x", self->payload[16 + i]);
      }
      stream_hex[32] = '\0';
      dtl_hv_set_cstr(hv, "stream_id", (dtl_dv_t *)dtl_sv_make_cstr(stream_hex), false);
      uint32_t consumed = 0;
      char *version = decode_string16(self->payload + 32, self->payload_size - 32, &consumed);
      dtl_hv_set_cstr(hv, "server_version", (dtl_dv_t *)dtl_sv_make_cstr(version ? version : ""), false);
      if (version != NULL)
      {
        free(version);
      }
    }
    break;

  case APX_LOG_EVENT_CONNECTION_CREATE:
    if (self->payload_size >= 4u)
    {
      dtl_hv_set_cstr(hv, "connection_type", (dtl_dv_t *)dtl_sv_make_u32((uint32_t)self->payload[0]), false);
      dtl_hv_set_cstr(hv, "connection_state", (dtl_dv_t *)dtl_sv_make_u32((uint32_t)self->payload[1]), false);
      uint32_t offset = 4u;
      uint32_t consumed = 0;
      char *tag = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      offset += consumed;
      char *endpoint = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      dtl_hv_set_cstr(hv, "tag", (dtl_dv_t *)dtl_sv_make_cstr(tag ? tag : ""), false);
      dtl_hv_set_cstr(hv, "endpoint", (dtl_dv_t *)dtl_sv_make_cstr(endpoint ? endpoint : ""), false);
      if (tag != NULL)
      {
        free(tag);
      }
      if (endpoint != NULL)
      {
        free(endpoint);
      }
    }
    break;

  case APX_LOG_EVENT_CONNECTION_CLOSE:
    if (self->payload_size >= 4u)
    {
      dtl_hv_set_cstr(hv, "reason", (dtl_dv_t *)dtl_sv_make_u32(unpack_le_u32(self->payload + 0)), false);
    }
    break;

  case APX_LOG_EVENT_NODE_ATTACH:
    if (self->payload_size >= 2u)
    {
      uint32_t consumed = 0;
      char *name = decode_string16(self->payload, self->payload_size, &consumed);
      dtl_hv_set_cstr(hv, "node_name", (dtl_dv_t *)dtl_sv_make_cstr(name ? name : ""), false);
      if (name != NULL)
      {
        free(name);
      }
    }
    break;

  case APX_LOG_EVENT_FILE_PUBLISH:
    if (self->payload_size >= 44u)
    {
      dtl_hv_set_cstr(hv, "remote_address", (dtl_dv_t *)dtl_sv_make_u32(unpack_le_u32(self->payload + 0)), false);
      dtl_hv_set_cstr(hv, "file_size", (dtl_dv_t *)dtl_sv_make_u32(unpack_le_u32(self->payload + 4)), false);
      dtl_hv_set_cstr(hv, "file_type", (dtl_dv_t *)dtl_sv_make_u32((uint32_t)unpack_le_u16(self->payload + 8)), false);
      uint32_t consumed = 0;
      char *fname = decode_string16(self->payload + 44, self->payload_size - 44, &consumed);
      dtl_hv_set_cstr(hv, "file_name", (dtl_dv_t *)dtl_sv_make_cstr(fname ? fname : ""), false);
      if (fname != NULL)
      {
        free(fname);
      }
    }
    break;

  case APX_LOG_EVENT_FILE_WRITE:
    if (self->payload_size >= 4u)
    {
      uint32_t offset = unpack_le_u32(self->payload + 0);
      uint32_t data_len = self->payload_size - 4u;
      dtl_hv_set_cstr(hv, "file_offset", (dtl_dv_t *)dtl_sv_make_u32(offset), false);
      dtl_hv_set_cstr(hv, "data_size", (dtl_dv_t *)dtl_sv_make_u32(data_len), false);
      char *hex = (char *)malloc((size_t)data_len * 2u + 1u);
      if (hex != NULL)
      {
        for (uint32_t i = 0; i < data_len; ++i)
        {
          sprintf(hex + ((size_t)i * 2u), "%02x", self->payload[4 + i]);
        }
        hex[(size_t)data_len * 2u] = '\0';
        dtl_hv_set_cstr(hv, "data_hex", (dtl_dv_t *)dtl_sv_make_cstr(hex), false);
        free(hex);
      }
    }
    break;

  case APX_LOG_EVENT_PROVIDER_CHANGE:
    if (self->payload_size >= 12u)
    {
      uint32_t old_conn_id = unpack_le_u32(self->payload + 0);
      uint32_t old_port_id = unpack_le_u32(self->payload + 4);
      uint8_t reason = self->payload[8];
      uint8_t policy = self->payload[9];
      uint32_t offset = 12u;
      uint32_t consumed = 0;
      char *old_node = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      offset += consumed;
      char *new_node = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      offset += consumed;
      char *port_name = decode_string16(self->payload + offset, self->payload_size - offset, &consumed);
      dtl_hv_set_cstr(hv, "old_connection_id", (dtl_dv_t *)dtl_sv_make_u32(old_conn_id), false);
      dtl_hv_set_cstr(hv, "old_port_id", (dtl_dv_t *)dtl_sv_make_u32(old_port_id), false);
      dtl_hv_set_cstr(hv, "new_connection_id", (dtl_dv_t *)dtl_sv_make_u32(self->connection_id), false);
      dtl_hv_set_cstr(hv, "new_port_id", (dtl_dv_t *)dtl_sv_make_u32(self->object_id), false);
      dtl_hv_set_cstr(hv, "reason", (dtl_dv_t *)dtl_sv_make_cstr(reason == 1u ? "TAKEOVER" : "DISCONNECT"), false);
      dtl_hv_set_cstr(hv, "policy", (dtl_dv_t *)dtl_sv_make_cstr(policy == 1u ? "LAST" : "FIRST"), false);
      dtl_hv_set_cstr(hv, "old_node_name", (dtl_dv_t *)dtl_sv_make_cstr(old_node ? old_node : ""), false);
      dtl_hv_set_cstr(hv, "new_node_name", (dtl_dv_t *)dtl_sv_make_cstr(new_node ? new_node : ""), false);
      dtl_hv_set_cstr(hv, "port_name", (dtl_dv_t *)dtl_sv_make_cstr(port_name ? port_name : ""), false);
      if (old_node != NULL)
      {
        free(old_node);
      }
      if (new_node != NULL)
      {
        free(new_node);
      }
      if (port_name != NULL)
      {
        free(port_name);
      }
    }
    break;

  case APX_LOG_EVENT_EVENTS_DROPPED:
    if (self->payload_size >= 16u)
    {
      dtl_hv_set_cstr(hv, "first_sequence", (dtl_dv_t *)dtl_sv_make_u64(unpack_le_u64(self->payload + 0)), false);
      dtl_hv_set_cstr(hv, "dropped_count", (dtl_dv_t *)dtl_sv_make_u64(unpack_le_u64(self->payload + 8)), false);
    }
    break;

  case APX_LOG_EVENT_STREAM_END:
    if (self->payload_size >= 4u)
    {
      dtl_hv_set_cstr(hv, "reason", (dtl_dv_t *)dtl_sv_make_u32(unpack_le_u32(self->payload + 0)), false);
    }
    break;

  default:
    break;
  }

  dtl_json_dump((dtl_dv_t *)hv, out, 0, false);
  fputc('\n', out);
  dtl_hv_delete(hv);
}
