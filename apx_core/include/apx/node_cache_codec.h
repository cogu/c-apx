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
#include "apx/remotefile.h"
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

// .apxc container constants (v1.0)
#define APXC_MAGIC_0 0x00u
#define APXC_MAGIC_1 ((uint8_t)'A')
#define APXC_MAGIC_2 ((uint8_t)'P')
#define APXC_MAGIC_3 ((uint8_t)'X')
#define APXC_FORMAT_MAJOR 1u
#define APXC_FORMAT_MINOR 0u
#define APXC_VM_MAJOR ((uint8_t)(APX_VM_HEADER_VERSION_MAJOR - '0'))
#define APXC_VM_MINOR ((uint8_t)(APX_VM_HEADER_VERSION_MINOR - '0'))
#define APXC_COMPILER_ABI_VERSION 1u

#define APXC_HEADER_SIZE 72u
#define APXC_SECTION_ENTRY_SIZE 12u
#define APXC_TRAILER_SIZE RMF_SHA256_SIZE
#define APXC_MIN_FILE_SIZE (APXC_HEADER_SIZE + APXC_TRAILER_SIZE)
#define APXC_MAX_SECTIONS 16u
#define APXC_MAX_PORTS 65535u

// Header flags
#define APXC_FLAG_HAS_VM_PROGRAMS 0x0001u
#define APXC_FLAG_HAS_DEF_TEXT 0x0002u
#define APXC_KNOWN_FLAGS (APXC_FLAG_HAS_VM_PROGRAMS | APXC_FLAG_HAS_DEF_TEXT)

// Section directory types
#define APXC_SEC_PROVIDE_PORTS 0x0001u
#define APXC_SEC_REQUIRE_PORTS 0x0002u
#define APXC_SEC_PROVIDE_INIT_DATA 0x0003u
#define APXC_SEC_REQUIRE_INIT_DATA 0x0004u
#define APXC_SEC_PROVIDE_VM_PROGS 0x0005u
#define APXC_SEC_REQUIRE_VM_PROGS 0x0006u
#define APXC_SEC_DEF_TEXT 0x0007u

// APXC_SEC_DEF_TEXT section flags
#define APXC_DEF_TEXT_FLAG_RAW 0x0000u
#define APXC_DEF_TEXT_FLAG_ZLIB 0x0001u

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

// Full .apxc container codec
char const *apx_node_cache_format_version_string(void);

apx_error_t apx_node_cache_encode_binary(struct apx_node_instance_tag const *node_instance,
  uint8_t const sha256[RMF_SHA256_SIZE], uint32_t logical_def_size, uint8_t const *opt_def_text, bool compress_def_text,
  bool include_vm_programs, uint8_t **out_buf, size_t *out_size);

/**
 * Validates the container integrity, format/VM/ABI version, identity binding,
 * and section bounds of an on-disk .apxc buffer without mutating any node.
 * Returns APX_VERSION_ERROR on format/VM/ABI mismatch, or another error on corruption.
 */
apx_error_t apx_node_cache_validate_binary(
  uint8_t const *buf, size_t buf_size, char const *expected_node_name, uint8_t const expected_sha256[RMF_SHA256_SIZE]);

/**
 * Validates and decodes an .apxc buffer into node_instance (populating IDL version,
 * provide/require ports, init data buffers, and optional VM programs).
 * Caller must attach node_data and call finalize_node_data / create_byte_port_map / build_connector_table.
 * If out_embedded_def_text is non-NULL and the .apxc contains APXC_SEC_DEF_TEXT,
 * allocates (*out_embedded_def_text) of size (logical_def_size + 1) with trailing NUL;
 * caller owns *out_embedded_def_text and must free() it.
 */
apx_error_t apx_node_cache_decode_binary(uint8_t const *buf, size_t buf_size, char const *expected_node_name,
  uint8_t const expected_sha256[RMF_SHA256_SIZE], uint32_t expected_logical_size, bool load_vm_programs,
  struct apx_node_instance_tag *node_instance, uint8_t **out_embedded_def_text);

#endif // APX_NODE_CACHE_CODEC_H
