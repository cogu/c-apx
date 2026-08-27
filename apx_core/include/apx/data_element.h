/*****************************************************************************
* \file      apx_data_element.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Data element data structure
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
struct apx_dataType_tag;


typedef struct apx_dataElement_tag
{
   char* name; //Used when dataElement is part of a record structure
   adt_ary_t* elements; //Used when type_code is APX_TYPE_CODE_RECORD. Contains strong references to apx_dataElement_t
   uint32_t array_len;
   apx_elementId_t element_id;
   apx_typeCode_t type_code;
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
      apx_typeId_t id; //used when type_code is APX_TYPE_CODE_REF_ID
      char* name; //used when type_code is APX_TYPE_CODE_REF_NAME
      struct apx_dataType_tag* ptr; //used when type_code is APX_TYPE_CODE_REF_PTR
   } type_ref;
} apx_dataElement_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_dataElement_t* apx_dataElement_new(apx_typeCode_t type_code);
void apx_dataElement_delete(apx_dataElement_t *self);
void apx_dataElement_vdelete(void *arg);
apx_error_t apx_dataElement_create(apx_dataElement_t *self, apx_typeCode_t baseType);
void apx_dataElement_destroy(apx_dataElement_t *self);
apx_dataElement_t* apx_dataElement_clone(apx_dataElement_t* self);
apx_error_t apx_dataElement_set_name_bstr(apx_dataElement_t* self, const uint8_t* begin, const uint8_t* end);
apx_error_t apx_dataElement_set_name_cstr(apx_dataElement_t* self, const char* name);
const char* apx_dataElement_get_name(apx_dataElement_t const* self);
apx_typeCode_t apx_dataElement_get_type_code(apx_dataElement_t const* self);
bool apx_dataElement_has_limits(apx_dataElement_t const* self);
void apx_dataElement_init_record_type(apx_dataElement_t *self);
apx_error_t apx_dataElement_set_array_length(apx_dataElement_t *self, uint32_t arrayLen);
uint32_t apx_dataElement_get_array_length(apx_dataElement_t const *self);
void apx_dataElement_set_dynamic_array(apx_dataElement_t *self);
bool apx_dataElement_is_array(apx_dataElement_t const* self);
bool apx_dataElement_is_dynamic_array(apx_dataElement_t const*self);
void apx_dataElement_append_child(apx_dataElement_t *self, apx_dataElement_t *child);
int32_t apx_dataElement_get_num_child_elements(apx_dataElement_t const* self);
apx_dataElement_t *apx_dataElement_get_child_at(apx_dataElement_t const* self, int32_t index);
void apx_dataElement_set_type_ref_id(apx_dataElement_t *self, apx_typeId_t typeId);
apx_typeId_t apx_dataElement_get_type_ref_id(apx_dataElement_t const* self);
apx_error_t apx_dataElement_set_type_ref_name_bstr(apx_dataElement_t *self, const uint8_t* begin, const uint8_t* end);
const char *apx_dataElement_get_type_ref_name(apx_dataElement_t const *self);
void apx_dataElement_set_type_ref_ptr(apx_dataElement_t *self, struct apx_dataType_tag *ptr);
struct apx_dataType_tag *apx_dataElement_get_type_ref_ptr(apx_dataElement_t const* self);
void apx_dataElement_set_limits_int32(apx_dataElement_t* self, int32_t lower, int32_t upper);
void apx_dataElement_set_limits_int64(apx_dataElement_t* self, int64_t lower, int64_t upper);
void apx_dataElement_set_limits_uint32(apx_dataElement_t* self, uint32_t lower, uint32_t upper);
void apx_dataElement_set_limits_uint64(apx_dataElement_t* self, uint64_t lower, uint64_t upper);
bool apx_dataElement_get_limits_int32(apx_dataElement_t const* self, int32_t* lower, int32_t* upper);
bool apx_dataElement_get_limits_int64(apx_dataElement_t const* self, int64_t* lower, int64_t* upper);
bool apx_dataElement_get_limits_uint32(apx_dataElement_t const* self, uint32_t* lower, uint32_t* upper);
bool apx_dataElement_get_limits_uint64(apx_dataElement_t const* self, uint64_t* lower, uint64_t* upper);
apx_error_t apx_dataElement_derive_types_on_element(apx_dataElement_t* self, adt_ary_t const* type_list, adt_hash_t const* type_map);
apx_error_t apx_dataElement_derive_proper_init_value(apx_dataElement_t* self, dtl_dv_t* parsed_value, dtl_dv_t** derived_value);
apx_error_t apx_dataElement_derive_data_element(apx_dataElement_t const* self, apx_dataElement_t** data_element, apx_dataElement_t** parent);
void apx_dataElement_set_id(apx_dataElement_t* self, apx_elementId_t id);
apx_elementId_t apx_dataElement_get_id(apx_dataElement_t const* self);
adt_str_t* apx_dataElement_to_string(apx_dataElement_t const* self, bool normalized);


#endif //APX_DATAELEMENT_H
