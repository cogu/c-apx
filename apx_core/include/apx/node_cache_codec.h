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
#include "apx/program.h"
#include "apx/types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
// Forward declarations
struct apx_node_instance_tag;
struct apx_port_instance_tag;

#define APXC_PORT_FLAG_DYNAMIC_DATA 0x01u
#define APXC_MAX_PROGRAM_SIZE (1024u * 1024u) // 1 MiB safety cap per port VM program

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

// Child element serializers/deserializers (port descriptors and VM programs)
apx_error_t apx_cache_encode_port_descriptor(apx_cache_writer_t *writer, struct apx_port_instance_tag const *port);
apx_error_t apx_cache_decode_port_descriptor(apx_cache_reader_t *reader, struct apx_node_instance_tag *parent,
  apx_port_type_t port_type, apx_port_id_t port_id, uint32_t expected_offset, struct apx_port_instance_tag *out_port,
  uint32_t *out_next_offset);
apx_error_t apx_cache_encode_program(apx_cache_writer_t *writer, apx_program_t const *program);
apx_error_t apx_cache_decode_program(apx_cache_reader_t *reader, bool allow_empty, apx_program_t **out_program);

#endif // APX_NODE_CACHE_CODEC_H
