/*****************************************************************************
* \file      compiler.c
* \author    Conny Gustafsson
* \date      2019-01-03
* \brief     APX bytecode compiler
*
* Copyright (c) 2019-2020 Conny Gustafsson
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
#include "apx/compiler.h"
#include "apx/vm_common.h"
#include "apx/util.h"
#include "pack.h"
#include <malloc.h>
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#else
#define vfree free
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void reset_internal_state(apx_compiler_t* self);
static void set_error(apx_compiler_t* self, apx_error_t *storage, apx_error_t error);
static apx_error_t compile_data_element(apx_compiler_t* self, apx_dataElement_t const* data_element, apx_programType_t program_type, uint32_t* data_size);
static apx_error_t compile_limit_instruction(apx_compiler_t* self, apx_dataElement_t const* data_element, bool is_signed_type, bool is_64_bit_type, bool is_array, uint8_t limit_variant);
static apx_error_t compile_limit_values_int32(apx_compiler_t* self, uint8_t limit_variant, int32_t lower_limit, int32_t upper_limit);
static apx_error_t compile_limit_values_uint32(apx_compiler_t* self, uint8_t limit_variant, uint32_t lower_limit, uint32_t upper_limit);
static apx_error_t compile_limit_values_int64(apx_compiler_t* self, uint8_t limit_variant, int64_t lower_limit, int64_t upper_limit);
static apx_error_t compile_limit_values_uint64(apx_compiler_t* self, uint8_t limit_variant, uint64_t lower_limit, uint64_t upper_limit);
static apx_error_t compile_array_size_instruction(apx_compiler_t* self, uint32_t array_size, bool is_dynamic_array);
static apx_error_t compile_record_fields(apx_compiler_t* self, apx_dataElement_t const* data_element, apx_programType_t program_type, uint32_t* record_size);
static apx_error_t compile_record_select_instruction(apx_compiler_t* self, apx_dataElement_t const* data_element, bool is_last_field);
static apx_error_t compile_array_next_instruction(apx_compiler_t* self);



//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_compiler_create(apx_compiler_t *self)
{
   if (self != 0)
   {
      self->program = NULL;
      self->last_error = APX_NO_ERROR;
      self->has_dynamic_data = false;
   }
}

void apx_compiler_destroy(apx_compiler_t *self)
{
   if (self != 0)
   {
      if (self->program != NULL)
      {
         APX_PROGRAM_DELETE(self->program);
      }
   }
}

apx_compiler_t* apx_compiler_new(void)
{
   apx_compiler_t *self = (apx_compiler_t*) malloc(sizeof(apx_compiler_t));
   if (self != 0)
   {
      apx_compiler_create(self);
   }
   return self;
}

void apx_compiler_delete(apx_compiler_t *self)
{
   if (self != 0)
   {
      apx_compiler_destroy(self);
      free(self);
   }
}

apx_program_t* apx_compiler_compile_port(apx_compiler_t* self, apx_port_t* port, apx_programType_t program_type, apx_error_t* error_code)
{
   if (self != NULL)
   {
      uint32_t element_size = 0u;
      reset_internal_state(self);

      self->program = APX_PROGRAM_NEW();
      if ( self->program == NULL )
      {
         set_error(self, error_code, APX_MEM_ERROR);
         return NULL;
      }
      else if (port == NULL)
      {
         set_error(self, error_code, APX_INVALID_ARGUMENT_ERROR);
         return NULL;
      }
      else
      {
         apx_dataElement_t* data_element = apx_port_get_effective_data_element(port);
         if (data_element != NULL)
         {
            self->last_error = compile_data_element(self, data_element, program_type, &element_size);
         }
         else
         {
            set_error(self, error_code, APX_NULL_PTR_ERROR);
            return NULL;
         }
      }
      if (self->last_error != APX_NO_ERROR)
      {
         if (error_code != NULL)
         {
            *error_code = self->last_error;
         }
         return NULL;
      }
      else
      {
         apx_program_t* program_with_header;
         uint32_t queue_length = apx_port_get_queue_length(port);
         program_with_header = APX_PROGRAM_NEW();
         if (program_with_header == NULL)
         {
            set_error(self, error_code, APX_MEM_ERROR);
            return NULL;
         }
         self->last_error = apx_program_encode_header(program_with_header, program_type, element_size, queue_length, self->has_dynamic_data);
         if (self->last_error == APX_NO_ERROR)
         {
            adt_error_t rc;
            uint8_t const* program_data = NULL;
            uint32_t program_length = adt_bytearray_length(self->program);
            program_data = adt_bytearray_data(self->program);
            assert(program_data != NULL && program_length > 0);
            rc = adt_bytearray_append(program_with_header, program_data, program_length);
            if (rc != ADT_NO_ERROR)
            {
               set_error(self, error_code, convert_from_adt_to_apx_error(rc));
               APX_PROGRAM_DELETE(program_with_header);
               return NULL;
            }
            else
            {
               self->last_error = APX_NO_ERROR;
            }
         }
         if (error_code != NULL)
         {
            *error_code = self->last_error;
         }
         APX_PROGRAM_DELETE(self->program);
         self->program = NULL;
         return program_with_header;
      }
   }
   set_error(self, error_code, APX_INVALID_ARGUMENT_ERROR);
   return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void reset_internal_state(apx_compiler_t* self)
{
   self->last_error = APX_NO_ERROR;
   self->has_dynamic_data = false;
   if (self->program != NULL)
   {
      APX_PROGRAM_DELETE(self->program);
      self->program = NULL;
   }
}

static void set_error(apx_compiler_t* self, apx_error_t* storage, apx_error_t error)
{
   if (self != NULL)
   {
      self->last_error = error;
   }
   if (storage != NULL)
   {
      *storage = error;
   }
}

static apx_error_t compile_data_element(apx_compiler_t* self, apx_dataElement_t const* data_element, apx_programType_t program_type, uint32_t* data_size)
{
   if ( (data_element != NULL) && (data_size != NULL) )
   {
      apx_error_t retval = APX_NO_ERROR;
      uint8_t limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_NONE;
      uint8_t data_variant = 0u;
      uint32_t const array_length = apx_dataElement_get_array_length(data_element);
      bool const is_array = array_length > 0u;
      bool const is_dynamic_array = apx_dataElement_is_dynamic_array(data_element);
      bool const has_limits = apx_dataElement_has_limits(data_element);
      bool const is_pack_prog = (program_type == APX_PACK_PROGRAM) ? true : false;
      apx_typeCode_t const type_code = apx_dataElement_get_type_code(data_element);
      bool is_signed_type = false;
      bool is_64_bit_type = false;
      bool is_record = false;

      uint8_t const opcode = is_pack_prog ? APX_VM_OPCODE_PACK : APX_VM_OPCODE_UNPACK;

      switch (type_code)
      {
      case APX_TYPE_CODE_UINT8:
         data_variant = APX_VM_VARIANT_UINT8;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_U8;
         *data_size = APX_VM_UINT8_SIZE;
         break;
      case APX_TYPE_CODE_UINT16:
         data_variant = APX_VM_VARIANT_UINT16;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_U16;
         *data_size = APX_VM_UINT16_SIZE;
         break;
      case APX_TYPE_CODE_UINT32:
         data_variant = APX_VM_VARIANT_UINT32;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_U32;
         *data_size = APX_VM_UINT32_SIZE;
         break;
      case APX_TYPE_CODE_UINT64:
         data_variant = APX_VM_VARIANT_UINT64;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_U64;
         *data_size = APX_VM_UINT64_SIZE;
         is_64_bit_type = true;
         break;
      case APX_TYPE_CODE_INT8:
         data_variant = APX_VM_VARIANT_INT8;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_S8;
         *data_size = APX_VM_INT8_SIZE;
         is_signed_type = true;
         break;
      case APX_TYPE_CODE_INT16:
         data_variant = APX_VM_VARIANT_INT16;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_S16;
         *data_size = APX_VM_INT16_SIZE;
         is_signed_type = true;
         break;
      case APX_TYPE_CODE_INT32:
         data_variant = APX_VM_VARIANT_INT32;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_S32;
         *data_size = APX_VM_INT32_SIZE;
         is_signed_type = true;
         break;
      case APX_TYPE_CODE_INT64:
         data_variant = APX_VM_VARIANT_INT64;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_S64;
         *data_size = APX_VM_INT64_SIZE;
         is_signed_type = true;
         is_64_bit_type = true;
         break;
      case APX_TYPE_CODE_BOOL:
         data_variant = APX_VM_VARIANT_BOOL;
         *data_size = APX_VM_BOOL_SIZE;
         break;
      case APX_TYPE_CODE_BYTE:
         data_variant = APX_VM_VARIANT_BYTE;
         limit_check_variant = APX_VM_VARIANT_LIMIT_CHECK_U8;
         *data_size = APX_VM_BYTE_SIZE;
         break;
      case APX_TYPE_CODE_CHAR:
         data_variant = APX_VM_VARIANT_CHAR;
         *data_size = APX_VM_CHAR_SIZE;
         break;
      case APX_TYPE_CODE_CHAR8:
         data_variant = APX_VM_VARIANT_CHAR8;
         *data_size = APX_VM_UINT8_SIZE;
         break;
      case APX_TYPE_CODE_RECORD:
         data_variant = APX_VM_VARIANT_RECORD;
         is_record = true;
         break;
      default:
         retval = APX_ELEMENT_TYPE_ERROR;
         break;
      }

      if (retval == APX_NO_ERROR)
      {
         if (is_record)
         {
            adt_error_t rc;
            uint8_t const instruction = apx_program_encode_instruction(opcode, data_variant, is_array);
            rc = adt_bytearray_push(self->program, instruction);
            if (rc != ADT_NO_ERROR)
            {
               return convert_from_adt_to_apx_error(rc);
            }
            if (is_array)
            {
               retval = compile_array_size_instruction(self, array_length, is_dynamic_array);
               if (retval != APX_NO_ERROR)
               {
                  return retval;
               }
               if (is_dynamic_array)
               {
                  self->has_dynamic_data = true;
               }
            }
            retval = compile_record_fields(self, data_element, program_type, data_size);
            if ((retval == APX_NO_ERROR) && (is_array))
            {
               retval = compile_array_next_instruction(self);
            }
         }
         else
         {
            adt_error_t rc;
            if (has_limits && is_pack_prog)
            {
               retval = compile_limit_instruction(self, data_element, is_signed_type, is_64_bit_type, is_array, limit_check_variant);
               if (retval != APX_NO_ERROR)
               {
                  return retval;
               }
            }
            uint8_t const instruction = apx_program_encode_instruction(opcode, data_variant, is_array);
            rc = adt_bytearray_push(self->program, instruction);
            if (rc != ADT_NO_ERROR)
            {
               return convert_from_adt_to_apx_error(rc);
            }
            if (is_array)
            {
               retval = compile_array_size_instruction(self, array_length, is_dynamic_array);
               if (retval != APX_NO_ERROR)
               {
                  return retval;
               }
               if (is_dynamic_array)
               {
                  self->has_dynamic_data = true;
               }
            }
            if (has_limits && !is_pack_prog)
            {
               retval = compile_limit_instruction(self, data_element, is_signed_type, is_64_bit_type, is_array, limit_check_variant);
               if (retval != APX_NO_ERROR)
               {
                  return retval;
               }
            }
         }
      }
      if (retval == APX_NO_ERROR)
      {
         if (*data_size > 0u)
         {
            if (is_array)
            {
               assert(array_length > 0u);
               *data_size *= array_length;
               if (is_dynamic_array)
               {
                  *data_size += apx_vm_size_type_to_size(apx_vm_size_to_size_type(array_length));
               }
            }
         }
         else
         {
            retval = APX_ELEMENT_TYPE_ERROR;
         }
      }
      return retval;
   }
   return APX_NULL_PTR_ERROR;
}

static apx_error_t compile_limit_instruction(apx_compiler_t* self, apx_dataElement_t const* data_element, bool is_signed_type, bool is_64_bit_type, bool is_array, uint8_t limit_variant)
{
   assert( (self != NULL) && (data_element != NULL) );
   if ( (limit_variant != APX_VM_VARIANT_LIMIT_CHECK_NONE))
   {
      apx_error_t retval = APX_NO_ERROR;
      uint8_t const opcode = APX_VM_OPCODE_DATA_CTRL;
      adt_error_t rc;
      uint8_t instruction_header = apx_program_encode_instruction(opcode, limit_variant, is_array);
      rc = adt_bytearray_push(self->program, instruction_header);
      if (rc != ADT_NO_ERROR)
      {
         return convert_from_adt_to_apx_error(rc);
      }
      if (is_64_bit_type)
      {
         if (is_signed_type)
         {
            int64_t lower_limit, upper_limit;
            if (apx_dataElement_get_limits_int64(data_element, &lower_limit, &upper_limit))
            {
               retval = compile_limit_values_int64(self, limit_variant, lower_limit, upper_limit);
            }
            else
            {
               retval = APX_VALUE_CONVERSION_ERROR;
            }
         }
         else
         {
            uint64_t lower_limit, upper_limit;
            if (apx_dataElement_get_limits_uint64(data_element, &lower_limit, &upper_limit))
            {
               retval = compile_limit_values_uint64(self, limit_variant, lower_limit, upper_limit);
            }
            else
            {
               retval = APX_VALUE_CONVERSION_ERROR;
            }
         }
      }
      else
      {
         if (is_signed_type)
         {
            int32_t lower_limit, upper_limit;
            if (apx_dataElement_get_limits_int32(data_element, &lower_limit, &upper_limit))
            {
               retval = compile_limit_values_int32(self, limit_variant, lower_limit, upper_limit);
            }
            else
            {
               retval = APX_VALUE_CONVERSION_ERROR;
            }
         }
         else
         {
            uint32_t lower_limit, upper_limit;
            if (apx_dataElement_get_limits_uint32(data_element, &lower_limit, &upper_limit))
            {
               retval = compile_limit_values_uint32(self, limit_variant, lower_limit, upper_limit);
            }
            else
            {
               retval = APX_VALUE_CONVERSION_ERROR;
            }
         }
      }
      return retval;
   }
   return APX_UNSUPPORTED_ERROR;
}

static apx_error_t compile_limit_values_int32(apx_compiler_t* self, uint8_t limit_variant, int32_t lower_limit, int32_t upper_limit)
{
   apx_error_t retval = APX_NO_ERROR;
   uint8_t buf[INT32_SIZE * 2];
   uint8_t* p = &buf[0];
   uint8_t elem_size = 0u;

   assert(sizeof(buf) == 8);
   switch (limit_variant)
   {
   case APX_VM_VARIANT_LIMIT_CHECK_S8:
      elem_size = INT8_SIZE;
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_S16:
      elem_size = INT16_SIZE;
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_S32:
      elem_size = INT32_SIZE;
      break;
   default:
      retval = APX_UNSUPPORTED_ERROR;
   }
   packLE(p, (uint32_t)lower_limit, elem_size);
   p += elem_size;
   packLE(p, (uint32_t)upper_limit, elem_size);
   if (retval == APX_NO_ERROR)
   {
      adt_error_t rc = adt_bytearray_append(self->program, &buf[0], elem_size * 2);
      retval = convert_from_adt_to_apx_error(rc);
   }
   return retval;
}

static apx_error_t compile_limit_values_uint32(apx_compiler_t* self, uint8_t limit_variant, uint32_t lower_limit, uint32_t upper_limit)
{
   apx_error_t retval = APX_NO_ERROR;
   uint8_t buf[UINT32_SIZE * 2];
   uint8_t* p = &buf[0];
   uint8_t elem_size = 0u;

   assert(sizeof(buf) == 8);
   switch (limit_variant)
   {
   case APX_VM_VARIANT_LIMIT_CHECK_U8:
      elem_size = UINT8_SIZE;
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_U16:
      elem_size = UINT16_SIZE;
      break;
   case APX_VM_VARIANT_LIMIT_CHECK_U32:
      elem_size = UINT32_SIZE;
      break;
   default:
      retval = APX_UNSUPPORTED_ERROR;
   }
   packLE(p, lower_limit, elem_size);
   p += elem_size;
   packLE(p, upper_limit, elem_size);
   if (retval == APX_NO_ERROR)
   {
      adt_error_t rc = adt_bytearray_append(self->program, &buf[0], elem_size * 2);
      retval = convert_from_adt_to_apx_error(rc);
   }
   return retval;
}

static apx_error_t compile_limit_values_int64(apx_compiler_t* self, uint8_t limit_variant, int64_t lower_limit, int64_t upper_limit)
{
   apx_error_t retval = APX_NO_ERROR;
   uint8_t buf[INT64_SIZE * 2];
   uint8_t* p = &buf[0];
   uint8_t elem_size = 0u;

   assert(sizeof(buf) == 16);
   switch (limit_variant)
   {
   case APX_VM_VARIANT_LIMIT_CHECK_S64:
      elem_size = INT64_SIZE;
      break;
   default:
      retval = APX_UNSUPPORTED_ERROR;
   }
   packLE64(p, (uint64_t)lower_limit, elem_size);
   p += elem_size;
   packLE64(p, (uint64_t)upper_limit, elem_size);
   if (retval == APX_NO_ERROR)
   {
      adt_error_t rc = adt_bytearray_append(self->program, &buf[0], elem_size * 2);
      retval = convert_from_adt_to_apx_error(rc);
   }
   return retval;
}

static apx_error_t compile_limit_values_uint64(apx_compiler_t* self, uint8_t limit_variant, uint64_t lower_limit, uint64_t upper_limit)
{
   apx_error_t retval = APX_NO_ERROR;
   uint8_t buf[UINT64_SIZE * 2];
   uint8_t* p = &buf[0];
   uint8_t elem_size = 0u;

   assert(sizeof(buf) == 16);
   switch (limit_variant)
   {
   case APX_VM_VARIANT_LIMIT_CHECK_U64:
      elem_size = UINT64_SIZE;
      break;
   default:
      retval = APX_UNSUPPORTED_ERROR;
   }
   packLE64(p, lower_limit, elem_size);
   p += elem_size;
   packLE64(p, upper_limit, elem_size);
   if (retval == APX_NO_ERROR)
   {
      adt_error_t rc = adt_bytearray_append(self->program, &buf[0], elem_size * 2);
      retval = convert_from_adt_to_apx_error(rc);
   }
   return retval;
}

static apx_error_t compile_array_size_instruction(apx_compiler_t* self, uint32_t array_size, bool is_dynamic_array)
{
   uint8_t const opcode = APX_VM_OPCODE_DATA_SIZE;
   uint8_t buf[UINT32_SIZE];
   uint8_t variant;
   uint8_t* p = &buf[0];
   uint32_t encoded_size = 0u;
   adt_error_t rc;
   uint8_t instruction_header;
   if (array_size > UINT16_MAX)
   {
      variant = APX_VM_VARIANT_ARRAY_SIZE_U32;
      encoded_size = UINT32_SIZE;
      packLE(p, array_size, (uint8_t)encoded_size);
      p += sizeof(uint32_t);
   }
   else if (array_size > UINT8_MAX)
   {
      variant = APX_VM_VARIANT_ARRAY_SIZE_U16;
      encoded_size = UINT16_SIZE;
      packLE(p, array_size, (uint8_t)encoded_size);
      p += sizeof(uint16_t);
   }
   else
   {
      variant = APX_VM_VARIANT_ARRAY_SIZE_U8;
      encoded_size = UINT8_SIZE;
      packLE(p, array_size, (uint8_t)encoded_size);
      p += sizeof(uint8_t);
   }
   instruction_header = apx_program_encode_instruction(opcode, variant, is_dynamic_array);
   rc = adt_bytearray_push(self->program, instruction_header);
   if (rc == ADT_NO_ERROR)
   {
      rc = adt_bytearray_append(self->program, &buf[0], encoded_size);
   }
   return convert_from_adt_to_apx_error(rc);
}

static apx_error_t compile_record_fields(apx_compiler_t* self, apx_dataElement_t const* data_element, apx_programType_t program_type, uint32_t* record_size)
{
   assert( (self != NULL) && (data_element != NULL) && (record_size != NULL) );
   int32_t i;
   int32_t num_children = apx_dataElement_get_num_child_elements(data_element);
   if (apx_dataElement_get_type_code(data_element) != APX_TYPE_CODE_RECORD)
   {
      return APX_ELEMENT_TYPE_ERROR;
   }
   if (num_children == 0)
   {
      return APX_EMPTY_RECORD_ERROR;
   }
   for (i = 0u; i < num_children; i++)
   {
      apx_error_t result = APX_NO_ERROR;
      uint32_t child_size = 0u;
      apx_dataElement_t const* child_element = apx_dataElement_get_child_at(data_element, i);
      assert(child_element != NULL);
      result = compile_record_select_instruction(self, child_element, (i == (num_children - 1)) ? true : false);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      apx_dataElement_t* derived_element = NULL;
      result = apx_dataElement_derive_data_element(child_element, &derived_element, NULL);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(derived_element != NULL);
      result = compile_data_element(self, derived_element, program_type, &child_size);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      if (child_size == 0u)
      {
         return APX_LENGTH_ERROR;
      }
      *record_size += child_size;
   }
   return APX_NO_ERROR;
}

static apx_error_t compile_record_select_instruction(apx_compiler_t* self, apx_dataElement_t const* data_element, bool is_last_field)
{
   assert((self != NULL) && (data_element != NULL));
   char const* name = apx_dataElement_get_name(data_element);
   adt_error_t rc = ADT_NO_ERROR;
   size_t name_size = 0u;
   if (name == NULL )
   {
      return APX_NAME_MISSING_ERROR;
   }
   name_size = strlen(name);
   if (name_size == 0u)
   {
      return APX_NAME_MISSING_ERROR;
   }
   if (name_size > APX_MAX_NAME_LEN)
   {
      return APX_NAME_TOO_LONG_ERROR;
   }
   uint8_t const instruction = apx_program_encode_instruction(APX_VM_OPCODE_DATA_CTRL, APX_VM_VARIANT_RECORD_SELECT, is_last_field);
   rc = adt_bytearray_push(self->program, instruction);
   if (rc == ADT_NO_ERROR)
   {
      rc = adt_bytearray_append(self->program, (uint8_t const*) name, (uint32_t) name_size);
      if (rc == ADT_NO_ERROR)
      {
         rc = adt_bytearray_push(self->program, 0u); //null-terminator;
      }
   }
   return convert_from_adt_to_apx_error(rc);
}

static apx_error_t compile_array_next_instruction(apx_compiler_t* self)
{
   assert(self != NULL);
   uint8_t const instruction = apx_program_encode_instruction(APX_VM_OPCODE_FLOW_CTRL, APX_VM_VARIANT_ARRAY_NEXT, false);
   adt_error_t rc = adt_bytearray_push(self->program, instruction);
   return convert_from_adt_to_apx_error(rc);
}