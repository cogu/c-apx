/*****************************************************************************
* \file      computation.h
* \author    Conny Gustafsson
* \date      2020-12-01
* \brief     APX Computations
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_COMPUTATION_H
#define APX_COMPUTATION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "adt_ary.h"
#include "adt_str.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////



typedef struct apx_computation_vtable_tag
{
   apx_void_ptr_func_t* destructor;
   apx_to_string_func_t* to_string;
} apx_computation_vtable_t;

typedef struct apx_computation_tag
{
   apx_computation_vtable_t vtable;
   apx_computation_type_t computation_type;
   bool is_signed_range;
   union
   {
      uint32_t u32;
      int32_t i32;
   } lower_limit;
   union
   {
      uint32_t u32;
      int32_t i32;
   } upper_limit;
} apx_computation_t;

typedef struct apx_value_table_tag
{
   apx_computation_t base;
   adt_ary_t values; //strong reference to apx_str_t
} apx_value_table_t;

typedef struct apx_rational_scaling_tag
{
   apx_computation_t base;
   double offset;
   int32_t numerator;
   int32_t denominator;
   char* unit;
} apx_rational_scaling_t;

typedef struct apx_computation_list_tag
{
   apx_computation_list_id_t computation_list_id;
   adt_ary_t computations; //strong references to either apx_value_table_t or apx_rational_scaling_t
} apx_computation_list_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//apx_computation_t API
void apx_computation_vtable_create(apx_computation_vtable_t* vtable, apx_void_ptr_func_t* destructor, apx_to_string_func_t* to_string);
void apx_computation_create(apx_computation_t* self, apx_computation_vtable_t const* vtable, apx_computation_type_t computation_type);
void apx_computation_destroy(apx_computation_t* self);
void apx_computation_vdelete(void* arg);
void apx_computation_set_range_signed(apx_computation_t* self, int32_t lower_limit, int32_t upper_limit);
void apx_computation_set_range_unsigned(apx_computation_t* self, uint32_t lower_limit, uint32_t upper_limit);
void apx_computation_set_upper_limit_signed(apx_computation_t* self, int32_t upper_limit);
void apx_computation_set_upper_limit_unsigned(apx_computation_t* self, uint32_t upper_limit);
int32_t apx_computation_get_lower_limit_signed(apx_computation_t const* self);
uint32_t apx_computation_get_lower_limit_unsigned(apx_computation_t const* self);
int32_t apx_computation_get_upper_limit_signed(apx_computation_t const* self);
uint32_t apx_computation_get_upper_limit_unsigned(apx_computation_t const* self);
bool apx_computation_is_range_signed(apx_computation_t const* self);

adt_str_t* apx_computation_to_string(apx_computation_t const* self);
void apx_computation_assign(apx_computation_t* lhs, apx_computation_t const* rhs);
apx_computation_type_t apx_computation_type(apx_computation_t const* self);

//apx_value_table_t API
void apx_valueTable_create(apx_value_table_t* self);
void apx_valueTable_destroy(apx_value_table_t* self);
apx_value_table_t* apx_valueTable_new(void);
void apx_valueTable_delete(apx_value_table_t* self);
apx_value_table_t* apx_valueTable_clone(apx_value_table_t const* other);
void apx_valueTable_set_range_signed(apx_value_table_t* self, int32_t lower_limit, int32_t upper_limit);
void apx_valueTable_set_range_unsigned(apx_value_table_t* self, uint32_t lower_limit, uint32_t upper_limit);
void apx_valueTable_set_upper_limit_signed(apx_value_table_t* self, int32_t upper_limit);
void apx_valueTable_set_upper_limit_unsigned(apx_value_table_t* self, uint32_t upper_limit);
int32_t apx_valueTable_length(apx_value_table_t* self);
apx_error_t apx_valueTable_move_values(apx_value_table_t* self, adt_ary_t* values);
adt_str_t* apx_valueTable_get_value(apx_value_table_t const* self, int32_t index);
char const* apx_valueTable_get_value_cstr(apx_value_table_t const* self, int32_t index);
adt_str_t* apx_valueTable_to_string(apx_value_table_t const* self);

//apx_rational_scaling_t API
apx_error_t apx_rationalScaling_create(apx_rational_scaling_t* self, double offset, int32_t numerator, int32_t denominator, char const* unit);
void apx_rationalScaling_destroy(apx_rational_scaling_t* self);
apx_rational_scaling_t* apx_rationalScaling_new(double offset, int32_t numerator, int32_t denominator, char const* unit);
void apx_rationalScaling_delete(apx_rational_scaling_t* self);
apx_rational_scaling_t* apx_rationalScaling_clone(apx_rational_scaling_t const* other);
void apx_rationalScaling_set_range_signed(apx_rational_scaling_t* self, int32_t lower_limit, int32_t upper_limit);
void apx_rationalScaling_set_range_unsigned(apx_rational_scaling_t* self, uint32_t lower_limit, uint32_t upper_limit);
adt_str_t* apx_rationalScaling_to_string(apx_rational_scaling_t const* self);
double apx_rationalScaling_offset(apx_rational_scaling_t const* self);
int32_t apx_rationalScaling_numerator(apx_rational_scaling_t const* self);
int32_t apx_rationalScaling_denominator(apx_rational_scaling_t const* self);
char const* apx_rationalScaling_unit(apx_rational_scaling_t const* self);

//apx_computation_list_t
void apx_computationList_create(apx_computation_list_t* self);
void apx_computationList_destroy(apx_computation_list_t* self);
apx_computation_list_t* apx_computationList_new(void);
void apx_computationList_delete(apx_computation_list_t* self);
void apx_computationList_vdelete(void* arg);
void apx_computationList_set_id(apx_computation_list_t* self, apx_computation_list_id_t computation_list_id);
apx_error_t apx_computationList_append_clone_of_computation(apx_computation_list_t* self, apx_computation_t const* computation);


#endif //APX_COMPUTATION_H
