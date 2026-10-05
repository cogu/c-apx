/*****************************************************************************
 * \file      file_info.h
 * \author    Conny Gustafsson
 * \date      2020-01-03
 * \brief     Disposable file info data structure
 *
 * Copyright (c) 2020-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef RMF_FILE_INFO_H
#define RMF_FILE_INFO_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_str.h"
#include "apx/error.h"
#include "apx/remotefile.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct rmf_file_info_tag
{
  uint32_t address;
  uint32_t size;
  rmf_file_type_t rmf_file_type;
  rmf_digest_type_t digest_type;
  uint8_t digest_data[RMF_SHA256_SIZE];
  rmf_signature_type_t signature_type;
  uint8_t signature_data[RMF_SIGNATURE_SIZE_ECDSA_P256];
  adt_str_t name;
} rmf_file_info_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
rmf_file_info_t *rmf_file_info_make_empty(void);
rmf_file_info_t *rmf_file_info_make_fixed(char const *name, uint32_t size, uint32_t address);
rmf_file_info_t *rmf_file_info_make_fixed_with_digest(
  char const *name, uint32_t size, uint32_t address, rmf_digest_type_t digest_type, uint8_t const *digest_data);
rmf_file_info_t *rmf_file_info_make_fixed_with_signature(char const *name, uint32_t size, uint32_t address,
  rmf_signature_type_t signature_type, uint8_t const *signature_data);

apx_error_t rmf_file_info_create(rmf_file_info_t *self, uint32_t address, uint32_t size, const char *name,
  rmf_file_type_t file_type, rmf_digest_type_t digest_type, const uint8_t *digest_data);
apx_error_t rmf_file_info_create_copy(rmf_file_info_t *self, rmf_file_info_t const *other);
void rmf_file_info_destroy(rmf_file_info_t *self);
rmf_file_info_t *rmf_file_info_new(uint32_t address, uint32_t size, const char *name, rmf_file_type_t file_type,
  rmf_digest_type_t digest_type, const uint8_t *digest_data);
void rmf_file_info_delete(rmf_file_info_t *self);
void rmf_file_info_vdelete(void *arg);

const char *rmf_file_info_name(rmf_file_info_t const *self);
uint32_t rmf_file_info_address(rmf_file_info_t const *self);
uint32_t rmf_file_info_address_without_flags(rmf_file_info_t const *self);
uint32_t rmf_file_info_size(rmf_file_info_t const *self);
rmf_file_type_t rmf_file_info_rmf_file_type(rmf_file_info_t const *self);
rmf_digest_type_t rmf_file_info_digest_type(rmf_file_info_t const *self);
uint8_t const *rmf_file_info_digest_data(rmf_file_info_t const *self);
rmf_signature_type_t rmf_file_info_signature_type(rmf_file_info_t const *self);
uint8_t const *rmf_file_info_signature_data(rmf_file_info_t const *self);
bool rmf_file_info_is_signed(rmf_file_info_t const *self);
apx_error_t rmf_file_info_assign(rmf_file_info_t *self, const rmf_file_info_t *other);
rmf_file_info_t *rmf_file_info_clone(const rmf_file_info_t *other);
void rmf_file_info_set_address(rmf_file_info_t *self, uint32_t address);
bool rmf_file_info_is_remote_address(rmf_file_info_t const *self);
bool rmf_file_info_name_ends_with(rmf_file_info_t const *self, const char *suffix);
char *rmf_file_info_base_name(rmf_file_info_t const *self);
void rmf_file_info_copy_base_name(rmf_file_info_t const *self, char *dest, uint32_t max_dest_len);

bool rmf_file_info_address_in_range(rmf_file_info_t const *self, uint32_t address);
apx_error_t rmf_file_info_set_digest_data(
  rmf_file_info_t *self, rmf_digest_type_t digest_type, const uint8_t *digest_data);
apx_error_t rmf_file_info_set_signature(
  rmf_file_info_t *self, rmf_signature_type_t signature_type, const uint8_t *signature_data);

#endif // RMF_FILE_INFO_H
