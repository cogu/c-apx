/*****************************************************************************
 * \file      node_cache_codec.h
 * \author    Conny Gustafsson
 * \date      2026-11-21
 * \brief     Binary cache (.apxc) codec and bounds-checked serialization I/O
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_NODE_CACHE_CODEC_H
#define APX_NODE_CACHE_CODEC_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "apx/error.h"
#include "apx/types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_cache_writer_tag
{
  adt_bytearray_t buf;
} apx_cache_writer_t;

typedef struct apx_cache_reader_tag
{
  uint8_t const *data;
  size_t size;
  size_t pos;
} apx_cache_reader_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
// Binary writer primitives (little-endian)
void apx_cache_writer_create(apx_cache_writer_t *self);
void apx_cache_writer_destroy(apx_cache_writer_t *self);
apx_error_t apx_cache_writer_reserve(apx_cache_writer_t *self, size_t capacity);
apx_error_t apx_cache_writer_write_u8(apx_cache_writer_t *self, uint8_t value);
apx_error_t apx_cache_writer_write_u16_le(apx_cache_writer_t *self, uint16_t value);
apx_error_t apx_cache_writer_write_u32_le(apx_cache_writer_t *self, uint32_t value);
apx_error_t apx_cache_writer_write_bytes(apx_cache_writer_t *self, uint8_t const *data, size_t size);
apx_error_t apx_cache_writer_write_str16(apx_cache_writer_t *self, char const *str);
apx_error_t apx_cache_writer_patch_u32_le(apx_cache_writer_t *self, size_t offset, uint32_t value);
uint8_t const *apx_cache_writer_data(apx_cache_writer_t const *self);
size_t apx_cache_writer_size(apx_cache_writer_t const *self);
apx_error_t apx_cache_writer_take_buffer(apx_cache_writer_t *self, uint8_t **out_buf, size_t *out_size);

// Bounds-checked binary reader primitives (little-endian)
void apx_cache_reader_create(apx_cache_reader_t *self, uint8_t const *data, size_t size);
size_t apx_cache_reader_remaining(apx_cache_reader_t const *self);
size_t apx_cache_reader_pos(apx_cache_reader_t const *self);
apx_error_t apx_cache_reader_seek(apx_cache_reader_t *self, size_t offset);
apx_error_t apx_cache_reader_read_u8(apx_cache_reader_t *self, uint8_t *out_val);
apx_error_t apx_cache_reader_read_u16_le(apx_cache_reader_t *self, uint16_t *out_val);
apx_error_t apx_cache_reader_read_u32_le(apx_cache_reader_t *self, uint32_t *out_val);
apx_error_t apx_cache_reader_read_bytes(apx_cache_reader_t *self, uint8_t *out_buf, size_t len);
apx_error_t apx_cache_reader_read_slice(apx_cache_reader_t *self, size_t len, uint8_t const **out_ptr);
apx_error_t apx_cache_reader_read_str16(apx_cache_reader_t *self, bool allow_empty, char **out_str);

#endif // APX_NODE_CACHE_CODEC_H
