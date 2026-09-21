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
void apx_value_table_create(apx_value_table_t* self);
void apx_value_table_destroy(apx_value_table_t* self);
apx_value_table_t* apx_value_table_new(void);
void apx_value_table_delete(apx_value_table_t* self);
apx_value_table_t* apx_value_table_clone(apx_value_table_t const* other);
void apx_value_table_set_range_signed(apx_value_table_t* self, int32_t lower_limit, int32_t upper_limit);
void apx_value_table_set_range_unsigned(apx_value_table_t* self, uint32_t lower_limit, uint32_t upper_limit);
void apx_value_table_set_upper_limit_signed(apx_value_table_t* self, int32_t upper_limit);
void apx_value_table_set_upper_limit_unsigned(apx_value_table_t* self, uint32_t upper_limit);
int32_t apx_value_table_length(apx_value_table_t* self);
apx_error_t apx_value_table_move_values(apx_value_table_t* self, adt_ary_t* values);
adt_str_t* apx_value_table_get_value(apx_value_table_t const* self, int32_t index);
char const* apx_value_table_get_value_cstr(apx_value_table_t const* self, int32_t index);
adt_str_t* apx_value_table_to_string(apx_value_table_t const* self);

//apx_rational_scaling_t API
apx_error_t apx_rational_scaling_create(apx_rational_scaling_t* self, double offset, int32_t numerator, int32_t denominator, char const* unit);
void apx_rational_scaling_destroy(apx_rational_scaling_t* self);
apx_rational_scaling_t* apx_rational_scaling_new(double offset, int32_t numerator, int32_t denominator, char const* unit);
void apx_rational_scaling_delete(apx_rational_scaling_t* self);
apx_rational_scaling_t* apx_rational_scaling_clone(apx_rational_scaling_t const* other);
void apx_rational_scaling_set_range_signed(apx_rational_scaling_t* self, int32_t lower_limit, int32_t upper_limit);
void apx_rational_scaling_set_range_unsigned(apx_rational_scaling_t* self, uint32_t lower_limit, uint32_t upper_limit);
adt_str_t* apx_rational_scaling_to_string(apx_rational_scaling_t const* self);
double apx_rational_scaling_offset(apx_rational_scaling_t const* self);
int32_t apx_rational_scaling_numerator(apx_rational_scaling_t const* self);
int32_t apx_rational_scaling_denominator(apx_rational_scaling_t const* self);
char const* apx_rational_scaling_unit(apx_rational_scaling_t const* self);

//apx_computation_list_t
void apx_computation_list_create(apx_computation_list_t* self);
void apx_computation_list_destroy(apx_computation_list_t* self);
apx_computation_list_t* apx_computation_list_new(void);
void apx_computation_list_delete(apx_computation_list_t* self);
void apx_computation_list_vdelete(void* arg);
void apx_computation_list_set_id(apx_computation_list_t* self, apx_computation_list_id_t computation_list_id);
apx_error_t apx_computation_list_append_clone_of_computation(apx_computation_list_t* self, apx_computation_t const* computation);


#endif //APX_COMPUTATION_H
