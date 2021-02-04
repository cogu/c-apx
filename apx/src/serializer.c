/*****************************************************************************
* \file      serializer.c
* \author    Conny Gustafsson
* \date      2021-01-03
* \brief     APX port data serializer
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
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "apx/serializer.h"
#include "apx/vm_defs.h"
#include "apx/vm_common.h"
#include "apx/program.h"
#include "apx/util.h"
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
//apx_vm_writeBuffer_t API
static void write_buffer_init(apx_vm_writeBuffer_t* self);
static void write_buffer_reset(apx_vm_writeBuffer_t* self, uint8_t* data, size_t size);
static bool write_buffer_is_valid(apx_vm_writeBuffer_t* self);

//apx_vm_writeState_t API
static void state_reset(apx_vm_writeState_t* self, dtl_dv_type_id type_id);
static void state_clear_value(apx_vm_writeState_t* self);
static void state_set_value(apx_vm_writeState_t* self, dtl_dv_t const* dv);
static apx_error_t state_determine_array_length_from_value(apx_vm_writeState_t* self);
static bool state_is_num_or_bool_type(apx_vm_writeState_t* self);
static bool state_is_record_type(apx_vm_writeState_t* self);
static bool state_is_char_type(apx_vm_writeState_t* self);
static bool state_is_bytes_type(apx_vm_writeState_t* self);
static apx_error_t state_default_range_check_value(apx_vm_writeState_t* self);
static apx_error_t state_read_scalar_value(apx_vm_writeState_t* self, apx_typeCode_t type_code);
static apx_error_t state_read_scalar_array_value(apx_vm_writeState_t* self, int32_t index, apx_typeCode_t type_code);
static apx_error_t state_read_scalar_value_internal(apx_vm_writeState_t* self, dtl_sv_t const* sv, apx_typeCode_t type_code);
static apx_error_t state_default_range_check_scalar(apx_vm_writeState_t* self);
static dtl_dv_t* state_get_child_value(apx_vm_writeState_t* self, char const* key);
static apx_error_t state_set_field_name(apx_vm_writeState_t* self, char const* name, bool is_last_field);

//apx_vm_queuedWriteState_t API
static void queued_write_state_init(apx_vm_queuedWriteState_t* self);
static bool queued_write_state_is_active(apx_vm_queuedWriteState_t* self);

//apx_vm_serializer_t API
static apx_error_t serializer_prepare_for_buffer_write(apx_vm_serializer_t* self, apx_typeCode_t type_code, uint32_t element_size);
static apx_error_t serializer_prepare_for_array(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
static apx_error_t serializer_pack_value(apx_vm_serializer_t* self);
static apx_error_t serializer_write_dynamic_value_to_buffer(apx_vm_serializer_t* self, uint32_t value, uint32_t value_size);
static apx_error_t serializer_pack_scalar_value(apx_vm_serializer_t* self);
static apx_error_t serializer_pack_record_value(apx_vm_serializer_t* self);
static apx_error_t serializer_pack_array_of_scalar(apx_vm_serializer_t* self);
static apx_error_t serializer_pack_string(apx_vm_serializer_t* self);
static apx_error_t serializer_pack_byte_array_internal(apx_vm_serializer_t* self);
static void serializer_pop_state(apx_vm_serializer_t* self);
static apx_error_t serializer_enter_new_child_state(apx_vm_serializer_t* self);
static apx_error_t serializer_pack_scalar_value_internal(apx_vm_serializer_t* self);
static apx_error_t serializer_pack_char_string(apx_vm_serializer_t* self, adt_str_t const* str, uint32_t max_target_size);


//Internal API
static apx_error_t write_dynamic_value_to_buffer(uint8_t* begin, uint8_t* end, uint32_t value, uint32_t value_size);
static apx_error_t read_dynamic_value_from_buffer(uint8_t const* begin, uint8_t const* end, uint32_t* value, uint32_t value_size);


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_vm_writeState_create(apx_vm_writeState_t* self)
{
   if (self != NULL)
   {
      self->parent = 0u;
      adt_str_create(&self->field_name);
      self->index = 0u;
      self->array_len = 0u;
      self->max_array_len = 0u;
      self->element_size = 0u;
      self->value_type = DTL_DV_NULL;
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_NONE;
      self->type_code = APX_TYPE_CODE_NONE;
      self->is_last_field = false;
      self->dynamic_size_type = APX_SIZE_TYPE_NONE;
      self->range_check_state = APX_RANGE_CHECK_STATE_NOT_CHECKED;
      self->scalar_value.i32 = 0u;
   }
}

void apx_vm_writeState_destroy(apx_vm_writeState_t* self)
{
   if (self != NULL)
   {
      adt_str_destroy(&self->field_name);
   }
}

apx_vm_writeState_t* apx_vm_writeState_new(void)
{
   apx_vm_writeState_t* self = (apx_vm_writeState_t*)malloc(sizeof(apx_vm_writeState_t));
   if (self != NULL)
   {
      apx_vm_writeState_create(self);
   }
   return self;
}

void apx_vm_writeState_delete(apx_vm_writeState_t* self)
{
   if (self != NULL)
   {
      apx_vm_writeState_destroy(self);
      free(self);
   }
}

void apx_vm_writeState_vdelete(void* arg)
{
   apx_vm_writeState_delete((apx_vm_writeState_t*)arg);
}


//apx_vm_serializer_t
apx_error_t apx_vm_serializer_create(apx_vm_serializer_t* self)
{
   if (self != NULL)
   {
      adt_stack_create(&self->stack, apx_vm_writeState_vdelete);
      self->state = apx_vm_writeState_new();
      if (self->state == NULL)
      {
         return APX_MEM_ERROR;
      }
      write_buffer_init(&self->buffer);
      queued_write_state_init(&self->queued_write_state);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_vm_serializer_destroy(apx_vm_serializer_t* self)
{
   if (self != NULL)
   {
      adt_stack_destroy(&self->stack);
      if (self->state != 0)
      {
         apx_vm_writeState_delete(self->state);
      }
   }
}

apx_vm_serializer_t* apx_vm_serializer_new(void)
{
   apx_vm_serializer_t* self = (apx_vm_serializer_t*)malloc(sizeof(apx_vm_serializer_t));
   if (self != NULL)
   {
      apx_error_t rc = apx_vm_serializer_create(self);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_vm_serializer_delete(apx_vm_serializer_t* self)
{
   if (self != NULL)
   {
      apx_vm_serializer_destroy(self);
      free(self);
   }
}

void apx_vm_serializer_reset(apx_vm_serializer_t* self)
{
   if (self != NULL)
   {
      while (adt_stack_size(&self->stack) > 0)
      {
         assert(self->state != NULL);
         apx_vm_writeState_delete(self->state);
         self->state = adt_stack_top(&self->stack);
         adt_stack_pop(&self->stack);
      }
      state_clear_value(self->state);
   }
}

apx_error_t apx_vm_serializer_set_write_buffer(apx_vm_serializer_t* self, uint8_t* data, size_t size)
{
   if ( (self != NULL) && (data != NULL) && (size > 0))
   {
      write_buffer_reset(&self->buffer, data, size);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

size_t apx_vm_serializer_bytes_written(apx_vm_serializer_t* self)
{
   if (self != NULL)
   {
      return self->buffer.next - self->buffer.begin;
   }
   return 0u;
}

apx_error_t apx_vm_serializer_set_value_dv(apx_vm_serializer_t* self, dtl_dv_t const* dv)
{
   if (self != NULL)
   {
      state_set_value(self->state, dv);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_set_value_sv(apx_vm_serializer_t* self, dtl_sv_t const* sv)
{
   return apx_vm_serializer_set_value_dv(self, (dtl_dv_t const*)sv);
}

apx_error_t apx_vm_serializer_set_value_av(apx_vm_serializer_t* self, dtl_av_t const* av)
{
   return apx_vm_serializer_set_value_dv(self, (dtl_dv_t const*)av);
}

apx_error_t apx_vm_serializer_set_value_hv(apx_vm_serializer_t* self, dtl_hv_t const* hv)
{
   return apx_vm_serializer_set_value_dv(self, (dtl_dv_t const*)hv);
}

apx_error_t apx_vm_serializer_pack_uint8(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_UINT8, UINT8_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_uint16(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_UINT16, UINT16_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_uint32(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_UINT32, UINT32_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_uint64(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_UINT64, UINT64_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_int8(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_INT8, INT8_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_int16(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_INT16, INT16_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_int32(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_INT32, INT32_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_int64(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_INT64, INT64_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


apx_error_t apx_vm_serializer_pack_char(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_CHAR, CHAR_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_char8(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_CHAR8, CHAR_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_bool(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_BOOL, BOOL_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_byte(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if ( self != NULL)
   {
      if (array_length == 0u)
      {
         array_length = 1u; //Byte arrays must have at least array length 1
      }
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_BYTE, BYTE_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_pack_record(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      apx_error_t result = serializer_prepare_for_buffer_write(self, APX_TYPE_CODE_RECORD, 0u);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      assert(self->state != NULL);
      result = serializer_prepare_for_array(self, array_length, dynamic_size_type);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return serializer_pack_value(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_record_select(apx_vm_serializer_t* self, char const* key, bool is_last_field)
{

   if ( (self != NULL) && (key != NULL) )
   {
      if (self->state->value_type == DTL_DV_HASH)
      {
         dtl_dv_t* child_value = state_get_child_value(self->state, key);
         if (child_value == NULL)
         {
            return APX_NOT_FOUND_ERROR;
         }
         state_set_field_name(self->state, key, is_last_field);
         serializer_enter_new_child_state(self);
         state_set_value(self->state, child_value); //m_state on this line is the newly entered child_state
         return APX_NO_ERROR;
      }
      return APX_VALUE_TYPE_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_check_value_range_int32(apx_vm_serializer_t* self, int32_t lower_limit, int32_t upper_limit)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->state->value_type == DTL_DV_SCALAR)
      {
         retval = state_read_scalar_value(self->state, APX_TYPE_CODE_INT32);
         if (retval == APX_NO_ERROR)
         {
            assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT32);
            retval = apx_vm_value_in_range_i32(self->state->scalar_value.i32, lower_limit, upper_limit);
            if (retval == APX_NO_ERROR)
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_OK;
            }
            else
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
            }
         }
         else
         {
            self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
         }
      }
      else if (self->state->value_type == DTL_DV_ARRAY)
      {
         uint32_t i;
         uint32_t length = (uint32_t)dtl_av_length(self->state->value.av);
         self->state->range_check_state = APX_RANGE_CHECK_STATE_OK;
         for (i = 0u; i < length; i++)
         {
            retval = state_read_scalar_array_value(self->state, i, APX_TYPE_CODE_INT32);
            if (retval == APX_NO_ERROR)
            {
               assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT32);
               retval = apx_vm_value_in_range_i32(self->state->scalar_value.i32, lower_limit, upper_limit);
               if (retval != APX_NO_ERROR)
               {
                  self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
                  break;
               }
            }
            else
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
               break;
            }
         }
      }
      else
      {
         retval = APX_UNSUPPORTED_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_check_value_range_uint32(apx_vm_serializer_t* self, uint32_t lower_limit, uint32_t upper_limit)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->state->value_type == DTL_DV_SCALAR)
      {
         retval = state_read_scalar_value(self->state, APX_TYPE_CODE_UINT32);
         if (retval == APX_NO_ERROR)
         {
            assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT32);
            retval = apx_vm_value_in_range_u32(self->state->scalar_value.u32, lower_limit, upper_limit);
            if (retval == APX_NO_ERROR)
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_OK;
            }
            else
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
            }
         }
         else
         {
            self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
         }
      }
      else if (self->state->value_type == DTL_DV_ARRAY)
      {
         uint32_t i;
         uint32_t length = (uint32_t) dtl_av_length(self->state->value.av);
         self->state->range_check_state = APX_RANGE_CHECK_STATE_OK;
         for (i = 0u; i < length; i++)
         {
            retval = state_read_scalar_array_value(self->state, i, APX_TYPE_CODE_UINT32);
            if (retval == APX_NO_ERROR)
            {
               assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT32);
               retval = apx_vm_value_in_range_u32(self->state->scalar_value.u32, lower_limit, upper_limit);
               if (retval != APX_NO_ERROR)
               {
                  self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
                  break;
               }
            }
            else
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
               break;
            }
         }
      }
      else
      {
         retval = APX_UNSUPPORTED_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_check_value_range_int64(apx_vm_serializer_t* self, int64_t lower_limit, int64_t upper_limit)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->state->value_type == DTL_DV_SCALAR)
      {
         retval = state_read_scalar_value(self->state, APX_TYPE_CODE_INT64);
         if (retval == APX_NO_ERROR)
         {
            assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT64);
            retval = apx_vm_value_in_range_i64(self->state->scalar_value.i64, lower_limit, upper_limit);
            if (retval == APX_NO_ERROR)
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_OK;
            }
            else
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
            }
         }
         else
         {
            self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
         }
      }
      else if (self->state->value_type == DTL_DV_ARRAY)
      {
         uint32_t i;
         uint32_t length = (uint32_t)dtl_av_length(self->state->value.av);
         self->state->range_check_state = APX_RANGE_CHECK_STATE_OK;
         for (i = 0u; i < length; i++)
         {
            retval = state_read_scalar_array_value(self->state, i, APX_TYPE_CODE_INT64);
            if (retval == APX_NO_ERROR)
            {
               assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT64);
               retval = apx_vm_value_in_range_i64(self->state->scalar_value.i64, lower_limit, upper_limit);
               if (retval != APX_NO_ERROR)
               {
                  self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
                  break;
               }
            }
            else
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
               break;
            }
         }
      }
      else
      {
         retval = APX_UNSUPPORTED_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_check_value_range_uint64(apx_vm_serializer_t* self, uint64_t lower_limit, uint64_t upper_limit)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->state->value_type == DTL_DV_SCALAR)
      {
         retval = state_read_scalar_value(self->state, APX_TYPE_CODE_UINT64);
         if (retval == APX_NO_ERROR)
         {
            assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT64);
            retval = apx_vm_value_in_range_u64(self->state->scalar_value.u64, lower_limit, upper_limit);
            if (retval == APX_NO_ERROR)
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_OK;
            }
            else
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
            }
         }
         else
         {
            self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
         }
      }
      else if (self->state->value_type == DTL_DV_ARRAY)
      {
         uint32_t i;
         uint32_t length = (uint32_t)dtl_av_length(self->state->value.av);
         self->state->range_check_state = APX_RANGE_CHECK_STATE_OK;
         for (i = 0u; i < length; i++)
         {
            retval = state_read_scalar_array_value(self->state, i, APX_TYPE_CODE_UINT64);
            if (retval == APX_NO_ERROR)
            {
               assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT64);
               retval = apx_vm_value_in_range_u64(self->state->scalar_value.u64, lower_limit, upper_limit);
               if (retval != APX_NO_ERROR)
               {
                  self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
                  break;
               }
            }
            else
            {
               self->state->range_check_state = APX_RANGE_CHECK_STATE_FAIL;
               break;
            }
         }
      }
      else
      {
         retval = APX_UNSUPPORTED_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_queued_write_begin(apx_vm_serializer_t* self, uint32_t element_size, uint32_t max_length, bool clear_queue)
{
   if (self != NULL)
   {
      self->queued_write_state.element_size = element_size;
      self->queued_write_state.length_ptr = self->buffer.next;
      self->queued_write_state.max_length = max_length;
      self->queued_write_state.is_active = true;
      self->queued_write_state.size_type = apx_vm_size_to_size_type(max_length);
      uint32_t const value_size = apx_vm_size_type_to_size(self->queued_write_state.size_type);
      if (clear_queue)
      {
         self->queued_write_state.current_length = 0u;
      }
      else
      {
         uint32_t tmp;
         apx_error_t result = read_dynamic_value_from_buffer(self->queued_write_state.length_ptr,
            self->queued_write_state.length_ptr + value_size, &tmp, value_size);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
         self->queued_write_state.current_length = tmp;
      }
      self->buffer.next += value_size;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_queued_write_end(apx_vm_serializer_t* self)
{
   if (self != NULL)
   {
      if (!self->queued_write_state.is_active)
      {
         return APX_INTERNAL_ERROR;
      }
      self->queued_write_state.is_active = false;
      uint32_t const value_size = apx_vm_size_type_to_size(self->queued_write_state.size_type);
      apx_error_t retval = write_dynamic_value_to_buffer(self->queued_write_state.length_ptr,
         self->queued_write_state.length_ptr + value_size, self->queued_write_state.current_length, value_size);
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_serializer_array_next(apx_vm_serializer_t* self, bool* is_last)
{
   if ((self != NULL) && (is_last != NULL))
   {
      *is_last = false;
      if (self->state->value_type == DTL_DV_ARRAY)
      {
         if (self->state->array_len > 0)
         {
            if (++self->state->index == self->state->array_len)
            {
               *is_last = true;
            }
            else
            {
               if (self->state->type_code == APX_TYPE_CODE_RECORD)
               {
                  dtl_dv_t* child_value = dtl_av_value(self->state->value.av, (int32_t) self->state->index);
                  if (child_value == NULL)
                  {
                     return APX_NULL_PTR_ERROR;
                  }
                  serializer_enter_new_child_state(self);
                  state_set_value(self->state, child_value); //m_state on this line is the newly entered child_state
               }
               else
               {
                  return APX_NOT_IMPLEMENTED_ERROR;
               }
            }
         }
         else
         {
            return APX_INTERNAL_ERROR;
         }
      }
      else
      {
         return APX_VALUE_TYPE_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void write_buffer_init(apx_vm_writeBuffer_t* self)
{
   if (self != NULL)
   {
      memset(self, 0, sizeof(apx_vm_writeBuffer_t));
   }
}

static void write_buffer_reset(apx_vm_writeBuffer_t* self, uint8_t* data, size_t size)
{
   assert(self != NULL);
   self->begin = self->next = data;
   self->end = data + size;
   self->padded_next = NULL;
}

static bool write_buffer_is_valid(apx_vm_writeBuffer_t* self)
{
   assert(self != NULL);
   return ((self->next != NULL) && (self->end != NULL) && (self->next <= self->end));
}

//apx_vm_writeState_t API
static void state_clear_value(apx_vm_writeState_t* self)
{
   assert(self != NULL);
   self->value_type = DTL_DV_NULL;
   self->value.dv = NULL;
}

static void state_reset(apx_vm_writeState_t* self, dtl_dv_type_id type_id)
{
   assert(self != NULL);
   self->value_type = type_id;
   adt_str_clear(&self->field_name);
   self->index = 0u;
   self->array_len = 0u;
   self->max_array_len = 0u;
   self->element_size = 0u;
   self->is_last_field = false;
   self->type_code = APX_TYPE_CODE_NONE;
   self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_NONE;
   self->range_check_state = APX_RANGE_CHECK_STATE_NOT_CHECKED;
   self->dynamic_size_type = APX_SIZE_TYPE_NONE;
   self->scalar_value.i32 = 0u;
}

static void state_set_value(apx_vm_writeState_t* self, dtl_dv_t const* dv)
{
   assert(self != NULL);
   state_reset(self, dtl_dv_type(dv));
   switch (self->value_type)
   {
   case DTL_DV_NULL:
      self->value.dv = NULL;
      break;
   case DTL_DV_SCALAR:
      self->value.sv = (dtl_sv_t*)dv;
      break;
   case DTL_DV_ARRAY:
      self->value.av = (dtl_av_t*)dv;
      break;
   case DTL_DV_HASH:
      self->value.hv = (dtl_hv_t*)dv;
      break;
   }
}

static apx_error_t state_determine_array_length_from_value(apx_vm_writeState_t* self)
{
   assert(self != NULL);
   dtl_sv_type_id sv_type;
   switch (self->value_type)
   {
   case DTL_DV_SCALAR:
      sv_type = dtl_sv_type(self->value.sv);
      if (sv_type == DTL_SV_STR)
      {
         bool ok = false;
         adt_str_t* tmp = dtl_sv_to_str(self->value.sv, &ok);
         if (ok)
         {
            assert(tmp != NULL);
            self->array_len = adt_str_size(tmp);
            adt_str_delete(tmp);
         }
         else
         {
            return APX_VALUE_CONVERSION_ERROR;
         }
      }
      else if (sv_type == DTL_SV_BYTEARRAY)
      {
         adt_bytearray_t const* tmp = dtl_sv_get_bytearray(self->value.sv);
         self->array_len = adt_bytearray_length(tmp);
      }
      break;
   case DTL_DV_ARRAY:
      self->array_len = dtl_av_length(self->value.av);
      break;
   case DTL_DV_HASH: //NOT ARRAY-COMPATIBLE
   case DTL_DV_NULL:     //NOT ARRAY-COMPATIBLE
   default:
      return APX_VALUE_TYPE_ERROR;
   }
   return APX_NO_ERROR;
}

static bool state_is_num_or_bool_type(apx_vm_writeState_t* self)
{
   assert(self != NULL);
   return ((self->type_code >= APX_TYPE_CODE_UINT8) && (self->type_code <= APX_TYPE_CODE_INT64)) ||
       (self->type_code == APX_TYPE_CODE_BOOL);
}

static bool state_is_record_type(apx_vm_writeState_t* self)
{
   assert(self != NULL);
   return (self->type_code == APX_TYPE_CODE_RECORD);
}

static bool state_is_char_type(apx_vm_writeState_t* self)
{
   assert(self != NULL);
   return (self->type_code == APX_TYPE_CODE_CHAR) ||
      (self->type_code == APX_TYPE_CODE_CHAR8) ||
      (self->type_code == APX_TYPE_CODE_CHAR16) ||
      (self->type_code == APX_TYPE_CODE_CHAR32);
}

static bool state_is_bytes_type(apx_vm_writeState_t* self)
{
   assert(self != NULL);
   return (self->type_code == APX_TYPE_CODE_BYTE);
}

static apx_error_t state_default_range_check_value(apx_vm_writeState_t* self)
{
   assert(self != NULL);
   apx_error_t retval = APX_NO_ERROR;
   if (self->value_type == DTL_DV_SCALAR)
   {
      retval = state_read_scalar_value(self, self->type_code);
      if (retval == APX_NO_ERROR)
      {
         retval = state_default_range_check_scalar(self);
      }
   }
   else if (self->value_type == DTL_DV_ARRAY)
   {
      int32_t i;
      int32_t len = dtl_av_length(self->value.av);
      for (i = 0u; i < len; i++)
      {
         retval = state_read_scalar_array_value(self, i, self->type_code);
         if (retval == APX_NO_ERROR)
         {
            retval = state_default_range_check_scalar(self);
            if (retval != APX_NO_ERROR)
            {
               break;
            }
         }
      }
   }
   else
   {
      retval = APX_UNSUPPORTED_ERROR;
   }
   return retval;
}

static apx_error_t state_read_scalar_value(apx_vm_writeState_t* self, apx_typeCode_t type_code)
{
   assert(self != NULL);
   if (self->value_type != DTL_DV_SCALAR)
   {
      return APX_VALUE_TYPE_ERROR;
   }
   return state_read_scalar_value_internal(self, self->value.sv, type_code);
}

static apx_error_t state_read_scalar_array_value(apx_vm_writeState_t* self, int32_t index, apx_typeCode_t type_code)
{
   if (self->value_type != DTL_DV_ARRAY)
   {
      return APX_VALUE_TYPE_ERROR;
   }
   dtl_dv_t *dv = dtl_av_value(self->value.av, index);
   if (dtl_dv_type(dv) != DTL_DV_SCALAR)
   {
      return APX_VALUE_TYPE_ERROR;
   }
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   if (sv == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   return state_read_scalar_value_internal(self, sv, type_code);
}

static apx_error_t state_read_scalar_value_internal(apx_vm_writeState_t* self, dtl_sv_t const* sv, apx_typeCode_t type_code)
{
   apx_error_t retval = APX_NO_ERROR;
   bool ok = false;
   assert(sv != 0);
   switch (type_code)
   {
   case APX_TYPE_CODE_UINT8:
   case APX_TYPE_CODE_UINT16:
   case APX_TYPE_CODE_UINT32:
      self->scalar_value.u32 = dtl_sv_to_u32(sv, &ok);
      if (ok)
      {
         self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT32;
      }
      else
      {
         retval = APX_VALUE_CONVERSION_ERROR;
      }
      break;
   case APX_TYPE_CODE_UINT64:
      self->scalar_value.u64 = dtl_sv_to_u64(sv, &ok);
      if (ok)
      {
         self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT64;
      }
      else
      {
         retval = APX_VALUE_CONVERSION_ERROR;
      }
      break;
   case APX_TYPE_CODE_INT8:
   case APX_TYPE_CODE_INT16:
   case APX_TYPE_CODE_INT32:
      self->scalar_value.i32 = dtl_sv_to_i32(sv, &ok);
      if (ok)
      {
         self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT32;
      }
      else
      {
         retval = APX_VALUE_CONVERSION_ERROR;
      }
      break;
   case APX_TYPE_CODE_INT64:
      self->scalar_value.i64 = dtl_sv_to_i64(sv, &ok);
      if (ok)
      {
         self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT64;
      }
      else
      {
         retval = APX_VALUE_CONVERSION_ERROR;
      }
      break;
   case APX_TYPE_CODE_BOOL:
      self->scalar_value.i64 = dtl_sv_to_bool(sv, &ok);
      if (ok)
      {
         self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_BOOL;
      }
      else
      {
         retval = APX_VALUE_CONVERSION_ERROR;
      }
      break;
   default:
      retval = APX_UNSUPPORTED_ERROR;
   }
   return retval;
}

static apx_error_t state_default_range_check_scalar(apx_vm_writeState_t* self)
{
   assert(self != NULL);
   if (self->type_code == APX_TYPE_CODE_BOOL)
   {
      return APX_NO_ERROR; //A Bool can fit into any data type, no need to check.
   }
   uint32_t u32_lower_limit = 0u;
   uint32_t u32_upper_limit = 0u;
   int32_t i32_lower_limit = 0;
   int32_t i32_upper_limit = 0;
   uint64_t u64_lower_limit = 0u;
   uint64_t u64_upper_limit = 0u;
   int64_t i64_lower_limit = 0;
   int64_t i64_upper_limit = 0;
   apx_error_t retval = APX_NO_ERROR;
   scalar_storage_type_t expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_NONE;

   if (retval == APX_NO_ERROR)
   {
      switch (self->type_code)
      {
      case APX_TYPE_CODE_UINT8:
         u32_lower_limit = 0u;
         u32_upper_limit = UINT8_MAX;
         expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT32;
         break;
      case APX_TYPE_CODE_UINT16:
         u32_lower_limit = 0u;
         u32_upper_limit = UINT16_MAX;
         expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT32;
         break;
      case APX_TYPE_CODE_UINT32:
         u32_lower_limit = 0u;
         u32_upper_limit = UINT32_MAX;
         expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT32;
         break;
      case APX_TYPE_CODE_UINT64:
         u64_lower_limit = 0u;
         u64_upper_limit = UINT64_MAX;
         expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT64;
         break;
      case APX_TYPE_CODE_INT8:
         i32_lower_limit = INT8_MIN;
         i32_upper_limit = INT8_MAX;
         expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT32;
         break;
      case APX_TYPE_CODE_INT16:
         i32_lower_limit = INT16_MIN;
         i32_upper_limit = INT16_MAX;
         expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT32;
         break;
      case APX_TYPE_CODE_INT32:
         i32_lower_limit = INT32_MIN;
         i32_upper_limit = INT32_MAX;
         expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT32;
         break;
      case APX_TYPE_CODE_INT64:
         i64_lower_limit = INT64_MIN;
         i64_upper_limit = INT64_MAX;
         expected_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT64;
         break;
      default:
         retval = APX_UNSUPPORTED_ERROR;
      }
      if ((retval == APX_NO_ERROR) && (self->scalar_storage_type == expected_storage_type))
      {
         switch (expected_storage_type)
         {
         case APX_VM_SCALAR_STORAGE_TYPE_INT32:
            retval = apx_vm_value_in_range_i32(self->scalar_value.i32, i32_lower_limit, i32_upper_limit);
            break;
         case APX_VM_SCALAR_STORAGE_TYPE_UINT32:
            retval = apx_vm_value_in_range_u32(self->scalar_value.u32, u32_lower_limit, u32_upper_limit);
            break;
         case APX_VM_SCALAR_STORAGE_TYPE_INT64:
            retval = apx_vm_value_in_range_i64(self->scalar_value.i64, i64_lower_limit, i64_upper_limit);
            break;
         case APX_VM_SCALAR_STORAGE_TYPE_UINT64:
            retval = apx_vm_value_in_range_u64(self->scalar_value.u64, u64_lower_limit, u64_upper_limit);
            break;
         default:
            retval = APX_UNSUPPORTED_ERROR;
         }
      }
      else
      {
         retval = APX_VALUE_TYPE_ERROR;
      }
   }
   return retval;
}

static dtl_dv_t* state_get_child_value(apx_vm_writeState_t* self, char const* key)
{
   assert(self != NULL);
   if (self->value_type == DTL_DV_HASH)
   {
      return dtl_hv_get_cstr(self->value.hv, key);
   }
   return NULL;
}

static apx_error_t state_set_field_name(apx_vm_writeState_t* self, char const* name, bool is_last_field)
{
   assert( self != NULL);
   if (name == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   self->is_last_field = is_last_field;
   return convert_from_adt_to_apx_error(adt_str_set_cstr(&self->field_name, name));
}


//apx_vm_queuedWriteState_t API
static void queued_write_state_init(apx_vm_queuedWriteState_t* self)
{
   if (self != NULL)
   {
      self->max_length = 0u;
      self->current_length = 0u;
      self->element_size = 0u;
      self->length_ptr = NULL;
      self->size_type = APX_SIZE_TYPE_NONE;
      self->is_active = false;
   }
}

static bool queued_write_state_is_active(apx_vm_queuedWriteState_t* self)
{
   assert(self != NULL);
   return self->is_active;
}

//apx_vm_serializer_t API

static apx_error_t serializer_prepare_for_buffer_write(apx_vm_serializer_t* self, apx_typeCode_t type_code, uint32_t element_size)
{
   assert(self != NULL);
   if (!write_buffer_is_valid(&self->buffer))
   {
      return APX_MISSING_BUFFER_ERROR;
   }
   if (self->buffer.padded_next != NULL)
   {
      if ((self->buffer.padded_next < self->buffer.begin) || (self->buffer.padded_next > self->buffer.end))
      {
         return APX_BUFFER_BOUNDARY_ERROR;
      }
      self->buffer.next = self->buffer.padded_next;
      self->buffer.padded_next = NULL;
   }
   self->state->type_code = type_code;
   self->state->element_size = element_size;
   return APX_NO_ERROR;
}

static apx_error_t serializer_prepare_for_array(apx_vm_serializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   assert(self != NULL);
   if (array_length > 0)
   {
      if (dynamic_size_type != APX_SIZE_TYPE_NONE)
      {
         apx_error_t result = APX_NO_ERROR;
         uint32_t length_size = apx_vm_size_type_to_size(dynamic_size_type);
         assert(length_size > 0u);
         self->state->max_array_len = array_length;
         result = state_determine_array_length_from_value(self->state);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
         //on success, the value self->state->array_len has been successfully updated.
         if (self->state->array_len > self->state->max_array_len)
         {
            return APX_VALUE_LENGTH_ERROR;
         }
         self->state->dynamic_size_type = dynamic_size_type;
         assert(self->state->element_size != 0);
         self->buffer.padded_next = self->buffer.next + length_size + (self->state->max_array_len * self->state->element_size);
         if (self->buffer.padded_next > self->buffer.end)
         {
            return APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      else
      {
         self->state->array_len = array_length;
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t serializer_pack_value(apx_vm_serializer_t* self)
{
   assert(self != NULL);
   apx_error_t retval = APX_NO_ERROR;
   assert(self->state != NULL);
   bool do_pop_state = true;
   if (queued_write_state_is_active(&self->queued_write_state))
   {
      if (self->queued_write_state.current_length >= self->queued_write_state.max_length)
      {
         return APX_QUEUE_FULL_ERROR;
      }
   }
   if (self->state->array_len == 0)
   {
      if (self->state->dynamic_size_type != APX_SIZE_TYPE_NONE)
      {
         //This is a zero-length array. Just write the array-size.
         retval = serializer_write_dynamic_value_to_buffer(self, self->state->array_len, apx_vm_size_type_to_size(self->state->dynamic_size_type));
      }
      else
      {
         if (state_is_num_or_bool_type(self->state))
         {
            retval = serializer_pack_scalar_value(self);
         }
         else if (state_is_record_type(self->state))
         {
            do_pop_state = false;
            retval = serializer_pack_record_value(self);
         }
         else
         {
            retval = APX_NOT_IMPLEMENTED_ERROR;
         }
      }
   }
   else
   {
      if (self->state->dynamic_size_type != APX_SIZE_TYPE_NONE)
      {
         retval = serializer_write_dynamic_value_to_buffer(self, self->state->array_len, apx_vm_size_type_to_size(self->state->dynamic_size_type));
      }
      if (retval == APX_NO_ERROR)
      {
         if (state_is_num_or_bool_type(self->state))
         {
            retval = serializer_pack_array_of_scalar(self);
         }
         else if (state_is_char_type(self->state))
         {
            retval = serializer_pack_string(self);
         }
         else if (state_is_bytes_type(self->state))
         {
            retval = serializer_pack_byte_array_internal(self);
         }
         else if (state_is_record_type(self->state))
         {
            do_pop_state = false;
            retval = serializer_pack_record_value(self);
         }
         else
         {
            retval = APX_NOT_IMPLEMENTED_ERROR;
         }
      }
   }
   if (retval == APX_NO_ERROR)
   {
      if (queued_write_state_is_active(&self->queued_write_state))
      {
         self->queued_write_state.current_length++;
      }
      if (do_pop_state)
      {
         serializer_pop_state(self);
      }
   }
   return retval;
}

static apx_error_t serializer_write_dynamic_value_to_buffer(apx_vm_serializer_t* self, uint32_t value, uint32_t value_size)
{
   assert(self != NULL);
   apx_error_t retval = write_dynamic_value_to_buffer(self->buffer.next, self->buffer.end, value, value_size);
   if (retval == APX_NO_ERROR)
   {
      self->buffer.next += value_size;
   }
   return retval;
}

static apx_error_t write_dynamic_value_to_buffer(uint8_t* begin, uint8_t* end, uint32_t value, uint32_t value_size)
{
   switch (value_size)
   {
   case UINT8_SIZE:
      if (value > (uint32_t) UINT8_MAX)
      {
         return APX_LENGTH_ERROR;
      }
      break;
   case UINT16_SIZE:
      if (value > (uint32_t)UINT16_MAX)
      {
         return APX_LENGTH_ERROR;
      }
      break;
   case UINT32_SIZE:
      //No check needed
      break;
   default:
      return APX_INTERNAL_ERROR;
   }
   if (begin + value_size <= end)
   {
      packLE(begin, value, (uint8_t)value_size);
   }
   else
   {
      return APX_BUFFER_FULL_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t read_dynamic_value_from_buffer(uint8_t const* begin, uint8_t const* end, uint32_t* value, uint32_t value_size)
{
   if ((begin != NULL) && (end != NULL) && (value != NULL) && (value_size <= UINT32_SIZE))
   {
      if (begin + value_size <= end)
      {
         *value = unpackLE(begin, (uint8_t)value_size);
      }
      else
      {
         return APX_BUFFER_BOUNDARY_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t serializer_pack_scalar_value(apx_vm_serializer_t* self)
{
   apx_error_t retval = APX_NO_ERROR;
   assert(self != NULL);
   if (self->state->value_type != DTL_DV_SCALAR)
   {
      retval = APX_VALUE_TYPE_ERROR;
   }
   else
   {
      assert(self->state->value.sv != NULL);
      if (self->state->range_check_state == APX_RANGE_CHECK_STATE_NOT_CHECKED)
      {
         retval = state_default_range_check_value(self->state);
      }
      else if (self->state->range_check_state == APX_RANGE_CHECK_STATE_FAIL)
      {
         retval = APX_VALUE_RANGE_ERROR;
      }
      if (retval == APX_NO_ERROR)
      {
         return serializer_pack_scalar_value_internal(self);
      }
   }
   return retval;
}

static apx_error_t serializer_pack_record_value(apx_vm_serializer_t* self)
{
   assert((self != NULL) && (self->state != NULL) );
   uint32_t const expected_array_len = self->state->array_len;
   if (expected_array_len > 0)
   {
      if (self->state->value_type == DTL_DV_ARRAY)
      {
         if (self->state->dynamic_size_type == APX_SIZE_TYPE_NONE)
         {
            //For non-dynamic arrays the length of the value must match exactly.
            uint32_t const value_array_length = (uint32_t) dtl_av_length(self->state->value.av);
            if (expected_array_len != value_array_length)
            {
               return APX_VALUE_LENGTH_ERROR;
            }
         }
         //select the first array value in a new child state.
         assert(self->state->index == 0u);
         dtl_dv_t const* child_value = dtl_av_value(self->state->value.av, (int32_t) self->state->index);

         if (child_value == NULL)
         {
            return APX_NULL_PTR_ERROR;
         }
         apx_error_t result = serializer_enter_new_child_state(self);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
         state_set_value(self->state, child_value); //self->state on this line is the newly entered child_state
      }
      else
      {
         return APX_VALUE_TYPE_ERROR;
      }
   }
   if (self->state->value_type != DTL_DV_HASH) //This check applies to both top-level state as well as (potentially newly entered) child-state
   {
      return APX_VALUE_TYPE_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t serializer_pack_array_of_scalar(apx_vm_serializer_t* self)
{
   if (self->state->value_type != DTL_DV_ARRAY)
   {
      return APX_VALUE_TYPE_ERROR;
   }
   assert(self->state->value.av != NULL);
   uint32_t const value_length = (uint32_t) dtl_av_length(self->state->value.av);
   if ((self->state->dynamic_size_type == APX_SIZE_TYPE_NONE) && (value_length != self->state->array_len))
   {
      return APX_VALUE_LENGTH_ERROR; //For non-dynamic arrays the array length of the value must match exactly.
   }
   if (self->state->range_check_state == APX_RANGE_CHECK_STATE_NOT_CHECKED)
   {
      apx_error_t result = state_default_range_check_value(self->state);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
   }
   else if (self->state->range_check_state == APX_RANGE_CHECK_STATE_FAIL)
   {
      return APX_VALUE_RANGE_ERROR;
   }
   uint32_t i;
   for (i = 0; i < value_length; i++)
   {
      apx_error_t result = state_read_scalar_array_value(self->state, i, self->state->type_code);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      result = serializer_pack_scalar_value_internal(self);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t serializer_pack_string(apx_vm_serializer_t* self)
{
   assert(self->state != NULL);
   if (self->state->value_type != DTL_DV_SCALAR)
   {
      return APX_VALUE_TYPE_ERROR;
   }

   bool ok = false;
   adt_str_t* value = dtl_sv_to_str(self->state->value.sv, &ok);
   if (!ok)
   {
      return APX_VALUE_CONVERSION_ERROR;
   }
   assert(value != NULL);
   uint32_t const target_string_size = self->state->element_size * self->state->array_len;
   apx_error_t retval =  APX_NO_ERROR;
   if ((self->buffer.next + target_string_size) <= self->buffer.end)
   {
      memset(self->buffer.next, 0, target_string_size);
      switch (self->state->type_code)
      {
      case APX_TYPE_CODE_CHAR:
         retval = serializer_pack_char_string(self, value, target_string_size);
         break;
      case APX_TYPE_CODE_CHAR8:
         ///For now, treat char and char8 the same way. In the future we need to separate the two types.
         retval = serializer_pack_char_string(self, value, target_string_size);
         break;
      default:
         retval = APX_NOT_IMPLEMENTED_ERROR;
      }
      if (retval == APX_NO_ERROR)
      {
         self->buffer.next += target_string_size;
      }
   }
   else
   {
      retval = APX_BUFFER_BOUNDARY_ERROR;
   }
   adt_str_delete(value);
   return retval;
}

static apx_error_t serializer_pack_byte_array_internal(apx_vm_serializer_t* self)
{
   assert(self->state != NULL);
   if ((self->state->value_type != DTL_DV_SCALAR) ||
      (dtl_sv_type(self->state->value.sv) != DTL_SV_BYTEARRAY))
   {
      return APX_VALUE_TYPE_ERROR;
   }
   uint32_t const array_len = self->state->array_len;
   adt_bytearray_t const* byte_array = dtl_sv_get_bytearray(self->state->value.sv);
   assert(byte_array != NULL);
   if ((self->state->dynamic_size_type == APX_SIZE_TYPE_NONE) && (array_len > 0u))
   {
      if (array_len != adt_bytearray_length(byte_array)) //For non-dynamic arrays the length of the value must match exactly.
      {
         return APX_VALUE_LENGTH_ERROR;
      }
   }
   apx_error_t retval = APX_NO_ERROR;
   if ((self->buffer.next + array_len) <= self->buffer.end)
   {
      memcpy(self->buffer.next, adt_bytearray_data(byte_array), array_len);
      self->buffer.next += array_len;
   }
   else
   {
      retval = APX_BUFFER_BOUNDARY_ERROR;
   }
   return retval;
}

static void serializer_pop_state(apx_vm_serializer_t* self)
{
   assert(self->state != NULL);
   while (adt_stack_size(&self->stack) > 0)
   {
      assert(self->state != NULL);
      apx_vm_writeState_delete(self->state);
      self->state = adt_stack_top(&self->stack);
      adt_stack_pop(&self->stack);
      if (!self->state->is_last_field)
      {
         break;
      }
   }
}

static apx_error_t serializer_enter_new_child_state(apx_vm_serializer_t* self)
{
   assert(self != NULL);
   apx_vm_writeState_t* child_state = apx_vm_writeState_new();
   if (child_state == NULL)
   {
      return APX_MEM_ERROR;
   }
   child_state->parent = self->state;
   adt_stack_push(&self->stack, self->state);
   self->state = child_state;
   return APX_NO_ERROR;
}

static apx_error_t serializer_pack_scalar_value_internal(apx_vm_serializer_t* self)
{
   assert(self != NULL);
   apx_error_t retval = APX_NO_ERROR;
   if ((self->buffer.next + self->state->element_size) <= self->buffer.end)
   {
      switch (self->state->type_code)
      {
      case APX_TYPE_CODE_UINT8:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT32);
         assert(self->state->element_size == UINT8_SIZE);
         packLE(self->buffer.next, self->state->scalar_value.u32, (uint8_t)self->state->element_size);
         break;
      case APX_TYPE_CODE_UINT16:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT32);
         assert(self->state->element_size == UINT16_SIZE);
         packLE(self->buffer.next, self->state->scalar_value.u32, (uint8_t)self->state->element_size);
         break;
      case APX_TYPE_CODE_UINT32:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT32);
         assert(self->state->element_size == UINT32_SIZE);
         packLE(self->buffer.next, self->state->scalar_value.u32, (uint8_t) self->state->element_size);
         break;
      case APX_TYPE_CODE_UINT64:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT64);
         assert(self->state->element_size == UINT64_SIZE);
         packLE64(self->buffer.next, self->state->scalar_value.u64, (uint8_t)self->state->element_size);
         break;
      case APX_TYPE_CODE_INT8:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT32);
         assert(self->state->element_size == INT8_SIZE);
         packLE(self->buffer.next, (uint32_t) self->state->scalar_value.i32, (uint8_t)self->state->element_size);
         break;
      case APX_TYPE_CODE_INT16:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT32);
         assert(self->state->element_size == INT16_SIZE);
         packLE(self->buffer.next, (uint32_t)self->state->scalar_value.i32, (uint8_t)self->state->element_size);
         break;
      case APX_TYPE_CODE_INT32:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT32);
         assert(self->state->element_size == INT32_SIZE);
         packLE(self->buffer.next, (uint32_t)self->state->scalar_value.i32, (uint8_t)self->state->element_size);
         break;
      case APX_TYPE_CODE_INT64:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT64);
         assert(self->state->element_size == INT64_SIZE);
         packLE64(self->buffer.next, (uint64_t)self->state->scalar_value.i64, (uint8_t)self->state->element_size);
         break;
      case APX_TYPE_CODE_BOOL:
         assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_BOOL);
         assert(self->state->element_size == BOOL_SIZE);
         packLE64(self->buffer.next, self->state->scalar_value.bl? 1u : 0u, (uint8_t)self->state->element_size);
         break;
      default:
         retval = APX_UNSUPPORTED_ERROR;
      }
      self->buffer.next += self->state->element_size;
   }
   else
   {
      retval = APX_BUFFER_BOUNDARY_ERROR;
   }
   return retval;

}

static apx_error_t serializer_pack_char_string(apx_vm_serializer_t* self, adt_str_t const* str, uint32_t max_target_size)
{
   assert(self != NULL);
   uint32_t const required_string_size = (uint32_t)adt_str_size(str);
   if (required_string_size > max_target_size)
   {
      return APX_BUFFER_BOUNDARY_ERROR;
   }
   memcpy(self->buffer.next, adt_str_data(str), required_string_size);
   return APX_NO_ERROR;
}
