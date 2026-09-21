/*****************************************************************************
* \file      program.c
* \author    Conny Gustafsson
* \date      2020-11-30
* \brief     Utility functions for handling byte-code programs
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdio.h>
#include "apx/program.h"
#include "apx/program_encoder.h"
#include "apx/program_decoder.h"
#include "apx/vm_common.h"
#include "pack.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_program_encode_header(apx_program_t* program, apx_program_type_t program_type, uint32_t element_size, uint32_t queue_size, bool is_dynamic)
{
   if (program == NULL)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   apx_program_encoder_t encoder;
   apx_program_encoder_create(&encoder);
   apx_error_t const result = apx_program_encoder_encode_program_header(&encoder, program_type, element_size, queue_size, is_dynamic);
   if (result == APX_NO_ERROR)
   {
      adt_bytearray_clear(program);
      adt_error_t const rc = adt_bytearray_append(program, adt_bytearray_data(&encoder.header), adt_bytearray_length(&encoder.header));
      if (rc != ADT_NO_ERROR)
      {
         apx_program_encoder_destroy(&encoder);
         return APX_MEM_ERROR;
      }
   }
   apx_program_encoder_destroy(&encoder);
   return result;
}

apx_error_t apx_program_decode_header(uint8_t const* begin, uint8_t const* end, uint8_t const** next, apx_program_header_t* header)
{
   if ((begin != NULL) && (end != NULL) && (next != NULL) && (header != NULL))
   {
      apx_program_decoder_t decoder;
      apx_program_decoder_create(&decoder);
      apx_error_t result = apx_program_decoder_select_program(&decoder, begin, (uint32_t)(end - begin));
      if (result == APX_NO_ERROR)
      {
         result = apx_program_decoder_parse_program_header(&decoder, header);
         if (result == APX_NO_ERROR)
         {
            *next = decoder.program_next;
         }
      }
      apx_program_decoder_destroy(&decoder);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

uint8_t apx_program_encode_instruction(uint8_t opcode, uint8_t variant, bool flag)
{
   uint8_t result = ((opcode & APX_VM_INST_OPCODE_MASK) << APX_VM_INST_OPCODE_SHIFT) | ((variant & APX_VM_INST_VARIANT_MASK) << APX_VM_INST_VARIANT_SHIFT);
   if (flag)
   {
      result |= APX_VM_INST_FLAG;
   }
   return result;
}

void apx_program_decode_instruction(uint8_t const instruction, uint8_t* opcode, uint8_t* variant, bool* flag)
{
   if ((opcode != NULL) && (variant != NULL) && (flag != NULL))
   {
      *opcode = (instruction >> APX_VM_INST_OPCODE_SHIFT) & APX_VM_INST_OPCODE_MASK;
      *variant = (instruction >> APX_VM_INST_VARIANT_SHIFT) & APX_VM_INST_VARIANT_MASK;
      *flag = (instruction & APX_VM_INST_FLAG) != 0;
   }
}

void apx_program_dump(apx_program_t const* program)
{
   if (program != NULL)
   {
      uint8_t const* program_next = adt_bytearray_data(program);
      uint8_t const* program_end = program_next + adt_bytearray_length(program);
      while(program_next < program_end)
      {
         uint8_t const instruction = *program_next++;
         uint8_t opcode = 0u;
         uint8_t variant = 0u;
         bool flag = false;
         apx_program_decode_instruction(instruction, &opcode, &variant, &flag);
         printf("Instruction: 0x%02X, Opcode: 0x%02X, Variant: 0x%02X, Flag: %d\n", instruction, opcode, variant, flag);
      }
   }
}