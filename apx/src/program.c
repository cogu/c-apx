/*****************************************************************************
* \file      program.c
* \author    Conny Gustafsson
* \date      2020-11-30
* \brief     Utility functions for handling byte-code programs
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
#include <assert.h>
#include "apx/program.h"
#include "apx/vm_common.h"
#include "pack.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static uint8_t encode_program_byte(apx_programType_t program_type, bool is_dynamic, bool is_queued, uint8_t data_size_variant);
static uint8_t calc_data_size_variant(uint8_t element_variant, uint8_t queue_variant);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_program_encode_header(apx_program_t* program, apx_programType_t program_type, uint32_t element_size, uint32_t queue_size, bool is_dynamic)
{
   adt_error_t rc = ADT_NO_ERROR;
   uint8_t encoded_size[APX_VM_UINT32_SIZE] = { 0u, 0u, 0u, 0u };
   uint8_t data_size_variant = 0u;
   uint32_t packed_size = 0u;
   uint32_t data_size = 0u;
   uint8_t queue_variant = 0u; //Only used when is_queued is true
   uint8_t element_variant = 0u; //Only used when is_queued is true
   uint8_t program_byte = 0u;
   uint8_t* p = &encoded_size[0];
   bool const is_queued = queue_size > 0u;

   if (is_queued)
   {
      queue_variant = (queue_size <= UINT8_MAX) ? APX_VM_VARIANT_UINT8 : (queue_size <= UINT16_MAX) ? APX_VM_VARIANT_UINT16 : APX_VM_VARIANT_UINT32;
      element_variant = (element_size <= UINT8_MAX) ? APX_VM_VARIANT_UINT8 : (element_size <= UINT16_MAX) ? APX_VM_VARIANT_UINT16 : APX_VM_VARIANT_UINT32;
      uint32_t const queue_length_size = (queue_variant == APX_VM_VARIANT_UINT8) ? UINT8_SIZE : (queue_variant == APX_VM_VARIANT_UINT16) ? UINT16_SIZE : UINT32_SIZE;
      uint64_t const tmp = queue_length_size + (((uint64_t)element_size) * queue_size);
      if (tmp > UINT32_MAX)
      {
         return APX_LENGTH_ERROR;
      }
      data_size = (uint32_t)tmp;
   }
   else
   {
      data_size = element_size;
   }

   //determine value of data_size_variant
   if (data_size <= UINT8_MAX)
   {
      data_size_variant = APX_VM_VARIANT_UINT8;
      packLE(p, data_size, (uint8_t)UINT8_SIZE);
      p += UINT8_SIZE;
   }
   else if (data_size <= UINT16_MAX)
   {
      data_size_variant = APX_VM_VARIANT_UINT16;
      packLE(p, data_size, (uint8_t)UINT16_SIZE);
      p += UINT16_SIZE;
   }
   else
   {
      data_size_variant = APX_VM_VARIANT_UINT32;
      packLE(p, data_size, (uint8_t)UINT32_SIZE);
      p += UINT32_SIZE;
   }
   packed_size = (uint32_t)(p - &encoded_size[0]);
   assert((packed_size > 0) && (packed_size <= UINT32_SIZE));

   program_byte = encode_program_byte(program_type, is_dynamic, is_queued, data_size_variant);
   rc = adt_bytearray_push(program, program_byte);
   if (rc == ADT_NO_ERROR)
   {
      rc = adt_bytearray_append(program, &encoded_size[0], packed_size);
   }
   if ((rc == ADT_NO_ERROR) && is_queued)
   {
      uint8_t const opcode = APX_VM_OPCODE_DATA_SIZE;
      uint8_t const variant = calc_data_size_variant(element_variant, queue_variant);
      uint8_t const instruction = apx_program_encode_instruction(opcode, variant, false);
      rc = adt_bytearray_push(program, instruction);
      if (rc == ADT_NO_ERROR)
      {
         p = &encoded_size[0];
         switch (element_variant)
         {
         case APX_VM_VARIANT_UINT8:
            packLE(p, element_size, (uint8_t)UINT8_SIZE);
            p += UINT8_SIZE;
            break;
         case APX_VM_VARIANT_UINT16:
            packLE(p, element_size, (uint8_t)UINT16_SIZE);
            p += UINT16_SIZE;
            break;
         case APX_VM_VARIANT_UINT32:
            packLE(p, element_size, (uint8_t)UINT32_SIZE);
            p += UINT32_SIZE;
            break;
         default:
            assert(0);
            return APX_VALUE_TYPE_ERROR;
         }
         packed_size = (uint32_t)(p - &encoded_size[0]);
         assert((packed_size > 0) && (packed_size <= UINT32_SIZE));
         rc = adt_bytearray_append(program, &encoded_size[0], packed_size);
      }
   }

   if (rc != ADT_NO_ERROR)
   {
      if (rc == ADT_MEM_ERROR)
      {
         return APX_MEM_ERROR;
      }
      else
      {
         return APX_INTERNAL_ERROR;
      }
   }
   return APX_NO_ERROR;
}

apx_error_t apx_program_decode_header(uint8_t const* begin, uint8_t const* end, uint8_t const** next, apx_programHeader_t* header)
{
   if ((begin != NULL) && (end != NULL) && ((begin + UINT8_SIZE) < end) && (next != NULL) && (header != NULL))
   {
      uint8_t const* result = NULL;
      header->element_size = 0u;
      header->queue_length = 0u;
      uint8_t data_variant = begin[0] & APX_VM_HEADER_DATA_VARIANT_MASK;
      header->program_type = ((begin[0] & APX_VM_HEADER_PROG_TYPE_PACK) == APX_VM_HEADER_PROG_TYPE_PACK) ? APX_PACK_PROGRAM : APX_UNPACK_PROGRAM;
      bool const is_queued_data = ((begin[0] & APX_VM_HEADER_FLAG_QUEUED_DATA) == APX_VM_HEADER_FLAG_QUEUED_DATA);
      header->has_dynamic_data = ((begin[0] & APX_VM_HEADER_FLAG_DYNAMIC_DATA) == APX_VM_HEADER_FLAG_DYNAMIC_DATA);
      *next = begin+1;
      result = apx_vm_parse_uint32_by_variant(*next, end, data_variant, &header->data_size);
      if ((result > *next) && (result <= end))
      {
         *next = result;
      }
      else
      {
         return APX_PARSE_ERROR;
      }

      if (is_queued_data)
      {
         uint8_t opcode = 0u;
         uint8_t variant = 0u;
         bool flag = false;;
         uint8_t element_variant = 0u;
         uint8_t queued_variant = 0u;
         uint32_t queued_elem_size = 0u;
         uint32_t tmp = 0u;
         if (*next >= end)
         {
            return APX_PARSE_ERROR;
         }
         apx_program_decode_instruction(**next, &opcode, &variant, &flag);
         if ((opcode != APX_VM_OPCODE_DATA_SIZE) || (variant < APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE) || (variant > APX_VM_VARIANT_ELEMENT_SIZE_LAST))
         {
            return APX_PARSE_ERROR;
         }
         (*next)++;
         if (variant < APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE) //variant is between 3..5
         {
            element_variant = APX_VM_VARIANT_UINT8;
            queued_variant = variant - APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE;
         }
         else if (variant < APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE) //variant is between 6..8
         {
            element_variant = APX_VM_VARIANT_UINT16;
            queued_variant = variant - APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE;
         }
         else //variant is between 9..11
         {
            element_variant = APX_VM_VARIANT_UINT32;
            queued_variant = variant - APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE;
         }
         queued_elem_size = (uint32_t)apx_vm_variant_to_size(queued_variant);
         if ((queued_elem_size == 0u) || (queued_elem_size > header->data_size))
         {
            return APX_PARSE_ERROR;
         }
         result = apx_vm_parse_uint32_by_variant(*next, end, element_variant, &header->element_size);
         if ((result > *next) && (result <= end))
         {
            *next = result;
         }
         else
         {
            return APX_PARSE_ERROR;
         }
         //Calculate element size by subtracting queued_elem_size from header->data_size then dividing by header->element_size
         if (header->element_size == 0)
         {
            return APX_PARSE_ERROR;
         }
         tmp = header->data_size - queued_elem_size;
         if (tmp % header->element_size != 0)
         {
            return APX_INVALID_HEADER_ERROR;
         }
         header->queue_length = tmp / header->element_size;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;

}

uint8_t apx_program_encode_instruction(uint8_t opcode, uint8_t variant, bool flag)
{
   uint8_t result = (opcode & APX_VM_INST_OPCODE_MASK) | ((variant & APX_VM_INST_VARIANT_MASK) << APX_VM_INST_VARIANT_SHIFT);
   if (flag)
   {
      result |= APX_VM_INST_FLAG;
   }
   return result;
}

void apx_program_decode_instruction(uint8_t instruction, uint8_t* opcode, uint8_t* variant, bool* flag)
{
   if ((opcode != NULL) && (variant != NULL) && (flag != NULL))
   {
      *opcode = instruction & APX_VM_INST_OPCODE_MASK;
      *variant = (instruction >> APX_VM_INST_VARIANT_SHIFT) & APX_VM_INST_VARIANT_MASK;
      *flag = (instruction & APX_VM_INST_FLAG) == APX_VM_INST_FLAG ? true : false;
   }
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static uint8_t encode_program_byte(apx_programType_t program_type, bool is_dynamic, bool is_queued, uint8_t data_size_variant)
{
   uint8_t retval = (program_type == APX_PACK_PROGRAM) ? APX_VM_HEADER_PROG_TYPE_PACK : APX_VM_HEADER_PROG_TYPE_UNPACK;
   retval |= (data_size_variant & APX_VM_HEADER_DATA_VARIANT_MASK);
   if (is_dynamic)
   {
      retval |= APX_VM_HEADER_FLAG_DYNAMIC_DATA;
   }
   if (is_queued)
   {
      retval |= APX_VM_HEADER_FLAG_QUEUED_DATA;
   }
   return retval;
}

static uint8_t calc_data_size_variant(uint8_t element_variant, uint8_t queue_variant)
{
   uint8_t retval = (element_variant == APX_VM_VARIANT_UINT8) ? APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE :
      (element_variant == APX_VM_VARIANT_UINT16) ? APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE : APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE;
   retval += queue_variant;
   return retval;
}

