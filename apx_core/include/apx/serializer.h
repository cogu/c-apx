/*****************************************************************************
* \file      serializer.h
* \author    Conny Gustafsson
* \date      2021-01-03
* \brief     APX port data serializer
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SERIALIZER_H
#define APX_SERIALIZER_H

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

typedef struct apx_vm_write_state_tag
{
   union apx_vm_write_state_value_tag
   {
      dtl_sv_t const *sv;
      dtl_av_t const *av;
      dtl_hv_t const *hv;
      dtl_dv_t const *dv;
   } value;

   union apx_vm_write_state_scalar_value_tag
   {
      int32_t i32;
      uint32_t u32;
      int64_t i64;
      uint64_t u64;
      bool bl;
   } scalar_value;
   struct apx_vm_write_state_tag *parent;
   adt_str_t field_name;
   uint32_t index; //array index
   uint32_t array_len; //array length of current object
   uint32_t max_array_len; //maximum array length of current object. This is only applicable for dynamic arrays
   uint32_t element_size;
   dtl_dv_type_id value_type; //describes which part of the value union is currently active
   scalar_storage_type_t scalar_storage_type; //describes which part of the scalar_value union is currently active
   apx_type_code_t type_code;
   apx_size_type_t dynamic_size_type;
   apx_range_check_state_t range_check_state;
} apx_vm_write_state_t;

typedef struct apx_vm_write_buffer_tag
{
   uint8_t *begin;
   uint8_t *end;
   uint8_t *next;
   uint8_t *padded_next;
   uint8_t* mark;
} apx_vm_write_buffer_t;

typedef struct apx_vm_queued_write_state_tag
{
   uint32_t max_length;
   uint32_t current_length;
   uint32_t element_size;
   uint8_t* length_ptr;
   apx_size_type_t size_type;
   bool is_active;
} apx_vm_queued_write_state_t;

typedef struct apx_vm_serializer_tag
{
   adt_stack_t stack; //stack containing strong references to apx_vm_write_state_t
   apx_vm_write_state_t* state; //current inner state
   apx_vm_write_buffer_t buffer;
   apx_vm_queued_write_state_t queued_write_state;
} apx_vm_serializer_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//apx_vm_write_state_t
void apx_vm_write_state_create(apx_vm_write_state_t *self);
void apx_vm_write_state_destroy(apx_vm_write_state_t *self);
apx_vm_write_state_t* apx_vm_write_state_new(void);
void apx_vm_write_state_delete(apx_vm_write_state_t *self);
void apx_vm_write_state_vdelete(void *arg);
void apx_vm_write_state_reset(apx_vm_write_state_t* self, dtl_dv_type_id type_id);


//apx_vm_serializer_t
apx_error_t apx_vm_serializer_create(apx_vm_serializer_t *self);
void apx_vm_serializer_destroy(apx_vm_serializer_t *self);
apx_vm_serializer_t* apx_vm_serializer_new(void);
void apx_vm_serializer_delete(apx_vm_serializer_t *self);
void apx_vm_serializer_reset(apx_vm_serializer_t* self);
apx_error_t apx_vm_serializer_set_write_buffer(apx_vm_serializer_t* self, uint8_t* data, size_t size);
size_t apx_vm_serializer_bytes_written(apx_vm_serializer_t* self);
apx_error_t apx_vm_serializer_set_value_dv(apx_vm_serializer_t* self, dtl_dv_t const* dv);
apx_error_t apx_vm_serializer_set_value_sv(apx_vm_serializer_t* self, dtl_sv_t const* sv);
apx_error_t apx_vm_serializer_set_value_av(apx_vm_serializer_t* self, dtl_av_t const* av);
apx_error_t apx_vm_serializer_set_value_hv(apx_vm_serializer_t* self, dtl_hv_t const* hv);
apx_error_t apx_vm_serializer_pack_uint8(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_uint16(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_uint32(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_uint64(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_int8(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_int16(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_int32(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_int64(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_char(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_char8(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_bool(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_byte(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_pack_record(apx_vm_serializer_t* self, uint32_t array_length, apx_size_type_t dynamic_size_type);
apx_error_t apx_vm_serializer_record_select(apx_vm_serializer_t* self, char const* key, bool const is_first_field);
apx_error_t apx_vm_serializer_record_end(apx_vm_serializer_t* self);
apx_error_t apx_vm_serializer_check_value_range_int32(apx_vm_serializer_t* self, int32_t lower_limit, int32_t upper_limit);
apx_error_t apx_vm_serializer_check_value_range_uint32(apx_vm_serializer_t* self, uint32_t lower_limit, uint32_t upper_limit);
apx_error_t apx_vm_serializer_check_value_range_int64(apx_vm_serializer_t* self, int64_t lower_limit, int64_t upper_limit);
apx_error_t apx_vm_serializer_check_value_range_uint64(apx_vm_serializer_t* self, uint64_t lower_limit, uint64_t upper_limit);
apx_error_t apx_vm_serializer_queued_write_begin(apx_vm_serializer_t* self, uint32_t element_size, uint32_t max_length, bool clear_queue);
apx_error_t apx_vm_serializer_queued_write_end(apx_vm_serializer_t* self);
apx_error_t apx_vm_serializer_array_next(apx_vm_serializer_t* self, bool* is_last);


#endif //APX_SERIALIZER_H
