/*****************************************************************************
 * \file      apx_log_file.c
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     .apxlog container file reader and writer implementation
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "apx_log_file.h"
#include "apx/numheader.h"
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void pack_le_u16(uint8_t *dst, uint16_t val)
{
  dst[0] = (uint8_t)(val & 0xFFu);
  dst[1] = (uint8_t)((val >> 8) & 0xFFu);
}

static void pack_le_u32(uint8_t *dst, uint32_t val)
{
  dst[0] = (uint8_t)(val & 0xFFu);
  dst[1] = (uint8_t)((val >> 8) & 0xFFu);
  dst[2] = (uint8_t)((val >> 16) & 0xFFu);
  dst[3] = (uint8_t)((val >> 24) & 0xFFu);
}

static void pack_le_u64(uint8_t *dst, uint64_t val)
{
  pack_le_u32(dst + 0, (uint32_t)(val & 0xFFFFFFFFu));
  pack_le_u32(dst + 4, (uint32_t)((val >> 32) & 0xFFFFFFFFu));
}

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

static void pack_header(uint8_t *buf, const apx_log_file_header_t *hdr)
{
  memcpy(buf + 0, hdr->magic, APX_LOG_MAGIC_SIZE);
  pack_le_u16(buf + 8, hdr->format_major);
  pack_le_u16(buf + 10, hdr->format_minor);
  pack_le_u32(buf + 12, hdr->header_size);
  pack_le_u32(buf + 16, hdr->flags);
  pack_le_u32(buf + 20, hdr->reserved);
  pack_le_u64(buf + 24, hdr->capture_start_unix_ns);
  pack_le_u64(buf + 32, hdr->capture_end_unix_ns);
  pack_le_u64(buf + 40, hdr->record_count);
  pack_le_u64(buf + 48, hdr->index_offset);
  pack_le_u64(buf + 56, hdr->data_offset);
}

static bool unpack_header(apx_log_file_header_t *hdr, const uint8_t *buf)
{
  memcpy(hdr->magic, buf + 0, APX_LOG_MAGIC_SIZE);
  if (memcmp(hdr->magic, APX_LOG_MAGIC, APX_LOG_MAGIC_SIZE) != 0)
  {
    return false;
  }
  hdr->format_major = unpack_le_u16(buf + 8);
  hdr->format_minor = unpack_le_u16(buf + 10);
  hdr->header_size = unpack_le_u32(buf + 12);
  hdr->flags = unpack_le_u32(buf + 16);
  hdr->reserved = unpack_le_u32(buf + 20);
  hdr->capture_start_unix_ns = unpack_le_u64(buf + 24);
  hdr->capture_end_unix_ns = unpack_le_u64(buf + 32);
  hdr->record_count = unpack_le_u64(buf + 40);
  hdr->index_offset = unpack_le_u64(buf + 48);
  hdr->data_offset = unpack_le_u64(buf + 56);
  return true;
}

//////////////////////////////////////////////////////////////////////////////
// WRITER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

void apx_log_writer_create(apx_log_writer_t *self)
{
  if (self != NULL)
  {
    self->fh = NULL;
    self->path = NULL;
    self->is_open = false;
    memset(&self->header, 0, sizeof(apx_log_file_header_t));
    memcpy(self->header.magic, APX_LOG_MAGIC, APX_LOG_MAGIC_SIZE);
    self->header.format_major = APX_LOG_FORMAT_MAJOR;
    self->header.format_minor = APX_LOG_FORMAT_MINOR;
    self->header.header_size = APX_LOG_HEADER_SIZE;
    self->header.data_offset = APX_LOG_HEADER_SIZE;
  }
}

void apx_log_writer_destroy(apx_log_writer_t *self)
{
  if (self != NULL)
  {
    apx_log_writer_close(self);
    if (self->path != NULL)
    {
      free(self->path);
      self->path = NULL;
    }
  }
}

apx_log_writer_t *apx_log_writer_new(void)
{
  apx_log_writer_t *self = (apx_log_writer_t *)malloc(sizeof(apx_log_writer_t));
  if (self != NULL)
  {
    apx_log_writer_create(self);
  }
  return self;
}

void apx_log_writer_delete(apx_log_writer_t *self)
{
  if (self != NULL)
  {
    apx_log_writer_destroy(self);
    free(self);
  }
}

apx_error_t apx_log_writer_open(apx_log_writer_t *self, const char *path)
{
  if (self == NULL || path == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (self->is_open)
  {
    apx_log_writer_close(self);
  }

  self->fh = fopen(path, "wb+");
  if (self->fh == NULL)
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }

  if (self->path != NULL)
  {
    free(self->path);
  }
  self->path = strdup(path);

  // Write initial 64-byte header
  uint8_t header_buf[APX_LOG_HEADER_SIZE];
  memset(header_buf, 0, sizeof(header_buf));
  pack_header(header_buf, &self->header);

  if (fwrite(header_buf, 1u, APX_LOG_HEADER_SIZE, self->fh) != APX_LOG_HEADER_SIZE)
  {
    fclose(self->fh);
    self->fh = NULL;
    return APX_FILE_WRITE_ERROR;
  }
  fflush(self->fh);

  self->is_open = true;
  return APX_NO_ERROR;
}

void apx_log_writer_set_start_time(apx_log_writer_t *self, uint64_t start_unix_ns)
{
  if (self != NULL)
  {
    self->header.capture_start_unix_ns = start_unix_ns;
  }
}

apx_error_t apx_log_writer_write_record(apx_log_writer_t *self, const uint8_t *data, uint32_t size)
{
  if (self == NULL || !self->is_open || self->fh == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  uint8_t header_buf[NUMHEADER32_LONG_SIZE];
  int32_t const header_len = numheader_encode32(header_buf, sizeof(header_buf), size);
  if (header_len <= 0)
  {
    return APX_LENGTH_ERROR;
  }

  if (fwrite(header_buf, 1u, (size_t)header_len, self->fh) != (size_t)header_len)
  {
    return APX_FILE_WRITE_ERROR;
  }

  if (size > 0u && data != NULL)
  {
    if (fwrite(data, 1u, (size_t)size, self->fh) != (size_t)size)
    {
      return APX_FILE_WRITE_ERROR;
    }
  }

  self->header.record_count++;
  return APX_NO_ERROR;
}

apx_error_t apx_log_writer_finalize(apx_log_writer_t *self, uint64_t end_unix_ns)
{
  if (self == NULL || !self->is_open || self->fh == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  fflush(self->fh);
  self->header.capture_end_unix_ns = end_unix_ns;
  self->header.flags |= APX_LOG_FLAG_FINALIZED;

  if (fseek(self->fh, 0, SEEK_SET) != 0)
  {
    return APX_FILE_WRITE_ERROR;
  }

  uint8_t header_buf[APX_LOG_HEADER_SIZE];
  pack_header(header_buf, &self->header);

  if (fwrite(header_buf, 1u, APX_LOG_HEADER_SIZE, self->fh) != APX_LOG_HEADER_SIZE)
  {
    return APX_FILE_WRITE_ERROR;
  }
  fflush(self->fh);

  // Seek back to end
  (void)fseek(self->fh, 0, SEEK_END);
  return APX_NO_ERROR;
}

void apx_log_writer_close(apx_log_writer_t *self)
{
  if (self != NULL && self->is_open)
  {
    if (self->fh != NULL)
    {
      fclose(self->fh);
      self->fh = NULL;
    }
    self->is_open = false;
  }
}

//////////////////////////////////////////////////////////////////////////////
// READER IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

void apx_log_reader_create(apx_log_reader_t *self)
{
  if (self != NULL)
  {
    self->fh = NULL;
    self->path = NULL;
    self->file_size = 0ULL;
    self->is_open = false;
    memset(&self->header, 0, sizeof(apx_log_file_header_t));
  }
}

void apx_log_reader_destroy(apx_log_reader_t *self)
{
  if (self != NULL)
  {
    apx_log_reader_close(self);
    if (self->path != NULL)
    {
      free(self->path);
      self->path = NULL;
    }
  }
}

apx_log_reader_t *apx_log_reader_new(void)
{
  apx_log_reader_t *self = (apx_log_reader_t *)malloc(sizeof(apx_log_reader_t));
  if (self != NULL)
  {
    apx_log_reader_create(self);
  }
  return self;
}

void apx_log_reader_delete(apx_log_reader_t *self)
{
  if (self != NULL)
  {
    apx_log_reader_destroy(self);
    free(self);
  }
}

apx_error_t apx_log_reader_open(apx_log_reader_t *self, const char *path)
{
  if (self == NULL || path == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (self->is_open)
  {
    apx_log_reader_close(self);
  }

  self->fh = fopen(path, "rb");
  if (self->fh == NULL)
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }

  // Query file size
  if (fseek(self->fh, 0, SEEK_END) != 0)
  {
    fclose(self->fh);
    self->fh = NULL;
    return APX_READ_ERROR;
  }
  long size = ftell(self->fh);
  if (size < (long)APX_LOG_HEADER_SIZE)
  {
    fclose(self->fh);
    self->fh = NULL;
    return APX_INVALID_FILE_ERROR;
  }
  self->file_size = (uint64_t)size;
  if (fseek(self->fh, 0, SEEK_SET) != 0)
  {
    fclose(self->fh);
    self->fh = NULL;
    return APX_READ_ERROR;
  }

  uint8_t header_buf[APX_LOG_HEADER_SIZE];
  if (fread(header_buf, 1u, APX_LOG_HEADER_SIZE, self->fh) != APX_LOG_HEADER_SIZE)
  {
    fclose(self->fh);
    self->fh = NULL;
    return APX_READ_ERROR;
  }

  if (!unpack_header(&self->header, header_buf))
  {
    fclose(self->fh);
    self->fh = NULL;
    return APX_INVALID_FILE_ERROR;
  }

  if (self->header.format_major != APX_LOG_FORMAT_MAJOR)
  {
    fclose(self->fh);
    self->fh = NULL;
    return APX_VERSION_ERROR;
  }

  if (self->path != NULL)
  {
    free(self->path);
  }
  self->path = strdup(path);

  // Position at data_offset
  if (fseek(self->fh, (long)self->header.data_offset, SEEK_SET) != 0)
  {
    fclose(self->fh);
    self->fh = NULL;
    return APX_READ_ERROR;
  }

  self->is_open = true;
  return APX_NO_ERROR;
}

apx_error_t apx_log_reader_read_next_record(apx_log_reader_t *self, uint8_t **out_data, uint32_t *out_size)
{
  if (self == NULL || !self->is_open || self->fh == NULL || out_data == NULL || out_size == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  *out_data = NULL;
  *out_size = 0u;

  // Stop if index trailer reached
  if ((self->header.flags & APX_LOG_FLAG_INDEX_PRESENT) && (self->header.index_offset > 0u))
  {
    long cur = ftell(self->fh);
    if (cur >= 0 && (uint64_t)cur >= self->header.index_offset)
    {
      return APX_LOG_EOF_ERROR;
    }
  }

  uint8_t first_byte = 0;
  size_t n = fread(&first_byte, 1u, 1u, self->fh);
  if (n == 0u)
  {
    return APX_LOG_EOF_ERROR;
  }

  uint32_t payload_size = 0u;
  if (first_byte & 0x80u)
  {
    uint8_t nh_buf[4];
    nh_buf[0] = first_byte;
    if (fread(nh_buf + 1, 1u, 3u, self->fh) != 3u)
    {
      return APX_READ_ERROR;
    }
    const uint8_t *end = numheader_decode32(nh_buf, nh_buf + 4, &payload_size);
    if (end != nh_buf + 4)
    {
      return APX_PARSE_ERROR;
    }
  }
  else
  {
    payload_size = (uint32_t)first_byte;
  }

  uint8_t *payload = (uint8_t *)malloc(payload_size > 0u ? payload_size : 1u);
  if (payload == NULL)
  {
    return APX_MEM_ERROR;
  }

  if (payload_size > 0u)
  {
    if (fread(payload, 1u, (size_t)payload_size, self->fh) != (size_t)payload_size)
    {
      free(payload);
      return APX_READ_ERROR;
    }
  }

  *out_data = payload;
  *out_size = payload_size;
  return APX_NO_ERROR;
}

void apx_log_reader_close(apx_log_reader_t *self)
{
  if (self != NULL && self->is_open)
  {
    if (self->fh != NULL)
    {
      fclose(self->fh);
      self->fh = NULL;
    }
    self->is_open = false;
  }
}
