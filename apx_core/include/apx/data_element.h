/*****************************************************************************
* \file      data_element.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Data element data structure
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_DATAELEMENT_H
#define APX_DATAELEMENT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "adt_ary.h"
#include "adt_str.h"
#include "dtl_type.h"
#include "apx/error.h"
#include "apx/types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_data_type_tag;


typedef struct apx_data_element_tag
{
   char* name; //Used when dataElement is part of a record structure
   adt_ary_t* elements; //Used when type_code is APX_TYPE_CODE_RECORD. Contains strong references to apx_data_element_t
   uint32_t array_len;
   apx_element_id_t element_id;
   apx_type_code_t type_code;
   bool is_dynamic_array;
   bool has_limits;
   union
   {
      int32_t  i32; //Used when type_code is signed but not 64-bits
      int64_t  i64; //Used when type_code is APX_TYPE_CODE_INT64
      uint32_t u32; //Used when type_code is unsigned but not 64-bits
      uint64_t u64; //Used when type_code is APX_TYPE_CODE_UINT64
   } lower_limit;
   union
   {
      int32_t  i32;
      int64_t  i64;
      uint32_t u32;
      uint64_t u64;
   } upper_limit;
   union
   {
      apx_type_id_t id; //used when type_code is APX_TYPE_CODE_REF_ID
      char* name; //used when type_code is APX_TYPE_CODE_REF_NAME
      struct apx_data_type_tag* ptr; //used when type_code is APX_TYPE_CODE_REF_PTR
   } type_ref;
} apx_data_element_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_data_element_t* apx_dataElement_new(apx_type_code_t type_code);
void apx_dataElement_delete(apx_data_element_t *self);
void apx_dataElement_vdelete(void *arg);
apx_error_t apx_dataElement_create(apx_data_element_t *self, apx_type_code_t baseType);
void apx_dataElement_destroy(apx_data_element_t *self);
apx_data_element_t* apx_dataElement_clone(apx_data_element_t* self);
apx_error_t apx_dataElement_set_name_bstr(apx_data_element_t* self, const uint8_t* begin, const uint8_t* end);
apx_error_t apx_dataElement_set_name_cstr(apx_data_element_t* self, const char* name);
const char* apx_dataElement_get_name(apx_data_element_t const* self);
apx_type_code_t apx_dataElement_get_type_code(apx_data_element_t const* self);
bool apx_dataElement_has_limits(apx_data_element_t const* self);
void apx_dataElement_init_record_type(apx_data_element_t *self);
apx_error_t apx_dataElement_set_array_length(apx_data_element_t *self, uint32_t arrayLen);
uint32_t apx_dataElement_get_array_length(apx_data_element_t const *self);
void apx_dataElement_set_dynamic_array(apx_data_element_t *self);
bool apx_dataElement_is_array(apx_data_element_t const* self);
bool apx_dataElement_is_dynamic_array(apx_data_element_t const*self);
void apx_dataElement_append_child(apx_data_element_t *self, apx_data_element_t *child);
int32_t apx_dataElement_get_num_child_elements(apx_data_element_t const* self);
apx_data_element_t *apx_dataElement_get_child_at(apx_data_element_t const* self, int32_t index);
void apx_dataElement_set_type_ref_id(apx_data_element_t *self, apx_type_id_t typeId);
apx_type_id_t apx_dataElement_get_type_ref_id(apx_data_element_t const* self);
apx_error_t apx_dataElement_set_type_ref_name_bstr(apx_data_element_t *self, const uint8_t* begin, const uint8_t* end);
const char *apx_dataElement_get_type_ref_name(apx_data_element_t const *self);
void apx_dataElement_set_type_ref_ptr(apx_data_element_t *self, struct apx_data_type_tag *ptr);
struct apx_data_type_tag *apx_dataElement_get_type_ref_ptr(apx_data_element_t const* self);
void apx_dataElement_set_limits_int32(apx_data_element_t* self, int32_t lower, int32_t upper);
void apx_dataElement_set_limits_int64(apx_data_element_t* self, int64_t lower, int64_t upper);
void apx_dataElement_set_limits_uint32(apx_data_element_t* self, uint32_t lower, uint32_t upper);
void apx_dataElement_set_limits_uint64(apx_data_element_t* self, uint64_t lower, uint64_t upper);
bool apx_dataElement_get_limits_int32(apx_data_element_t const* self, int32_t* lower, int32_t* upper);
bool apx_dataElement_get_limits_int64(apx_data_element_t const* self, int64_t* lower, int64_t* upper);
bool apx_dataElement_get_limits_uint32(apx_data_element_t const* self, uint32_t* lower, uint32_t* upper);
bool apx_dataElement_get_limits_uint64(apx_data_element_t const* self, uint64_t* lower, uint64_t* upper);
apx_error_t apx_dataElement_derive_types_on_element(apx_data_element_t* self, adt_ary_t const* type_list, adt_hash_t const* type_map);
apx_error_t apx_dataElement_derive_proper_init_value(apx_data_element_t* self, dtl_dv_t* parsed_value, dtl_dv_t** derived_value);
apx_error_t apx_dataElement_derive_data_element(apx_data_element_t const* self, apx_data_element_t** data_element, apx_data_element_t** parent);
void apx_dataElement_set_id(apx_data_element_t* self, apx_element_id_t id);
apx_element_id_t apx_dataElement_get_id(apx_data_element_t const* self);
adt_str_t* apx_dataElement_to_string(apx_data_element_t const* self, bool normalized);


#endif //APX_DATAELEMENT_H
