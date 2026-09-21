/*****************************************************************************
* \file      program_encoder.c
* \author    Conny Gustafsson
* \date      2021-01-15
* \brief     APX VM 2.1 bytecode program encoder
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
#include "apx/program_encoder.h"
#include "apx/vm_common.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static uint8_t encode_program_type_byte(apx_program_type_t program_type, uint8_t data_size_variant, bool is_dynamic, bool is_queued);
static uint8_t calc_data_size_variant(uint8_t elem_variant, uint8_t queue_variant);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_program_encoder_create(apx_program_encoder_t* self)
{
   if (self != NULL)
   {
      APX_PROGRAM_CREATE(&self->header);
      APX_PROGRAM_CREATE(&self->buffer);
      self->last_type_code = APX_TYPE_CODE_NONE;
   }
}

void apx_program_encoder_destroy(apx_program_encoder_t* self)
{
   if (self != NULL)
   {
      APX_PROGRAM_DESTROY(&self->header);
      APX_PROGRAM_DESTROY(&self->buffer);
   }
}

void apx_program_encoder_clear(apx_program_encoder_t* self)
{
   if (self != NULL)
   {
      adt_bytearray_clear(&self->header);
      adt_bytearray_clear(&self->buffer);
      self->last_type_code = APX_TYPE_CODE_NONE;
   }
}

apx_error_t apx_program_encoder_encode_instruction(apx_program_encoder_t* self, uint8_t opcode, uint8_t variant, bool flag)
{
   if (self == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   if ( (opcode == APX_VM_OPCODE_PACK) || (opcode == APX_VM_OPCODE_UNPACK) )
   {
      self->last_type_code = apx_vm_variant_to_type_code(variant);
   }
   else
   {
      self->last_type_code = APX_TYPE_CODE_NONE;
   }
   uint8_t const instr = apx_program_encode_instruction(opcode, variant, flag);
   adt_error_t const rc = adt_bytearray_push(&self->buffer, instr);
   return (rc == ADT_NO_ERROR) ? APX_NO_ERROR : APX_MEM_ERROR;
}

apx_error_t apx_program_encoder_encode_header_instruction(apx_program_encoder_t* self, uint8_t opcode, uint8_t variant, bool flag)
{
   if (self == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   uint8_t const instr = apx_program_encode_instruction(opcode, variant, flag);
   adt_error_t const rc = adt_bytearray_push(&self->header, instr);
   return (rc == ADT_NO_ERROR) ? APX_NO_ERROR : APX_MEM_ERROR;
}

apx_error_t apx_program_encoder_encode_array_size(apx_program_encoder_t* self, uint32_t array_size, bool is_dynamic)
{
   if (self == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   uint8_t variant = 0u;
   uint8_t encoded_bytes[UINT32_SIZE];
   uint32_t byte_count = 0u;
   if (array_size <= UINT8_MAX)
   {
      variant = APX_VM_VARIANT_ARRAY_SIZE_U8;
      packLE(&encoded_bytes[0], array_size, (uint8_t)UINT8_SIZE);
      byte_count = UINT8_SIZE;
   }
   else if (array_size <= UINT16_MAX)
   {
      variant = APX_VM_VARIANT_ARRAY_SIZE_U16;
      packLE(&encoded_bytes[0], array_size, (uint8_t)UINT16_SIZE);
      byte_count = UINT16_SIZE;
   }
   else
   {
      variant = APX_VM_VARIANT_ARRAY_SIZE_U32;
      packLE(&encoded_bytes[0], array_size, (uint8_t)UINT32_SIZE);
      byte_count = UINT32_SIZE;
   }
   apx_error_t err = apx_program_encoder_encode_instruction(self, APX_VM_OPCODE_DATA_SIZE, variant, is_dynamic);
   if (err == APX_NO_ERROR)
   {
      adt_error_t rc = adt_bytearray_append(&self->buffer, &encoded_bytes[0], byte_count);
      if (rc != ADT_NO_ERROR)
      {
         return APX_MEM_ERROR;
      }
   }
   return err;
}

apx_error_t apx_program_encoder_encode_limit_check_instruction(apx_program_encoder_t* self, uint8_t variant, int64_t lower_limit, int64_t upper_limit, bool is_array)
{
   if (self == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   if (self->last_type_code == APX_TYPE_CODE_BOOL)
   {
      return APX_INVALID_INSTRUCTION_ERROR;
   }
   if ( (variant < APX_VM_VARIANT_LIMIT_CHECK_U8) || (variant > APX_VM_VARIANT_LIMIT_CHECK_LAST) )
   {
      return APX_INVALID_INSTRUCTION_ERROR;
   }
   apx_error_t err = apx_program_encoder_encode_instruction(self, APX_VM_OPCODE_DATA_CTRL, variant, is_array);
   if (err == APX_NO_ERROR)
   {
      err = apx_program_encoder_encode_limit_values(self, variant, lower_limit, upper_limit);
   }
   return err;
}

apx_error_t apx_program_encoder_encode_limit_values(apx_program_encoder_t* self, uint8_t variant, int64_t lower_limit, int64_t upper_limit)
{
   if (self == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   uint8_t data[UINT64_SIZE * 2u];
   uint32_t elem_size = 0u;
   switch (variant)
   {
   case APX_VM_VARIANT_LIMIT_CHECK_U8:
      elem_size = UINT8_SIZE;
      packLE(&data[0], (uint32_t)(uint8_t)lower_limit, (uint8_t)UINT8_SIZE);
      packLE(&data[elem_size], (uint32_t)(uint8_t)upper_limit, (uint8_t)UINT8_SIZE);
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_U16:
      elem_size = UINT16_SIZE;
      packLE(&data[0], (uint32_t)(uint16_t)lower_limit, (uint8_t)UINT16_SIZE);
      packLE(&data[elem_size], (uint32_t)(uint16_t)upper_limit, (uint8_t)UINT16_SIZE);
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_U32:
      elem_size = UINT32_SIZE;
      packLE(&data[0], (uint32_t)lower_limit, (uint8_t)UINT32_SIZE);
      packLE(&data[elem_size], (uint32_t)upper_limit, (uint8_t)UINT32_SIZE);
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_U64:
      elem_size = UINT64_SIZE;
      packLE64(&data[0], (uint64_t)lower_limit, (uint8_t)elem_size);
      packLE64(&data[elem_size], (uint64_t)upper_limit, (uint8_t)elem_size);
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_S8:
      elem_size = INT8_SIZE;
      packLE(&data[0], (uint32_t)(uint8_t)(int8_t)lower_limit, (uint8_t)UINT8_SIZE);
      packLE(&data[elem_size], (uint32_t)(uint8_t)(int8_t)upper_limit, (uint8_t)UINT8_SIZE);
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_S16:
      elem_size = INT16_SIZE;
      packLE(&data[0], (uint32_t)(uint16_t)(int16_t)lower_limit, (uint8_t)UINT16_SIZE);
      packLE(&data[elem_size], (uint32_t)(uint16_t)(int16_t)upper_limit, (uint8_t)UINT16_SIZE);
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_S32:
      elem_size = INT32_SIZE;
      packLE(&data[0], (uint32_t)(int32_t)lower_limit, (uint8_t)UINT32_SIZE);
      packLE(&data[elem_size], (uint32_t)(int32_t)upper_limit, (uint8_t)UINT32_SIZE);
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_S64:
      elem_size = INT64_SIZE;
      packLE64(&data[0], (uint64_t)lower_limit, (uint8_t)elem_size);
      packLE64(&data[elem_size], (uint64_t)upper_limit, (uint8_t)elem_size);
      break;
   default:
      return APX_INVALID_INSTRUCTION_ERROR;
   }
   adt_error_t const rc = adt_bytearray_append(&self->buffer, &data[0], elem_size * 2u);
   return (rc == ADT_NO_ERROR) ? APX_NO_ERROR : APX_MEM_ERROR;
}

apx_error_t apx_program_encoder_encode_program_header(apx_program_encoder_t* self, apx_program_type_t program_type, uint32_t elem_size, uint32_t queue_size, bool is_dynamic)
{
   if (self == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   adt_bytearray_clear(&self->header);

   adt_error_t rc = adt_bytearray_push(&self->header, APX_VM_HEADER_VERSION_MAJOR);
   if (rc == ADT_NO_ERROR)
   {
      rc = adt_bytearray_push(&self->header, APX_VM_HEADER_VERSION_MINOR);
   }
   if (rc != ADT_NO_ERROR)
   {
      return APX_MEM_ERROR;
   }

   bool const is_queued = (queue_size > 0u);
   uint8_t queue_variant = 0u;
   uint8_t elem_variant = 0u;
   uint8_t data_size_variant = 0u;
   uint32_t max_data_size = 0u;

   if (is_queued)
   {
      queue_variant = (queue_size <= UINT8_MAX) ? APX_VM_VARIANT_UINT8 :
                      (queue_size <= UINT16_MAX) ? APX_VM_VARIANT_UINT16 : APX_VM_VARIANT_UINT32;
      elem_variant = (elem_size <= UINT8_MAX) ? APX_VM_VARIANT_UINT8 :
                     (elem_size <= UINT16_MAX) ? APX_VM_VARIANT_UINT16 : APX_VM_VARIANT_UINT32;
      uint32_t const queue_storage_size = (queue_variant == APX_VM_VARIANT_UINT8) ? UINT8_SIZE :
                                          (queue_variant == APX_VM_VARIANT_UINT16) ? UINT16_SIZE : UINT32_SIZE;
      uint64_t const total = (uint64_t)queue_storage_size + ((uint64_t)elem_size * queue_size);
      if (total > UINT32_MAX)
      {
         return APX_LENGTH_ERROR;
      }
      max_data_size = (uint32_t)total;
      data_size_variant = calc_data_size_variant(elem_variant, queue_variant);
   }
   else
   {
      max_data_size = elem_size;
   }

   uint8_t const max_data_size_variant = (max_data_size <= UINT8_MAX) ? APX_VM_VARIANT_UINT8 :
                                        (max_data_size <= UINT16_MAX) ? APX_VM_VARIANT_UINT16 : APX_VM_VARIANT_UINT32;
   uint8_t const type_byte = encode_program_type_byte(program_type, max_data_size_variant, is_dynamic, is_queued);
   rc = adt_bytearray_push(&self->header, type_byte);
   if (rc != ADT_NO_ERROR)
   {
      return APX_MEM_ERROR;
   }

   uint8_t size_bytes[UINT32_SIZE];
   uint32_t const size_bytes_len = (max_data_size_variant == APX_VM_VARIANT_UINT8) ? UINT8_SIZE :
                                  (max_data_size_variant == APX_VM_VARIANT_UINT16) ? UINT16_SIZE : UINT32_SIZE;
   packLE(&size_bytes[0], max_data_size, (uint8_t)size_bytes_len);
   rc = adt_bytearray_append(&self->header, &size_bytes[0], size_bytes_len);
   if (rc != ADT_NO_ERROR)
   {
      return APX_MEM_ERROR;
   }

   if (is_queued)
   {
      apx_error_t const err = apx_program_encoder_encode_header_instruction(self, APX_VM_OPCODE_DATA_SIZE, data_size_variant, false);
      if (err != APX_NO_ERROR)
      {
         return err;
      }
      uint32_t const elem_size_bytes_len = (elem_variant == APX_VM_VARIANT_UINT8) ? UINT8_SIZE :
                                          (elem_variant == APX_VM_VARIANT_UINT16) ? UINT16_SIZE : UINT32_SIZE;
      packLE(&size_bytes[0], elem_size, (uint8_t)elem_size_bytes_len);
      rc = adt_bytearray_append(&self->header, &size_bytes[0], elem_size_bytes_len);
      if (rc != ADT_NO_ERROR)
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_NO_ERROR;
}

apx_error_t apx_program_encoder_encode_field_name(apx_program_encoder_t* self, const char* name)
{
   if ( (self == NULL) || (name == NULL) )
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   size_t const len = strlen(name);
   adt_error_t rc = adt_bytearray_append(&self->buffer, (uint8_t const*)name, (uint32_t)len);
   if (rc == ADT_NO_ERROR)
   {
      rc = adt_bytearray_push(&self->buffer, 0u);
   }
   return (rc == ADT_NO_ERROR) ? APX_NO_ERROR : APX_MEM_ERROR;
}

apx_program_t const* apx_program_encoder_get_header(apx_program_encoder_t const* self)
{
   return (self != NULL) ? &self->header : NULL;
}

apx_program_t const* apx_program_encoder_get_buffer(apx_program_encoder_t const* self)
{
   return (self != NULL) ? &self->buffer : NULL;
}

apx_error_t apx_program_encoder_get_program(apx_program_encoder_t const* self, apx_program_t* out_program)
{
   if ( (self == NULL) || (out_program == NULL) )
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   adt_bytearray_clear(out_program);
   uint32_t const header_len = adt_bytearray_length(&self->header);
   uint32_t const buffer_len = adt_bytearray_length(&self->buffer);
   if (header_len > 0u)
   {
      adt_error_t const rc = adt_bytearray_append(out_program, adt_bytearray_data(&self->header), header_len);
      if (rc != ADT_NO_ERROR)
      {
         return APX_MEM_ERROR;
      }
   }
   if (buffer_len > 0u)
   {
      adt_error_t const rc = adt_bytearray_append(out_program, adt_bytearray_data(&self->buffer), buffer_len);
      if (rc != ADT_NO_ERROR)
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static uint8_t encode_program_type_byte(apx_program_type_t program_type, uint8_t data_size_variant, bool is_dynamic, bool is_queued)
{
   uint8_t value = data_size_variant & APX_VM_HEADER_DATA_VARIANT_MASK;
   if (program_type == APX_PACK_PROGRAM)
   {
      value |= APX_VM_HEADER_PROG_TYPE_PACK;
   }
   if (is_dynamic)
   {
      value |= APX_VM_HEADER_FLAG_DYNAMIC_DATA;
   }
   if (is_queued)
   {
      value |= APX_VM_HEADER_FLAG_QUEUED_DATA;
   }
   return value;
}

static uint8_t calc_data_size_variant(uint8_t elem_variant, uint8_t queue_variant)
{
   uint8_t retval = (elem_variant == APX_VM_VARIANT_UINT8) ? APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE :
      (elem_variant == APX_VM_VARIANT_UINT16) ? APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE : APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE;
   retval += queue_variant;
   return retval;
}
