/*****************************************************************************
* \file      file_info.h
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Disposable file info data structure
*
* Copyright (c) 2020-2021 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
#ifndef RMF_FILE_INFO_H
#define RMF_FILE_INFO_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "adt_str.h"
#include "apx/remotefile.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct rmf_fileInfo_tag
{
   uint32_t address;
   uint32_t size;
   rmf_fileType_t rmf_file_type;
   rmf_digestType_t digest_type;
   uint8_t digest_data[RMF_SHA256_SIZE];
   adt_str_t name;
} rmf_fileInfo_t;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
rmf_fileInfo_t* rmf_fileInfo_make_empty(void);
rmf_fileInfo_t* rmf_fileInfo_make_fixed(char const* name, uint32_t size, uint32_t address);
rmf_fileInfo_t* rmf_fileInfo_make_fixed_with_digest(char const* name, uint32_t size, uint32_t address, rmf_digestType_t digest_type, uint8_t const* digest_data);

apx_error_t rmf_fileInfo_create(rmf_fileInfo_t *self, uint32_t address, uint32_t size, const char *name, rmf_fileType_t file_type, rmf_digestType_t digest_type, const uint8_t *digest_data);
apx_error_t rmf_fileInfo_create_copy(rmf_fileInfo_t* self, rmf_fileInfo_t const* other);
void rmf_fileInfo_destroy(rmf_fileInfo_t *self);
rmf_fileInfo_t* rmf_fileInfo_new(uint32_t address, uint32_t size, const char *name, rmf_fileType_t file_type, rmf_digestType_t digest_type, const uint8_t *digest_data);
void rmf_fileInfo_delete(rmf_fileInfo_t* self);
void rmf_fileInfo_vdelete(void* arg);

const char* rmf_fileInfo_name(rmf_fileInfo_t const* self);
uint32_t rmf_fileInfo_address(rmf_fileInfo_t const* self);
uint32_t rmf_fileInfo_address_without_flags(rmf_fileInfo_t const* self);
uint32_t rmf_fileInfo_size(rmf_fileInfo_t const* self);
rmf_fileType_t rmf_fileInfo_rmf_file_type(rmf_fileInfo_t const* self);
rmf_digestType_t rmf_fileInfo_digest_type(rmf_fileInfo_t const* self);
uint8_t const* rmf_fileInfo_digest_data(rmf_fileInfo_t const* self);
apx_error_t rmf_fileInfo_assign(rmf_fileInfo_t *self, const rmf_fileInfo_t *other);
rmf_fileInfo_t* rmf_fileInfo_clone(const rmf_fileInfo_t *other);
void rmf_fileInfo_set_address(rmf_fileInfo_t *self, uint32_t address);
bool rmf_fileInfo_is_remote_address(rmf_fileInfo_t const* self);
bool rmf_fileInfo_name_ends_with(rmf_fileInfo_t const* self, const char* suffix);
char *rmf_fileInfo_base_name(rmf_fileInfo_t const* self);
void rmf_fileInfo_copy_base_name(rmf_fileInfo_t const* self, char *dest, uint32_t max_dest_len);

bool rmf_fileInfo_address_in_range(rmf_fileInfo_t const* self, uint32_t address);
apx_error_t rmf_fileInfo_set_digest_data(rmf_fileInfo_t* self, rmf_digestType_t digest_type, const uint8_t* digest_data);

//stateless functions
apx_size_t rmf_encode_publish_file_cmd(uint8_t* buf, apx_size_t buf_size, rmf_fileInfo_t const* file);
apx_size_t rmf_decode_publish_file_cmd(uint8_t const* buf, apx_size_t buf_size, rmf_fileInfo_t* file_info);
bool rmf_value_to_file_type(uint16_t value, rmf_fileType_t* file_type);
bool rmf_value_to_digest_type(uint16_t value, rmf_digestType_t* digest_type);


#endif //RMF_FILE_INFO_H
