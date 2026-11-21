/*****************************************************************************
 * \file      node_cache_codec.c
 * \author    Conny Gustafsson
 * \date      2026-11-21
 * \brief     Binary cache (.apxc) codec and bounds-checked serialization I/O
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/node_cache_codec.h"
#include "apx/util.h"
#include "pack.h"
#include <stdlib.h>
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
// PUBLIC FUNCTIONS: BINARY WRITER
//////////////////////////////////////////////////////////////////////////////
void apx_cache_writer_create(apx_cache_writer_t *self)
{
  if (self != NULL)
  {
    adt_bytearray_create(&self->buf);
  }
}

void apx_cache_writer_destroy(apx_cache_writer_t *self)
{
  if (self != NULL)
  {
    adt_bytearray_destroy(&self->buf);
  }
}

apx_error_t apx_cache_writer_reserve(apx_cache_writer_t *self, size_t capacity)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (capacity > (size_t)UINT32_MAX)
  {
    return APX_LENGTH_ERROR;
  }
  return convert_from_adt_to_apx_error(adt_bytearray_reserve(&self->buf, (uint32_t)capacity));
}

apx_error_t apx_cache_writer_write_u8(apx_cache_writer_t *self, uint8_t value)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  return convert_from_adt_to_apx_error(adt_bytearray_push(&self->buf, value));
}

apx_error_t apx_cache_writer_write_u16_le(apx_cache_writer_t *self, uint16_t value)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  uint8_t raw[2];
  cutil_pack_le(raw, (uint32_t)value, 2u);
  return convert_from_adt_to_apx_error(adt_bytearray_append(&self->buf, raw, 2u));
}

apx_error_t apx_cache_writer_write_u32_le(apx_cache_writer_t *self, uint32_t value)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  uint8_t raw[4];
  cutil_pack_le(raw, value, 4u);
  return convert_from_adt_to_apx_error(adt_bytearray_append(&self->buf, raw, 4u));
}

apx_error_t apx_cache_writer_write_bytes(apx_cache_writer_t *self, uint8_t const *data, size_t size)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (size == 0u)
  {
    return APX_NO_ERROR;
  }
  if (data == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (size > (size_t)UINT32_MAX)
  {
    return APX_LENGTH_ERROR;
  }
  uint32_t cur_len = adt_bytearray_length(&self->buf);
  if ((uint32_t)size > UINT32_MAX - cur_len)
  {
    return APX_LENGTH_ERROR;
  }
  return convert_from_adt_to_apx_error(adt_bytearray_append(&self->buf, data, (uint32_t)size));
}

apx_error_t apx_cache_writer_write_str16(apx_cache_writer_t *self, char const *str)
{
  if (self == NULL || str == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  size_t len = strlen(str);
  if (len > (size_t)UINT16_MAX)
  {
    return APX_LENGTH_ERROR;
  }
  apx_error_t rc = apx_cache_writer_write_u16_le(self, (uint16_t)len);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }
  return apx_cache_writer_write_bytes(self, (uint8_t const *)str, len);
}

apx_error_t apx_cache_writer_patch_u32_le(apx_cache_writer_t *self, size_t offset, uint32_t value)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  size_t cur_len = (size_t)adt_bytearray_length(&self->buf);
  if (offset > cur_len || (cur_len - offset) < 4u)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  uint8_t *raw = adt_bytearray_data(&self->buf);
  if (raw == NULL)
  {
    return APX_INTERNAL_ERROR;
  }
  cutil_pack_le(raw + offset, value, 4u);
  return APX_NO_ERROR;
}

uint8_t const *apx_cache_writer_data(apx_cache_writer_t const *self)
{
  if (self == NULL)
  {
    return NULL;
  }
  return adt_bytearray_const_data(&self->buf);
}

size_t apx_cache_writer_size(apx_cache_writer_t const *self)
{
  if (self == NULL)
  {
    return 0u;
  }
  return (size_t)adt_bytearray_length(&self->buf);
}

apx_error_t apx_cache_writer_take_buffer(apx_cache_writer_t *self, uint8_t **out_buf, size_t *out_size)
{
  if (self == NULL || out_buf == NULL || out_size == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  *out_buf = NULL;
  *out_size = 0u;
  size_t len = (size_t)adt_bytearray_length(&self->buf);
  if (len == 0u)
  {
    return APX_NO_ERROR;
  }
  uint8_t const *src = adt_bytearray_const_data(&self->buf);
  if (src == NULL)
  {
    return APX_INTERNAL_ERROR;
  }
  uint8_t *copy = (uint8_t *)malloc(len);
  if (copy == NULL)
  {
    return APX_MEM_ERROR;
  }
  memcpy(copy, src, len);
  adt_bytearray_clear(&self->buf);
  *out_buf = copy;
  *out_size = len;
  return APX_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS: BINARY READER
//////////////////////////////////////////////////////////////////////////////
void apx_cache_reader_create(apx_cache_reader_t *self, uint8_t const *data, size_t size)
{
  if (self != NULL)
  {
    self->data = data;
    self->size = (data != NULL) ? size : 0u;
    self->pos = 0u;
  }
}

size_t apx_cache_reader_remaining(apx_cache_reader_t const *self)
{
  if (self == NULL || self->pos > self->size)
  {
    return 0u;
  }
  return self->size - self->pos;
}

size_t apx_cache_reader_pos(apx_cache_reader_t const *self)
{
  if (self == NULL)
  {
    return 0u;
  }
  return self->pos;
}

apx_error_t apx_cache_reader_seek(apx_cache_reader_t *self, size_t offset)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (offset > self->size)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  self->pos = offset;
  return APX_NO_ERROR;
}

apx_error_t apx_cache_reader_read_u8(apx_cache_reader_t *self, uint8_t *out_val)
{
  if (self == NULL || out_val == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (apx_cache_reader_remaining(self) < 1u)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  *out_val = self->data[self->pos];
  self->pos += 1u;
  return APX_NO_ERROR;
}

apx_error_t apx_cache_reader_read_u16_le(apx_cache_reader_t *self, uint16_t *out_val)
{
  if (self == NULL || out_val == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (apx_cache_reader_remaining(self) < 2u)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  *out_val = (uint16_t)cutil_unpack_le(self->data + self->pos, 2u);
  self->pos += 2u;
  return APX_NO_ERROR;
}

apx_error_t apx_cache_reader_read_u32_le(apx_cache_reader_t *self, uint32_t *out_val)
{
  if (self == NULL || out_val == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (apx_cache_reader_remaining(self) < 4u)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  *out_val = cutil_unpack_le(self->data + self->pos, 4u);
  self->pos += 4u;
  return APX_NO_ERROR;
}

apx_error_t apx_cache_reader_read_bytes(apx_cache_reader_t *self, uint8_t *out_buf, size_t len)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (len == 0u)
  {
    return APX_NO_ERROR;
  }
  if (out_buf == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (apx_cache_reader_remaining(self) < len)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  memcpy(out_buf, self->data + self->pos, len);
  self->pos += len;
  return APX_NO_ERROR;
}

apx_error_t apx_cache_reader_read_slice(apx_cache_reader_t *self, size_t len, uint8_t const **out_ptr)
{
  if (self == NULL || out_ptr == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (apx_cache_reader_remaining(self) < len)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  *out_ptr = (len > 0u) ? (self->data + self->pos) : NULL;
  self->pos += len;
  return APX_NO_ERROR;
}

apx_error_t apx_cache_reader_read_str16(apx_cache_reader_t *self, bool allow_empty, char **out_str)
{
  if (self == NULL || out_str == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  *out_str = NULL;
  uint16_t len = 0u;
  apx_error_t rc = apx_cache_reader_read_u16_le(self, &len);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }
  if (!allow_empty && len == 0u)
  {
    return APX_LENGTH_ERROR;
  }
  uint8_t const *slice = NULL;
  rc = apx_cache_reader_read_slice(self, (size_t)len, &slice);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }
  if (len > 0u && memchr(slice, '\0', (size_t)len) != NULL)
  {
    return APX_INVALID_NAME_ERROR;
  }
  char *copy = (char *)malloc((size_t)len + 1u);
  if (copy == NULL)
  {
    return APX_MEM_ERROR;
  }
  if (len > 0u)
  {
    memcpy(copy, slice, (size_t)len);
  }
  copy[len] = '\0';
  *out_str = copy;
  return APX_NO_ERROR;
}
