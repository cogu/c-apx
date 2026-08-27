/*****************************************************************************
* \file      computation.h
* \author    Conny Gustafsson
* \date      2020-12-01
* \brief     APX Computations
*
* Copyright (c) 2020 Conny Gustafsson
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
   apx_computationType_t computation_type;
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

typedef struct apx_valueTable_tag
{
   apx_computation_t base;
   adt_ary_t values; //strong reference to apx_str_t
} apx_valueTable_t;

typedef struct apx_rationalScaling_tag
{
   apx_computation_t base;
   double offset;
   int32_t numerator;
   int32_t denominator;
   char* unit;
} apx_rationalScaling_t;

typedef struct apx_computationList_tag
{
   apx_computationListId_t computation_list_id;
   adt_ary_t computations; //strong references to either apx_valueTable_t or apx_rationalScaling_t
} apx_computationList_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//apx_computation_t API
void apx_computation_vtable_create(apx_computation_vtable_t* vtable, apx_void_ptr_func_t* destructor, apx_to_string_func_t* to_string);
void apx_computation_create(apx_computation_t* self, apx_computation_vtable_t const* vtable, apx_computationType_t computation_type);
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
apx_computationType_t apx_computation_type(apx_computation_t const* self);

//apx_valueTable_t API
void apx_valueTable_create(apx_valueTable_t* self);
void apx_valueTable_destroy(apx_valueTable_t* self);
apx_valueTable_t* apx_valueTable_new(void);
void apx_valueTable_delete(apx_valueTable_t* self);
apx_valueTable_t* apx_valueTable_clone(apx_valueTable_t const* other);
void apx_valueTable_set_range_signed(apx_valueTable_t* self, int32_t lower_limit, int32_t upper_limit);
void apx_valueTable_set_range_unsigned(apx_valueTable_t* self, uint32_t lower_limit, uint32_t upper_limit);
void apx_valueTable_set_upper_limit_signed(apx_valueTable_t* self, int32_t upper_limit);
void apx_valueTable_set_upper_limit_unsigned(apx_valueTable_t* self, uint32_t upper_limit);
int32_t apx_valueTable_length(apx_valueTable_t* self);
apx_error_t apx_valueTable_move_values(apx_valueTable_t* self, adt_ary_t* values);
adt_str_t* apx_valueTable_get_value(apx_valueTable_t const* self, int32_t index);
char const* apx_valueTable_get_value_cstr(apx_valueTable_t const* self, int32_t index);
adt_str_t* apx_valueTable_to_string(apx_valueTable_t const* self);

//apx_rationalScaling_t API
apx_error_t apx_rationalScaling_create(apx_rationalScaling_t* self, double offset, int32_t numerator, int32_t denominator, char const* unit);
void apx_rationalScaling_destroy(apx_rationalScaling_t* self);
apx_rationalScaling_t* apx_rationalScaling_new(double offset, int32_t numerator, int32_t denominator, char const* unit);
void apx_rationalScaling_delete(apx_rationalScaling_t* self);
apx_rationalScaling_t* apx_rationalScaling_clone(apx_rationalScaling_t const* other);
void apx_rationalScaling_set_range_signed(apx_rationalScaling_t* self, int32_t lower_limit, int32_t upper_limit);
void apx_rationalScaling_set_range_unsigned(apx_rationalScaling_t* self, uint32_t lower_limit, uint32_t upper_limit);
adt_str_t* apx_rationalScaling_to_string(apx_rationalScaling_t const* self);
double apx_rationalScaling_offset(apx_rationalScaling_t const* self);
int32_t apx_rationalScaling_numerator(apx_rationalScaling_t const* self);
int32_t apx_rationalScaling_denominator(apx_rationalScaling_t const* self);
char const* apx_rationalScaling_unit(apx_rationalScaling_t const* self);

//apx_computationList_t
void apx_computationList_create(apx_computationList_t* self);
void apx_computationList_destroy(apx_computationList_t* self);
apx_computationList_t* apx_computationList_new(void);
void apx_computationList_delete(apx_computationList_t* self);
void apx_computationList_vdelete(void* arg);
void apx_computationList_set_id(apx_computationList_t* self, apx_computationListId_t computation_list_id);
apx_error_t apx_computationList_append_clone_of_computation(apx_computationList_t* self, apx_computation_t const* computation);


#endif //APX_COMPUTATION_H
