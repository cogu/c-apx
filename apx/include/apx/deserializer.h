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
#ifndef APX_DESERIALIZER_H
#define APX_DESERIALIZER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_stack.h"
#include "dtl_type.h"
#include "adt_str.h"
#include "apx/error.h"
#include "apx/vm_defs.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_vm_readBuffer_tag
{
   uint8_t const* begin;
   uint8_t const* end;
   uint8_t const* next;
   uint8_t const* padded_next;
} apx_vm_readBuffer_t;


typedef struct apx_vm_readState_tag
{
   union apx_vm_read_state_value_tag
   {
      dtl_sv_t* sv;
      dtl_av_t* av;
      dtl_hv_t* hv;
      dtl_dv_t* dv;
   } value;

   union apx_vm_read_state_scalar_value_tag
   {
      int32_t i32;
      uint32_t u32;
      int64_t i64;
      uint64_t u64;
      bool bl;
      char cr;
      uint8_t byte;
   } scalar_value;

   struct apx_vm_readState_tag* parent;
   adt_str_t field_name;
   uint32_t index; //array index
   uint32_t array_len; //array length of current object
   uint32_t max_array_len; //maximum array length of current object. This is only applicable for dynamic arrays
   uint32_t element_size;
   dtl_dv_type_id value_type; //describes which part of the value union is currently active
   scalar_storage_type_t scalar_storage_type; //describes which part of the scalar_value union is currently active
   apx_typeCode_t type_code;
   bool is_last_field;
   apx_sizeType_t dynamic_size_type;
   apx_rangeCheckState_t range_check_state;
} apx_vm_readState_t;


typedef struct apx_vm_queuedReadState_tag
{
   uint32_t max_length;
   uint32_t current_length;
   uint32_t element_size;
   uint32_t index;
   apx_sizeType_t size_type;
   bool is_active;
} apx_vm_queuedReadState_t;

typedef struct apx_vm_deserializer_tag
{
   adt_stack_t stack; //stack containing strong references to apx_vmReadState_t
   apx_vm_readBuffer_t buffer;
   apx_vm_queuedReadState_t queued_read_state;
   apx_vm_readState_t* state; //current inner state
   bool hasValidReadBuf;
} apx_vm_deserializer_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//apx_vm_serializer_t
apx_error_t apx_vm_deserializer_create(apx_vm_deserializer_t* self);
void apx_vm_deserializer_destroy(apx_vm_deserializer_t* self);
apx_vm_deserializer_t* apx_vm_deserializer_new(void);
void apx_vm_deserializer_delete(apx_vm_deserializer_t* self);
apx_error_t apx_vm_deserializer_set_read_buffer(apx_vm_deserializer_t* self, uint8_t const* data, size_t size);
size_t apx_vm_deserializer_bytes_read(apx_vm_deserializer_t* self);
dtl_dv_type_id apx_vm_deserializer_value_type(apx_vm_deserializer_t* self);
dtl_sv_t* apx_vm_deserializer_take_sv(apx_vm_deserializer_t* self);
dtl_av_t* apx_vm_deserializer_take_av(apx_vm_deserializer_t* self);
dtl_hv_t* apx_vm_deserializer_take_hv(apx_vm_deserializer_t* self);
apx_error_t apx_vm_deserializer_unpack_uint8(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_uint16(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_uint32(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_uint64(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_int8(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_int16(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_int32(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_int64(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_char(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_char8(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_bool(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_byte(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_unpack_record(apx_vm_deserializer_t* self, uint32_t array_length, apx_sizeType_t dynamic_size_type);
apx_error_t apx_vm_deserializer_record_select(apx_vm_deserializer_t* self, char const* key, bool is_last_field);
apx_error_t apx_vm_deserializer_check_value_range_int32(apx_vm_deserializer_t* self, int32_t lower_limit, int32_t upper_limit);
apx_error_t apx_vm_deserializer_check_value_range_uint32(apx_vm_deserializer_t* self, uint32_t lower_limit, uint32_t upper_limit);
apx_error_t apx_vm_deserializer_check_value_range_int64(apx_vm_deserializer_t* self, int64_t lower_limit, int64_t upper_limit);
apx_error_t apx_vm_deserializer_check_value_range_uint64(apx_vm_deserializer_t* self, uint64_t lower_limit, uint64_t upper_limit);
apx_error_t apx_vm_deserializer_array_next(apx_vm_deserializer_t* self, bool *is_last);
apx_error_t  apx_vm_deserializer_queued_read_begin(apx_vm_deserializer_t* self, uint32_t element_size, uint32_t max_length);
apx_error_t  apx_vm_deserializer_queued_read_next(apx_vm_deserializer_t* self, bool* is_last);


#endif //APX_DESERIALIZER_H
