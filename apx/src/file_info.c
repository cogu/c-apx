/*****************************************************************************
* \file      file_info.c
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Disposable file info data structure
*
* Copyright (c) 2020 Conny Gustafsson
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "apx/file_info.h"
#include "apx/util.h"
#include "bstr.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

rmf_fileInfo_t* rmf_fileInfo_make_empty(void)
{
   return rmf_fileInfo_new(RMF_INVALID_ADDRESS, 0u, NULL, RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
}

rmf_fileInfo_t* rmf_fileInfo_make_fixed(char const* name, uint32_t size, uint32_t address)
{
   return rmf_fileInfo_new(address, size, name, RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
}

rmf_fileInfo_t* rmf_fileInfo_make_fixed_with_digest(char const* name, uint32_t size, uint32_t address, rmf_digestType_t digest_type, uint8_t const* digest_data)
{
   return rmf_fileInfo_new(address, size, name, RMF_FILE_TYPE_FIXED, digest_type, digest_data);
}

apx_error_t rmf_fileInfo_create(rmf_fileInfo_t* self, uint32_t address, uint32_t size, const char* name, rmf_fileType_t file_type, rmf_digestType_t digest_type, const uint8_t* digest_data)
{
   if ( self != NULL)
   {
      adt_error_t result;
      self->address = address;
      self->size = size;
      self->rmf_file_type = file_type;
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
      return rmf_fileInfo_set_digest_data(self, digest_type, digest_data);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t rmf_fileInfo_create_copy(rmf_fileInfo_t* self, rmf_fileInfo_t const* other)
{
   if (self != NULL)
   {
      adt_error_t result;
      self->address = other->address;
      self->size = other->size;
      self->rmf_file_type = other->rmf_file_type;
      self->digest_type = other->digest_type;
      adt_str_create(&self->name);
      result = adt_str_set(&self->name, &other->name);
      if (result != ADT_NO_ERROR)
      {
         return convert_from_adt_to_apx_error(result);
      }
      return rmf_fileInfo_set_digest_data(self, other->digest_type, &other->digest_data[0]);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void rmf_fileInfo_destroy(rmf_fileInfo_t *self)
{
   if (self != 0)
   {
      adt_str_destroy(&self->name);
   }
}

rmf_fileInfo_t* rmf_fileInfo_new(uint32_t address, uint32_t size, const char* name, rmf_fileType_t file_type, rmf_digestType_t digest_type, const uint8_t* digest_data)
{
   rmf_fileInfo_t *self = (rmf_fileInfo_t*) malloc(sizeof(rmf_fileInfo_t));
   if (self != 0)
   {
      apx_error_t rc = rmf_fileInfo_create(self, address, size, name, file_type, digest_type, digest_data);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = (rmf_fileInfo_t*) 0;
      }
   }
   return self;
}

void rmf_fileInfo_delete(rmf_fileInfo_t* self)
{
   if (self != 0)
   {
      rmf_fileInfo_destroy(self);
      free(self);
   }
}

void rmf_fileInfo_vdelete(void* arg)
{
   rmf_fileInfo_delete((rmf_fileInfo_t*)arg);
}

const char* rmf_fileInfo_name(rmf_fileInfo_t const* self)
{
   if (self != NULL)
   {
      return adt_str_cstr((adt_str_t*)&self->name);
   }
   return NULL;
}

uint32_t rmf_fileInfo_address(rmf_fileInfo_t const* self)
{
   if (self != NULL)
   {
      return self->address;
   }
   return RMF_INVALID_ADDRESS;
}

uint32_t rmf_fileInfo_size(rmf_fileInfo_t const* self)
{
   if (self != NULL)
   {
      return self->size;
   }
   return 0u;
}

rmf_fileType_t rmf_fileInfo_rmf_file_type(rmf_fileInfo_t const* self)
{
   if (self != NULL)
   {
      return self->rmf_file_type;
   }
   return RMF_FILE_TYPE_FIXED;
}

rmf_digestType_t rmf_fileInfo_digest_type(rmf_fileInfo_t const* self)
{
   if (self != NULL)
   {
      return self->digest_type;
   }
   return RMF_DIGEST_TYPE_NONE;
}

uint8_t const* rmf_fileInfo_digest_data(rmf_fileInfo_t const* self)
{
   if (self != NULL)
   {
      return &self->digest_data[0];
   }
   return NULL;
}

apx_error_t rmf_fileInfo_assign(rmf_fileInfo_t *self, const rmf_fileInfo_t *other)
{
   if ( (self != NULL) && (other != NULL) )
   {
      adt_error_t result;
      self->address = other->address;
      self->size = other->size;
      self->rmf_file_type = other->rmf_file_type;
      self->digest_type = other->digest_type;
      result = adt_str_set(&self->name, &other->name);
      if (result != ADT_NO_ERROR)
      {
         return convert_from_adt_to_apx_error(result);
      }
      return rmf_fileInfo_set_digest_data(self, other->digest_type, &other->digest_data[0]);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

rmf_fileInfo_t* rmf_fileInfo_clone(const rmf_fileInfo_t *other)
{
   if (other != 0)
   {
      rmf_fileInfo_t *self = (rmf_fileInfo_t*) malloc(sizeof(rmf_fileInfo_t));
      if (self != 0)
      {
         apx_error_t rc = rmf_fileInfo_create(self, other->address, other->size, adt_str_cstr((adt_str_t*)&other->name),
            other->rmf_file_type, other->digest_type, other->digest_data);
         if (rc != APX_NO_ERROR)
         {
            free(self);
            self = (rmf_fileInfo_t*) 0;
         }
      }
      return self;
   }
   return (rmf_fileInfo_t*) 0;
}

void rmf_fileInfo_set_address(rmf_fileInfo_t *self, uint32_t address)
{
   if (self != NULL)
   {
      self->address = address;
   }
}

bool rmf_fileInfo_is_remote_address(rmf_fileInfo_t const* self)
{
   if (self != 0)
   {
      return ((self->address != RMF_INVALID_ADDRESS) && ((self->address & RMF_REMOTE_ADDRESS_BIT) != 0u));
   }
   return false;
}

bool rmf_fileInfo_name_ends_with(rmf_fileInfo_t const* self, const char* suffix)
{
   if ( (self != NULL) && (suffix != NULL) && (adt_str_size(&self->name)>0u) )
   {
      size_t str_len = adt_str_length(&self->name);
      size_t suffix_len = strlen(suffix);
      return (str_len >= suffix_len) && (strcmp(adt_str_cstr((adt_str_t*)&self->name) + (str_len-suffix_len), suffix) == 0);
   }
   return false;
}

char* rmf_fileInfo_base_name(rmf_fileInfo_t const* self)
{
   if ( (self != NULL))
   {
      char const* name = adt_str_cstr((adt_str_t*)&self->name);
      char *dot = strchr(name, '.');
      if (dot != 0)
      {
         return bstr_make_cstr((const uint8_t*) name, (const uint8_t*) dot);
      }
   }
   return (char*) 0;
}

void rmf_fileInfo_copy_base_name(rmf_fileInfo_t const* self, char* dest, uint32_t max_dest_len)
{
   if ( (self != NULL) && (max_dest_len > 1))
   {
      char const* name = adt_str_cstr((adt_str_t*)&self->name);
      char *dot = strchr(name, '.');
      if (dot != 0)
      {
         uint32_t len = (uint32_t) (dot - name);
         if (max_dest_len < len)
         {
            strncpy(dest, name, max_dest_len -1);
            dest[max_dest_len -1] = '\0';
         }
         else
         {
            strncpy(dest, name, len);
            dest[len] = '\0';
         }
      }
   }
}

uint32_t rmf_fileInfo_address_without_flags(rmf_fileInfo_t const* self)
{
   if (self != NULL)
   {
      return self->address & APX_ADDRESS_MASK_INTERNAL;
   }
   return RMF_INVALID_ADDRESS;
}

bool rmf_fileInfo_address_in_range(rmf_fileInfo_t const* self, uint32_t address)
{
   if (self != NULL)
   {
      uint32_t address_without_flags = self->address & APX_ADDRESS_MASK_INTERNAL;
      return ((address_without_flags <= address) && (address < (address_without_flags + self->size)));
   }
   return false;
}

apx_error_t rmf_fileInfo_set_digest_data(rmf_fileInfo_t* self, rmf_digestType_t digest_type, const uint8_t* digest_data)
{
   if (self != NULL)
   {
      if (digest_type != RMF_DIGEST_TYPE_NONE)
      {
         if (digest_data == NULL)
         {
            return APX_INVALID_ARGUMENT_ERROR;
         }
         size_t size = (digest_type == RMF_DIGEST_TYPE_SHA1) ? RMF_SHA1_SIZE : RMF_SHA256_SIZE;
         memcpy(&self->digest_data[0], digest_data, size);
      }
      else
      {
         memset(&self->digest_data[0], 0, sizeof(self->digest_data));
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//stateless functions
apx_size_t rmf_encode_publish_file_cmd(uint8_t* buf, apx_size_t buf_size, rmf_fileInfo_t const* file)
{
   if ((buf != NULL) && (file != NULL))
   {
      const char* name = rmf_fileInfo_name(file);
      apx_size_t const name_size = (apx_size_t)strlen(name);
      apx_size_t const required_size = RMF_FILE_INFO_HEADER_SIZE + name_size + 1u; //Add 1 for null-terminator
      uint8_t* p = buf;
      uint8_t const* digest_data = rmf_fileInfo_digest_data(file);
      if (required_size > buf_size)
      {
         return 0u;
      }
      if (digest_data == NULL)
      {
         return 0;
      }
      packLE(p, RMF_CMD_PUBLISH_FILE_MSG, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, rmf_fileInfo_address_without_flags(file), (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, rmf_fileInfo_size(file), (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, (uint32_t)rmf_fileInfo_rmf_file_type(file), (uint8_t)UINT16_SIZE); p += UINT16_SIZE;
      packLE(p, (uint32_t)rmf_fileInfo_digest_type(file), (uint8_t)UINT16_SIZE); p += UINT16_SIZE;
      switch (rmf_fileInfo_digest_type(file))
      {
      case RMF_DIGEST_TYPE_NONE:
         memset(p, 0, RMF_SHA256_SIZE);
         break;
      case RMF_DIGEST_TYPE_SHA1:
         memcpy(p, digest_data, RMF_SHA1_SIZE);
         memset(p + RMF_SHA1_SIZE, 0, RMF_SHA256_SIZE - RMF_SHA1_SIZE);
         break;
      case RMF_DIGEST_TYPE_SHA256:
         memcpy(p, digest_data, RMF_SHA256_SIZE);
         break;
      }
      p += RMF_SHA256_SIZE;
      assert((p + name_size + 1) == buf + required_size);
      memcpy(p, name, name_size); p += name_size;
      *p = 0u;
      return required_size;
   }
   return 0u;
}

apx_size_t rmf_decode_publish_file_cmd(uint8_t const* buf, apx_size_t buf_size, rmf_fileInfo_t* file_info)
{
   if ((buf != NULL) && (file_info != NULL))
   {
      uint8_t const* next = buf;
      uint8_t const* end = buf + buf_size;
      if ((next + RMF_FILE_INFO_HEADER_SIZE) < end)
      {
         uint32_t const cmd_type = unpackLE(next, UINT32_SIZE); next += UINT32_SIZE;
         uint16_t value1;
         uint16_t value2;
         if (cmd_type != RMF_CMD_PUBLISH_FILE_MSG)
         {
            //Invalid command type
            return 0u;
         }
         file_info->address = unpackLE(next, (uint8_t)UINT32_SIZE); next += UINT32_SIZE;
         file_info->size = unpackLE(next, (uint8_t)UINT32_SIZE); next += UINT32_SIZE;
         value1 = (uint16_t)unpackLE(next, (uint8_t)UINT16_SIZE); next += sizeof(uint16_t);
         value2 = (uint16_t)unpackLE(next, (uint8_t)UINT16_SIZE); next += sizeof(uint16_t);
         memcpy(&file_info->digest_data[0], next, RMF_SHA256_SIZE); next += RMF_SHA256_SIZE;
         if (!rmf_value_to_file_type(value1, &file_info->rmf_file_type))
         {
            return 0u;
         }
         if (!rmf_value_to_digest_type(value2, &file_info->digest_type))
         {
            return 0u;
         }
         uint8_t const* result = bstr_while_predicate(next, end, bstr_pred_is_not_zero);
         if ((result > next) && (result < end))
         {
            adt_str_set_bstr(&file_info->name, next, result);
         }
         else
         {
            //null-terminator not found within message buffer
            return 0u;
         }
         return (apx_size_t)(result - buf);
      }
   }
   return 0u;
}

bool rmf_value_to_file_type(uint16_t value, rmf_fileType_t* file_type)
{
   if (file_type != NULL)
   {
      switch (value)
      {
      case RMF_U16_FILE_TYPE_FIXED:
         *file_type = RMF_FILE_TYPE_FIXED;
         break;
      case RMF_U16_FILE_TYPE_DYNAMIC8:
         *file_type = RMF_FILE_TYPE_DYNAMIC8;
         break;
      case RMF_U16_FILE_TYPE_DYNAMIC16:
         *file_type = RMF_FILE_TYPE_DYNAMIC16;
         break;
      case RMF_U16_FILE_TYPE_DYNAMIC32:
         *file_type = RMF_FILE_TYPE_DYNAMIC32;
         break;
      case RMF_U16_FILE_TYPE_DEVICE:
         *file_type = RMF_FILE_TYPE_DEVICE;
         break;
      case RMF_U16_FILE_TYPE_STREAM:
         *file_type = RMF_FILE_TYPE_STREAM;
         break;
      default:
         return false;
      }
      return true;
   }
   return false;
}

bool rmf_value_to_digest_type(uint16_t value, rmf_digestType_t* digest_type)
{
   if (digest_type != NULL)
   {
      switch (value)
      {
      case RMF_U16_DIGEST_TYPE_NONE:
         *digest_type = RMF_DIGEST_TYPE_NONE;
         break;
      case RMF_U16_DIGEST_TYPE_SHA1:
         *digest_type = RMF_DIGEST_TYPE_SHA1;
         break;
      case RMF_U16_DIGEST_TYPE_SHA256:
         *digest_type = RMF_DIGEST_TYPE_SHA256;
         break;
      default:
         return false;
      }
      return true;
   }
   return false;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


