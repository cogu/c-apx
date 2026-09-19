/*****************************************************************************
* \file      program_decoder.c
* \author    Conny Gustafsson
* \date      2021-01-15
* \brief     APX VM 2.1 bytecode program decoder
*
* Copyright (c) 2021 Conny Gustafsson
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
#include <assert.h>
#include <string.h>
#include "apx/program_decoder.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t decode_next_instruction_internal(apx_programDecoder_t* self);
static void reset_pack_unpack_info(apx_programDecoder_t* self, apx_typeCode_t type_code);
static apx_error_t decode_array_size(apx_programDecoder_t* self);
static apx_error_t decode_range_check_uint32(apx_programDecoder_t* self, uint8_t variant);
static apx_error_t decode_range_check_uint64(apx_programDecoder_t* self, uint8_t variant);
static apx_error_t decode_range_check_int32(apx_programDecoder_t* self, uint8_t variant);
static apx_error_t decode_range_check_int64(apx_programDecoder_t* self, uint8_t variant);
static apx_error_t decode_record_select(apx_programDecoder_t* self, bool is_first_field);
static apx_error_t decode_record_end(apx_programDecoder_t* self);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_programDecoder_create(apx_programDecoder_t* self)
{
   if (self != NULL)
   {
      self->program_begin = NULL;
      self->program_next = NULL;
      self->program_end = NULL;
      self->program_mark = NULL;
      self->operation_type = APX_OPERATION_TYPE_PROGRAM_END;
      self->last_type_code = APX_TYPE_CODE_NONE;
      self->pack_unpack_info.type_code = APX_TYPE_CODE_NONE;
      self->pack_unpack_info.array_length = 0u;
      self->pack_unpack_info.is_dynamic_array = false;
      self->range_check_uint32_info.lower_limit = 0u;
      self->range_check_uint32_info.upper_limit = 0u;
      self->range_check_uint64_info.lower_limit = 0u;
      self->range_check_uint64_info.upper_limit = 0u;
      self->range_check_int32_info.lower_limit = 0;
      self->range_check_int32_info.upper_limit = 0;
      self->range_check_int64_info.lower_limit = 0;
      self->range_check_int64_info.upper_limit = 0;
      adt_str_create(&self->field_name);
      self->is_first_field = false;
      self->is_array_limit = false;
   }
}

void apx_programDecoder_destroy(apx_programDecoder_t* self)
{
   if (self != NULL)
   {
      adt_str_destroy(&self->field_name);
   }
}

apx_error_t apx_programDecoder_select_program(apx_programDecoder_t* self, uint8_t const* data, uint32_t size)
{
   if ( (self != NULL) && (data != NULL) )
   {
      self->program_begin = self->program_next = data;
      self->program_end = data + size;
      self->program_mark = NULL;
      self->operation_type = APX_OPERATION_TYPE_PROGRAM_END;
      self->last_type_code = APX_TYPE_CODE_NONE;
      self->pack_unpack_info.type_code = APX_TYPE_CODE_NONE;
      self->pack_unpack_info.array_length = 0u;
      self->pack_unpack_info.is_dynamic_array = false;
      self->range_check_uint32_info.lower_limit = 0u;
      self->range_check_uint32_info.upper_limit = 0u;
      self->range_check_uint64_info.lower_limit = 0u;
      self->range_check_uint64_info.upper_limit = 0u;
      self->range_check_int32_info.lower_limit = 0;
      self->range_check_int32_info.upper_limit = 0;
      self->range_check_int64_info.lower_limit = 0;
      self->range_check_int64_info.upper_limit = 0;
      adt_str_clear(&self->field_name);
      self->is_first_field = false;
      self->is_array_limit = false;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_programDecoder_parse_program_header(apx_programDecoder_t* self, apx_programHeader_t* header)
{
   if ( (self == NULL) || (header == NULL) )
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   if ( (self->program_next == NULL) || (self->program_end == NULL) )
   {
      return APX_NULL_PTR_ERROR;
   }
   // Minimum header length in VM 2.1 is 4 bytes (2 version, 1 type, 1 uint8 size)
   if ( (self->program_next + 4u) > self->program_end )
   {
      return APX_PARSE_ERROR;
   }
   if ( (self->program_next[0] != APX_VM_HEADER_VERSION_MAJOR) ||
        (self->program_next[1] != APX_VM_HEADER_VERSION_MINOR) )
   {
      return APX_INVALID_HEADER_ERROR;
   }
   uint8_t const type_byte = self->program_next[2];
   self->program_next += 3u;

   header->element_size = 0u;
   header->queue_length = 0u;
   uint8_t const data_variant = type_byte & APX_VM_HEADER_DATA_VARIANT_MASK;
   header->program_type = ((type_byte & APX_VM_HEADER_PROG_TYPE_PACK) == APX_VM_HEADER_PROG_TYPE_PACK) ? APX_PACK_PROGRAM : APX_UNPACK_PROGRAM;
   bool const is_queued = ((type_byte & APX_VM_HEADER_FLAG_QUEUED_DATA) == APX_VM_HEADER_FLAG_QUEUED_DATA);
   header->has_dynamic_data = ((type_byte & APX_VM_HEADER_FLAG_DYNAMIC_DATA) == APX_VM_HEADER_FLAG_DYNAMIC_DATA);

   if (data_variant > APX_VM_VARIANT_UINT32)
   {
      return APX_INVALID_HEADER_ERROR;
   }

   uint8_t const* result = apx_vm_parse_uint32_by_variant(self->program_next, self->program_end, data_variant, &header->data_size);
   if ( (result > self->program_next) && (result <= self->program_end) )
   {
      self->program_next = result;
   }
   else
   {
      return APX_PARSE_ERROR;
   }

   if (is_queued)
   {
      if (self->program_next >= self->program_end)
      {
         return APX_PARSE_ERROR;
      }
      uint8_t opcode = 0u;
      uint8_t variant = 0u;
      bool flag = false;
      apx_program_decode_instruction(*self->program_next, &opcode, &variant, &flag);
      if ( (opcode != APX_VM_OPCODE_DATA_SIZE) || (variant < APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE) || (variant > APX_VM_VARIANT_ELEMENT_SIZE_LAST) )
      {
         return APX_PARSE_ERROR;
      }
      self->program_next++;

      uint8_t element_variant = 0u;
      uint8_t queued_variant = 0u;
      if (variant < APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE) // 3..5
      {
         element_variant = APX_VM_VARIANT_UINT8;
         queued_variant = variant - APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE;
      }
      else if (variant < APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE) // 6..8
      {
         element_variant = APX_VM_VARIANT_UINT16;
         queued_variant = variant - APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE;
      }
      else // 9..11
      {
         element_variant = APX_VM_VARIANT_UINT32;
         queued_variant = variant - APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE;
      }
      uint32_t const queue_storage_size = (uint32_t) apx_vm_variant_to_size(queued_variant);
      if ( (queue_storage_size == 0u) || (queue_storage_size > header->data_size) )
      {
         return APX_PARSE_ERROR;
      }

      result = apx_vm_parse_uint32_by_variant(self->program_next, self->program_end, element_variant, &header->element_size);
      if ( (result > self->program_next) && (result <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_PARSE_ERROR;
      }

      if (header->element_size == 0u)
      {
         return APX_PARSE_ERROR;
      }
      uint32_t const rem = header->data_size - queue_storage_size;
      if (rem % header->element_size != 0u)
      {
         return APX_INVALID_HEADER_ERROR;
      }
      header->queue_length = rem / header->element_size;
   }
   return APX_NO_ERROR;
}

apx_error_t apx_programDecoder_parse_next_operation(apx_programDecoder_t* self, apx_operationType_t* operation_type)
{
   if ( (self != NULL) && (operation_type != NULL) )
   {
      if ( (self->program_next == NULL) || (self->program_end == NULL) )
      {
         return APX_NULL_PTR_ERROR;
      }
      if (self->program_next == self->program_end)
      {
         self->operation_type = APX_OPERATION_TYPE_PROGRAM_END;
         *operation_type = APX_OPERATION_TYPE_PROGRAM_END;
      }
      else
      {
         apx_error_t const result = decode_next_instruction_internal(self);
         if (result == APX_NO_ERROR)
         {
            *operation_type = self->operation_type;
         }
         else
         {
            return result;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_programDecoder_get_pack_unpack_info(apx_programDecoder_t const* self, apx_packUnpackOperationInfo_t* info)
{
   if ( (self != NULL) && (info != NULL) )
   {
      *info = self->pack_unpack_info;
   }
}

void apx_programDecoder_range_check_info_int32(apx_programDecoder_t const* self, apx_rangeCheckInt32OperationInfo_t* info)
{
   if ( (self != NULL) && (info != NULL) )
   {
      *info = self->range_check_int32_info;
   }
}

void apx_programDecoder_range_check_info_uint32(apx_programDecoder_t const* self, apx_rangeCheckUInt32OperationInfo_t* info)
{
   if ( (self != NULL) && (info != NULL) )
   {
      *info = self->range_check_uint32_info;
   }
}

void apx_programDecoder_range_check_info_int64(apx_programDecoder_t const* self, apx_rangeCheckInt64OperationInfo_t* info)
{
   if ( (self != NULL) && (info != NULL) )
   {
      *info = self->range_check_int64_info;
   }
}

void apx_programDecoder_range_check_info_uint64(apx_programDecoder_t const* self, apx_rangeCheckUInt64OperationInfo_t* info)
{
   if ( (self != NULL) && (info != NULL) )
   {
      *info = self->range_check_uint64_info;
   }
}

char const* apx_programDecoder_get_field_name(apx_programDecoder_t* self)
{
   if (self != NULL)
   {
      return adt_str_cstr(&self->field_name);
   }
   return NULL;
}

void apx_programDecoder_save_program_position(apx_programDecoder_t* self)
{
   if (self != NULL)
   {
      if (self->program_next != NULL)
      {
         self->program_mark = self->program_next;
      }
   }
}

void apx_programDecoder_recall_program_position(apx_programDecoder_t* self)
{
   if (self != NULL)
   {
      if (self->program_mark != NULL)
      {
         self->program_next = self->program_mark;
      }
   }
}

bool apx_programDecoder_has_saved_program_position(apx_programDecoder_t* self)
{
   if (self != NULL)
   {
      return self->program_mark != NULL;
   }
   return false;
}

bool apx_programDecoder_is_first_field(apx_programDecoder_t* self)
{
   if (self != NULL)
   {
      return self->is_first_field;
   }
   return false;
}

bool apx_programDecoder_is_array_limit(apx_programDecoder_t* self)
{
   if (self != NULL)
   {
      return self->is_array_limit;
   }
   return false;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t decode_next_instruction_internal(apx_programDecoder_t* self)
{
   assert(self != NULL);
   uint8_t const instruction = *self->program_next++;
   uint8_t opcode = 0u;
   uint8_t variant = 0u;
   bool flag = false;
   apx_program_decode_instruction(instruction, &opcode, &variant, &flag);
   switch (opcode)
   {
   case APX_VM_OPCODE_PACK:
      self->operation_type = APX_OPERATION_TYPE_PACK;
      if (variant > APX_VM_VARIANT_LAST)
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      break;
   case APX_VM_OPCODE_UNPACK:
      self->operation_type = APX_OPERATION_TYPE_UNPACK;
      if (variant > APX_VM_VARIANT_LAST)
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      break;
   case APX_VM_OPCODE_DATA_CTRL:
      if (variant == APX_VM_VARIANT_RECORD_SELECT)
      {
         return decode_record_select(self, flag);
      }
      else if (variant == APX_VM_VARIANT_RECORD_END)
      {
         return decode_record_end(self);
      }
      else if ( (variant >= APX_VM_VARIANT_LIMIT_CHECK_U8) && (variant <= APX_VM_VARIANT_LIMIT_CHECK_LAST) )
      {
         if (self->last_type_code == APX_TYPE_CODE_BOOL)
         {
            return APX_INVALID_INSTRUCTION_ERROR;
         }
         self->last_type_code = APX_TYPE_CODE_NONE;
         self->is_array_limit = flag;
         switch (variant)
         {
         case APX_VM_VARIANT_LIMIT_CHECK_U8:
            return decode_range_check_uint32(self, APX_VM_VARIANT_UINT8);
         case APX_VM_VARIANT_LIMIT_CHECK_U16:
            return decode_range_check_uint32(self, APX_VM_VARIANT_UINT16);
         case APX_VM_VARIANT_LIMIT_CHECK_U32:
            return decode_range_check_uint32(self, APX_VM_VARIANT_UINT32);
         case APX_VM_VARIANT_LIMIT_CHECK_U64:
            return decode_range_check_uint64(self, APX_VM_VARIANT_UINT64);
         case APX_VM_VARIANT_LIMIT_CHECK_S8:
            return decode_range_check_int32(self, APX_VM_VARIANT_INT8);
         case APX_VM_VARIANT_LIMIT_CHECK_S16:
            return decode_range_check_int32(self, APX_VM_VARIANT_INT16);
         case APX_VM_VARIANT_LIMIT_CHECK_S32:
            return decode_range_check_int32(self, APX_VM_VARIANT_INT32);
         case APX_VM_VARIANT_LIMIT_CHECK_S64:
            return decode_range_check_int64(self, APX_VM_VARIANT_INT64);
         default:
            return APX_INVALID_INSTRUCTION_ERROR;
         }
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      break;
   case APX_VM_OPCODE_FLOW_CTRL:
      self->last_type_code = APX_TYPE_CODE_NONE;
      if (variant == APX_VM_VARIANT_ARRAY_NEXT)
      {
         self->operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
         return APX_NO_ERROR;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      break;
   default:
      return APX_INVALID_INSTRUCTION_ERROR;
   }

   if ( (opcode == APX_VM_OPCODE_UNPACK) || (opcode == APX_VM_OPCODE_PACK) )
   {
      apx_typeCode_t const type_code = apx_vm_variant_to_type_code(variant);
      if (type_code == APX_TYPE_CODE_NONE)
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      self->last_type_code = type_code;
      reset_pack_unpack_info(self, type_code);
      if (flag)
      {
         return decode_array_size(self);
      }
   }
   return APX_NO_ERROR;
}

static void reset_pack_unpack_info(apx_programDecoder_t* self, apx_typeCode_t type_code)
{
   assert(self != NULL);
   self->pack_unpack_info.array_length = 0u;
   self->pack_unpack_info.is_dynamic_array = false;
   self->pack_unpack_info.type_code = type_code;
}

static apx_error_t decode_array_size(apx_programDecoder_t* self)
{
   assert(self != NULL);
   if (self->program_next < self->program_end)
   {
      uint8_t const instruction = *self->program_next++;
      uint8_t opcode = 0u;
      uint8_t variant = 0u;
      apx_program_decode_instruction(instruction, &opcode, &variant, &self->pack_unpack_info.is_dynamic_array);
      if (opcode == APX_VM_OPCODE_DATA_SIZE)
      {
         if (variant > APX_VM_VARIANT_ARRAY_SIZE_LAST)
         {
            return APX_INVALID_INSTRUCTION_ERROR;
         }
         uint8_t const* const result = apx_vm_parse_uint32_by_variant(self->program_next, self->program_end, variant, &self->pack_unpack_info.array_length);
         if ( (result > self->program_next) && (self->program_next <= self->program_end) )
         {
            self->program_next = result;
         }
         else
         {
            return APX_INVALID_INSTRUCTION_ERROR;
         }
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_UNEXPECTED_END_ERROR;
}

static apx_error_t decode_range_check_uint32(apx_programDecoder_t* self, uint8_t variant)
{
   self->operation_type = APX_OPERATION_TYPE_RANGE_CHECK_UINT32;
   size_t const data_size = (size_t) apx_vm_variant_to_size(variant);
   if (self->program_next + (data_size * 2u) <= self->program_end)
   {
      uint8_t const* result = apx_vm_parse_uint32_by_variant(self->program_next, self->program_end, variant, &self->range_check_uint32_info.lower_limit);
      if ( (result > self->program_next) && (self->program_next <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      result = apx_vm_parse_uint32_by_variant(self->program_next, self->program_end, variant, &self->range_check_uint32_info.upper_limit);
      if ( (result > self->program_next) && (self->program_next <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_UNEXPECTED_END_ERROR;
}

static apx_error_t decode_range_check_uint64(apx_programDecoder_t* self, uint8_t variant)
{
   self->operation_type = APX_OPERATION_TYPE_RANGE_CHECK_UINT64;
   size_t const data_size = (size_t) apx_vm_variant_to_size(variant);
   if (self->program_next + (data_size * 2u) <= self->program_end)
   {
      uint8_t const* result = apx_vm_parse_uint64_by_variant(self->program_next, self->program_end, variant, &self->range_check_uint64_info.lower_limit);
      if ( (result > self->program_next) && (self->program_next <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      result = apx_vm_parse_uint64_by_variant(self->program_next, self->program_end, variant, &self->range_check_uint64_info.upper_limit);
      if ( (result > self->program_next) && (self->program_next <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_UNEXPECTED_END_ERROR;
}

static apx_error_t decode_range_check_int32(apx_programDecoder_t* self, uint8_t variant)
{
   self->operation_type = APX_OPERATION_TYPE_RANGE_CHECK_INT32;
   size_t const data_size = (size_t) apx_vm_variant_to_size(variant);
   if (self->program_next + (data_size * 2u) <= self->program_end)
   {
      uint8_t const* result = apx_vm_parse_int32_by_variant(self->program_next, self->program_end, variant, &self->range_check_int32_info.lower_limit);
      if ( (result > self->program_next) && (self->program_next <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      result = apx_vm_parse_int32_by_variant(self->program_next, self->program_end, variant, &self->range_check_int32_info.upper_limit);
      if ( (result > self->program_next) && (self->program_next <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_UNEXPECTED_END_ERROR;
}

static apx_error_t decode_range_check_int64(apx_programDecoder_t* self, uint8_t variant)
{
   self->operation_type = APX_OPERATION_TYPE_RANGE_CHECK_INT64;
   size_t const data_size = (size_t) apx_vm_variant_to_size(variant);
   if (self->program_next + (data_size * 2u) <= self->program_end)
   {
      uint8_t const* result = apx_vm_parse_int64_by_variant(self->program_next, self->program_end, variant, &self->range_check_int64_info.lower_limit);
      if ( (result > self->program_next) && (self->program_next <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      result = apx_vm_parse_int64_by_variant(self->program_next, self->program_end, variant, &self->range_check_int64_info.upper_limit);
      if ( (result > self->program_next) && (self->program_next <= self->program_end) )
      {
         self->program_next = result;
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_UNEXPECTED_END_ERROR;
}

static apx_error_t decode_record_select(apx_programDecoder_t* self, bool is_first_field)
{
   assert(self != NULL);
   self->last_type_code = APX_TYPE_CODE_NONE;
   self->operation_type = APX_OPERATION_TYPE_RECORD_SELECT;
   uint8_t const* const result = bstr_find_byte(self->program_next, self->program_end, 0);
   if ( (result >= self->program_next) && (result < self->program_end) )
   {
      adt_str_set_bstr(&self->field_name, self->program_next, result);
      self->program_next = result + UINT8_SIZE; // Skip past null-terminator
      self->is_first_field = is_first_field;
      return APX_NO_ERROR;
   }
   return APX_INVALID_INSTRUCTION_ERROR;
}

static apx_error_t decode_record_end(apx_programDecoder_t* self)
{
   assert(self != NULL);
   self->last_type_code = APX_TYPE_CODE_NONE;
   self->operation_type = APX_OPERATION_TYPE_RECORD_END;
   return APX_NO_ERROR;
}
