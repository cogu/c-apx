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
#include "apx/cfg.h"
#include "apx/compress.h"
#include "apx/crypto.h"
#include "apx/node_instance.h"
#include "apx/port_instance.h"
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
#define APXC_FORMAT_VERSION_STAMP "1/vm2.1/abi1\n"

typedef struct apxc_section_entry_tag
{
  uint16_t type;
  uint16_t flags;
  uint32_t offset;
  uint32_t size;
  bool present;
} apxc_section_entry_t;

typedef struct apxc_parsed_container_tag
{
  uint16_t flags;
  uint16_t idl_major;
  uint16_t idl_minor;
  uint32_t logical_def_size;
  uint32_t num_provide_ports;
  uint32_t num_require_ports;
  apxc_section_entry_t provide_ports;
  apxc_section_entry_t require_ports;
  apxc_section_entry_t provide_init;
  apxc_section_entry_t require_init;
  apxc_section_entry_t provide_vm;
  apxc_section_entry_t require_vm;
  apxc_section_entry_t def_text;
} apxc_parsed_container_t;

typedef struct apxc_build_section_tag
{
  uint16_t type;
  uint16_t flags;
  uint8_t const *data;
  size_t size;
} apxc_build_section_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t parse_and_validate_container(uint8_t const *buf, size_t buf_size, char const *expected_node_name,
  uint8_t const expected_sha256[RMF_SHA256_SIZE], uint32_t expected_logical_size, apxc_parsed_container_t *out_parsed);
static void reset_node_instance_decoded_state(apx_node_instance_t *node_instance);
static apx_error_t decode_port_table_section(uint8_t const *buf, apxc_section_entry_t const *sec,
  apx_node_instance_t *node_instance, apx_port_type_t port_type, uint32_t num_ports, uint32_t expected_total_data_size);
static apx_error_t decode_vm_programs_section(uint8_t const *buf, apxc_section_entry_t const *sec,
  apx_node_instance_t *node_instance, apx_port_type_t port_type, uint32_t num_ports);
static apx_error_t decode_def_text_section(uint8_t const *buf, apxc_section_entry_t const *sec,
  uint32_t logical_def_size, uint8_t const expected_sha256[RMF_SHA256_SIZE], uint8_t **out_def_text);

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

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS: CHILD ELEMENT SERIALIZERS / DESERIALIZERS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_cache_encode_port_descriptor(apx_cache_writer_t *writer, apx_port_instance_t const *port)
{
  if (writer == NULL || port == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  char const *name = port->name;
  char const *sig = port->port_signature;
  if (name == NULL || sig == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  size_t name_len = strlen(name);
  size_t sig_len = strlen(sig);
  if (name_len == 0u || name_len > (size_t)UINT16_MAX || sig_len == 0u || sig_len > (size_t)UINT16_MAX)
  {
    return APX_LENGTH_ERROR;
  }
  if (port->data_size == 0u)
  {
    return APX_LENGTH_ERROR;
  }
  if (port->queue_length > 0u && port->element_size == 0u)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  uint8_t port_flags = port->has_dynamic_data ? APXC_PORT_FLAG_DYNAMIC_DATA : 0u;
  apx_error_t rc = apx_cache_writer_write_u32_le(writer, port->data_offset);
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_u32_le(writer, port->data_size);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_u32_le(writer, port->element_size);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_u32_le(writer, port->queue_length);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_u8(writer, port_flags);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_u8(writer, 0u); // reserved
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_u16_le(writer, (uint16_t)name_len);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_u16_le(writer, (uint16_t)sig_len);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_bytes(writer, (uint8_t const *)name, name_len);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_writer_write_bytes(writer, (uint8_t const *)sig, sig_len);
  }
  return rc;
}

apx_error_t apx_cache_decode_port_descriptor(apx_cache_reader_t *reader, apx_node_instance_t *parent,
  apx_port_type_t port_type, apx_port_id_t port_id, uint32_t expected_offset, apx_port_instance_t *out_port,
  uint32_t *out_next_offset)
{
  if (reader == NULL || out_port == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  uint32_t data_offset = 0u;
  uint32_t data_size = 0u;
  uint32_t element_size = 0u;
  uint32_t queue_length = 0u;
  uint8_t port_flags = 0u;
  uint8_t reserved = 0u;
  uint16_t name_len = 0u;
  uint16_t sig_len = 0u;

  apx_error_t rc = apx_cache_reader_read_u32_le(reader, &data_offset);
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u32_le(reader, &data_size);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u32_le(reader, &element_size);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u32_le(reader, &queue_length);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u8(reader, &port_flags);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u8(reader, &reserved);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u16_le(reader, &name_len);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u16_le(reader, &sig_len);
  }
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }

  if ((port_flags & ~((unsigned int)APXC_PORT_FLAG_DYNAMIC_DATA)) != 0u || reserved != 0u)
  {
    return APX_INVALID_HEADER_ERROR;
  }
  if (data_size == 0u || data_offset != expected_offset)
  {
    return APX_LENGTH_ERROR;
  }
  if (expected_offset > (uint32_t)APX_MAX_FILE_SIZE || data_size > ((uint32_t)APX_MAX_FILE_SIZE - expected_offset))
  {
    return APX_FILE_TOO_LARGE_ERROR;
  }
  if (queue_length > 0u && element_size == 0u)
  {
    return APX_LENGTH_ERROR;
  }
  if (name_len == 0u || sig_len == 0u)
  {
    return APX_LENGTH_ERROR;
  }

  uint8_t const *name_slice = NULL;
  uint8_t const *sig_slice = NULL;
  rc = apx_cache_reader_read_slice(reader, (size_t)name_len, &name_slice);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }
  rc = apx_cache_reader_read_slice(reader, (size_t)sig_len, &sig_slice);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }
  if (memchr(name_slice, '\0', (size_t)name_len) != NULL || memchr(sig_slice, '\0', (size_t)sig_len) != NULL)
  {
    return APX_INVALID_NAME_ERROR;
  }

  char *name_copy = (char *)malloc((size_t)name_len + 1u);
  if (name_copy == NULL)
  {
    return APX_MEM_ERROR;
  }
  memcpy(name_copy, name_slice, (size_t)name_len);
  name_copy[name_len] = '\0';

  char *sig_copy = (char *)malloc((size_t)sig_len + 1u);
  if (sig_copy == NULL)
  {
    free(name_copy);
    return APX_MEM_ERROR;
  }
  memcpy(sig_copy, sig_slice, (size_t)sig_len);
  sig_copy[sig_len] = '\0';

  bool has_dynamic = (port_flags & APXC_PORT_FLAG_DYNAMIC_DATA) != 0u;
  rc = apx_port_instance_create_from_cache(out_port, parent, port_type, port_id, name_copy, sig_copy, data_offset,
    data_size, element_size, queue_length, has_dynamic, NULL, NULL);
  free(name_copy);
  free(sig_copy);
  if (rc == APX_NO_ERROR && out_next_offset != NULL)
  {
    *out_next_offset = expected_offset + data_size;
  }
  return rc;
}

apx_error_t apx_cache_encode_program(apx_cache_writer_t *writer, apx_program_t const *program)
{
  if (writer == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (program == NULL)
  {
    return apx_cache_writer_write_u32_le(writer, 0u);
  }
  uint32_t len = adt_bytearray_length(program);
  if (len > APXC_MAX_PROGRAM_SIZE)
  {
    return APX_LENGTH_ERROR;
  }
  apx_error_t rc = apx_cache_writer_write_u32_le(writer, len);
  if (rc == APX_NO_ERROR && len > 0u)
  {
    uint8_t const *data = adt_bytearray_const_data(program);
    if (data == NULL)
    {
      return APX_INTERNAL_ERROR;
    }
    rc = apx_cache_writer_write_bytes(writer, data, (size_t)len);
  }
  return rc;
}

apx_error_t apx_cache_decode_program(apx_cache_reader_t *reader, bool allow_empty, apx_program_t **out_program)
{
  if (reader == NULL || out_program == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  *out_program = NULL;
  uint32_t len = 0u;
  apx_error_t rc = apx_cache_reader_read_u32_le(reader, &len);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }
  if (len == 0u)
  {
    return allow_empty ? APX_NO_ERROR : APX_INVALID_PROGRAM_ERROR;
  }
  if (len > APXC_MAX_PROGRAM_SIZE)
  {
    return APX_LENGTH_ERROR;
  }
  uint8_t const *slice = NULL;
  rc = apx_cache_reader_read_slice(reader, (size_t)len, &slice);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }
  apx_program_header_t header;
  uint8_t const *next = NULL;
  rc = apx_program_decode_header(slice, slice + (size_t)len, &next, &header);
  if (rc != APX_NO_ERROR)
  {
    return APX_INVALID_PROGRAM_ERROR;
  }
  apx_program_t *program = adt_bytearray_make(slice, len);
  if (program == NULL)
  {
    return APX_MEM_ERROR;
  }
  *out_program = program;
  return APX_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS: FULL .APXC CONTAINER CODEC
//////////////////////////////////////////////////////////////////////////////
char const *apx_node_cache_format_version_string(void) { return APXC_FORMAT_VERSION_STAMP; }

apx_error_t apx_node_cache_encode_binary(apx_node_instance_t const *node_instance,
  uint8_t const sha256[RMF_SHA256_SIZE], uint32_t logical_def_size, uint8_t const *opt_def_text, bool compress_def_text,
  bool include_vm_programs, uint8_t **out_buf, size_t *out_size)
{
  if (node_instance == NULL || sha256 == NULL || out_buf == NULL || out_size == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  *out_buf = NULL;
  *out_size = 0u;
  if (logical_def_size == 0u || logical_def_size > (uint32_t)APX_MAX_FILE_SIZE)
  {
    return APX_LENGTH_ERROR;
  }
  char const *node_name = apx_node_instance_get_name(node_instance);
  if (node_name == NULL)
  {
    return APX_NAME_MISSING_ERROR;
  }
  size_t node_name_len = strlen(node_name);
  if (node_name_len == 0u || node_name_len > (size_t)RMF_MAX_FILE_NAME_SIZE)
  {
    return APX_NAME_TOO_LONG_ERROR;
  }

  int32_t idl_major = apx_node_instance_get_major_version(node_instance);
  int32_t idl_minor = apx_node_instance_get_minor_version(node_instance);
  if (idl_major < 0 || idl_major > (int32_t)UINT16_MAX || idl_minor < 0 || idl_minor > (int32_t)UINT16_MAX)
  {
    return APX_VERSION_ERROR;
  }

  apx_size_t num_provide = apx_node_instance_get_num_provide_ports(node_instance);
  apx_size_t num_require = apx_node_instance_get_num_require_ports(node_instance);
  if (num_provide > APXC_MAX_PORTS || num_require > APXC_MAX_PORTS)
  {
    return APX_TOO_MANY_PORTS_ERROR;
  }

  apx_size_t provide_init_size = apx_node_instance_get_provide_port_init_data_size(node_instance);
  apx_size_t require_init_size = apx_node_instance_get_require_port_init_data_size(node_instance);
  uint8_t const *provide_init_data = apx_node_instance_get_provide_port_init_data(node_instance);
  uint8_t const *require_init_data = apx_node_instance_get_require_port_init_data(node_instance);
  if ((num_provide > 0u && (provide_init_size == 0u || provide_init_data == NULL)) ||
    (num_require > 0u && (require_init_size == 0u || require_init_data == NULL)))
  {
    return APX_INVALID_STATE_ERROR;
  }

  apx_cache_writer_t provide_ports_w;
  apx_cache_writer_t require_ports_w;
  apx_cache_writer_t provide_vm_w;
  apx_cache_writer_t require_vm_w;
  apx_cache_writer_t out_w;
  uint8_t *compressed_def_buf = NULL;
  size_t compressed_def_len = 0u;

  apx_cache_writer_create(&provide_ports_w);
  apx_cache_writer_create(&require_ports_w);
  apx_cache_writer_create(&provide_vm_w);
  apx_cache_writer_create(&require_vm_w);
  apx_cache_writer_create(&out_w);

  apx_error_t rc = APX_NO_ERROR;
  bool emit_vm = include_vm_programs && (num_provide > 0u || num_require > 0u);

  for (apx_size_t i = 0u; i < num_provide && rc == APX_NO_ERROR; i++)
  {
    apx_port_instance_t const *port = apx_node_instance_get_provide_port(node_instance, (apx_port_id_t)i);
    rc = apx_cache_encode_port_descriptor(&provide_ports_w, port);
    if (rc == APX_NO_ERROR && emit_vm)
    {
      if (port->pack_program == NULL)
      {
        emit_vm = false;
      }
      else
      {
        rc = apx_cache_encode_program(&provide_vm_w, port->pack_program);
      }
    }
  }
  for (apx_size_t i = 0u; i < num_require && rc == APX_NO_ERROR; i++)
  {
    apx_port_instance_t const *port = apx_node_instance_get_require_port(node_instance, (apx_port_id_t)i);
    rc = apx_cache_encode_port_descriptor(&require_ports_w, port);
    if (rc == APX_NO_ERROR && emit_vm)
    {
      if (port->pack_program == NULL || port->unpack_program == NULL)
      {
        emit_vm = false;
      }
      else
      {
        rc = apx_cache_encode_program(&require_vm_w, port->pack_program);
        if (rc == APX_NO_ERROR)
        {
          rc = apx_cache_encode_program(&require_vm_w, port->unpack_program);
        }
      }
    }
  }

  uint16_t def_text_flags = APXC_DEF_TEXT_FLAG_RAW;
  uint8_t const *def_text_payload = NULL;
  size_t def_text_payload_size = 0u;
  if (rc == APX_NO_ERROR && opt_def_text != NULL)
  {
    if (compress_def_text)
    {
      size_t bound = apx_compress_bound((size_t)logical_def_size);
      if (bound == 0u)
      {
        rc = APX_COMPRESSION_ERROR;
      }
      else
      {
        compressed_def_buf = (uint8_t *)malloc(bound);
        if (compressed_def_buf == NULL)
        {
          rc = APX_MEM_ERROR;
        }
        else
        {
          compressed_def_len = bound;
          rc = apx_compress(compressed_def_buf, &compressed_def_len, opt_def_text, (size_t)logical_def_size);
          if (rc == APX_NO_ERROR)
          {
            def_text_flags = APXC_DEF_TEXT_FLAG_ZLIB;
            def_text_payload = compressed_def_buf;
            def_text_payload_size = compressed_def_len;
          }
        }
      }
    }
    else
    {
      def_text_flags = APXC_DEF_TEXT_FLAG_RAW;
      def_text_payload = opt_def_text;
      def_text_payload_size = (size_t)logical_def_size;
    }
  }

  apxc_build_section_t sections[APXC_MAX_SECTIONS];
  uint16_t section_count = 0u;
  uint16_t header_flags = 0u;

  if (rc == APX_NO_ERROR)
  {
    if (num_provide > 0u)
    {
      sections[section_count].type = APXC_SEC_PROVIDE_PORTS;
      sections[section_count].flags = 0u;
      sections[section_count].data = apx_cache_writer_data(&provide_ports_w);
      sections[section_count].size = apx_cache_writer_size(&provide_ports_w);
      section_count++;

      sections[section_count].type = APXC_SEC_PROVIDE_INIT_DATA;
      sections[section_count].flags = 0u;
      sections[section_count].data = provide_init_data;
      sections[section_count].size = (size_t)provide_init_size;
      section_count++;
    }
    if (num_require > 0u)
    {
      sections[section_count].type = APXC_SEC_REQUIRE_PORTS;
      sections[section_count].flags = 0u;
      sections[section_count].data = apx_cache_writer_data(&require_ports_w);
      sections[section_count].size = apx_cache_writer_size(&require_ports_w);
      section_count++;

      sections[section_count].type = APXC_SEC_REQUIRE_INIT_DATA;
      sections[section_count].flags = 0u;
      sections[section_count].data = require_init_data;
      sections[section_count].size = (size_t)require_init_size;
      section_count++;
    }
    if (emit_vm)
    {
      header_flags |= APXC_FLAG_HAS_VM_PROGRAMS;
      if (num_provide > 0u)
      {
        sections[section_count].type = APXC_SEC_PROVIDE_VM_PROGS;
        sections[section_count].flags = 0u;
        sections[section_count].data = apx_cache_writer_data(&provide_vm_w);
        sections[section_count].size = apx_cache_writer_size(&provide_vm_w);
        section_count++;
      }
      if (num_require > 0u)
      {
        sections[section_count].type = APXC_SEC_REQUIRE_VM_PROGS;
        sections[section_count].flags = 0u;
        sections[section_count].data = apx_cache_writer_data(&require_vm_w);
        sections[section_count].size = apx_cache_writer_size(&require_vm_w);
        section_count++;
      }
    }
    if (def_text_payload != NULL)
    {
      header_flags |= APXC_FLAG_HAS_DEF_TEXT;
      sections[section_count].type = APXC_SEC_DEF_TEXT;
      sections[section_count].flags = def_text_flags;
      sections[section_count].data = def_text_payload;
      sections[section_count].size = def_text_payload_size;
      section_count++;
    }

    size_t dir_end =
      (size_t)APXC_HEADER_SIZE + node_name_len + ((size_t)section_count * (size_t)APXC_SECTION_ENTRY_SIZE);
    size_t total_size = dir_end;
    for (uint16_t i = 0u; i < section_count; i++)
    {
      if (sections[i].size > ((size_t)APX_MAX_FILE_SIZE - total_size))
      {
        rc = APX_FILE_TOO_LARGE_ERROR;
        break;
      }
      total_size += sections[i].size;
    }
    if (rc == APX_NO_ERROR)
    {
      if ((size_t)APXC_TRAILER_SIZE > ((size_t)APX_MAX_FILE_SIZE - total_size))
      {
        rc = APX_FILE_TOO_LARGE_ERROR;
      }
      else
      {
        total_size += (size_t)APXC_TRAILER_SIZE;
      }
    }

    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_reserve(&out_w, total_size);
    }

    // Write 72-byte Fixed Header
    if (rc == APX_NO_ERROR)
    {
      uint8_t const magic[4] = {APXC_MAGIC_0, APXC_MAGIC_1, APXC_MAGIC_2, APXC_MAGIC_3};
      rc = apx_cache_writer_write_bytes(&out_w, magic, 4u);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u8(&out_w, APXC_FORMAT_MAJOR);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u8(&out_w, APXC_FORMAT_MINOR);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u8(&out_w, APXC_VM_MAJOR);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u8(&out_w, APXC_VM_MINOR);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u16_le(&out_w, APXC_COMPILER_ABI_VERSION);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u16_le(&out_w, header_flags);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u16_le(&out_w, section_count);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u16_le(&out_w, (uint16_t)node_name_len);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u32_le(&out_w, (uint32_t)total_size);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u32_le(&out_w, logical_def_size);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_bytes(&out_w, sha256, RMF_SHA256_SIZE);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u16_le(&out_w, (uint16_t)idl_major);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u16_le(&out_w, (uint16_t)idl_minor);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u32_le(&out_w, (uint32_t)num_provide);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u32_le(&out_w, (uint32_t)num_require);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_u32_le(&out_w, 0u); // reserved
    }

    // Node name bytes
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_write_bytes(&out_w, (uint8_t const *)node_name, node_name_len);
    }

    // Section Directory
    size_t next_offset = dir_end;
    for (uint16_t i = 0u; i < section_count && rc == APX_NO_ERROR; i++)
    {
      rc = apx_cache_writer_write_u16_le(&out_w, sections[i].type);
      if (rc == APX_NO_ERROR)
      {
        rc = apx_cache_writer_write_u16_le(&out_w, sections[i].flags);
      }
      if (rc == APX_NO_ERROR)
      {
        rc = apx_cache_writer_write_u32_le(&out_w, (uint32_t)next_offset);
      }
      if (rc == APX_NO_ERROR)
      {
        rc = apx_cache_writer_write_u32_le(&out_w, (uint32_t)sections[i].size);
      }
      next_offset += sections[i].size;
    }

    // Section Payloads
    for (uint16_t i = 0u; i < section_count && rc == APX_NO_ERROR; i++)
    {
      rc = apx_cache_writer_write_bytes(&out_w, sections[i].data, sections[i].size);
    }

    // Whole-file SHA-256 trailer
    if (rc == APX_NO_ERROR)
    {
      uint8_t file_hash[RMF_SHA256_SIZE];
      rc = apx_crypto_sha256(apx_cache_writer_data(&out_w), apx_cache_writer_size(&out_w), file_hash);
      if (rc == APX_NO_ERROR)
      {
        rc = apx_cache_writer_write_bytes(&out_w, file_hash, RMF_SHA256_SIZE);
      }
    }

    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_writer_take_buffer(&out_w, out_buf, out_size);
    }
  }

  free(compressed_def_buf);
  apx_cache_writer_destroy(&provide_ports_w);
  apx_cache_writer_destroy(&require_ports_w);
  apx_cache_writer_destroy(&provide_vm_w);
  apx_cache_writer_destroy(&require_vm_w);
  apx_cache_writer_destroy(&out_w);
  return rc;
}

apx_error_t apx_node_cache_validate_binary(
  uint8_t const *buf, size_t buf_size, char const *expected_node_name, uint8_t const expected_sha256[RMF_SHA256_SIZE])
{
  apxc_parsed_container_t parsed;
  return parse_and_validate_container(buf, buf_size, expected_node_name, expected_sha256, 0u, &parsed);
}

apx_error_t apx_node_cache_decode_binary(uint8_t const *buf, size_t buf_size, char const *expected_node_name,
  uint8_t const expected_sha256[RMF_SHA256_SIZE], uint32_t expected_logical_size, bool load_vm_programs,
  apx_node_instance_t *node_instance, uint8_t **out_embedded_def_text)
{
  if (out_embedded_def_text != NULL)
  {
    *out_embedded_def_text = NULL;
  }
  if (node_instance == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  apxc_parsed_container_t parsed;
  apx_error_t rc =
    parse_and_validate_container(buf, buf_size, expected_node_name, expected_sha256, expected_logical_size, &parsed);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }

  apx_node_instance_set_version(node_instance, (int32_t)parsed.idl_major, (int32_t)parsed.idl_minor);
  rc = apx_node_instance_alloc_port_instance_memory(
    node_instance, (apx_size_t)parsed.num_provide_ports, (apx_size_t)parsed.num_require_ports);
  if (rc != APX_NO_ERROR)
  {
    reset_node_instance_decoded_state(node_instance);
    return rc;
  }

  if (parsed.num_provide_ports > 0u)
  {
    rc = decode_port_table_section(
      buf, &parsed.provide_ports, node_instance, APX_PROVIDE_PORT, parsed.num_provide_ports, parsed.provide_init.size);
  }
  if (rc == APX_NO_ERROR && parsed.num_require_ports > 0u)
  {
    rc = decode_port_table_section(
      buf, &parsed.require_ports, node_instance, APX_REQUIRE_PORT, parsed.num_require_ports, parsed.require_init.size);
  }
  if (rc == APX_NO_ERROR)
  {
    uint8_t const *prov_init = parsed.provide_init.present ? (buf + parsed.provide_init.offset) : NULL;
    apx_size_t prov_size = parsed.provide_init.present ? (apx_size_t)parsed.provide_init.size : 0u;
    uint8_t const *req_init = parsed.require_init.present ? (buf + parsed.require_init.offset) : NULL;
    apx_size_t req_size = parsed.require_init.present ? (apx_size_t)parsed.require_init.size : 0u;
    rc = apx_node_instance_set_init_data_from_cache(node_instance, prov_init, prov_size, req_init, req_size);
  }

  if (rc == APX_NO_ERROR && load_vm_programs && ((parsed.flags & APXC_FLAG_HAS_VM_PROGRAMS) != 0u))
  {
    if (parsed.num_provide_ports > 0u)
    {
      rc =
        decode_vm_programs_section(buf, &parsed.provide_vm, node_instance, APX_PROVIDE_PORT, parsed.num_provide_ports);
    }
    if (rc == APX_NO_ERROR && parsed.num_require_ports > 0u)
    {
      rc =
        decode_vm_programs_section(buf, &parsed.require_vm, node_instance, APX_REQUIRE_PORT, parsed.num_require_ports);
    }
  }

  if (rc == APX_NO_ERROR && out_embedded_def_text != NULL && parsed.def_text.present)
  {
    rc =
      decode_def_text_section(buf, &parsed.def_text, parsed.logical_def_size, expected_sha256, out_embedded_def_text);
  }

  if (rc != APX_NO_ERROR)
  {
    reset_node_instance_decoded_state(node_instance);
    if (out_embedded_def_text != NULL && *out_embedded_def_text != NULL)
    {
      free(*out_embedded_def_text);
      *out_embedded_def_text = NULL;
    }
  }
  return rc;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t parse_and_validate_container(uint8_t const *buf, size_t buf_size, char const *expected_node_name,
  uint8_t const expected_sha256[RMF_SHA256_SIZE], uint32_t expected_logical_size, apxc_parsed_container_t *out_parsed)
{
  if (buf == NULL || expected_node_name == NULL || expected_sha256 == NULL || out_parsed == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  memset(out_parsed, 0, sizeof(*out_parsed));
  if (buf_size < (size_t)APXC_MIN_FILE_SIZE)
  {
    return APX_LENGTH_ERROR;
  }
  if (buf_size > (size_t)APX_MAX_FILE_SIZE)
  {
    return APX_FILE_TOO_LARGE_ERROR;
  }

  // Step 1: Verify magic and format/VM/ABI version before interpreting payload layout
  if (buf[0] != APXC_MAGIC_0 || buf[1] != APXC_MAGIC_1 || buf[2] != APXC_MAGIC_2 || buf[3] != APXC_MAGIC_3)
  {
    return APX_INVALID_HEADER_ERROR;
  }
  uint8_t format_major = buf[4];
  uint8_t vm_major = buf[6];
  uint8_t vm_minor = buf[7];
  uint16_t compiler_abi = (uint16_t)cutil_unpack_le(buf + 8, 2u);
  if (format_major != APXC_FORMAT_MAJOR || vm_major != APXC_VM_MAJOR || vm_minor != APXC_VM_MINOR ||
    compiler_abi != APXC_COMPILER_ABI_VERSION)
  {
    return APX_VERSION_ERROR;
  }

  // Step 2: Verify exact file size and whole-file SHA-256 trailer before trusting any field
  uint32_t total_file_size = cutil_unpack_le(buf + 16, 4u);
  if ((size_t)total_file_size != buf_size)
  {
    return APX_LENGTH_ERROR;
  }
  size_t payload_end = buf_size - (size_t)APXC_TRAILER_SIZE;
  uint8_t actual_file_hash[RMF_SHA256_SIZE];
  apx_error_t rc = apx_crypto_sha256(buf, payload_end, actual_file_hash);
  if (rc != APX_NO_ERROR || memcmp(actual_file_hash, buf + payload_end, RMF_SHA256_SIZE) != 0)
  {
    return APX_CHECKSUM_ERROR;
  }

  // Step 3: Parse remaining fixed header fields via bounds-checked reader
  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, buf, payload_end);
  rc = apx_cache_reader_seek(&reader, 10u);
  uint16_t flags = 0u;
  uint16_t section_count = 0u;
  uint16_t node_name_len = 0u;
  uint32_t total_size_check = 0u;
  uint32_t logical_def_size = 0u;
  uint8_t def_sha256[RMF_SHA256_SIZE];
  uint16_t idl_major = 0u;
  uint16_t idl_minor = 0u;
  uint32_t num_provide_ports = 0u;
  uint32_t num_require_ports = 0u;
  uint32_t reserved = 0u;

  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u16_le(&reader, &flags);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u16_le(&reader, &section_count);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u16_le(&reader, &node_name_len);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u32_le(&reader, &total_size_check);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u32_le(&reader, &logical_def_size);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_bytes(&reader, def_sha256, RMF_SHA256_SIZE);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u16_le(&reader, &idl_major);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u16_le(&reader, &idl_minor);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u32_le(&reader, &num_provide_ports);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u32_le(&reader, &num_require_ports);
  }
  if (rc == APX_NO_ERROR)
  {
    rc = apx_cache_reader_read_u32_le(&reader, &reserved);
  }
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }

  if ((flags & ~((unsigned int)APXC_KNOWN_FLAGS)) != 0u || reserved != 0u || section_count > APXC_MAX_SECTIONS)
  {
    return APX_INVALID_HEADER_ERROR;
  }
  if (logical_def_size == 0u || logical_def_size > (uint32_t)APX_MAX_FILE_SIZE)
  {
    return APX_LENGTH_ERROR;
  }
  if (expected_logical_size > 0u && logical_def_size != expected_logical_size)
  {
    return APX_LENGTH_ERROR;
  }
  if (memcmp(def_sha256, expected_sha256, RMF_SHA256_SIZE) != 0)
  {
    return APX_CHECKSUM_ERROR;
  }
  if (num_provide_ports > APXC_MAX_PORTS || num_require_ports > APXC_MAX_PORTS)
  {
    return APX_TOO_MANY_PORTS_ERROR;
  }

  // Step 4: Validate node name binding
  size_t expected_name_len = strlen(expected_node_name);
  if (node_name_len == 0u || (size_t)node_name_len != expected_name_len ||
    node_name_len > (uint16_t)RMF_MAX_FILE_NAME_SIZE)
  {
    return APX_INVALID_NAME_ERROR;
  }
  uint8_t const *name_slice = NULL;
  rc = apx_cache_reader_read_slice(&reader, (size_t)node_name_len, &name_slice);
  if (rc != APX_NO_ERROR)
  {
    return rc;
  }
  if (memcmp(name_slice, expected_node_name, (size_t)node_name_len) != 0)
  {
    return APX_INVALID_NAME_ERROR;
  }

  // Step 5: Validate Section Directory bounds and uniqueness
  size_t dir_bytes = (size_t)section_count * (size_t)APXC_SECTION_ENTRY_SIZE;
  if (apx_cache_reader_remaining(&reader) < dir_bytes)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  size_t dir_end = apx_cache_reader_pos(&reader) + dir_bytes;

  uint16_t seen_types[APXC_MAX_SECTIONS];
  for (uint16_t i = 0u; i < section_count; i++)
  {
    uint16_t sec_type = 0u;
    uint16_t sec_flags = 0u;
    uint32_t sec_offset = 0u;
    uint32_t sec_size = 0u;
    rc = apx_cache_reader_read_u16_le(&reader, &sec_type);
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_reader_read_u16_le(&reader, &sec_flags);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_reader_read_u32_le(&reader, &sec_offset);
    }
    if (rc == APX_NO_ERROR)
    {
      rc = apx_cache_reader_read_u32_le(&reader, &sec_size);
    }
    if (rc != APX_NO_ERROR)
    {
      return rc;
    }

    for (uint16_t j = 0u; j < i; j++)
    {
      if (seen_types[j] == sec_type)
      {
        return APX_INVALID_HEADER_ERROR;
      }
    }
    seen_types[i] = sec_type;

    if ((size_t)sec_offset < dir_end || (size_t)sec_offset > payload_end ||
      (size_t)sec_size > (payload_end - (size_t)sec_offset))
    {
      return APX_BUFFER_BOUNDARY_ERROR;
    }

    apxc_section_entry_t *slot = NULL;
    switch (sec_type)
    {
    case APXC_SEC_PROVIDE_PORTS:
      slot = &out_parsed->provide_ports;
      break;
    case APXC_SEC_REQUIRE_PORTS:
      slot = &out_parsed->require_ports;
      break;
    case APXC_SEC_PROVIDE_INIT_DATA:
      slot = &out_parsed->provide_init;
      break;
    case APXC_SEC_REQUIRE_INIT_DATA:
      slot = &out_parsed->require_init;
      break;
    case APXC_SEC_PROVIDE_VM_PROGS:
      slot = &out_parsed->provide_vm;
      break;
    case APXC_SEC_REQUIRE_VM_PROGS:
      slot = &out_parsed->require_vm;
      break;
    case APXC_SEC_DEF_TEXT:
      if (sec_flags != APXC_DEF_TEXT_FLAG_RAW && sec_flags != APXC_DEF_TEXT_FLAG_ZLIB)
      {
        return APX_INVALID_HEADER_ERROR;
      }
      slot = &out_parsed->def_text;
      break;
    default:
      // Forward compatibility: unknown optional sections are bounds-checked and ignored
      break;
    }
    if (slot != NULL)
    {
      slot->type = sec_type;
      slot->flags = sec_flags;
      slot->offset = sec_offset;
      slot->size = sec_size;
      slot->present = true;
    }
  }

  // Step 6: Validate required/optional section consistency against header counts and flags
  if (num_provide_ports > 0u)
  {
    if (!out_parsed->provide_ports.present || out_parsed->provide_ports.size == 0u ||
      !out_parsed->provide_init.present || out_parsed->provide_init.size == 0u)
    {
      return APX_INVALID_HEADER_ERROR;
    }
  }
  else
  {
    if (out_parsed->provide_ports.present || out_parsed->provide_init.present || out_parsed->provide_vm.present)
    {
      return APX_INVALID_HEADER_ERROR;
    }
  }

  if (num_require_ports > 0u)
  {
    if (!out_parsed->require_ports.present || out_parsed->require_ports.size == 0u ||
      !out_parsed->require_init.present || out_parsed->require_init.size == 0u)
    {
      return APX_INVALID_HEADER_ERROR;
    }
  }
  else
  {
    if (out_parsed->require_ports.present || out_parsed->require_init.present || out_parsed->require_vm.present)
    {
      return APX_INVALID_HEADER_ERROR;
    }
  }

  if ((flags & APXC_FLAG_HAS_VM_PROGRAMS) != 0u)
  {
    if ((num_provide_ports > 0u && (!out_parsed->provide_vm.present || out_parsed->provide_vm.size == 0u)) ||
      (num_require_ports > 0u && (!out_parsed->require_vm.present || out_parsed->require_vm.size == 0u)))
    {
      return APX_INVALID_HEADER_ERROR;
    }
  }
  if ((flags & APXC_FLAG_HAS_DEF_TEXT) != 0u)
  {
    if (!out_parsed->def_text.present || out_parsed->def_text.size == 0u)
    {
      return APX_INVALID_HEADER_ERROR;
    }
  }

  out_parsed->flags = flags;
  out_parsed->idl_major = idl_major;
  out_parsed->idl_minor = idl_minor;
  out_parsed->logical_def_size = logical_def_size;
  out_parsed->num_provide_ports = num_provide_ports;
  out_parsed->num_require_ports = num_require_ports;
  return APX_NO_ERROR;
}

static void reset_node_instance_decoded_state(apx_node_instance_t *node_instance)
{
  if (node_instance == NULL)
  {
    return;
  }
  if (node_instance->provide_ports != NULL)
  {
    for (apx_size_t i = 0u; i < node_instance->num_provide_ports; i++)
    {
      apx_port_instance_destroy(&node_instance->provide_ports[i]);
    }
    free(node_instance->provide_ports);
    node_instance->provide_ports = NULL;
  }
  if (node_instance->require_ports != NULL)
  {
    for (apx_size_t i = 0u; i < node_instance->num_require_ports; i++)
    {
      apx_port_instance_destroy(&node_instance->require_ports[i]);
    }
    free(node_instance->require_ports);
    node_instance->require_ports = NULL;
  }
  node_instance->num_provide_ports = 0u;
  node_instance->num_require_ports = 0u;
  free(node_instance->provide_port_init_data);
  node_instance->provide_port_init_data = NULL;
  node_instance->provide_port_init_data_size = 0u;
  free(node_instance->require_port_init_data);
  node_instance->require_port_init_data = NULL;
  node_instance->require_port_init_data_size = 0u;
}

static apx_error_t decode_port_table_section(uint8_t const *buf, apxc_section_entry_t const *sec,
  apx_node_instance_t *node_instance, apx_port_type_t port_type, uint32_t num_ports, uint32_t expected_total_data_size)
{
  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, buf + sec->offset, (size_t)sec->size);
  uint32_t running_offset = 0u;
  for (uint32_t i = 0u; i < num_ports; i++)
  {
    apx_port_instance_t *port = (port_type == APX_PROVIDE_PORT) ?
      apx_node_instance_get_provide_port(node_instance, (apx_port_id_t)i) :
      apx_node_instance_get_require_port(node_instance, (apx_port_id_t)i);
    if (port == NULL)
    {
      return APX_INTERNAL_ERROR;
    }
    apx_error_t rc = apx_cache_decode_port_descriptor(
      &reader, node_instance, port_type, (apx_port_id_t)i, running_offset, port, &running_offset);
    if (rc != APX_NO_ERROR)
    {
      return rc;
    }
  }
  if (apx_cache_reader_remaining(&reader) != 0u || running_offset != expected_total_data_size)
  {
    return APX_LENGTH_ERROR;
  }
  return APX_NO_ERROR;
}

static apx_error_t decode_vm_programs_section(uint8_t const *buf, apxc_section_entry_t const *sec,
  apx_node_instance_t *node_instance, apx_port_type_t port_type, uint32_t num_ports)
{
  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, buf + sec->offset, (size_t)sec->size);
  for (uint32_t i = 0u; i < num_ports; i++)
  {
    apx_port_instance_t *port = (port_type == APX_PROVIDE_PORT) ?
      apx_node_instance_get_provide_port(node_instance, (apx_port_id_t)i) :
      apx_node_instance_get_require_port(node_instance, (apx_port_id_t)i);
    if (port == NULL)
    {
      return APX_INTERNAL_ERROR;
    }
    apx_program_t *pack_prog = NULL;
    apx_program_t *unpack_prog = NULL;
    apx_error_t rc = apx_cache_decode_program(&reader, false, &pack_prog);
    if (rc != APX_NO_ERROR)
    {
      return rc;
    }
    if (port_type == APX_REQUIRE_PORT)
    {
      rc = apx_cache_decode_program(&reader, false, &unpack_prog);
      if (rc != APX_NO_ERROR)
      {
        APX_PROGRAM_DELETE(pack_prog);
        return rc;
      }
    }
    apx_port_instance_set_programs(port, pack_prog, unpack_prog);
  }
  if (apx_cache_reader_remaining(&reader) != 0u)
  {
    return APX_LENGTH_ERROR;
  }
  return APX_NO_ERROR;
}

static apx_error_t decode_def_text_section(uint8_t const *buf, apxc_section_entry_t const *sec,
  uint32_t logical_def_size, uint8_t const expected_sha256[RMF_SHA256_SIZE], uint8_t **out_def_text)
{
  uint8_t *text_buf = (uint8_t *)malloc((size_t)logical_def_size + 1u);
  if (text_buf == NULL)
  {
    return APX_MEM_ERROR;
  }
  uint8_t const *sec_data = buf + sec->offset;
  if (sec->flags == APXC_DEF_TEXT_FLAG_RAW)
  {
    if (sec->size != logical_def_size)
    {
      free(text_buf);
      return APX_LENGTH_ERROR;
    }
    memcpy(text_buf, sec_data, (size_t)logical_def_size);
  }
  else if (sec->flags == APXC_DEF_TEXT_FLAG_ZLIB)
  {
    size_t dst_len = (size_t)logical_def_size;
    apx_error_t rc = apx_decompress(text_buf, &dst_len, sec_data, (size_t)sec->size, (size_t)logical_def_size);
    if (rc != APX_NO_ERROR || dst_len != (size_t)logical_def_size)
    {
      free(text_buf);
      return (rc != APX_NO_ERROR) ? rc : APX_LENGTH_ERROR;
    }
  }
  else
  {
    free(text_buf);
    return APX_INVALID_HEADER_ERROR;
  }

  uint8_t actual_hash[RMF_SHA256_SIZE];
  apx_error_t hash_rc = apx_crypto_sha256(text_buf, (size_t)logical_def_size, actual_hash);
  if (hash_rc != APX_NO_ERROR || memcmp(actual_hash, expected_sha256, RMF_SHA256_SIZE) != 0)
  {
    free(text_buf);
    return APX_CHECKSUM_ERROR;
  }
  text_buf[logical_def_size] = '\0';
  *out_def_text = text_buf;
  return APX_NO_ERROR;
}
