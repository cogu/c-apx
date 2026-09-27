/*****************************************************************************
* \file      remotefile.c
* \author    Conny Gustafsson
* \date      2021-01-20
* \brief     Remotefile layer
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <string.h>
#include "apx/remotefile.h"
#include "apx/file_info.h"
#include "bstr.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_size_t rmf_needed_encoding_size(uint32_t address)
{
   return (address < RMF_HIGH_ADDR_MIN) ? UINT16_SIZE : UINT32_SIZE;
}

apx_size_t rmf_address_encode(uint8_t* buf, apx_size_t buf_size, uint32_t address, bool more_bit)
{
   if ((buf == NULL) || (buf_size == 0) || (address > RMF_HIGH_ADDR_MAX))
   {
      return 0; //Invalid argument
   }
   apx_size_t encoding_size = rmf_needed_encoding_size(address);
   if (encoding_size <= buf_size)
   {
      if (encoding_size == UINT16_SIZE)
      {
         uint16_t value = more_bit ? RMF_MORE_BIT_LOW_ADDR : 0u;
         value |= (uint16_t)address;
         packBE(buf, (uint32_t)value, (uint8_t) UINT16_SIZE);
      }
      else
      {
         assert(encoding_size == UINT32_SIZE);
         uint32_t value = more_bit ? (RMF_HIGH_ADDR_BIT | RMF_MORE_BIT_HIGH_ADDR) : RMF_HIGH_ADDR_BIT;
         value |= address;
         packBE(buf, value, (uint8_t)UINT32_SIZE);
      }
   }
   else
   {
      return 0u; //Not enough bytes in buffer
   }
   return encoding_size;
}

apx_size_t rmf_address_decode(uint8_t const* begin, uint8_t const* end, uint32_t* address, bool* more_bit)
{
   apx_size_t retval = 0u;
   if ((begin == NULL) || (end == NULL) || (address == NULL) || (more_bit == NULL) || (begin >= end))
   {
      return 0u; //Invalid argument
   }
   uint8_t const first_byte = *begin;
   *more_bit = (first_byte & RMF_U8_MORE_BIT) ? true : false;
   if (first_byte & RMF_U8_HIGH_ADDR_BIT)
   {
      if (begin +  UINT32_SIZE <= end)
      {
         uint32_t value = unpackBE(begin, (uint8_t) UINT32_SIZE);
         *address = value & RMF_HIGH_ADDR_MASK;
         retval = UINT32_SIZE;
      }
   }
   else
   {
      if (begin + UINT16_SIZE <= end)
      {
         uint32_t value = unpackBE(begin, UINT16_SIZE);
         *address = (value & RMF_LOW_ADDR_MASK);
         retval = UINT16_SIZE;
      }
   }
   return retval;
}

apx_size_t rmf_encode_open_file_cmd(uint8_t* buf, apx_size_t buf_size, uint32_t address)
{
   apx_size_t const required_size = RMF_CMD_TYPE_SIZE + RMF_FILE_OPEN_CMD_SIZE;
   if ((address > RMF_HIGH_ADDR_MAX) || (required_size > buf_size))
   {
      return 0;
   }
   uint8_t* p = buf;
   packLE(p, RMF_CMD_OPEN_FILE_MSG, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   packLE(p, address, (uint8_t)UINT32_SIZE);
   return required_size;
}

apx_size_t rmf_encode_acknowledge_cmd(uint8_t* buf, apx_size_t buf_size)
{
   apx_size_t const required_size =RMF_CMD_TYPE_SIZE;
   if (required_size > buf_size)
   {
      return 0u;
   }
   packLE(buf, RMF_CMD_ACK_MSG, (uint8_t)UINT32_SIZE);
   return required_size;
}

apx_size_t rmf_encode_nack_cmd(uint8_t* buf, apx_size_t buf_size, uint32_t error_code, char const* name)
{
   apx_size_t const name_size = (name != NULL) ? (apx_size_t)strlen(name) : 0u;
   apx_size_t const required_size = RMF_CMD_NACK_SIZE + name_size + 1u;
   if ((buf == NULL) || (required_size > buf_size))
   {
      return 0u;
   }
   uint8_t* p = buf;
   packLE(p, RMF_CMD_NACK_MSG, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   packLE(p, error_code, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   if (name_size > 0u)
   {
      memcpy(p, name, name_size); p += name_size;
   }
   *p = 0u;
   return required_size;
}

apx_size_t rmf_encode_header_accepted(uint8_t* buf, apx_size_t buf_size, uint32_t connection_id)
{
   apx_size_t const required_size = RMF_CMD_TYPE_SIZE + UINT32_SIZE;
   if (required_size > buf_size)
   {
      return 0;
   }
   uint8_t* p = buf;
   packLE(p, RMF_CMD_ACCEPT_HEADER, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   packLE(p, connection_id, (uint8_t)UINT32_SIZE);
   return required_size;
}

apx_size_t rmf_decode_cmd_type(uint8_t const* begin, uint8_t const* end, uint32_t* cmd_type)
{
   if ((begin == NULL) || (end == NULL) || (begin >= end) || (cmd_type == NULL))
   {
      return 0u;
   }
   if (begin + RMF_CMD_TYPE_SIZE <= end)
   {
      *cmd_type = unpackLE(begin, UINT32_SIZE);
      return RMF_CMD_TYPE_SIZE;
   }
   return 0u;
}

apx_size_t rmf_decode_nack_cmd(uint8_t const* begin, uint8_t const* end, uint32_t* error_code, char const** name)
{
   if ((begin == NULL) || (end == NULL) || (begin >= end) || (error_code == NULL))
   {
      return 0u;
   }
   if (begin + RMF_CMD_NACK_SIZE <= end)
   {
      uint32_t const cmd_type = unpackLE(begin, UINT32_SIZE);
      if (cmd_type == RMF_CMD_NACK_MSG)
      {
         *error_code = unpackLE(begin + RMF_CMD_TYPE_SIZE, UINT32_SIZE);
         uint8_t const* next = begin + RMF_CMD_NACK_SIZE;
         if (next < end)
         {
            uint8_t const* result = bstr_find_byte(next, end, 0);
            if ((result >= next) && (result < end))
            {
               if (name != NULL)
               {
                  *name = (result > next) ? (char const*)next : NULL;
               }
               return (apx_size_t)((result + 1u) - begin);
            }
         }
         if (name != NULL)
         {
            *name = NULL;
         }
         return RMF_CMD_NACK_SIZE;
      }
   }
   if (begin + UINT32_SIZE <= end)
   {
      *error_code = unpackLE(begin, UINT32_SIZE);
      if (name != NULL)
      {
         *name = NULL;
      }
      return UINT32_SIZE;
   }
   return 0u;
}

apx_size_t rmf_decode_header_accepted(uint8_t const* begin, uint8_t const* end, uint32_t* connection_id)
{
   if ((begin == NULL) || (end == NULL) || (begin >= end) || (connection_id == NULL))
   {
      return 0u;
   }
   if (begin + RMF_CMD_TYPE_SIZE <= end)
   {
      *connection_id = unpackLE(begin, UINT32_SIZE);
      return UINT32_SIZE;
   }
   return 0u;      
}

apx_size_t rmf_encode_connection_create(uint8_t* buf, apx_size_t buf_size, uint32_t connection_id, uint8_t connection_state, char const* tag)
{
   apx_size_t tag_size = 0u;
   apx_size_t required_size = RMF_CMD_TYPE_SIZE + UINT32_SIZE + UINT8_SIZE + CHAR_SIZE; //Reserve 1 byte for null-terminator
   if (tag != NULL)
   {
      tag_size = (apx_size_t)strlen(tag);
      required_size += tag_size;
   }
   if (required_size > buf_size)
   {
      return 0;
   }
   uint8_t* p = buf;
   packLE(p, RMF_CMD_CONNECTION_CREATE, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   packLE(p, connection_id, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   packLE(p, (uint32_t)connection_state, (uint8_t)UINT8_SIZE); p += UINT8_SIZE;
   if (tag_size > 0u)
   {
      memcpy(p, tag, tag_size); p += tag_size;      
   }
   *p = 0u; //null-terminator is always added 
   return required_size;
}

apx_size_t rmf_decode_connection_create(uint8_t const* begin, uint8_t const* end, uint32_t* connection_id, uint8_t* connection_state, char** tag)
{
   (void)begin;
   (void)end;
   (void)connection_id;
   (void)connection_state;
   (void)tag;
   return 0;
}

apx_size_t rmf_encode_publish_file_cmd(uint8_t* buf, apx_size_t buf_size, rmf_file_info_t const* file)
{
   if ((buf != NULL) && (file != NULL))
   {
      const char* name = rmf_file_info_name(file);
      apx_size_t const name_size = (apx_size_t)strlen(name);
      apx_size_t const required_size = RMF_FILE_INFO_HEADER_SIZE + name_size + 1u; //Add 1 for null-terminator
      uint8_t* p = buf;
      uint8_t const* digest_data = rmf_file_info_digest_data(file);
      if (required_size > buf_size)
      {
         return 0u;
      }
      if (digest_data == NULL)
      {
         return 0;
      }
      packLE(p, RMF_CMD_PUBLISH_FILE_MSG, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, rmf_file_info_address_without_flags(file), (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, rmf_file_info_size(file), (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, (uint32_t)rmf_file_info_rmf_file_type(file), (uint8_t)UINT16_SIZE); p += UINT16_SIZE;
      packLE(p, (uint32_t)rmf_file_info_digest_type(file), (uint8_t)UINT16_SIZE); p += UINT16_SIZE;
      switch (rmf_file_info_digest_type(file))
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

/**
* Returns number of bytes consumed from buffer
*/
apx_size_t rmf_decode_publish_file_cmd(uint8_t const* buf, apx_size_t buf_size, rmf_file_info_t* file_info)
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
         //The file name can either end with an optional null-terminator or the name string continues until end of message.
         //Both variants are acceptable using the two lines below
         uint8_t const* result = bstr_find_byte(next, end, 0);
         if ((result > next) && (result <= end))
         {
            adt_str_set_bstr(&file_info->name, next, result);
         }
         else
         {
            return 0u;
         }
         return (apx_size_t)(result - buf);
      }
   }
   return 0u;
}

bool rmf_value_to_file_type(uint16_t value, rmf_file_type_t* file_type)
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

bool rmf_value_to_digest_type(uint16_t value, rmf_digest_type_t* digest_type)
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

apx_size_t rmf_encode_publish_signed_file_cmd(uint8_t* buf, apx_size_t buf_size, rmf_file_info_t const* file)
{
   if ((buf != NULL) && (file != NULL))
   {
      const char* name = rmf_file_info_name(file);
      apx_size_t const name_size = (apx_size_t)strlen(name);
      apx_size_t const required_size = RMF_SIGNED_FILE_INFO_HEADER_SIZE + name_size + 1u; //Add 1 for null-terminator
      uint8_t* p = buf;
      uint8_t const* signature_data = rmf_file_info_signature_data(file);
      if (required_size > buf_size)
      {
         return 0u;
      }
      if (signature_data == NULL)
      {
         return 0u;
      }
      packLE(p, RMF_CMD_PUBLISH_SIGNED_FILE_MSG, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, rmf_file_info_address_without_flags(file), (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, rmf_file_info_size(file), (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
      packLE(p, (uint32_t)rmf_file_info_rmf_file_type(file), (uint8_t)UINT16_SIZE); p += UINT16_SIZE;
      packLE(p, (uint32_t)rmf_file_info_signature_type(file), (uint8_t)UINT16_SIZE); p += UINT16_SIZE;
      switch (rmf_file_info_signature_type(file))
      {
      case RMF_SIGNATURE_TYPE_NONE:
         memset(p, 0, RMF_SIGNATURE_SIZE_ECDSA_P256);
         break;
      case RMF_SIGNATURE_TYPE_ECDSA_P256:
         memcpy(p, signature_data, RMF_SIGNATURE_SIZE_ECDSA_P256);
         break;
      default:
         return 0u;
      }
      p += RMF_SIGNATURE_SIZE_ECDSA_P256;
      assert((p + name_size + 1) == buf + required_size);
      memcpy(p, name, name_size); p += name_size;
      *p = 0u;
      return required_size;
   }
   return 0u;
}

/**
* Returns number of bytes consumed from buffer
*/
apx_size_t rmf_decode_publish_signed_file_cmd(uint8_t const* buf, apx_size_t buf_size, rmf_file_info_t* file_info)
{
   if ((buf != NULL) && (file_info != NULL))
   {
      uint8_t const* next = buf;
      uint8_t const* end = buf + buf_size;
      if ((next + RMF_SIGNED_FILE_INFO_HEADER_SIZE) < end)
      {
         uint32_t const cmd_type = unpackLE(next, UINT32_SIZE); next += UINT32_SIZE;
         uint16_t value1;
         uint16_t value2;
         if (cmd_type != RMF_CMD_PUBLISH_SIGNED_FILE_MSG)
         {
            //Invalid command type
            return 0u;
         }
         file_info->address = unpackLE(next, (uint8_t)UINT32_SIZE); next += UINT32_SIZE;
         file_info->size = unpackLE(next, (uint8_t)UINT32_SIZE); next += UINT32_SIZE;
         value1 = (uint16_t)unpackLE(next, (uint8_t)UINT16_SIZE); next += sizeof(uint16_t);
         value2 = (uint16_t)unpackLE(next, (uint8_t)UINT16_SIZE); next += sizeof(uint16_t);
         memcpy(&file_info->signature_data[0], next, RMF_SIGNATURE_SIZE_ECDSA_P256); next += RMF_SIGNATURE_SIZE_ECDSA_P256;
         if (!rmf_value_to_file_type(value1, &file_info->rmf_file_type))
         {
            return 0u;
         }
         if (!rmf_value_to_signature_type(value2, &file_info->signature_type))
         {
            return 0u;
         }
         //The file name can either end with an optional null-terminator or the name string continues until end of message.
         //Both variants are acceptable using the two lines below
         uint8_t const* result = bstr_find_byte(next, end, 0);
         if ((result > next) && (result <= end))
         {
            adt_str_set_bstr(&file_info->name, next, result);
         }
         else
         {
            return 0u;
         }
         return (apx_size_t)(result - buf);
      }
   }
   return 0u;
}

bool rmf_value_to_signature_type(uint16_t value, rmf_signature_type_t* signature_type)
{
   if (signature_type != NULL)
   {
      switch (value)
      {
      case RMF_U16_SIGNATURE_TYPE_NONE:
         *signature_type = RMF_SIGNATURE_TYPE_NONE;
         break;
      case RMF_U16_SIGNATURE_TYPE_ECDSA_P256:
         *signature_type = RMF_SIGNATURE_TYPE_ECDSA_P256;
         break;
      default:
         return false;
      }
      return true;
   }
   return false;
}



//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


