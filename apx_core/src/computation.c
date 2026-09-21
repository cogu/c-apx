/*****************************************************************************
* \file      computation.c
* \author    Conny Gustafsson
* \date      2020-12-01
* \brief     APX Computations
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "apx/computation.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define TMP_BUF_SIZE 80

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_computation_delete(apx_computation_t* self);
static void apx_value_table_vdestroy(void* arg);
static adt_str_t* apx_value_table_vto_string(void* arg);
static void apx_rational_scaling_vdestroy(void* arg);
static adt_str_t* apx_rational_scaling_vto_string(void* arg);
static adt_str_t* computation_limit_to_string(apx_computation_t const* self);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//apx_computation_t API
void apx_computation_vtable_create(apx_computation_vtable_t* vtable, apx_void_ptr_func_t* destructor, apx_to_string_func_t* to_string)
{
   if (vtable != NULL)
   {
      vtable->destructor = destructor;
      vtable->to_string = to_string;
   }
}

void apx_computation_create(apx_computation_t* self, apx_computation_vtable_t const* vtable, apx_computation_type_t computation_type)
{
   if (self != NULL)
   {
      if (vtable != NULL)
      {
         memcpy(&self->vtable, vtable, sizeof(apx_computation_vtable_t));
      }
      else
      {
         memset(&self->vtable, 0u, sizeof(apx_computation_vtable_t));
      }
      self->computation_type = computation_type;
      self->is_signed_range = false;
      self->lower_limit.i32 = 0;
      self->upper_limit.i32 = 0;
   }
}

void apx_computation_destroy(apx_computation_t* self)
{
   if (self != NULL)
   {
      //nothing to do
   }
}

void apx_computation_vdelete(void* arg)
{
   apx_computation_delete((apx_computation_t*)arg);
}

void apx_computation_set_range_signed(apx_computation_t* self, int32_t lower_limit, int32_t upper_limit)
{
   if (self != NULL)
   {
      self->lower_limit.i32 = lower_limit;
      self->upper_limit.i32 = upper_limit;
      self->is_signed_range = true;
   }
}

void apx_computation_set_range_unsigned(apx_computation_t* self, uint32_t lower_limit, uint32_t upper_limit)
{
   if (self != NULL)
   {
      self->lower_limit.u32 = lower_limit;
      self->upper_limit.u32 = upper_limit;
      self->is_signed_range = false;
   }
}

void apx_computation_set_upper_limit_signed(apx_computation_t* self, int32_t upper_limit)
{
   if ((self != NULL) && (self->is_signed_range))
   {
      self->upper_limit.i32 = upper_limit;
   }
}

void apx_computation_set_upper_limit_unsigned(apx_computation_t* self, uint32_t upper_limit)
{
   if ((self != NULL) && (!self->is_signed_range))
   {
      self->upper_limit.u32 = upper_limit;
   }
}

int32_t apx_computation_get_lower_limit_signed(apx_computation_t const* self)
{
   if (self != NULL)
   {
      return self->lower_limit.i32;
   }
   return 0u;
}

uint32_t apx_computation_get_lower_limit_unsigned(apx_computation_t const* self)
{
   if (self != NULL)
   {
      return self->lower_limit.u32;
   }
   return 0u;
}

int32_t apx_computation_get_upper_limit_signed(apx_computation_t const* self)
{
   if (self != NULL)
   {
      return self->upper_limit.i32;
   }
   return 0u;
}

uint32_t apx_computation_get_upper_limit_unsigned(apx_computation_t const* self)
{
   if (self != NULL)
   {
      return self->upper_limit.u32;
   }
   return 0u;
}

bool apx_computation_is_range_signed(apx_computation_t const* self)
{
   if (self != NULL)
   {
      return self->is_signed_range;
   }
   return false;
}

adt_str_t* apx_computation_to_string(apx_computation_t const* self)
{
   if (self != NULL)
   {
      if (self->vtable.to_string != NULL)
      {
         return self->vtable.to_string((void*)self);
      }
   }
   return NULL;
}

void apx_computation_assign(apx_computation_t* lhs, apx_computation_t const* rhs)
{
   if ( (lhs != NULL) && (rhs != NULL))
   {
      if (rhs->is_signed_range)
      {
         lhs->is_signed_range = true;
         lhs->lower_limit.i32 = rhs->lower_limit.i32;
         lhs->upper_limit.i32 = rhs->upper_limit.i32;
      }
      else
      {
         lhs->is_signed_range = false;
         lhs->lower_limit.u32 = rhs->lower_limit.u32;
         lhs->upper_limit.u32 = rhs->upper_limit.u32;
      }
   }
}

apx_computation_type_t apx_computation_type(apx_computation_t const* self)
{
   if (self != NULL)
   {
      return self->computation_type;
   }
   return APX_COMPUTATION_TYPE_VALUE_TABLE;
}

//apx_value_table_t API
void apx_value_table_create(apx_value_table_t* self)
{
   if (self != NULL)
   {
      apx_computation_vtable_t vtable;
      apx_computation_vtable_create(&vtable, apx_value_table_vdestroy, apx_value_table_vto_string);
      apx_computation_create(&self->base, &vtable, APX_COMPUTATION_TYPE_VALUE_TABLE);
      adt_ary_create(&self->values, adt_str_vdelete);
   }
}

void apx_value_table_destroy(apx_value_table_t* self)
{
   if (self != NULL)
   {
      adt_ary_destroy(&self->values);
   }
}

apx_value_table_t* apx_value_table_new(void)
{
   apx_value_table_t* self = (apx_value_table_t*)malloc(sizeof(apx_value_table_t));
   if (self != NULL)
   {
      apx_value_table_create(self);
   }
   return self;
}

void apx_value_table_delete(apx_value_table_t* self)
{
   if (self != NULL)
   {
      apx_value_table_destroy(self);
      free(self);
   }
}

apx_value_table_t* apx_value_table_clone(apx_value_table_t const* other)
{
   if (other != NULL)
   {
      apx_value_table_t* self = apx_value_table_new();
      if (self != NULL)
      {
         int32_t i;
         int32_t num_values = adt_ary_length(&other->values);
         apx_computation_assign(&self->base, &other->base);
         for (i = 0; i < num_values; i++)
         {
            adt_str_t* value = adt_str_clone(adt_ary_value(&other->values, i));
            if (value != NULL)
            {
               adt_ary_push(&self->values, value);
            }
            else
            {
               apx_value_table_delete(self);
               self = NULL;
               break;
            }
         }
      }
      return self;
   }
   return NULL;
}

void apx_value_table_set_range_signed(apx_value_table_t* self, int32_t lower_limit, int32_t upper_limit)
{
   if (self != NULL)
   {
      apx_computation_set_range_signed(&self->base, lower_limit, upper_limit);
   }
}

void apx_value_table_set_range_unsigned(apx_value_table_t* self, uint32_t lower_limit, uint32_t upper_limit)
{
   if (self != NULL)
   {
      apx_computation_set_range_unsigned(&self->base, lower_limit, upper_limit);
   }
}

void apx_value_table_set_upper_limit_signed(apx_value_table_t* self, int32_t upper_limit)
{
   if (self != NULL)
   {
      apx_computation_set_upper_limit_signed(&self->base, upper_limit);
   }
}

void apx_value_table_set_upper_limit_unsigned(apx_value_table_t* self, uint32_t upper_limit)
{
   if (self != NULL)
   {
      apx_computation_set_upper_limit_unsigned(&self->base, upper_limit);
   }
}

int32_t apx_value_table_length(apx_value_table_t* self)
{
   if (self != NULL)
   {
      return adt_ary_length(&self->values);
   }
   return -1;
}

apx_error_t apx_value_table_move_values(apx_value_table_t* self, adt_ary_t* values)
{
   if (self != NULL && values != NULL)
   {
      int32_t num_values = adt_ary_length(values);
      if (num_values > 0)
      {
         int32_t i;
         adt_error_t result = adt_ary_resize(&self->values, num_values);
         if (result != ADT_NO_ERROR)
         {
            if (result == ADT_MEM_ERROR)
            {
               return APX_MEM_ERROR;
            }
            else
            {
               return APX_INTERNAL_ERROR;
            }
         }
         for (i = 0; i < num_values; i++)
         {
            void* tmp = adt_ary_value(values, i);
            assert(tmp != NULL);
            adt_ary_set(&self->values, i, tmp);
         }
         adt_ary_destructor_enable(values, false);
         adt_ary_clear(values);
         adt_ary_destructor_enable(values, true);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

adt_str_t* apx_value_table_get_value(apx_value_table_t const* self, int32_t index)
{
   if (self != NULL)
   {
      return (adt_str_t*) adt_ary_value(&self->values, index);
   }
   return NULL;
}

char const* apx_value_table_get_value_cstr(apx_value_table_t const* self, int32_t index)
{
   if (self != NULL)
   {
      adt_str_t* tmp = (adt_str_t*) adt_ary_value(&self->values, index);
      if (tmp != NULL)
      {
         return adt_str_cstr(tmp);
      }
   }
   return NULL;
}

adt_str_t* apx_value_table_to_string(apx_value_table_t const* self)
{
   if (self != NULL)
   {
      adt_str_t* retval = adt_str_new();
      if (retval != NULL)
      {
         adt_str_append_cstr(retval, "VT(");
         adt_str_t* limit_str = computation_limit_to_string(&self->base);
         if (limit_str == NULL)
         {
            adt_str_delete(retval);
            return NULL;
         }
         adt_str_append(retval, limit_str);
         adt_str_delete(limit_str);
         adt_str_push(retval, ',');
      }
      int32_t i;
      int32_t num_values = adt_ary_length(&self->values);
      for (i = 0; i < num_values; i++)
      {
         if (i > 0)
         {
            adt_str_push(retval, ',');
         }
         adt_str_push(retval, '"');
         adt_str_append(retval, apx_value_table_get_value(self, i));
         adt_str_push(retval, '"');
      }
      adt_str_push(retval, ')');
      return retval;
   }
   return NULL;
}

//apx_rational_scaling_t API
apx_error_t apx_rational_scaling_create(apx_rational_scaling_t* self, double offset, int32_t numerator, int32_t denominator, char const* unit)
{
   if (self != NULL)
   {
      apx_computation_vtable_t vtable;
      apx_computation_vtable_create(&vtable, apx_rational_scaling_vdestroy, apx_rational_scaling_vto_string);
      apx_computation_create(&self->base, &vtable, APX_COMPUTATION_TYPE_RATIONAL_SCALING);
      self->offset = offset;
      self->numerator = numerator;
      self->denominator = denominator;
      if (unit != NULL)
      {
         self->unit = STRDUP(unit);
         if (unit == NULL)
         {
            return APX_MEM_ERROR;
         }
      }
      else
      {
         self->unit = NULL;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_rational_scaling_destroy(apx_rational_scaling_t* self)
{
   if (self != NULL)
   {
      if (self->unit != NULL)
      {
         free(self->unit);
      }
   }
}

apx_rational_scaling_t* apx_rational_scaling_new(double offset, int32_t numerator, int32_t denominator, char const* unit)
{
   apx_rational_scaling_t* self = (apx_rational_scaling_t*)malloc(sizeof(apx_rational_scaling_t));
   if (self != NULL)
   {
      apx_error_t result = apx_rational_scaling_create(self, offset, numerator, denominator, unit);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_rational_scaling_delete(apx_rational_scaling_t* self)
{
   if (self != NULL)
   {
      apx_rational_scaling_destroy(self);
      free(self);
   }
}

apx_rational_scaling_t* apx_rational_scaling_clone(apx_rational_scaling_t const* other)
{
   if (other != NULL)
   {
      apx_rational_scaling_t* self = apx_rational_scaling_new(other->offset, other->numerator, other->denominator, other->unit);
      if (self != NULL)
      {
         apx_computation_assign(&self->base, &other->base);
      }
      return self;
   }
   return NULL;
}

void apx_rational_scaling_set_range_signed(apx_rational_scaling_t* self, int32_t lower_limit, int32_t upper_limit)
{
   if (self != NULL)
   {
      apx_computation_set_range_signed(&self->base, lower_limit, upper_limit);
   }
}

void apx_rational_scaling_set_range_unsigned(apx_rational_scaling_t* self, uint32_t lower_limit, uint32_t upper_limit)
{
   if (self != NULL)
   {
      apx_computation_set_range_unsigned(&self->base, lower_limit, upper_limit);
   }
}

adt_str_t* apx_rational_scaling_to_string(apx_rational_scaling_t const* self)
{
   if (self != NULL)
   {
      char buf[TMP_BUF_SIZE];
      adt_str_t* retval = adt_str_new();
      if (retval != NULL)
      {
         adt_str_append_cstr(retval, "RS(");
         adt_str_t* limit_str = computation_limit_to_string(&self->base);
         if (limit_str == NULL)
         {
            adt_str_delete(retval);
            return NULL;
         }
         adt_str_append(retval, limit_str);
         adt_str_delete(limit_str);
         adt_str_push(retval, ',');
         sprintf(buf, "%.8f", self->offset);
         adt_str_append_cstr(retval, buf);
         adt_str_push(retval, ',');
         sprintf(buf, "%d,%d", self->numerator, self->denominator);
         adt_str_append_cstr(retval, buf);
         adt_str_push(retval, ',');
         adt_str_push(retval, '"');
         adt_str_append_cstr(retval, self->unit);
         adt_str_push(retval, '"');
         adt_str_push(retval, ')');
      }
      return retval;
   }
   return NULL;
}

double apx_rational_scaling_offset(apx_rational_scaling_t const* self)
{
   if (self != NULL)
   {
      return self->offset;
   }
   return 0.0;
}

int32_t apx_rational_scaling_numerator(apx_rational_scaling_t const* self)
{
   if (self != NULL)
   {
      return self->numerator;
   }
   return 0;
}

int32_t apx_rational_scaling_denominator(apx_rational_scaling_t const* self)
{
   if (self != NULL)
   {
      return self->denominator;
   }
   return 0;
}

char const* apx_rational_scaling_unit(apx_rational_scaling_t const* self)
{
   {
      if (self != NULL)
      {
         return self->unit;
      }
      return NULL;
   }
}


//apx_computation_list_t
void apx_computation_list_create(apx_computation_list_t* self)
{
   if (self != NULL)
   {
      self->computation_list_id = APX_INVALID_COMPUTATION_LIST_ID;
      adt_ary_create(&self->computations, apx_computation_vdelete);
   }
}

void apx_computation_list_destroy(apx_computation_list_t* self)
{
   if (self != NULL)
   {
      adt_ary_destroy(&self->computations);
   }
}

apx_computation_list_t* apx_computation_list_new(void)
{
   apx_computation_list_t* self = (apx_computation_list_t*)malloc(sizeof(apx_computation_list_t));
   if (self != NULL)
   {
      apx_computation_list_create(self);
   }
   return self;
}

void apx_computation_list_delete(apx_computation_list_t* self)
{
   if (self != NULL)
   {
      apx_computation_list_destroy(self);
      free(self);
   }
}

void apx_computation_list_vdelete(void* arg)
{
   apx_computation_list_delete((apx_computation_list_t*)arg);
}

void apx_computation_list_set_id(apx_computation_list_t* self, apx_computation_list_id_t computation_list_id)
{
   if (self != NULL)
   {
      self->computation_list_id = computation_list_id;
   }
}

apx_error_t apx_computation_list_append_clone_of_computation(apx_computation_list_t* self, apx_computation_t const* computation)
{
   if ( (self != NULL) && (computation != NULL) )
   {
      apx_value_table_t* vt = NULL;
      apx_rational_scaling_t* rs = NULL;
      apx_value_table_t const* vt_tmp = NULL;
      apx_rational_scaling_t const* rs_tmp = NULL;
      switch (computation->computation_type)
      {
      case APX_COMPUTATION_TYPE_VALUE_TABLE:
         vt_tmp = (apx_value_table_t const*)computation;
         vt = apx_value_table_clone(vt_tmp);
         if (vt == NULL)
         {
            return APX_MEM_ERROR;
         }
         adt_ary_push(&self->computations, vt);
         break;
      case APX_COMPUTATION_TYPE_RATIONAL_SCALING:
         rs_tmp = (apx_rational_scaling_t const*)computation;
         rs = apx_rational_scaling_clone(rs_tmp);
         if (rs == NULL)
         {
            return APX_MEM_ERROR;
         }
         adt_ary_push(&self->computations, rs);
         break;
      default:
         return APX_NOT_IMPLEMENTED_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_computation_delete(apx_computation_t* self)
{
   if (self != NULL)
   {
      if (self->vtable.destructor != NULL)
      {
         self->vtable.destructor((void*)self);
      }
      free(self);
   }
}

static void apx_value_table_vdestroy(void* arg)
{
   apx_value_table_destroy((apx_value_table_t*)arg);
}

static adt_str_t* apx_value_table_vto_string(void* arg)
{
   return apx_value_table_to_string((apx_value_table_t*)arg);
}

static void apx_rational_scaling_vdestroy(void* arg)
{
   apx_rational_scaling_destroy((apx_rational_scaling_t*) arg);
}

static adt_str_t* apx_rational_scaling_vto_string(void* arg)
{
   return apx_rational_scaling_to_string((apx_rational_scaling_t*)arg);
}

static adt_str_t* computation_limit_to_string(apx_computation_t const* self)
{
   adt_str_t* retval = adt_str_new();
   if (retval != NULL)
   {
      char buf[TMP_BUF_SIZE];
      if (self->is_signed_range)
      {
         sprintf(buf, "%d,%d", (int)self->lower_limit.i32, (int)self->upper_limit.i32);
      }
      else
      {
         sprintf(buf, "%u,%u", (unsigned int)self->lower_limit.u32, (unsigned int)self->upper_limit.u32);
      }
      adt_str_append_cstr(retval, buf);
   }
   return retval;
}

