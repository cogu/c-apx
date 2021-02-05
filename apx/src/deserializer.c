/*****************************************************************************
* \file      deserializer.h
* \author    Conny Gustafsson
* \date      2021-01-08
* \brief     APX port data deserializer
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
#include "apx/deserializer.h"
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

//apx_vm_readState_t
static void state_create(apx_vm_readState_t* self);
static void state_destroy(apx_vm_readState_t* self);
static apx_vm_readState_t* state_new(void);
static void state_delete(apx_vm_readState_t* self);
static void state_vdelete(void* arg);
static void state_reset(apx_vm_readState_t* self);
static void state_clear_value(apx_vm_readState_t* self);
static void state_set_type_and_size(apx_vm_readState_t* self, apx_typeCode_t type_code, uint32_t element_size);
static bool state_is_num_or_bool_type(apx_vm_readState_t* self);
static bool state_is_record_type(apx_vm_readState_t* self);
static bool state_is_char_type(apx_vm_readState_t* self);
static bool state_is_byte_type(apx_vm_readState_t* self);
static apx_error_t state_init_scalar_value(apx_vm_readState_t* self);
static apx_error_t state_init_array_value(apx_vm_readState_t* self);
static apx_error_t state_init_hash_value(apx_vm_readState_t* self);
static apx_error_t state_read_scalar_value(apx_vm_readState_t* self, apx_vm_readBuffer_t* buffer);
static apx_error_t state_set_scalar_value_from_storage(apx_vm_readState_t* self);
static apx_error_t state_store_scalar_value(apx_vm_readState_t* self, dtl_sv_t* sv, apx_typeCode_t type_code);
static apx_error_t state_store_scalar_array_value(apx_vm_readState_t* self, int32_t index, apx_typeCode_t type_code);
static apx_error_t state_determine_array_length_from_read_buffer(apx_vm_readState_t* self, apx_vm_readBuffer_t* buffer);
static dtl_sv_t* state_take_sv(apx_vm_readState_t* self);
static apx_error_t state_push_array_value(apx_vm_readState_t* self, dtl_dv_t* dv);
static apx_error_t state_read_byte_array(apx_vm_readState_t* self, apx_vm_readBuffer_t* buffer);
static apx_error_t state_set_field_name(apx_vm_readState_t* self, char const* name, bool is_last_field);
static apx_error_t state_create_child_value_from_child_state(apx_vm_readState_t* self, apx_vm_readState_t* child_state);
static apx_error_t state_push_value_from_child_state(apx_vm_readState_t* self, apx_vm_readState_t* child_state);


//apx_vm_queuedWriteState_t API
static void queued_read_state_init(apx_vm_queuedReadState_t* self);
static bool queued_read_state_is_active(apx_vm_queuedReadState_t* self);

//apx_vm_readBuffer_t API
static void read_buffer_init(apx_vm_readBuffer_t* self);
static void read_buffer_reset(apx_vm_readBuffer_t* self, uint8_t const* data, size_t size);
static bool read_buffer_is_valid(apx_vm_readBuffer_t* self);

//apx_vm_deserializer_t API
static void deserializer_reset(apx_vm_deserializer_t* self);
static apx_error_t deserializer_prepare_for_buffer_read(apx_vm_deserializer_t* self, apx_typeCode_t type_code, uint32_t element_size);
apx_error_t deserializer_unpack_value(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
static apx_error_t deserializer_prepare_for_array(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
static apx_error_t deserializer_unpack_scalar_value(apx_vm_deserializer_t* self, dtl_sv_t *sv);
static apx_error_t deserializer_unpack_record_value(apx_vm_deserializer_t* self);
static apx_error_t deserializer_unpack_array_of_scalar(apx_vm_deserializer_t* self);
static apx_error_t deserializer_unpack_string(apx_vm_deserializer_t* self);
static apx_error_t deserializer_unpack_byte_array_internal(apx_vm_deserializer_t* self);
//static apx_error_t deserializer_unpack_scalar_value_internal(apx_vm_deserializer_t* self);
//static apx_error_t deserializer_unpack_char_string(apx_vm_deserializer_t* self, adt_str_t const* str, uint32_t max_target_size);
static apx_error_t deserializer_pop_state(apx_vm_deserializer_t* self);
static apx_error_t deserializer_enter_new_child_state(apx_vm_deserializer_t* self);



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_vm_deserializer_create(apx_vm_deserializer_t* self)
{
   if (self != NULL)
   {
      adt_stack_create(&self->stack, state_vdelete);
      self->state = state_new();
      if (self->state == NULL)
      {
         return APX_MEM_ERROR;
      }
      read_buffer_init(&self->buffer);
      queued_read_state_init(&self->queued_read_state);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_vm_deserializer_destroy(apx_vm_deserializer_t* self)
{
   if (self != NULL)
   {
      adt_stack_destroy(&self->stack);
      if (self->state != 0)
      {
         state_delete(self->state);
      }
   }
}

apx_vm_deserializer_t* apx_vm_deserializer_new(void)
{
   apx_vm_deserializer_t* self = (apx_vm_deserializer_t*) malloc(sizeof(apx_vm_deserializer_t));
   if (self != NULL)
   {
      apx_error_t rc = apx_vm_deserializer_create(self);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_vm_deserializer_delete(apx_vm_deserializer_t* self)
{
   if (self != NULL)
   {
      apx_vm_deserializer_destroy(self);
      free(self);
   }
}

apx_error_t apx_vm_deserializer_set_read_buffer(apx_vm_deserializer_t* self, uint8_t const* data, size_t size)
{
   if ((self != NULL) && (data != NULL) && (size > 0))
   {
      read_buffer_reset(&self->buffer, data, size);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

size_t apx_vm_deserializer_bytes_read(apx_vm_deserializer_t* self)
{
   if (self != NULL)
   {
      return self->buffer.next - self->buffer.begin;
   }
   return 0u;
}

dtl_dv_type_id apx_vm_deserializer_value_type(apx_vm_deserializer_t* self)
{
   if (self != NULL)
   {
      return self->state->value_type;
   }
   return DTL_DV_NULL;
}

dtl_sv_t* apx_vm_deserializer_take_sv(apx_vm_deserializer_t* self)
{
   if (self != NULL)
   {
      return state_take_sv(self->state);
   }
   return NULL;
}

dtl_av_t* apx_vm_deserializer_take_av(apx_vm_deserializer_t* self)
{
   if ((self != NULL) && (self->state->value_type == DTL_DV_ARRAY))
   {
      dtl_av_t* retval = self->state->value.av;
      self->state->value_type = DTL_DV_NULL;
      self->state->value.dv = NULL;
      return retval;
   }
   return NULL;
}

dtl_hv_t* apx_vm_deserializer_take_hv(apx_vm_deserializer_t* self)
{
   if ((self != NULL) && (self->state->value_type == DTL_DV_HASH))
   {
      dtl_hv_t* retval = self->state->value.hv;
      self->state->value_type = DTL_DV_NULL;
      self->state->value.dv = NULL;
      return retval;
   }
   return NULL;
}

apx_error_t apx_vm_deserializer_unpack_uint8(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_UINT8, UINT8_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_uint16(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_UINT16, UINT16_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_uint32(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_UINT32, UINT32_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_uint64(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_UINT64, UINT64_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_int8(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_INT8, INT8_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_int16(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_INT16, INT16_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_int32(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_INT32, INT32_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_int64(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_INT64, INT64_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_char(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_CHAR, CHAR_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_char8(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   return apx_vm_deserializer_unpack_char(self, array_length, dynamic_size_type); //TODO: Implement correct version later
}

apx_error_t apx_vm_deserializer_unpack_bool(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_BOOL, BOOL_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_byte(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_BYTE, BYTE_SIZE);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_unpack_record(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   if (self != NULL)
   {
      assert(self->state != NULL);
      apx_error_t result = deserializer_prepare_for_buffer_read(self, APX_TYPE_CODE_RECORD, 0u);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      return deserializer_unpack_value(self, array_length, dynamic_size_type);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_record_select(apx_vm_deserializer_t* self, char const* key, bool is_last_field)
{
   if ( (self != NULL) && (key != NULL) )
   {
      if (self->state->value_type == DTL_DV_HASH)
      {
         state_set_field_name(self->state, key, is_last_field);
         deserializer_enter_new_child_state(self);
         return APX_NO_ERROR;
      }
      return APX_VALUE_TYPE_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_vm_deserializer_check_value_range_int32(apx_vm_deserializer_t* self, int32_t lower_limit, int32_t upper_limit)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->state->value_type == DTL_DV_SCALAR)
      {
         retval = state_store_scalar_value(self->state, self->state->value.sv, APX_TYPE_CODE_INT32);
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
            retval = state_store_scalar_array_value(self->state, i, APX_TYPE_CODE_INT32);
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

apx_error_t apx_vm_deserializer_check_value_range_uint32(apx_vm_deserializer_t* self, uint32_t lower_limit, uint32_t upper_limit)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->state->value_type == DTL_DV_SCALAR)
      {
         retval = state_store_scalar_value(self->state, self->state->value.sv, APX_TYPE_CODE_UINT32);
         if (retval == APX_NO_ERROR)
         {
            assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT32);
            retval = apx_vm_value_in_range_i32(self->state->scalar_value.u32, lower_limit, upper_limit);
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
            retval = state_store_scalar_array_value(self->state, i, APX_TYPE_CODE_UINT32);
            if (retval == APX_NO_ERROR)
            {
               assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_UINT32);
               retval = apx_vm_value_in_range_i32(self->state->scalar_value.u32, lower_limit, upper_limit);
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

apx_error_t apx_vm_deserializer_check_value_range_int64(apx_vm_deserializer_t* self, int64_t lower_limit, int64_t upper_limit)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->state->value_type == DTL_DV_SCALAR)
      {
         retval = state_store_scalar_value(self->state, self->state->value.sv, APX_TYPE_CODE_INT64);
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
            retval = state_store_scalar_array_value(self->state, i, APX_TYPE_CODE_INT32);
            if (retval == APX_NO_ERROR)
            {
               assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT32);
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

apx_error_t apx_vm_deserializer_check_value_range_uint64(apx_vm_deserializer_t* self, uint64_t lower_limit, uint64_t upper_limit)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->state->value_type == DTL_DV_SCALAR)
      {
         retval = state_store_scalar_value(self->state, self->state->value.sv, APX_TYPE_CODE_INT64);
         if (retval == APX_NO_ERROR)
         {
            assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT64);
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
            retval = state_store_scalar_array_value(self->state, i, APX_TYPE_CODE_INT32);
            if (retval == APX_NO_ERROR)
            {
               assert(self->state->scalar_storage_type == APX_VM_SCALAR_STORAGE_TYPE_INT32);
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

apx_error_t apx_vm_deserializer_array_next(apx_vm_deserializer_t* self, bool* is_last)
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
                  apx_error_t result;
                  result = deserializer_enter_new_child_state(self);
                  if (result == APX_NO_ERROR)
                  {
                     result = state_init_hash_value(self->state);
                  }
                  if (result == APX_NO_ERROR)
                  {
                     state_set_type_and_size(self->state, APX_TYPE_CODE_RECORD, 0u);
                  }
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

apx_error_t apx_vm_deserializer_queued_read_begin(apx_vm_deserializer_t* self, uint32_t element_size, uint32_t max_length)
{
   if ((self != NULL) && (element_size > 0) && (max_length > 0))
   {
      uint8_t const* result = NULL;
      self->queued_read_state.element_size = element_size;
      self->queued_read_state.max_length = max_length;
      self->queued_read_state.is_active = true;
      self->queued_read_state.index = 0u;
      self->queued_read_state.size_type = apx_vm_size_to_size_type(max_length);
      result = apx_vm_parse_uint32_by_size_type(self->buffer.next,
         self->buffer.end,
         self->queued_read_state.size_type,
         &self->queued_read_state.current_length);
      if (result >= self->buffer.next)
      {
         self->buffer.next = result;
      }
      else
      {
         return APX_BUFFER_BOUNDARY_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t  apx_vm_deserializer_queued_read_next(apx_vm_deserializer_t* self, bool* is_last)
{
   if ( (self != NULL) && (is_last != NULL) )
   {

      if (self->queued_read_state.is_active)
      {
         self->queued_read_state.index++;
         if (self->queued_read_state.index >= self->queued_read_state.current_length)
         {
            self->queued_read_state.is_active = false;
            *is_last = true;
         }
         else
         {
            *is_last = false;
         }
      }
      else
      {
         return APX_INTERNAL_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//apx_vm_readState_t
static void state_create(apx_vm_readState_t* self)
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

static void state_destroy(apx_vm_readState_t* self)
{
   if (self != NULL)
   {
      state_clear_value(self);
      adt_str_destroy(&self->field_name);
   }
}

static apx_vm_readState_t* state_new(void)
{
   apx_vm_readState_t* self = (apx_vm_readState_t*)malloc(sizeof(apx_vm_readState_t));
   if (self != NULL)
   {
      state_create(self);
   }
   return self;
}

static void state_delete(apx_vm_readState_t* self)
{
   if (self != NULL)
   {
      state_destroy(self);
      free(self);
   }
}

static void state_vdelete(void* arg)
{
   state_delete((apx_vm_readState_t*)arg);
}

static void state_reset(apx_vm_readState_t* self)
{
   assert(self != NULL);
   state_clear_value(self);
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

static void state_clear_value(apx_vm_readState_t* self)
{
   assert(self != NULL);
   switch (self->value_type)
   {
   case DTL_DV_INVALID:
   case DTL_DV_NULL:
      //Already cleared
      break;
   case DTL_DV_SCALAR:
      dtl_dec_ref(self->value.sv);
      break;
   case DTL_DV_ARRAY:
      dtl_dec_ref(self->value.av);
      break;
   case DTL_DV_HASH:
      dtl_dec_ref(self->value.hv);
      break;
   }
   self->value_type = DTL_DV_NULL;
   self->value.dv = NULL;
}

static void state_set_type_and_size(apx_vm_readState_t* self, apx_typeCode_t type_code, uint32_t element_size)
{
   assert(self != NULL);
   self->type_code = type_code;
   self->element_size = element_size;
}

static bool state_is_num_or_bool_type(apx_vm_readState_t* self)
{
   assert(self != NULL);
   return ((self->type_code >= APX_TYPE_CODE_UINT8) && (self->type_code <= APX_TYPE_CODE_INT64)) ||
      (self->type_code == APX_TYPE_CODE_BOOL) ||
      ((self->type_code == APX_TYPE_CODE_CHAR) && (self->array_len == 0u));
}

static bool state_is_record_type(apx_vm_readState_t* self)
{
   assert(self != NULL);
   return (self->type_code == APX_TYPE_CODE_RECORD);
}

static bool state_is_char_type(apx_vm_readState_t* self)
{
   assert(self != NULL);
   return (self->type_code == APX_TYPE_CODE_CHAR) ||
      (self->type_code == APX_TYPE_CODE_CHAR8) ||
      (self->type_code == APX_TYPE_CODE_CHAR16) ||
      (self->type_code == APX_TYPE_CODE_CHAR32);
}

static bool state_is_byte_type(apx_vm_readState_t* self)
{
   assert(self != NULL);
   return (self->type_code == APX_TYPE_CODE_BYTE);
}

static apx_error_t state_init_scalar_value(apx_vm_readState_t* self)
{
   assert(self->value_type == DTL_DV_NULL); //If untrue we will cause a memory leak
   self->value.sv = dtl_sv_new();
   if (self->value.sv != NULL)
   {
      self->value_type = DTL_DV_SCALAR;
   }
   else
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_init_array_value(apx_vm_readState_t* self)
{
   assert(self->value_type == DTL_DV_NULL); //If untrue we will cause a memory leak
   self->value.av = dtl_av_new();
   if (self->value.av != NULL)
   {
      self->value_type = DTL_DV_ARRAY;
   }
   else
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_init_hash_value(apx_vm_readState_t* self)
{
   assert(self->value_type == DTL_DV_NULL); //If untrue we will cause a memory leak
   self->value.hv = dtl_hv_new();
   if (self->value.hv != NULL)
   {
      self->value_type = DTL_DV_HASH;
   }
   else
   {
      return APX_MEM_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_read_scalar_value(apx_vm_readState_t* self, apx_vm_readBuffer_t *buffer)
{
   assert( (self != NULL) && (buffer != NULL) );
   assert(self->element_size != 0u);
   apx_error_t retval = APX_NO_ERROR;
   if (self->element_size > (buffer->end - buffer->next))
   {
      return APX_BUFFER_BOUNDARY_ERROR;
   }
   switch (self->type_code)
   {
   case APX_TYPE_CODE_UINT8:
      self->scalar_value.u32 = (uint32_t) *buffer->next;
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT32;
      break;
   case APX_TYPE_CODE_UINT16:
   case APX_TYPE_CODE_UINT32:
      self->scalar_value.u32 = unpackLE(buffer->next, (uint8_t)self->element_size);
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT32;
      break;
   case APX_TYPE_CODE_UINT64:
      assert(self->element_size == UINT64_SIZE);
      self->scalar_value.u64 = unpackLE64(buffer->next, (uint8_t)self->element_size);
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT64;
      break;
   case APX_TYPE_CODE_INT8:
      self->scalar_value.i32 = (int32_t)((int8_t)*buffer->next);
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT32;
      break;
   case APX_TYPE_CODE_INT16:
      self->scalar_value.i32 = (int32_t)((int16_t)unpackLE(buffer->next, (uint8_t)self->element_size));
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT32;
      break;
   case APX_TYPE_CODE_INT32:
      self->scalar_value.i32 = (int32_t)unpackLE(buffer->next, (uint8_t)self->element_size);
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT32;
      break;
   case APX_TYPE_CODE_INT64:
      assert(self->element_size == INT64_SIZE);
      self->scalar_value.i64 = unpackLE64(buffer->next, (uint8_t)self->element_size);
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_INT64;
      break;
   case APX_TYPE_CODE_CHAR:
      self->scalar_value.cr = (char)*buffer->next;
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_CHAR;
      break;
   case APX_TYPE_CODE_CHAR8:
   case APX_TYPE_CODE_CHAR16:
   case APX_TYPE_CODE_CHAR32:
      self->scalar_value.u32 = (uint32_t)*buffer->next;
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_UINT32;
      break;
   case APX_TYPE_CODE_BOOL:
      self->scalar_value.bl = (*buffer->next == 0) ? false : true;
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_BOOL;
      break;
   case APX_TYPE_CODE_BYTE:
      self->scalar_value.byte = *buffer->next;
      self->scalar_storage_type = APX_VM_SCALAR_STORAGE_TYPE_BYTE;
      break;
   default:
      retval = APX_UNSUPPORTED_ERROR;
   }
   if (retval == APX_NO_ERROR)
   {
      buffer->next += self->element_size;
   }
   return retval;
}

static apx_error_t state_set_scalar_value_from_storage(apx_vm_readState_t* self)
{
   assert( (self != NULL) && (self->value_type == DTL_DV_SCALAR) );
   apx_error_t retval = APX_NO_ERROR;
   switch (self->scalar_storage_type)
   {
   case APX_VM_SCALAR_STORAGE_TYPE_NONE:
      retval = APX_INTERNAL_ERROR;
      break;
   case APX_VM_SCALAR_STORAGE_TYPE_INT32:
      dtl_sv_set_i32(self->value.sv, self->scalar_value.i32);
      break;
   case APX_VM_SCALAR_STORAGE_TYPE_UINT32:
      dtl_sv_set_u32(self->value.sv, self->scalar_value.i32);
      break;
   case APX_VM_SCALAR_STORAGE_TYPE_INT64:
      dtl_sv_set_i64(self->value.sv, self->scalar_value.i64);
      break;
   case APX_VM_SCALAR_STORAGE_TYPE_UINT64:
      dtl_sv_set_u64(self->value.sv, self->scalar_value.u64);
      break;
   case APX_VM_SCALAR_STORAGE_TYPE_BOOL:
      dtl_sv_set_bool(self->value.sv, self->scalar_value.bl);
      break;
   case APX_VM_SCALAR_STORAGE_TYPE_CHAR:
      dtl_sv_set_char(self->value.sv, self->scalar_value.cr);
      break;
   case APX_VM_SCALAR_STORAGE_TYPE_BYTE:
      dtl_sv_set_bytearray_raw(self->value.sv, &self->scalar_value.byte, UINT8_SIZE);
      break;
   }
   return retval;
}

static apx_error_t state_determine_array_length_from_read_buffer(apx_vm_readState_t* self, apx_vm_readBuffer_t* buffer)
{
   if (self->dynamic_size_type == APX_SIZE_TYPE_NONE)
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   uint8_t const* result = apx_vm_parse_uint32_by_size_type(buffer->next, buffer->end, self->dynamic_size_type, &self->array_len);
   if ((result > buffer->next) && (result <= buffer->end))
   {
      buffer->next = result;
   }
   else
   {
      return APX_BUFFER_BOUNDARY_ERROR;
   }
   return APX_NO_ERROR;
}

static dtl_sv_t* state_take_sv(apx_vm_readState_t* self)
{
   if ((self != NULL) && (self->value_type == DTL_DV_SCALAR))
   {
      dtl_sv_t* retval = self->value.sv;
      self->value_type = DTL_DV_NULL;
      self->value.dv = NULL;
      return retval;
   }
   return NULL;
}

static apx_error_t state_push_array_value(apx_vm_readState_t* self, dtl_dv_t* dv)
{
   if ( (self != NULL) && (dv != NULL) &&(self->value_type == DTL_DV_ARRAY))
   {
      dtl_av_push(self->value.av, dv, false);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t state_store_scalar_value(apx_vm_readState_t* self, dtl_sv_t *sv, apx_typeCode_t type_code)
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

static apx_error_t state_store_scalar_array_value(apx_vm_readState_t* self, int32_t index, apx_typeCode_t type_code)
{
   if (self->value_type != DTL_DV_ARRAY)
   {
      return APX_VALUE_TYPE_ERROR;
   }
   dtl_dv_t* dv = dtl_av_value(self->value.av, index);
   if (dtl_dv_type(dv) != DTL_DV_SCALAR)
   {
      return APX_VALUE_TYPE_ERROR;
   }
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   if (sv == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   return state_store_scalar_value(self, sv, type_code);
}

static apx_error_t state_read_byte_array(apx_vm_readState_t* self, apx_vm_readBuffer_t* buffer)
{
   if (buffer->next + self->array_len <= buffer->end)
   {
      dtl_sv_set_bytearray_raw(self->value.sv, buffer->next, self->array_len);
      buffer->next += self->array_len;
   }
   else
   {
      return APX_BUFFER_BOUNDARY_ERROR;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_set_field_name(apx_vm_readState_t* self, char const* name, bool is_last_field)
{
   if ((self != NULL) && (name != NULL))
   {
      apx_error_t retval = APX_NO_ERROR;
      adt_error_t rc;
      self->is_last_field = is_last_field;
      rc = adt_str_set_cstr(&self->field_name, name);
      if (rc != ADT_NO_ERROR)
      {
         retval = convert_from_adt_to_apx_error(rc);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t state_create_child_value_from_child_state(apx_vm_readState_t* self, apx_vm_readState_t* child_state)
{
   assert( (self != NULL) && (child_state != NULL) );
   assert(self->value_type == DTL_DV_HASH);
   if (adt_str_is_empty(&self->field_name))
   {
      return APX_NAME_MISSING_ERROR;
   }
   switch (child_state->value_type)
   {
   case DTL_DV_INVALID:
   case DTL_DV_NULL:
      return APX_VALUE_TYPE_ERROR;
   case DTL_DV_SCALAR:
      dtl_hv_set_cstr(self->value.hv, adt_str_cstr(&self->field_name), (dtl_dv_t*)child_state->value.sv, true);
      break;
   case DTL_DV_ARRAY:
      dtl_hv_set_cstr(self->value.hv, adt_str_cstr(&self->field_name), (dtl_dv_t*)child_state->value.av, true);
      break;
   case DTL_DV_HASH:
      dtl_hv_set_cstr(self->value.hv, adt_str_cstr(&self->field_name), (dtl_dv_t*)child_state->value.hv, true);
      break;
   }
   return APX_NO_ERROR;
}

static apx_error_t state_push_value_from_child_state(apx_vm_readState_t* self, apx_vm_readState_t* child_state)
{
   assert((self != NULL) && (child_state != NULL));
   assert(self->value_type == DTL_DV_ARRAY);
   switch (child_state->value_type)
   {
   case DTL_DV_INVALID:
   case DTL_DV_NULL:
      return APX_VALUE_TYPE_ERROR;
   case DTL_DV_SCALAR:
      dtl_av_push(self->value.av, (dtl_dv_t*)child_state->value.sv, true);
      break;
   case DTL_DV_ARRAY:
      dtl_av_push(self->value.av, (dtl_dv_t*)child_state->value.av, true);
      break;
   case DTL_DV_HASH:
      dtl_av_push(self->value.av, (dtl_dv_t*)child_state->value.hv, true);
      break;
   }
   return APX_NO_ERROR;
}

//apx_vm_readBuffer_t API
static void read_buffer_init(apx_vm_readBuffer_t* self)
{
   assert(self != NULL);
   memset(self, 0, sizeof(apx_vm_readBuffer_t));
}

static void read_buffer_reset(apx_vm_readBuffer_t* self, uint8_t const* data, size_t size)
{
   assert(self != NULL);
   self->begin = self->next = data;
   self->end = data + size;
   self->padded_next = NULL;
}

static bool read_buffer_is_valid(apx_vm_readBuffer_t* self)
{
   assert(self != NULL);
   return ((self->next != NULL) && (self->end != NULL) && (self->next <= self->end));
}

//apx_vm_queuedWriteState_t API
static void queued_read_state_init(apx_vm_queuedReadState_t* self)
{
   assert(self != NULL);
   self->max_length = 0u;
   self->current_length = 0u;
   self->element_size = 0u;
   self->index = 0u;
   self->size_type = APX_SIZE_TYPE_NONE;
   self->is_active = false;
}

static bool queued_read_state_is_active(apx_vm_queuedReadState_t* self)
{
   assert(self != NULL);
   return self->is_active;
}


//apx_vm_deserializer_t
static void deserializer_reset(apx_vm_deserializer_t* self)
{
   if (self != NULL)
   {
      while (adt_stack_size(&self->stack) > 0)
      {
         assert(self->state != NULL);
         state_delete(self->state);
         self->state = adt_stack_top(&self->stack);
         adt_stack_pop(&self->stack);
      }
      state_clear_value(self->state);
   }
}

static apx_error_t deserializer_prepare_for_buffer_read(apx_vm_deserializer_t* self, apx_typeCode_t type_code, uint32_t element_size)
{
   assert(self != NULL);
   if (!read_buffer_is_valid(&self->buffer))
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
   state_reset(self->state);
   state_set_type_and_size(self->state, type_code, element_size);
   return APX_NO_ERROR;
}

apx_error_t deserializer_unpack_value(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   assert(self != NULL);
   apx_error_t retval = APX_NO_ERROR;
   assert(self->state != NULL);
   bool do_pop_state = true;
   retval = deserializer_prepare_for_array(self, array_length, dynamic_size_type);
   if (retval == APX_NO_ERROR)
   {
      if (queued_read_state_is_active(&self->queued_read_state))
      {
         if (self->queued_read_state.index >= self->queued_read_state.current_length)
         {
            return APX_INDEX_ERROR;
         }
      }
      if (self->state->array_len == 0)
      {
         if (self->state->dynamic_size_type != APX_SIZE_TYPE_NONE)
         {
            return APX_INVALID_ARGUMENT_ERROR;
         }
         else
         {
            if (state_is_num_or_bool_type(self->state) || state_is_byte_type(self->state))
            {
               retval = state_init_scalar_value(self->state);
               if (retval == APX_NO_ERROR)
               {
                  retval = deserializer_unpack_scalar_value(self, self->state->value.sv);
               }
            }
            else if (state_is_record_type(self->state))
            {
               do_pop_state = false;
               retval = deserializer_unpack_record_value(self);
            }
            else
            {
               retval = APX_NOT_IMPLEMENTED_ERROR;
            }
         }
      }
      else
      {
         if (retval == APX_NO_ERROR)
         {
            if (state_is_num_or_bool_type(self->state))
            {
               retval = deserializer_unpack_array_of_scalar(self);
            }
            else if (state_is_char_type(self->state))
            {
               retval = deserializer_unpack_string(self);
            }
            else if (state_is_byte_type(self->state))
            {
               retval = deserializer_unpack_byte_array_internal(self);
            }
            else if (state_is_record_type(self->state))
            {
               do_pop_state = false;
               retval = deserializer_unpack_record_value(self);
            }
            else
            {
               retval = APX_NOT_IMPLEMENTED_ERROR;
            }
         }
      }
      if (retval == APX_NO_ERROR)
      {
         if (do_pop_state)
         {
            retval = deserializer_pop_state(self);
         }
      }
   }
   return retval;
}

static apx_error_t deserializer_prepare_for_array(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type)
{
   assert(self != NULL);
   if (array_length > 0)
   {
      if (dynamic_size_type != APX_SIZE_TYPE_NONE)
      {
         apx_error_t result = APX_NO_ERROR;
         self->state->dynamic_size_type = dynamic_size_type;
         self->state->max_array_len = array_length;
         result = state_determine_array_length_from_read_buffer(self->state, &self->buffer);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
         //on success, the value self->state->array_len has been successfully updated.
         if (self->state->array_len > self->state->max_array_len)
         {
            return APX_VALUE_LENGTH_ERROR;
         }
         assert(self->state->element_size != 0);
         self->buffer.padded_next = self->buffer.next + (((size_t)self->state->max_array_len) * self->state->element_size);
      }
      else
      {
         self->state->array_len = array_length;
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t deserializer_unpack_scalar_value(apx_vm_deserializer_t* self, dtl_sv_t* sv)
{
   assert((self != NULL) && (sv != NULL));
   apx_error_t retval = state_read_scalar_value(self->state, &self->buffer);
   if (retval == APX_NO_ERROR)
   {
      retval = state_set_scalar_value_from_storage(self->state);
   }
   return retval;
}

static apx_error_t deserializer_unpack_record_value(apx_vm_deserializer_t* self)
{
   assert((self != NULL) && (self->state != NULL));
   apx_error_t retval = APX_NO_ERROR;
   if (self->state->array_len > 0)
   {
      retval = state_init_array_value(self->state);
      if (retval == APX_NO_ERROR)
      {
         retval = deserializer_enter_new_child_state(self);
         if (retval == APX_NO_ERROR)
         {
            retval = state_init_hash_value(self->state);
            state_set_type_and_size(self->state, APX_TYPE_CODE_RECORD, 0u);
         }
      }
   }
   else
   {
      retval = state_init_hash_value(self->state);
   }
   return APX_NO_ERROR;
}

static apx_error_t deserializer_unpack_array_of_scalar(apx_vm_deserializer_t* self)
{
   assert(self != NULL);
   apx_error_t retval = state_init_array_value(self->state);
   if (retval == APX_NO_ERROR)
   {
      apx_vm_readState_t child_state;
      state_create(&child_state);
      state_set_type_and_size(&child_state, self->state->type_code, self->state->element_size);

      uint32_t i;
      for (i = 0; i < self->state->array_len; i++)
      {
         retval = state_init_scalar_value(&child_state);
         if (retval == APX_NO_ERROR)
         {
            retval = state_read_scalar_value(&child_state, &self->buffer);
            if (retval == APX_NO_ERROR)
            {
               retval = state_set_scalar_value_from_storage(&child_state);
               if (retval == APX_NO_ERROR)
               {
                  retval = state_push_array_value(self->state, (dtl_dv_t*) state_take_sv(&child_state));
               }
            }
            if (retval != APX_NO_ERROR)
            {
               break;
            }
         }
      }

      state_destroy(&child_state);
   }
   return retval;
}

static apx_error_t deserializer_unpack_string(apx_vm_deserializer_t* self)
{
   apx_error_t retval = state_init_scalar_value(self->state);
   if (retval == APX_NO_ERROR)
   {
      bool is_dynamic = (self->state->dynamic_size_type != APX_SIZE_TYPE_NONE);
      adt_str_t s;
      uint32_t i;
      uint8_t const* adjusted_next = self->buffer.next + CHAR_SIZE * self->state->array_len;
      adt_str_create(&s);
      for (i = 0u; i < self->state->array_len; i++)
      {
         if (self->buffer.next + CHAR_SIZE <= self->buffer.end)
         {
            char c = (char)*self->buffer.next++;
            if (!is_dynamic && c == '\0')
            {
               self->buffer.next = adjusted_next;
               break;
            }
            else
            {
               adt_str_push(&s, c);
            }
         }
         else
         {
            retval = APX_BUFFER_BOUNDARY_ERROR;
         }
      }
      if (retval == APX_NO_ERROR)
      {
         dtl_sv_set_str(self->state->value.sv, &s);
         adt_str_destroy(&s);
      }
   }
   return retval;
}

static apx_error_t deserializer_unpack_byte_array_internal(apx_vm_deserializer_t* self)
{
   assert(self->state != NULL && self->state->array_len > 0);
   apx_error_t retval = state_init_scalar_value(self->state);
   if (retval == APX_NO_ERROR)
   {
      retval = state_read_byte_array(self->state, &self->buffer);
   }
   return retval;
}

static apx_error_t deserializer_pop_state(apx_vm_deserializer_t* self)
{
   assert(self->state != NULL);
   while (adt_stack_size(&self->stack) > 0)
   {
      apx_vm_readState_t* child_state = self->state;
      assert(child_state != NULL);
      self->state = adt_stack_top(&self->stack);
      adt_stack_pop(&self->stack);
      if (self->state->type_code == APX_TYPE_CODE_RECORD)
      {
         apx_error_t result = APX_NO_ERROR;
         if (self->state->value_type == DTL_DV_HASH) //Normal record
         {
            result = state_create_child_value_from_child_state(self->state, child_state);
         }
         else if (self->state->value_type == DTL_DV_ARRAY) //Array of records
         {
            result = state_push_value_from_child_state(self->state, child_state);
         }
         else
         {
            state_delete(child_state);
            result = APX_NOT_IMPLEMENTED_ERROR;
         }
         if (result != APX_NO_ERROR)
         {
            state_delete(child_state);
            return result;
         }
      }
      else
      {
         state_delete(child_state);
         return APX_NOT_IMPLEMENTED_ERROR;
      }
      state_delete(child_state);
      if (!self->state->is_last_field)
      {
         break;
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t deserializer_enter_new_child_state(apx_vm_deserializer_t* self)
{
   assert(self != NULL);
   apx_vm_readState_t* child_state = state_new();
   if (child_state == NULL)
   {
      return APX_MEM_ERROR;
   }
   child_state->parent = self->state;
   adt_stack_push(&self->stack, self->state);
   self->state = child_state;
   return APX_NO_ERROR;
}

