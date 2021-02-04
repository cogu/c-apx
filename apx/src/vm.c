/*****************************************************************************
* \file      apx_vm.c
* \author    Conny Gustafsson
* \date      2019-02-24
* \brief     APX virtual machine (implements v2 of APX byte code language)
*
* Copyright (c) 2019 Conny Gustafsson
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
#include <assert.h>
#include <string.h>
#include "apx/vm.h"
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
static bool is_pack_prog(apx_vm_t* self);
static apx_error_t run_pack_program(apx_vm_t* self);
static apx_error_t run_unpack_program(apx_vm_t* self);
static apx_error_t run_pack_instruction(apx_vm_t* self);
static apx_error_t run_unpack_instruction(apx_vm_t* self);
static apx_error_t run_range_check_pack_int32(apx_vm_t* self);
static apx_error_t run_range_check_pack_uint32(apx_vm_t* self);
static apx_error_t run_range_check_pack_int64(apx_vm_t* self);
static apx_error_t run_range_check_pack_uint64(apx_vm_t* self);
static apx_error_t run_range_check_unpack_int32(apx_vm_t* self);
static apx_error_t run_range_check_unpack_uint32(apx_vm_t* self);
static apx_error_t run_range_check_unpack_int64(apx_vm_t* self);
static apx_error_t run_range_check_unpack_uint64(apx_vm_t* self);
static apx_error_t run_pack_record_select(apx_vm_t* self);
static apx_error_t run_unpack_record_select(apx_vm_t* self);
static apx_error_t run_array_next(apx_vm_t* self);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_vm_create(apx_vm_t *self)
{
   if (self != NULL)
   {
      apx_error_t result = apx_vm_serializer_create(&self->serializer);
      if (result == APX_NO_ERROR)
      {
         result = apx_vm_deserializer_create(&self->deserializer);
      }
      apx_vm_decoder_create(&self->decoder);
      memset(&self->program_header, 0, sizeof(self->program_header));
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_vm_destroy(apx_vm_t *self)
{
   if (self != NULL)
   {
      apx_vm_serializer_destroy(&self->serializer);
      apx_vm_deserializer_destroy(&self->deserializer);
      apx_vm_decoder_destroy(&self->decoder);
   }
}

apx_vm_t* apx_vm_new(void)
{
   apx_vm_t *self = (apx_vm_t*) malloc(sizeof(apx_vm_t));
   if (self != NULL)
   {
      apx_error_t result = apx_vm_create(self);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_vm_delete(apx_vm_t *self)
{
   if (self != NULL)
   {
      apx_vm_destroy(self);
      free(self);
   }
}



/**
 * Accepts a byte-code program by parsing the program header to see if its valid.
 * Returns APX_NO_ERROR on success
 */
apx_error_t apx_vm_select_program(apx_vm_t* self, apx_program_t const* program)
{
   if ( (self != NULL) && (program != NULL) )
   {
      uint8_t const* program_begin = adt_bytearray_data(program);
      uint32_t const program_size = adt_bytearray_length(program);
      apx_error_t result = apx_vm_decoder_select_program(&self->decoder,program_begin, program_size);
      if (result == APX_NO_ERROR)
      {
         result = apx_vm_decoder_parse_program_header(&self->decoder, &self->program_header);
      }
      return result;

   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_set_write_buffer(apx_vm_t* self, uint8_t* data, uint32_t size)
{
   if (self != 0)
   {
      return apx_vm_serializer_set_write_buffer(&self->serializer, data, size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_set_read_buffer(apx_vm_t* self, uint8_t const* data, uint32_t size)
{
   if (self != 0)
   {
      return apx_vm_deserializer_set_read_buffer(&self->deserializer, data, size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}



apx_error_t apx_vm_pack_value(apx_vm_t* self, dtl_dv_t const* dv)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->program_header.program_type != APX_PACK_PROGRAM)
      {
         return APX_INVALID_PROGRAM_ERROR;
      }
      retval = apx_vm_serializer_set_value_dv(&self->serializer, dv);
      if (retval == APX_NO_ERROR)
      {
         retval = run_pack_program(self);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_unpack_value(apx_vm_t* self, dtl_dv_t** dv)
{
   if ( (self != NULL) && (dv != NULL) )
   {
      if (self->program_header.program_type != APX_UNPACK_PROGRAM)
      {
         return APX_INVALID_PROGRAM_ERROR;
      }
      apx_error_t retval = run_unpack_program(self);
      if (retval == APX_NO_ERROR)
      {
         switch (apx_vm_deserializer_value_type(&self->deserializer))
         {
         case DTL_DV_SCALAR:
            *dv = (dtl_dv_t*) apx_vm_deserializer_take_sv(&self->deserializer);
            break;
         case DTL_DV_ARRAY:
            *dv = (dtl_dv_t*) apx_vm_deserializer_take_av(&self->deserializer);
            break;
         case DTL_DV_HASH:
            *dv = (dtl_dv_t*) apx_vm_deserializer_take_hv(&self->deserializer);
            break;
         default:
            retval = APX_VALUE_TYPE_ERROR;
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
size_t apx_vm_get_bytes_written(apx_vm_t* self)
{
   size_t retval = 0u;
   if (self != 0)
   {
      retval = apx_vm_serializer_bytes_written(&self->serializer);
   }
   return retval;
}

size_t apx_vm_get_bytes_read(apx_vm_t* self)
{
   size_t retval = 0u;
   if (self != 0)
   {
      retval = apx_vm_deserializer_bytes_read(&self->deserializer);
   }
   return retval;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static bool is_pack_prog(apx_vm_t* self)
{
   return self->program_header.program_type == APX_PACK_PROGRAM;
}

static apx_error_t run_pack_program(apx_vm_t* self)
{
   assert(self != NULL);
   apx_operationType_t operation_type = APX_OPERATION_TYPE_PROGRAM_END;
   do
   {
      apx_error_t result = apx_vm_decoder_parse_next_operation(&self->decoder, &operation_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      switch (operation_type)
      {
      case APX_OPERATION_TYPE_UNPACK:
         return APX_INVALID_INSTRUCTION_ERROR;
      case APX_OPERATION_TYPE_PACK:
         result = run_pack_instruction(self);
         break;
      case APX_OPERATION_TYPE_RANGE_CHECK_INT32:
         result = run_range_check_pack_int32(self);
         break;
      case APX_OPERATION_TYPE_RANGE_CHECK_UINT32:
         result = run_range_check_pack_uint32(self);
         break;
      case APX_OPERATION_TYPE_RANGE_CHECK_INT64:
         result = run_range_check_pack_int64(self);
         break;
      case APX_OPERATION_TYPE_RANGE_CHECK_UINT64:
         result = run_range_check_pack_uint64(self);
         break;
      case APX_OPERATION_TYPE_RECORD_SELECT:
         result = run_pack_record_select(self);
         break;
      case APX_OPERATION_TYPE_ARRAY_NEXT:
         result = run_array_next(self);
         break;
      case APX_OPERATION_TYPE_PROGRAM_END:
         break;
      }
      if (result != APX_NO_ERROR)
      {
         return result;
      }
   } while (operation_type != APX_OPERATION_TYPE_PROGRAM_END);
   return APX_NO_ERROR;
}

static apx_error_t run_unpack_program(apx_vm_t* self)
{
   assert(self != NULL);
   apx_operationType_t operation_type = APX_OPERATION_TYPE_PROGRAM_END;
   do
   {
      apx_error_t result = apx_vm_decoder_parse_next_operation(&self->decoder, &operation_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      switch (operation_type)
      {
      case APX_OPERATION_TYPE_UNPACK:
         result = run_unpack_instruction(self);
      case APX_OPERATION_TYPE_PACK:
         return APX_INVALID_INSTRUCTION_ERROR;
         break;
      case APX_OPERATION_TYPE_RANGE_CHECK_INT32:
         result = run_range_check_unpack_int32(self);
         break;
      case APX_OPERATION_TYPE_RANGE_CHECK_UINT32:
         result = run_range_check_unpack_uint32(self);
         break;
      case APX_OPERATION_TYPE_RANGE_CHECK_INT64:
         result = run_range_check_unpack_int64(self);
         break;
      case APX_OPERATION_TYPE_RANGE_CHECK_UINT64:
         result = run_range_check_unpack_uint64(self);
         break;
      case APX_OPERATION_TYPE_RECORD_SELECT:
         result = APX_NOT_IMPLEMENTED_ERROR;
         break;
      case APX_OPERATION_TYPE_ARRAY_NEXT:
         result = APX_NOT_IMPLEMENTED_ERROR;
         break;
      case APX_OPERATION_TYPE_PROGRAM_END:
         break;
      }
      if (result != APX_NO_ERROR)
      {
         return result;
      }
   } while (operation_type != APX_OPERATION_TYPE_PROGRAM_END);
   return APX_NO_ERROR;
}

static apx_error_t run_pack_instruction(apx_vm_t* self)
{
   apx_error_t retval = APX_NOT_IMPLEMENTED_ERROR;
   apx_packUnpackOperationInfo_t operation;
   apx_sizeType_t dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_decoder_get_pack_unpack_info(&self->decoder, &operation);
   if (operation.is_dynamic_array)
   {
      dynamic_size_type = apx_vm_size_to_size_type(operation.array_length);
   }
   switch (operation.type_code)
   {
   case APX_TYPE_CODE_UINT8:
      retval = apx_vm_serializer_pack_uint8(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_UINT16:
      retval = apx_vm_serializer_pack_uint16(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_UINT32:
      retval = apx_vm_serializer_pack_uint32(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_UINT64:
      retval = apx_vm_serializer_pack_uint64(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_INT8:
      retval = apx_vm_serializer_pack_int8(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_INT16:
      retval = apx_vm_serializer_pack_int16(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_INT32:
      retval = apx_vm_serializer_pack_int32(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_INT64:
      retval = apx_vm_serializer_pack_int64(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_CHAR:
      retval = apx_vm_serializer_pack_char(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_CHAR8:
      retval = apx_vm_serializer_pack_char8(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_BOOL:
      retval = apx_vm_serializer_pack_bool(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_BYTE:
      retval = apx_vm_serializer_pack_byte(&self->serializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_RECORD:
      retval = apx_vm_serializer_pack_record(&self->serializer, operation.array_length, dynamic_size_type);
      if (operation.array_length > 0u)
      {
         apx_vm_decoder_save_program_position(&self->decoder);
      }
      break;
   }
   return retval;
}

static apx_error_t run_unpack_instruction(apx_vm_t* self)
{
   apx_error_t retval = APX_NOT_IMPLEMENTED_ERROR;
   apx_packUnpackOperationInfo_t operation;
   apx_sizeType_t dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_decoder_get_pack_unpack_info(&self->decoder, &operation);
   if (operation.is_dynamic_array)
   {
      dynamic_size_type = apx_vm_size_to_size_type(operation.array_length);
   }
   switch (operation.type_code)
   {
   case APX_TYPE_CODE_UINT8:
      retval = apx_vm_deserializer_unpack_uint8(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_UINT16:
      retval = apx_vm_deserializer_unpack_uint16(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_UINT32:
      retval = apx_vm_deserializer_unpack_uint32(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_UINT64:
      retval = apx_vm_deserializer_unpack_uint64(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_INT8:
      retval = apx_vm_deserializer_unpack_int8(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_INT16:
      retval = apx_vm_deserializer_unpack_int16(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_INT32:
      retval = apx_vm_deserializer_unpack_int32(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_INT64:
      retval = apx_vm_deserializer_unpack_int64(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_CHAR:
      retval = apx_vm_deserializer_unpack_char(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_CHAR8:
      retval = apx_vm_deserializer_unpack_char8(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_BOOL:
      retval = apx_vm_deserializer_unpack_bool(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_BYTE:
      retval = apx_vm_deserializer_unpack_byte(&self->deserializer, operation.array_length, dynamic_size_type);
      break;
   case APX_TYPE_CODE_RECORD:
      retval = apx_vm_deserializer_unpack_record(&self->deserializer, operation.array_length, dynamic_size_type);
      if (operation.array_length > 0u)
      {
         apx_vm_decoder_save_program_position(&self->decoder);
      }
      break;
   }
   return retval;
}

static apx_error_t run_range_check_pack_int32(apx_vm_t* self)
{
   apx_rangeCheckInt32OperationInfo_t info = { 0,0 };
   apx_vm_decoder_range_check_info_int32(&self->decoder, &info);
   return apx_vm_serializer_check_value_range_int32(&self->serializer, info.lower_limit, info.upper_limit);
}

static apx_error_t run_range_check_pack_uint32(apx_vm_t* self)
{
   apx_rangeCheckUInt32OperationInfo_t info = { 0,0 };
   apx_vm_decoder_range_check_info_uint32(&self->decoder, &info);
   return apx_vm_serializer_check_value_range_uint32(&self->serializer, info.lower_limit, info.upper_limit);
}

static apx_error_t run_range_check_pack_int64(apx_vm_t* self)
{
   apx_rangeCheckInt64OperationInfo_t info = { 0,0 };
   apx_vm_decoder_range_check_info_int64(&self->decoder, &info);
   return apx_vm_serializer_check_value_range_int64(&self->serializer, info.lower_limit, info.upper_limit);
}

static apx_error_t run_range_check_pack_uint64(apx_vm_t* self)
{
   apx_rangeCheckUInt64OperationInfo_t info = { 0,0 };
   apx_vm_decoder_range_check_info_uint64(&self->decoder, &info);
   return apx_vm_serializer_check_value_range_uint64(&self->serializer, info.lower_limit, info.upper_limit);
}

static apx_error_t run_range_check_unpack_int32(apx_vm_t* self)
{
   apx_rangeCheckInt32OperationInfo_t info = { 0,0 };
   apx_vm_decoder_range_check_info_int32(&self->decoder, &info);
   return apx_vm_deserializer_check_value_range_int32(&self->deserializer, info.lower_limit, info.upper_limit);
}

static apx_error_t run_range_check_unpack_uint32(apx_vm_t* self)
{
   apx_rangeCheckUInt32OperationInfo_t info = { 0,0 };
   apx_vm_decoder_range_check_info_uint32(&self->decoder, &info);
   return apx_vm_deserializer_check_value_range_uint32(&self->deserializer, info.lower_limit, info.upper_limit);
}

static apx_error_t run_range_check_unpack_int64(apx_vm_t* self)
{
   apx_rangeCheckInt64OperationInfo_t info = { 0,0 };
   apx_vm_decoder_range_check_info_int64(&self->decoder, &info);
   return apx_vm_deserializer_check_value_range_int64(&self->deserializer, info.lower_limit, info.upper_limit);
}

static apx_error_t run_range_check_unpack_uint64(apx_vm_t* self)
{
   apx_rangeCheckUInt64OperationInfo_t info = { 0,0 };
   apx_vm_decoder_range_check_info_uint64(&self->decoder, &info);
   return apx_vm_deserializer_check_value_range_uint64(&self->deserializer, info.lower_limit, info.upper_limit);
}

static apx_error_t run_pack_record_select(apx_vm_t* self)
{
   char const* field_name = apx_vm_decoder_get_field_name(&self->decoder);
   bool is_last_field = apx_vm_decoder_is_last_field(&self->decoder);
   assert(field_name != NULL);
   return apx_vm_serializer_record_select(&self->serializer, field_name, is_last_field);
}

static apx_error_t run_unpack_record_select(apx_vm_t* self)
{
   char const* field_name = apx_vm_decoder_get_field_name(&self->decoder);
   bool is_last_field = apx_vm_decoder_is_last_field(&self->decoder);
   assert(field_name != NULL);
   return apx_vm_deserializer_record_select(&self->deserializer, field_name, is_last_field);

}

static apx_error_t run_array_next(apx_vm_t* self)
{
   bool is_last_index = false;
   apx_error_t result = APX_NO_ERROR;
   if (is_pack_prog(self))
   {
      result = apx_vm_serializer_array_next(&self->serializer, &is_last_index);
   }
   else
   {
      result = apx_vm_deserializer_array_next(&self->deserializer, &is_last_index);
   }
   if (result != APX_NO_ERROR)
   {
      return result;
   }
   if (!is_last_index)
   {
      if (apx_vm_decoder_has_saved_program_position(&self->decoder))
      {
         apx_vm_decoder_recall_program_position(&self->decoder);
      }
      else
      {
         return APX_INVALID_INSTRUCTION_ERROR;
      }
   }
   return APX_NO_ERROR;
}

