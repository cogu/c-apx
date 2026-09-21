/*****************************************************************************
* \file      extended_file_info.h
* \author    Conny Gustafsson
* \date      2020-04-10
* \brief     Disposable extended file info data structure
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_EXTENDED_FILE_INFO_H
#define APX_EXTENDED_FILE_INFO_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "adt_str.h"
#include "apx/remotefile.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct rmf_extended_file_info_tag
{
   uint32_t connection_id;
   uint32_t address;
   uint32_t size;
   rmf_file_type_t rmf_file_type;
   rmf_digest_type_t digest_type;
   apx_data_state_t data_state;
   uint8_t digest_data[RMF_SHA256_SIZE];
   adt_str_t name;
} rmf_extended_file_info_t;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
rmf_extended_file_info_t* rmf_extended_file_info_make_empty(void);
rmf_extended_file_info_t* rmf_extended_file_info_make_fixed(char const* name, uint32_t size, uint32_t address, uint32_t connection_id, apx_data_state_t data_state);
rmf_extended_file_info_t* rmf_extended_file_info_make_fixed_with_digest(char const* name, uint32_t size, uint32_t address, uint32_t connection_id, apx_data_state_t data_state, rmf_digest_type_t digest_type, uint8_t const* digest_data);

apx_error_t rmf_extended_file_info_create(rmf_extended_file_info_t *self, uint32_t connection_id, uint32_t address, uint32_t size, const char *name, rmf_file_type_t file_type, apx_data_state_t data_state, rmf_digest_type_t digest_type, const uint8_t *digest_data);
apx_error_t rmf_extended_file_info_create_copy(rmf_extended_file_info_t* self, rmf_extended_file_info_t const* other);
void rmf_extended_file_info_destroy(rmf_extended_file_info_t *self);
rmf_extended_file_info_t* rmf_extended_file_info_new(uint32_t address, uint32_t size, const char *name, rmf_file_type_t file_type, rmf_digest_type_t digest_type, const uint8_t *digest_data);
void rmf_extended_file_info_delete(rmf_extended_file_info_t* self);
void rmf_extended_file_info_vdelete(void* arg);

const char* rmf_extended_file_info_name(rmf_extended_file_info_t const* self);
uint32_t rmf_extended_file_info_address(rmf_extended_file_info_t const* self);
uint32_t rmf_extended_file_info_address_without_flags(rmf_extended_file_info_t const* self);
uint32_t rmf_extended_file_info_size(rmf_extended_file_info_t const* self);
rmf_file_type_t rmf_extended_file_info_rmf_file_type(rmf_extended_file_info_t const* self);
rmf_digest_type_t rmf_extended_file_info_digest_type(rmf_extended_file_info_t const* self);
uint8_t const* rmf_extended_file_info_digest_data(rmf_extended_file_info_t const* self);
apx_error_t rmf_extended_file_info_assign(rmf_extended_file_info_t *self, const rmf_extended_file_info_t *other);
rmf_extended_file_info_t* rmf_extended_file_info_clone(const rmf_extended_file_info_t *other);
void rmf_extended_file_info_set_address(rmf_extended_file_info_t *self, uint32_t address);
bool rmf_extended_file_info_is_remote_address(rmf_extended_file_info_t const* self);
bool rmf_extended_file_info_name_ends_with(rmf_extended_file_info_t const* self, const char* suffix);
char *rmf_extended_file_info_base_name(rmf_extended_file_info_t const* self);
void rmf_extended_file_info_copy_base_name(rmf_extended_file_info_t const* self, char *dest, uint32_t max_dest_len);

bool rmf_extended_file_info_address_in_range(rmf_extended_file_info_t const* self, uint32_t address);
apx_error_t rmf_extended_file_info_set_digest_data(rmf_extended_file_info_t* self, rmf_digest_type_t digest_type, const uint8_t* digest_data);

//stateless functions
apx_size_t rmf_encode_publish_remote_file_cmd(uint8_t* buf, apx_size_t buf_size, rmf_extended_file_info_t const* file_info);
apx_size_t rmf_decode_publish_remote_file_cmd(uint8_t const* buf, apx_size_t buf_size, rmf_extended_file_info_t* file_info);
bool apx_value_to_data_state(uint8_t value, apx_data_state_t* data_state);


#endif //APX_EXTENDED_FILE_INFO_H
