/*****************************************************************************
* \file      file_info.c
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Disposable file info data structure
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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

rmf_file_info_t* rmf_file_info_make_empty(void)
{
   return rmf_file_info_new(RMF_INVALID_ADDRESS, 0u, NULL, RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
}

rmf_file_info_t* rmf_file_info_make_fixed(char const* name, uint32_t size, uint32_t address)
{
   return rmf_file_info_new(address, size, name, RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
}

rmf_file_info_t* rmf_file_info_make_fixed_with_digest(char const* name, uint32_t size, uint32_t address, rmf_digest_type_t digest_type, uint8_t const* digest_data)
{
   return rmf_file_info_new(address, size, name, RMF_FILE_TYPE_FIXED, digest_type, digest_data);
}

rmf_file_info_t* rmf_file_info_make_fixed_with_signature(char const* name, uint32_t size, uint32_t address, rmf_signature_type_t signature_type, uint8_t const* signature_data)
{
   rmf_file_info_t* self = rmf_file_info_new(address, size, name, RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
   if (self != NULL)
   {
      apx_error_t rc = rmf_file_info_set_signature(self, signature_type, signature_data);
      if (rc != APX_NO_ERROR)
      {
         rmf_file_info_delete(self);
         self = NULL;
      }
   }
   return self;
}

apx_error_t rmf_file_info_create(rmf_file_info_t* self, uint32_t address, uint32_t size, const char* name, rmf_file_type_t file_type, rmf_digest_type_t digest_type, const uint8_t* digest_data)
{
   if (self != NULL)
   {
      adt_error_t result;
      self->address = address;
      self->size = size;
      self->rmf_file_type = file_type;
      self->digest_type = digest_type;
      self->signature_type = RMF_SIGNATURE_TYPE_NONE;
      memset(&self->signature_data[0], 0, sizeof(self->signature_data));
      adt_str_create(&self->name);
      if (name != NULL)
      {
         result = adt_str_set_cstr(&self->name, name);
         if (result != ADT_NO_ERROR)
         {
            return convert_from_adt_to_apx_error(result);
         }
      }
      return rmf_file_info_set_digest_data(self, digest_type, digest_data);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t rmf_file_info_create_copy(rmf_file_info_t* self, rmf_file_info_t const* other)
{
   if (self != NULL)
   {
      adt_error_t result;
      self->address = other->address;
      self->size = other->size;
      self->rmf_file_type = other->rmf_file_type;
      self->digest_type = other->digest_type;
      self->signature_type = other->signature_type;
      memcpy(&self->signature_data[0], &other->signature_data[0], sizeof(self->signature_data));
      adt_str_create(&self->name);
      result = adt_str_set(&self->name, &other->name);
      if (result != ADT_NO_ERROR)
      {
         return convert_from_adt_to_apx_error(result);
      }
      return rmf_file_info_set_digest_data(self, other->digest_type, &other->digest_data[0]);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void rmf_file_info_destroy(rmf_file_info_t* self)
{
   if (self != NULL)
   {
      adt_str_destroy(&self->name);
   }
}

rmf_file_info_t* rmf_file_info_new(uint32_t address, uint32_t size, const char* name, rmf_file_type_t file_type, rmf_digest_type_t digest_type, const uint8_t* digest_data)
{
   rmf_file_info_t* self = (rmf_file_info_t*)malloc(sizeof(rmf_file_info_t));
   if (self != NULL)
   {
      apx_error_t rc = rmf_file_info_create(self, address, size, name, file_type, digest_type, digest_data);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void rmf_file_info_delete(rmf_file_info_t* self)
{
   if (self != NULL)
   {
      rmf_file_info_destroy(self);
      free(self);
   }
}

void rmf_file_info_vdelete(void* arg)
{
   rmf_file_info_delete((rmf_file_info_t*)arg);
}

const char* rmf_file_info_name(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return adt_str_cstr((adt_str_t*)&self->name);
   }
   return NULL;
}

uint32_t rmf_file_info_address(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return self->address;
   }
   return RMF_INVALID_ADDRESS;
}

uint32_t rmf_file_info_size(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return self->size;
   }
   return 0u;
}

rmf_file_type_t rmf_file_info_rmf_file_type(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return self->rmf_file_type;
   }
   return RMF_FILE_TYPE_FIXED;
}

rmf_digest_type_t rmf_file_info_digest_type(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return self->digest_type;
   }
   return RMF_DIGEST_TYPE_NONE;
}

uint8_t const* rmf_file_info_digest_data(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return &self->digest_data[0];
   }
   return NULL;
}

rmf_signature_type_t rmf_file_info_signature_type(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return self->signature_type;
   }
   return RMF_SIGNATURE_TYPE_NONE;
}

uint8_t const* rmf_file_info_signature_data(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return &self->signature_data[0];
   }
   return NULL;
}

bool rmf_file_info_is_signed(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return self->signature_type != RMF_SIGNATURE_TYPE_NONE;
   }
   return false;
}

apx_error_t rmf_file_info_assign(rmf_file_info_t* self, const rmf_file_info_t* other)
{
   if ((self != NULL) && (other != NULL))
   {
      adt_error_t result;
      self->address = other->address;
      self->size = other->size;
      self->rmf_file_type = other->rmf_file_type;
      self->digest_type = other->digest_type;
      self->signature_type = other->signature_type;
      memcpy(&self->signature_data[0], &other->signature_data[0], sizeof(self->signature_data));
      result = adt_str_set(&self->name, &other->name);
      if (result != ADT_NO_ERROR)
      {
         return convert_from_adt_to_apx_error(result);
      }
      return rmf_file_info_set_digest_data(self, other->digest_type, &other->digest_data[0]);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

rmf_file_info_t* rmf_file_info_clone(const rmf_file_info_t* other)
{
   if (other != NULL)
   {
      rmf_file_info_t* self = (rmf_file_info_t*)malloc(sizeof(rmf_file_info_t));
      if (self != NULL)
      {
         apx_error_t rc = rmf_file_info_create_copy(self, other);
         if (rc != APX_NO_ERROR)
         {
            free(self);
            self = NULL;
         }
      }
      return self;
   }
   return NULL;
}

void rmf_file_info_set_address(rmf_file_info_t* self, uint32_t address)
{
   if (self != NULL)
   {
      self->address = address;
   }
}

bool rmf_file_info_is_remote_address(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return ((self->address != RMF_INVALID_ADDRESS) && ((self->address & RMF_REMOTE_ADDRESS_BIT) != 0u));
   }
   return false;
}

bool rmf_file_info_name_ends_with(rmf_file_info_t const* self, const char* suffix)
{
   if ((self != NULL) && (suffix != NULL) && (adt_str_size(&self->name) > 0))
   {
      size_t str_len = adt_str_length(&self->name);
      size_t suffix_len = strlen(suffix);
      return (str_len >= suffix_len) && (strcmp(adt_str_cstr((adt_str_t*)&self->name) + (str_len - suffix_len), suffix) == 0);
   }
   return false;
}

char* rmf_file_info_base_name(rmf_file_info_t const* self)
{
   if ((self != NULL))
   {
      char const* name = adt_str_cstr((adt_str_t*)&self->name);
      char* dot = strchr(name, '.');
      if (dot != NULL)
      {
         return bstr_make_cstr((const uint8_t*)name, (const uint8_t*)dot);
      }
   }
   return NULL;
}

void rmf_file_info_copy_base_name(rmf_file_info_t const* self, char* dest, uint32_t max_dest_len)
{
   if ((self != NULL) && (max_dest_len > 1))
   {
      char const* name = adt_str_cstr((adt_str_t*)&self->name);
      char* dot = strchr(name, '.');
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
   }
}

uint32_t rmf_file_info_address_without_flags(rmf_file_info_t const* self)
{
   if (self != NULL)
   {
      return self->address & APX_ADDRESS_MASK_INTERNAL;
   }
   return RMF_INVALID_ADDRESS;
}

bool rmf_file_info_address_in_range(rmf_file_info_t const* self, uint32_t address)
{
   if (self != NULL)
   {
      uint32_t address_without_flags = self->address & APX_ADDRESS_MASK_INTERNAL;
      return ((address_without_flags <= address) && (address < (address_without_flags + self->size)));
   }
   return false;
}

apx_error_t rmf_file_info_set_digest_data(rmf_file_info_t* self, rmf_digest_type_t digest_type, const uint8_t* digest_data)
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

apx_error_t rmf_file_info_set_signature(rmf_file_info_t* self, rmf_signature_type_t signature_type, const uint8_t* signature_data)
{
   if (self != NULL)
   {
      if (signature_type == RMF_SIGNATURE_TYPE_NONE)
      {
         self->signature_type = RMF_SIGNATURE_TYPE_NONE;
         memset(&self->signature_data[0], 0, sizeof(self->signature_data));
         return APX_NO_ERROR;
      }
      else if (signature_type == RMF_SIGNATURE_TYPE_ECDSA_P256)
      {
         if (signature_data == NULL)
         {
            return APX_INVALID_ARGUMENT_ERROR;
         }
         self->signature_type = signature_type;
         memcpy(&self->signature_data[0], signature_data, RMF_SIGNATURE_SIZE_ECDSA_P256);
         return APX_NO_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


