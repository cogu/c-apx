/*****************************************************************************
 * \file      extended_file_info.c
 * \author    Conny Gustafsson
 * \date      2020-04-10
 * \brief     Extended file information implementation
 *
 * Copyright (c) 2020-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extended_file_info.h"
#include "apx/util.h"
#include "bstr.h"
#include "pack.h"
#include <assert.h>
#include <malloc.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define RMF_EXTENDED_FILE_INFO_HEADER_SIZE 53u

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

rmf_extended_file_info_t *rmf_extended_file_info_make_empty(void)
{
  return rmf_extended_file_info_new(APX_INVALID_CONNECTION_ID, RMF_INVALID_ADDRESS, 0u, NULL, RMF_FILE_TYPE_FIXED,
    APX_DATA_STATE_INIT, RMF_DIGEST_TYPE_NONE, NULL);
}

rmf_extended_file_info_t *rmf_extended_file_info_make_fixed(
  char const *name, uint32_t size, uint32_t address, uint32_t connection_id, apx_data_state_t data_state)
{
  return rmf_extended_file_info_new(
    connection_id, address, size, name, RMF_FILE_TYPE_FIXED, data_state, RMF_DIGEST_TYPE_NONE, NULL);
}

rmf_extended_file_info_t *rmf_extended_file_info_make_fixed_with_digest(char const *name, uint32_t size,
  uint32_t address, uint32_t connection_id, apx_data_state_t data_state, rmf_digest_type_t digest_type,
  uint8_t const *digest_data)
{
  return rmf_extended_file_info_new(
    connection_id, address, size, name, RMF_FILE_TYPE_FIXED, data_state, digest_type, digest_data);
}

apx_error_t rmf_extended_file_info_create(rmf_extended_file_info_t *self, uint32_t connection_id, uint32_t address,
  uint32_t size, const char *name, rmf_file_type_t file_type, apx_data_state_t data_state,
  rmf_digest_type_t digest_type, const uint8_t *digest_data)
{
  if (self != NULL)
  {
    adt_error_t result;
    self->connection_id = connection_id;
    self->address = address;
    self->size = size;
    self->rmf_file_type = file_type;
    self->data_state = data_state;
    self->digest_type = digest_type;
    adt_str_create(&self->name);
    if (name != NULL)
    {
      result = adt_str_set_cstr(&self->name, name);
      if (result != ADT_NO_ERROR)
      {
        return convert_from_adt_to_apx_error(result);
      }
    }
    return rmf_extended_file_info_set_digest_data(self, digest_type, digest_data);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t rmf_extended_file_info_create_copy(rmf_extended_file_info_t *self, rmf_extended_file_info_t const *other)
{
  if ((self != NULL) && (other != NULL))
  {
    adt_error_t result;
    self->connection_id = other->connection_id;
    self->address = other->address;
    self->size = other->size;
    self->rmf_file_type = other->rmf_file_type;
    self->data_state = other->data_state;
    self->digest_type = other->digest_type;
    adt_str_create(&self->name);
    result = adt_str_set(&self->name, &other->name);
    if (result != ADT_NO_ERROR)
    {
      return convert_from_adt_to_apx_error(result);
    }
    return rmf_extended_file_info_set_digest_data(self, other->digest_type, &other->digest_data[0]);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t rmf_extended_file_info_create_from_file_info(
  rmf_extended_file_info_t *self, uint32_t connection_id, rmf_file_info_t const *file_info, apx_data_state_t data_state)
{
  if ((self != NULL) && (file_info != NULL))
  {
    return rmf_extended_file_info_create(self, connection_id, rmf_file_info_address(file_info),
      rmf_file_info_size(file_info), rmf_file_info_name(file_info), rmf_file_info_rmf_file_type(file_info), data_state,
      rmf_file_info_digest_type(file_info), rmf_file_info_digest_data(file_info));
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

void rmf_extended_file_info_destroy(rmf_extended_file_info_t *self)
{
  if (self != NULL)
  {
    adt_str_destroy(&self->name);
  }
}

rmf_extended_file_info_t *rmf_extended_file_info_new(uint32_t connection_id, uint32_t address, uint32_t size,
  const char *name, rmf_file_type_t file_type, apx_data_state_t data_state, rmf_digest_type_t digest_type,
  const uint8_t *digest_data)
{
  rmf_extended_file_info_t *self = (rmf_extended_file_info_t *)malloc(sizeof(rmf_extended_file_info_t));
  if (self != NULL)
  {
    apx_error_t rc = rmf_extended_file_info_create(
      self, connection_id, address, size, name, file_type, data_state, digest_type, digest_data);
    if (rc != APX_NO_ERROR)
    {
      free(self);
      self = NULL;
    }
  }
  return self;
}

void rmf_extended_file_info_delete(rmf_extended_file_info_t *self)
{
  if (self != NULL)
  {
    rmf_extended_file_info_destroy(self);
    free(self);
  }
}

void rmf_extended_file_info_vdelete(void *arg) { rmf_extended_file_info_delete((rmf_extended_file_info_t *)arg); }

const char *rmf_extended_file_info_name(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return adt_str_cstr((adt_str_t *)&self->name);
  }
  return NULL;
}

uint32_t rmf_extended_file_info_connection_id(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return self->connection_id;
  }
  return APX_INVALID_CONNECTION_ID;
}

void rmf_extended_file_info_set_connection_id(rmf_extended_file_info_t *self, uint32_t connection_id)
{
  if (self != NULL)
  {
    self->connection_id = connection_id;
  }
}

uint32_t rmf_extended_file_info_address(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return self->address;
  }
  return RMF_INVALID_ADDRESS;
}

uint32_t rmf_extended_file_info_address_without_flags(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return self->address & RMF_ADDRESS_MASK_INTERNAL;
  }
  return RMF_INVALID_ADDRESS;
}

uint32_t rmf_extended_file_info_size(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return self->size;
  }
  return 0u;
}

rmf_file_type_t rmf_extended_file_info_rmf_file_type(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return self->rmf_file_type;
  }
  return RMF_FILE_TYPE_FIXED;
}

rmf_digest_type_t rmf_extended_file_info_digest_type(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return self->digest_type;
  }
  return RMF_DIGEST_TYPE_NONE;
}

uint8_t const *rmf_extended_file_info_digest_data(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return &self->digest_data[0];
  }
  return NULL;
}

apx_data_state_t rmf_extended_file_info_data_state(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return self->data_state;
  }
  return APX_DATA_STATE_INIT;
}

void rmf_extended_file_info_set_data_state(rmf_extended_file_info_t *self, apx_data_state_t data_state)
{
  if (self != NULL)
  {
    self->data_state = data_state;
  }
}

apx_error_t rmf_extended_file_info_assign(rmf_extended_file_info_t *self, const rmf_extended_file_info_t *other)
{
  if ((self != NULL) && (other != NULL))
  {
    adt_error_t result;
    self->connection_id = other->connection_id;
    self->address = other->address;
    self->size = other->size;
    self->rmf_file_type = other->rmf_file_type;
    self->data_state = other->data_state;
    self->digest_type = other->digest_type;
    memcpy(&self->digest_data[0], &other->digest_data[0], sizeof(self->digest_data));
    result = adt_str_set(&self->name, &other->name);
    if (result != ADT_NO_ERROR)
    {
      return convert_from_adt_to_apx_error(result);
    }
    return APX_NO_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

rmf_extended_file_info_t *rmf_extended_file_info_clone(const rmf_extended_file_info_t *other)
{
  if (other != NULL)
  {
    rmf_extended_file_info_t *copy = (rmf_extended_file_info_t *)malloc(sizeof(rmf_extended_file_info_t));
    if (copy != NULL)
    {
      apx_error_t rc = rmf_extended_file_info_create_copy(copy, other);
      if (rc != APX_NO_ERROR)
      {
        free(copy);
        copy = NULL;
      }
    }
    return copy;
  }
  return NULL;
}

void rmf_extended_file_info_set_address(rmf_extended_file_info_t *self, uint32_t address)
{
  if (self != NULL)
  {
    self->address = address;
  }
}

bool rmf_extended_file_info_is_remote_address(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    return ((self->address != RMF_INVALID_ADDRESS) && ((self->address & RMF_REMOTE_ADDRESS_BIT) != 0u));
  }
  return false;
}

bool rmf_extended_file_info_name_ends_with(rmf_extended_file_info_t const *self, const char *suffix)
{
  if ((self != NULL) && (suffix != NULL) && (adt_str_size(&self->name) > 0))
  {
    size_t str_len = adt_str_length(&self->name);
    size_t suffix_len = strlen(suffix);
    return (str_len >= suffix_len) &&
      (strcmp(adt_str_cstr((adt_str_t *)&self->name) + (str_len - suffix_len), suffix) == 0);
  }
  return false;
}

char *rmf_extended_file_info_base_name(rmf_extended_file_info_t const *self)
{
  if (self != NULL)
  {
    char const *name = adt_str_cstr((adt_str_t *)&self->name);
    char *dot = strchr(name, '.');
    if (dot != NULL)
    {
      return bstr_make_cstr((const uint8_t *)name, (const uint8_t *)dot);
    }
  }
  return NULL;
}

void rmf_extended_file_info_copy_base_name(rmf_extended_file_info_t const *self, char *dest, uint32_t max_dest_len)
{
  if ((self != NULL) && (dest != NULL) && (max_dest_len > 1))
  {
    char const *name = adt_str_cstr((adt_str_t *)&self->name);
    char *dot = strchr(name, '.');
    if (dot != NULL)
    {
      uint32_t len = (uint32_t)(dot - name);
      if (max_dest_len < len)
      {
        strncpy(dest, name, max_dest_len - 1);
        dest[max_dest_len - 1] = '\0';
      }
      else
      {
        strncpy(dest, name, len);
        dest[len] = '\0';
      }
    }
    else
    {
      dest[0] = '\0';
    }
  }
}

bool rmf_extended_file_info_address_in_range(rmf_extended_file_info_t const *self, uint32_t address)
{
  if (self != NULL)
  {
    uint32_t const start_address = rmf_extended_file_info_address_without_flags(self);
    if (start_address != RMF_INVALID_ADDRESS)
    {
      uint32_t const end_address = start_address + self->size;
      return (address >= start_address) && (address < end_address);
    }
  }
  return false;
}

apx_error_t rmf_extended_file_info_set_digest_data(
  rmf_extended_file_info_t *self, rmf_digest_type_t digest_type, const uint8_t *digest_data)
{
  if (self != NULL)
  {
    self->digest_type = digest_type;
    if (digest_type == RMF_DIGEST_TYPE_NONE)
    {
      memset(&self->digest_data[0], 0, sizeof(self->digest_data));
      return APX_NO_ERROR;
    }
    else if ((digest_type == RMF_DIGEST_TYPE_SHA256) && (digest_data != NULL))
    {
      memcpy(&self->digest_data[0], digest_data, RMF_SHA256_SIZE);
      return APX_NO_ERROR;
    }
    return APX_INVALID_ARGUMENT_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

// stateless functions

apx_size_t rmf_encode_publish_remote_file_cmd(
  uint8_t *buf, apx_size_t buf_size, rmf_extended_file_info_t const *file_info)
{
  if ((buf != NULL) && (file_info != NULL))
  {
    const char *name = rmf_extended_file_info_name(file_info);
    if (name == NULL)
    {
      return 0u;
    }
    apx_size_t const name_size = (apx_size_t)strlen(name);
    apx_size_t const required_size = RMF_EXTENDED_FILE_INFO_HEADER_SIZE + name_size + 1u;
    if (required_size > buf_size)
    {
      return 0u;
    }
    uint8_t *p = buf;
    packLE(p, RMF_CMD_PUBLISH_REMOTE_FILE_MSG, (uint8_t)UINT32_SIZE);
    p += UINT32_SIZE;
    packLE(p, file_info->connection_id, (uint8_t)UINT32_SIZE);
    p += UINT32_SIZE;
    packLE(p, (uint32_t)file_info->data_state, (uint8_t)UINT8_SIZE);
    p += UINT8_SIZE;
    packLE(p, rmf_extended_file_info_address_without_flags(file_info), (uint8_t)UINT32_SIZE);
    p += UINT32_SIZE;
    packLE(p, file_info->size, (uint8_t)UINT32_SIZE);
    p += UINT32_SIZE;
    packLE(p, (uint32_t)file_info->rmf_file_type, (uint8_t)UINT16_SIZE);
    p += UINT16_SIZE;
    packLE(p, (uint32_t)file_info->digest_type, (uint8_t)UINT16_SIZE);
    p += UINT16_SIZE;
    if (file_info->digest_type == RMF_DIGEST_TYPE_SHA256)
    {
      memcpy(p, file_info->digest_data, RMF_SHA256_SIZE);
    }
    else
    {
      memset(p, 0, RMF_SHA256_SIZE);
    }
    p += RMF_SHA256_SIZE;
    memcpy(p, name, name_size);
    p += name_size;
    *p = 0u;
    return required_size;
  }
  return 0u;
}

apx_size_t rmf_decode_publish_remote_file_cmd(
  uint8_t const *buf, apx_size_t buf_size, rmf_extended_file_info_t *file_info)
{
  if ((buf != NULL) && (file_info != NULL) && (buf_size >= RMF_EXTENDED_FILE_INFO_HEADER_SIZE + 1u))
  {
    uint8_t const *p = buf;
    uint32_t const cmd_type = unpackLE(p, UINT32_SIZE);
    if (cmd_type != RMF_CMD_PUBLISH_REMOTE_FILE_MSG)
    {
      return 0u;
    }
    p += UINT32_SIZE;
    uint32_t const connection_id = unpackLE(p, UINT32_SIZE);
    p += UINT32_SIZE;
    uint8_t const raw_data_state = (uint8_t)unpackLE(p, UINT8_SIZE);
    apx_data_state_t data_state = APX_DATA_STATE_INIT;
    if (!apx_value_to_data_state(raw_data_state, &data_state))
    {
      return 0u;
    }
    p += UINT8_SIZE;
    uint32_t const address = unpackLE(p, UINT32_SIZE);
    p += UINT32_SIZE;
    uint32_t const size = unpackLE(p, UINT32_SIZE);
    p += UINT32_SIZE;
    rmf_file_type_t const file_type = (rmf_file_type_t)unpackLE(p, UINT16_SIZE);
    p += UINT16_SIZE;
    rmf_digest_type_t const digest_type = (rmf_digest_type_t)unpackLE(p, UINT16_SIZE);
    p += UINT16_SIZE;
    uint8_t const *digest_data = p;
    p += RMF_SHA256_SIZE;

    // Remaining bytes must contain null-terminated name
    uint8_t const *end = buf + buf_size;
    uint8_t const *name_end = memchr(p, 0, end - p);
    if (name_end == NULL)
    {
      return 0u;
    }
    const char *name = (const char *)p;
    apx_error_t rc = rmf_extended_file_info_create(
      file_info, connection_id, address, size, name, file_type, data_state, digest_type, digest_data);
    if (rc != APX_NO_ERROR)
    {
      return 0u;
    }
    return (apx_size_t)(name_end - buf + 1u);
  }
  return 0u;
}

bool apx_value_to_data_state(uint8_t value, apx_data_state_t *data_state)
{
  if (value <= APX_DATA_STATE_DISCONNECTED)
  {
    if (data_state != NULL)
    {
      *data_state = (apx_data_state_t)value;
    }
    return true;
  }
  return false;
}
