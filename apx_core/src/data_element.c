/*****************************************************************************
* \file      apx_data_element.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Data element (parse tree) data structure
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "apx/data_element.h"
#include "apx/error.h"
#include "apx/types.h"
#include "apx/data_type.h"
#include "pack.h"
#include "bstr.h"
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
static apx_error_t derive_hash_init_value(apx_dataElement_t* self, dtl_av_t* parsed_av, dtl_hv_t** derived_hv);
static adt_str_t* limits_to_string(apx_dataElement_t const* self);
static adt_str_t* array_to_string(apx_dataElement_t const* self, bool normalized);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_dataElement_t *apx_dataElement_new(apx_typeCode_t type_code)
{
   apx_dataElement_t *self = (apx_dataElement_t*) malloc(sizeof(apx_dataElement_t));
   if(self != NULL)
   {
      apx_error_t result = apx_dataElement_create(self, type_code);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_dataElement_delete(apx_dataElement_t *self)
{
   if(self != 0)
   {
      apx_dataElement_destroy(self);
      free(self);
   }
}

void apx_dataElement_vdelete(void *arg)
{
   apx_dataElement_delete((apx_dataElement_t*) arg);
}

apx_error_t apx_dataElement_create(apx_dataElement_t *self, apx_typeCode_t type_code)
{
   if (self != NULL)
   {
      self->name = NULL;
      self->type_code = type_code;
      if (type_code == APX_TYPE_CODE_RECORD)
      {
         self->elements = adt_ary_new(apx_dataElement_vdelete);
      }
      else
      {
         self->elements = (adt_ary_t*) NULL;
      }
      self->array_len = 0;
      self->lower_limit.i32 = 0;
      self->upper_limit.i32 = 0;
      self->is_dynamic_array = false;      
      self->has_limits = false;
      self->element_id = APX_INVALID_ELEMENT_ID;
      if (type_code == APX_TYPE_CODE_REF_NAME)
      {
         self->type_ref.name = NULL;
      }
      else
      {
         self->type_ref.id = APX_INVALID_TYPE_ID;
      }
   }
   return APX_NO_ERROR;
}

void apx_dataElement_destroy(apx_dataElement_t *self)
{
   if (self != NULL)
   {
      if (self->name != NULL)
      {
         free(self->name);
      }      
      if (self->elements != NULL)
      {
         adt_ary_delete(self->elements);
      }
      if ( (self->type_code == APX_TYPE_CODE_REF_NAME) && (self->type_ref.name != NULL) )
      {
         free(self->type_ref.name);
      }
   }
}

apx_dataElement_t* apx_dataElement_clone(apx_dataElement_t* self)
{
   apx_dataElement_t* clone = apx_dataElement_new(self->type_code);
   if (clone != NULL)
   {
      clone->array_len = self->array_len;
      clone->element_id = self->element_id;
      clone->is_dynamic_array = self->is_dynamic_array;
      if (self->name != NULL)
      {
         clone->name = STRDUP(self->name);
         if (clone->name == NULL)
         {
            apx_dataElement_delete(clone);
            return NULL;
         }
      }
      switch (self->type_code)
      {
      case APX_TYPE_CODE_REF_ID:
         clone->type_ref.id = self->type_ref.id;
         break;
      case APX_TYPE_CODE_REF_NAME:
         clone->type_ref.name = STRDUP(self->type_ref.name);
         if (clone->type_ref.name == NULL)
         {
            apx_dataElement_delete(clone);
            return NULL;
         }
         break;
      case APX_TYPE_CODE_REF_PTR:
         clone->type_ref.ptr = self->type_ref.ptr;
         break;
      default:
         {}
      //Default value already set in constructor
      }
      
      if (self->has_limits)
      {
         clone->has_limits = true;
         switch (self->type_code)
         {
         case APX_TYPE_CODE_UINT8:
         case APX_TYPE_CODE_UINT16:
         case APX_TYPE_CODE_UINT32:
         case APX_TYPE_CODE_CHAR8:
         case APX_TYPE_CODE_CHAR16:
         case APX_TYPE_CODE_CHAR32:
            clone->lower_limit.u32 = self->lower_limit.u32;
            clone->upper_limit.u32 = self->upper_limit.u32;
            break;
         case APX_TYPE_CODE_INT8:
         case APX_TYPE_CODE_INT16:
         case APX_TYPE_CODE_INT32:
         case APX_TYPE_CODE_CHAR:
            clone->lower_limit.i32 = self->lower_limit.i32;
            clone->upper_limit.i32 = self->upper_limit.i32;
            break;
         case APX_TYPE_CODE_UINT64:
            clone->lower_limit.u64 = self->lower_limit.u64;
            clone->upper_limit.u64 = self->upper_limit.u64;
            break;
         case APX_TYPE_CODE_INT64:
            clone->lower_limit.i64 = self->lower_limit.i64;
            clone->upper_limit.i64 = self->upper_limit.i64;
            break;
         default:
            {} //Other elements cannot have limits
         }
      }
      if (self->type_code == APX_TYPE_CODE_RECORD)
      {
         int32_t i;
         int32_t num_elements = apx_dataElement_get_num_child_elements(self);
         for (i = 0; i < num_elements; i++)
         {
            apx_dataElement_t* child_clone;
            apx_dataElement_t* child_element = apx_dataElement_get_child_at(self, i);
            child_clone = apx_dataElement_clone(child_element);
            if (child_clone != NULL)
            {
               adt_ary_push(clone->elements, child_clone);
            }
            else
            {
               apx_dataElement_delete(clone);
               return NULL;
            }
         }
      }
   }
   return clone;
}

apx_error_t apx_dataElement_set_name_bstr(apx_dataElement_t* self, const uint8_t* begin, const uint8_t* end)
{
   if (self != NULL)
   {
      self->name = bstr_make_cstr(begin, end);
      if (self->name == NULL)
      {
         return APX_MEM_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_dataElement_set_name_cstr(apx_dataElement_t* self, const char* name)
{
   if (self != NULL)
   {
      self->name = STRDUP(name);
      if (self->name == NULL)
      {
         return APX_MEM_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

const char* apx_dataElement_get_name(apx_dataElement_t const* self)
{
   if (self != NULL)
   {
      return self->name;
   }
   return NULL;
}

apx_typeCode_t apx_dataElement_get_type_code(apx_dataElement_t const* self)
{
   if (self != NULL)
   {
      return self->type_code;
   }
   return APX_TYPE_CODE_NONE;
}

bool apx_dataElement_has_limits(apx_dataElement_t const* self)
{
   if (self != NULL)
   {
      return self->has_limits;
   }
   return false;
}


void apx_dataElement_init_record_type(apx_dataElement_t* self)
{
   self->type_code=APX_TYPE_CODE_RECORD;
   if (self->elements != 0)
   {
      adt_ary_delete(self->elements);
   }
   self->elements = adt_ary_new(apx_dataElement_vdelete);
}

apx_error_t apx_dataElement_set_array_length(apx_dataElement_t* self, uint32_t array_len)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self != 0)
   {
      self->array_len = array_len;
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

uint32_t apx_dataElement_get_array_length(apx_dataElement_t const* self)
{
   if (self != 0)
   {
      return self->array_len;
   }
   return 0;
}

bool apx_dataElement_is_array(apx_dataElement_t const* self)
{
   if (self != 0)
   {
      return (bool) (self->array_len > 0u);
   }
   return false;
}

void apx_dataElement_set_dynamic_array(apx_dataElement_t* self)
{   
   if (self != 0)
   {
      self->is_dynamic_array = true;      
   }
}

bool apx_dataElement_is_dynamic_array(apx_dataElement_t const* self)
{
   if (self != 0)
   {
      return self->is_dynamic_array;
   }
   return false;
}


void apx_dataElement_set_type_ref_id(apx_dataElement_t* self, apx_typeId_t type_id)
{
   if (self != 0)
   {
      if ( (self->type_code == APX_TYPE_CODE_REF_NAME) && (self->type_ref.name != 0) )
      {
         free(self->type_ref.name);
      }
      if (self->type_code != APX_TYPE_CODE_REF_ID)
      {
         self->type_code = APX_TYPE_CODE_REF_ID;
      }
      self->type_ref.id = type_id;
   }
}

apx_typeId_t apx_dataElement_get_type_ref_id(apx_dataElement_t const* self)
{
   if ( (self != 0) && (self->type_code == APX_TYPE_CODE_REF_ID))
   {
      return self->type_ref.id;
   }
   return APX_INVALID_TYPE_ID;
}

apx_error_t apx_dataElement_set_type_ref_name_bstr(apx_dataElement_t* self, const uint8_t* begin, const uint8_t* end)
{
   if (self != 0)
   {
      if ( (self->type_code == APX_TYPE_CODE_REF_NAME) && (self->type_ref.name != 0) )
      {
         free(self->type_ref.name);
      }
      if (self->type_code != APX_TYPE_CODE_REF_NAME)
      {
         self->type_code = APX_TYPE_CODE_REF_NAME;
      }
      self->type_ref.name = bstr_make_cstr(begin, end);
      if (self->type_ref.name == NULL)
      {
         return APX_MEM_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}



const char * apx_dataElement_get_type_ref_name(apx_dataElement_t const* self)
{
   if ( (self != 0) && (self->type_code == APX_TYPE_CODE_REF_NAME) )
   {
      return self->type_ref.name;
   }
   return (const char*) 0;
}

void apx_dataElement_set_type_ref_ptr(apx_dataElement_t *self, struct apx_dataType_tag *ptr)
{
   if (self != 0)
   {
      if ( (self->type_code == APX_TYPE_CODE_REF_NAME) && (self->type_ref.name != 0) )
      {
         free(self->type_ref.name);
      }
      if (self->type_code != APX_TYPE_CODE_REF_PTR)
      {
         self->type_code = APX_TYPE_CODE_REF_PTR;
      }
      self->type_ref.ptr = ptr;
   }
}

apx_dataType_t * apx_dataElement_get_type_ref_ptr(apx_dataElement_t const* self)
{
   if ( (self != 0) && (self->type_code == APX_TYPE_CODE_REF_PTR) )
   {
      return self->type_ref.ptr;
   }
   return (apx_dataType_t*) 0;
}

void apx_dataElement_append_child(apx_dataElement_t* self, apx_dataElement_t* child)
{
   if( (self != 0) && (child != 0) && (self->type_code == APX_TYPE_CODE_RECORD))
   {
      if (self->elements == NULL)
      {
         self->elements = adt_ary_new(apx_dataElement_vdelete);
      }
      adt_ary_push(self->elements, child);
   }
}

int32_t apx_dataElement_get_num_child_elements(apx_dataElement_t const* self)
{
   if ( (self != 0) && (self->elements != 0) )
   {
      return adt_ary_length(self->elements);
   }
   return -1;
}

apx_dataElement_t* apx_dataElement_get_child_at(apx_dataElement_t const* self, int32_t index)
{
   if ( (self != 0) && (self->elements != 0) )
   {
      void **ptr = adt_ary_get(self->elements, index);
      if (ptr != 0)
      {
         return (apx_dataElement_t*) *ptr;
      }
   }
   return 0;
}

void apx_dataElement_set_limits_int32(apx_dataElement_t* self, int32_t lower, int32_t upper)
{
   if (self != NULL)
   {
      self->lower_limit.i32 = lower;
      self->upper_limit.i32 = upper;
      self->has_limits = true;
   }
}

void apx_dataElement_set_limits_int64(apx_dataElement_t* self, int64_t lower, int64_t upper)
{
   if (self != NULL)
   {
      self->lower_limit.i64 = lower;
      self->upper_limit.i64 = upper;
      self->has_limits = true;
   }
}

void apx_dataElement_set_limits_uint32(apx_dataElement_t* self, uint32_t lower, uint32_t upper)
{
   if (self != NULL)
   {
      self->lower_limit.u32 = lower;
      self->upper_limit.u32 = upper;
      self->has_limits = true;
   }
}

void apx_dataElement_set_limits_uint64(apx_dataElement_t* self, uint64_t lower, uint64_t upper)
{
   if (self != NULL)
   {
      self->lower_limit.u64 = lower;
      self->upper_limit.u64 = upper;
      self->has_limits = true;
   }
}

bool apx_dataElement_get_limits_int32(apx_dataElement_t const* self, int32_t* lower, int32_t* upper)
{
   if ( (self != NULL) && (lower != NULL) && (upper != NULL) && self->has_limits)
   {
      *lower = self->lower_limit.i32;
      *upper = self->upper_limit.i32;
      return true;
   }
   return false;
}

bool apx_dataElement_get_limits_int64(apx_dataElement_t const* self, int64_t* lower, int64_t* upper)
{
   if ((self != NULL) && (lower != NULL) && (upper != NULL) && self->has_limits)
   {
      *lower = self->lower_limit.i64;
      *upper = self->upper_limit.i64;
      return true;
   }
   return false;
}

bool apx_dataElement_get_limits_uint32(apx_dataElement_t const* self, uint32_t* lower, uint32_t* upper)
{
   if ((self != NULL) && (lower != NULL) && (upper != NULL) && self->has_limits)
   {
      *lower = self->lower_limit.u32;
      *upper = self->upper_limit.u32;
      return true;
   }
   return false;
}

bool apx_dataElement_get_limits_uint64(apx_dataElement_t const* self, uint64_t* lower, uint64_t* upper)
{
   if ((self != NULL) && (lower != NULL) && (upper != NULL) && self->has_limits)
   {
      *lower = self->lower_limit.u64;
      *upper = self->upper_limit.u64;
      return true;
   }
   return false;
}

apx_error_t apx_dataElement_derive_types_on_element(apx_dataElement_t* self, adt_ary_t const* type_list, adt_hash_t const* type_map)
{
   if ( (self != NULL) && (type_list != NULL) && (type_map != NULL) )
   {
      apx_typeCode_t const type_code = self->type_code;
      if (type_code == APX_TYPE_CODE_RECORD)
      {
         apx_error_t result = APX_NO_ERROR;
         int32_t i;
         int32_t num_elements = apx_dataElement_get_num_child_elements(self);
         for (i = 0; i < num_elements; i++)
         {
            apx_dataElement_t* child_element = apx_dataElement_get_child_at(self, i);            
            if (child_element == NULL)
            {
               return APX_NULL_PTR_ERROR;
            }
            result = apx_dataElement_derive_types_on_element(child_element, type_list, type_map);            
            if (result != APX_NO_ERROR)
            {
               return result;
            }
         }
      }
      else if (type_code == APX_TYPE_CODE_REF_ID)
      {
         apx_dataType_t* data_type = NULL;
         apx_error_t result = APX_NO_ERROR;
         int32_t type_index = (int32_t) apx_dataElement_get_type_ref_id(self);         
         if ((type_index < 0) || (type_index > adt_ary_length(type_list)))
         {
            return APX_INVALID_TYPE_REF_ERROR;
         }
         data_type = adt_ary_value(type_list, type_index);
         if (data_type == NULL)
         {
            return APX_NULL_PTR_ERROR;
         }
         result = apx_dataType_derive_types_on_element(data_type, type_list, type_map);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
         apx_dataElement_set_type_ref_ptr(self, (void*) data_type);
      }
      else if (type_code == APX_TYPE_CODE_REF_NAME)
      {
         apx_dataType_t* data_type = NULL;         
         char const* type_name = apx_dataElement_get_type_ref_name(self);
         data_type = (apx_dataType_t*)adt_hash_value(type_map, type_name);         
         if (data_type != NULL)
         {            
            apx_error_t result = apx_dataType_derive_types_on_element(data_type, type_list, type_map);            
            if (result != APX_NO_ERROR)
            {
               return result;
            }
            apx_dataElement_set_type_ref_ptr(self, (void*)data_type);
         }
         else
         {
            return APX_INVALID_TYPE_REF_ERROR;
         }
      }
      else
      {
         //Type already derived
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_dataElement_derive_proper_init_value(apx_dataElement_t* self, dtl_dv_t* parsed_value, dtl_dv_t** derived_value)
{
   apx_typeCode_t type_code;
   if ((self == NULL) || (parsed_value == NULL) || (derived_value == NULL))
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   type_code = self->type_code;
   if ((type_code == APX_TYPE_CODE_NONE) ||
      (type_code == APX_TYPE_CODE_REF_ID) ||
      (type_code == APX_TYPE_CODE_REF_NAME) ||
      (type_code == APX_TYPE_CODE_REF_PTR))
   {
      return APX_UNSUPPORTED_ERROR;
   }

   if (type_code == APX_TYPE_CODE_RECORD)
   {
      if (apx_dataElement_is_array(self)) //Array of records?
      {
         if (dtl_dv_type(parsed_value) == DTL_DV_ARRAY)
         {
            dtl_av_t* parsed_av = (dtl_av_t*)parsed_value;
            if (apx_dataElement_is_dynamic_array(self))
            {
               if (dtl_av_length(parsed_av) != 0)
               {
                  return APX_NOT_IMPLEMENTED_ERROR;
               }
               else
               {
                  *derived_value = (dtl_dv_t*)dtl_av_new();
               }
            }
            else
            {
               dtl_av_t* derived_av = dtl_av_new();
               if (((uint32_t)dtl_av_length(parsed_av)) != self->array_len)
               {
                  return APX_VALUE_LENGTH_ERROR;
               }
               for (uint32_t i = 0u; i < self->array_len; i++)
               {
                  dtl_dv_t* parsed_child_dv = dtl_av_value(parsed_av, (int32_t)i);
                  if (dtl_dv_type(parsed_child_dv) == DTL_DV_ARRAY)
                  {
                     dtl_av_t* parsed_child_av = (dtl_av_t*)parsed_child_dv;
                     dtl_hv_t* derived_hv = NULL;
                     apx_error_t result;
                     assert(parsed_child_av != NULL);
                     result = derive_hash_init_value(self, parsed_child_av, &derived_hv);
                     if (result == APX_NO_ERROR)
                     {
                        assert(derived_hv != NULL);
                        dtl_av_push(derived_av, (dtl_dv_t*)derived_hv, false);
                     }
                     else
                     {
                        return result;
                     }
                  }
               }
               *derived_value = (dtl_dv_t*)derived_av;
            }
         }
         else
         {
            return APX_VALUE_TYPE_ERROR;
         }
      }
      else if (dtl_dv_type(parsed_value) == DTL_DV_ARRAY)
      {
         dtl_av_t* parsed_av = (dtl_av_t*)parsed_value;
         dtl_hv_t* derived_hv = NULL;
         apx_error_t result = derive_hash_init_value(self, parsed_av, &derived_hv);
         if (result == APX_NO_ERROR)
         {
            assert(derived_hv != NULL);
            *derived_value = (dtl_dv_t*)derived_hv;
         }
         else
         {
            assert(derived_hv == NULL);
            return result;
         }
      }
      else
      {
         return APX_VALUE_TYPE_ERROR;
      }
   }
   else if (apx_dataElement_is_array(self))
   {
      if (dtl_dv_type(parsed_value) == DTL_DV_ARRAY)
      {
         dtl_av_t* parsed_av = (dtl_av_t*)parsed_value;
         if (apx_dataElement_is_dynamic_array(self))
         {
            if (dtl_av_length(parsed_av) == 0u)
            {
               *derived_value = (dtl_dv_t*)dtl_av_new();
            }
            else
            {
               return APX_UNSUPPORTED_ERROR; //Only empty initializers are supported for dynamic arrays
            }
         }
         else
         {
            if (self->array_len == (uint32_t)dtl_av_length(parsed_av))
            {
               dtl_av_t* derived_av = dtl_av_new();
               for (uint32_t i = 0; i < self->array_len; i++)
               {
                  dtl_dv_t* parsed_child_dv = dtl_av_value(parsed_av, i);
                  if (dtl_dv_type(parsed_child_dv) == DTL_DV_SCALAR)
                  {
                     dtl_av_push(derived_av, parsed_child_dv, true);
                  }
                  else
                  {
                     dtl_av_delete(derived_av);
                     return APX_VALUE_TYPE_ERROR;
                  }
               }
               *derived_value = (dtl_dv_t*)derived_av;
            }
            else
            {
               return APX_VALUE_LENGTH_ERROR;
            }
         }
      }
      else if ((dtl_dv_type(parsed_value) == DTL_DV_SCALAR) &&
         ((type_code == APX_TYPE_CODE_CHAR) || (type_code == APX_TYPE_CODE_CHAR8)))
      {
         dtl_sv_t* parsed_sv = (dtl_sv_t*)parsed_value;
         if (dtl_sv_type(parsed_sv) == DTL_SV_STR)
         {
            *derived_value = (dtl_dv_t*)parsed_sv;
            dtl_inc_ref(parsed_value);
         }
         else
         {
            return APX_VALUE_TYPE_ERROR;
         }
      }
      else
      {
         return APX_VALUE_TYPE_ERROR;
      }
   }
   else
   {
      if (dtl_dv_type(parsed_value) == DTL_DV_SCALAR)
      {
         *derived_value = parsed_value;
         dtl_inc_ref(parsed_value);
      }
      else
      {
         return APX_VALUE_TYPE_ERROR;
      }
   }
   return APX_NO_ERROR;
}

apx_error_t apx_dataElement_derive_data_element(apx_dataElement_t const* self, apx_dataElement_t** data_element, apx_dataElement_t** parent)
{   
   apx_error_t retval = APX_NO_ERROR;
   if ( (self == NULL) || (data_element == NULL) ) //parent argument is optional
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   apx_typeCode_t type_code = self->type_code;
   *data_element = (apx_dataElement_t*) self; //Initial guess, might change later
   if ((type_code == APX_TYPE_CODE_REF_ID) || (type_code == APX_TYPE_CODE_REF_NAME))
   {
      return APX_UNSUPPORTED_ERROR;
   }

   if (type_code == APX_TYPE_CODE_REF_PTR)
   {
      apx_dataType_t* data_type = apx_dataElement_get_type_ref_ptr(self);
      if (data_type != NULL)
      {
         retval = apx_dataType_derive_data_element(data_type, data_element, parent);
         assert(*data_element != NULL);
      }
      else
      {
         retval = APX_NULL_PTR_ERROR;
      }
   }
   return retval;
}

void apx_dataElement_set_id(apx_dataElement_t* self, apx_elementId_t id)
{
   if (self != NULL)
   {
      self->element_id = id;
   }
}

apx_elementId_t apx_dataElement_get_id(apx_dataElement_t const* self)
{
   if (self != NULL)
   {
      return self->element_id;
   }
   return APX_INVALID_ELEMENT_ID;
}

adt_str_t* apx_dataElement_to_string(apx_dataElement_t const* self, bool normalized)
{
   if (self != NULL)
   {
      adt_str_t* str = adt_str_new();
      if (self->type_code == APX_TYPE_CODE_RECORD)
      {
         int32_t i;
         int32_t num_elements = adt_ary_length(self->elements);
         adt_str_push(str, '{');
         for (i=0; i < num_elements; i++)
         {
            apx_dataElement_t* child_element = (apx_dataElement_t*) adt_ary_value(self->elements, i);
            adt_str_push(str, '"');
            adt_str_append_cstr(str,  apx_dataElement_get_name(child_element));
            adt_str_push(str, '"');
            adt_str_t* child_signature = apx_dataElement_to_string(child_element, normalized);
            if (child_signature == NULL)
            {
               adt_str_delete(str);
               return NULL;
            }
            adt_str_append(str, child_signature);
            adt_str_delete(child_signature);
         }
         adt_str_push(str, '}');
      }
      else
      {
         char type_code = '\0';
         switch (self->type_code)
         {
         case APX_TYPE_CODE_UINT8:
            type_code = 'C';
            break;
         case APX_TYPE_CODE_UINT16:
            type_code = 'S';
            break;
         case APX_TYPE_CODE_UINT32:
            type_code = 'L';
            break;
         case APX_TYPE_CODE_UINT64:
            type_code = 'Q';
            break;
         case APX_TYPE_CODE_INT8:
            type_code = 'c';
            break;
         case APX_TYPE_CODE_INT16:
            type_code = 's';
            break;
         case APX_TYPE_CODE_INT32:
            type_code = 'l';
            break;
         case APX_TYPE_CODE_INT64:
            type_code = 'q';
            break;
         case APX_TYPE_CODE_CHAR:
            type_code = 'a';
            break;
         case APX_TYPE_CODE_CHAR8:
            type_code = 'A';
            break;
         case APX_TYPE_CODE_CHAR16:
            type_code = 'u';
            break;
         case APX_TYPE_CODE_CHAR32:
            type_code = 'U';
            break;
         case APX_TYPE_CODE_BOOL:
            type_code = 'b';
            break;
         case APX_TYPE_CODE_BYTE:
            type_code = 'B';
            break;
         }
         if (type_code == '\0')
         {
            adt_str_delete(str);
            return NULL;
         }
         adt_str_push(str, type_code);
         if (apx_dataElement_has_limits(self))
         {
            adt_str_t* limits_str = limits_to_string(self);
            if (limits_str == NULL)
            {
               adt_str_delete(str);
               return NULL;
            }
            adt_str_append(str, limits_str);
            adt_str_delete(limits_str);
         }
      }
      if (apx_dataElement_is_array(self))
      {
         adt_str_t* array_str = array_to_string(self, normalized);
         if (array_str == NULL)
         {
            adt_str_delete(str);
            return NULL;
         }
         adt_str_append(str, array_str);
         adt_str_delete(array_str);
      }
      return str;
   }
   return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t derive_hash_init_value(apx_dataElement_t* self, dtl_av_t* parsed_av, dtl_hv_t** derived_hv)
{
   dtl_hv_t* hv;
   int32_t num_children = apx_dataElement_get_num_child_elements(self);
   hv = dtl_hv_new();
   if (hv == NULL)
   {
      return APX_MEM_ERROR;
   }
   if (num_children != dtl_av_length(parsed_av))
   {
      return APX_VALUE_LENGTH_ERROR;
   }
   for (int32_t i = 0; i < num_children; i++)
   {
      apx_error_t result;
      apx_dataElement_t* derived_element = NULL;      
      dtl_dv_t* parsed_dv = NULL;
      dtl_dv_t* derived_dv = NULL;
      apx_dataElement_t* child_element = apx_dataElement_get_child_at(self, i);
      char const* child_name = NULL;
      assert(child_element != NULL);
      child_name = apx_dataElement_get_name(child_element);
      assert(child_name != NULL);
      result = apx_dataElement_derive_data_element(child_element, &derived_element, NULL);
      if (result != APX_NO_ERROR)
      {
         dtl_hv_delete(hv);
         return result;
      }
      assert(derived_element != NULL);
      parsed_dv = dtl_av_value(parsed_av, i);
      result = apx_dataElement_derive_proper_init_value(derived_element, parsed_dv, &derived_dv);
      if (result != APX_NO_ERROR)
      {
         assert(derived_dv == NULL);
         dtl_hv_delete(hv);
         return result;
      }      
      dtl_hv_set_cstr(hv, child_name, derived_dv, false);
   }
   *derived_hv = hv;
   return APX_NO_ERROR;
}

static adt_str_t* limits_to_string(apx_dataElement_t const* self)
{
   adt_str_t* str = adt_str_new();
   char buf[TMP_BUF_SIZE];
   if (str != NULL)
   {
      switch (self->type_code)
      {
      case APX_TYPE_CODE_UINT8:         
      case APX_TYPE_CODE_UINT16:
      case APX_TYPE_CODE_UINT32:
      case APX_TYPE_CODE_BYTE:
         sprintf(buf, "(%u,%u)", (unsigned int)self->lower_limit.u32, (unsigned int)self->upper_limit.u32);
         break;
      case APX_TYPE_CODE_UINT64:
         sprintf(buf, "(%llu,%llu)", (unsigned long long)self->lower_limit.u64, (unsigned long long)self->upper_limit.u64);
         break;
      case APX_TYPE_CODE_INT8:         
      case APX_TYPE_CODE_INT16:
      case APX_TYPE_CODE_INT32:
         sprintf(buf, "(%d,%d)", (int)self->lower_limit.i32, (int)self->upper_limit.i32);
         break;
      case APX_TYPE_CODE_INT64:
         sprintf(buf, "(%lld,%lld)", (long long)self->lower_limit.i64, (long long)self->upper_limit.i64);
         break;
      default:
         adt_str_delete(str);
         return NULL;
      }
      adt_str_append_cstr(str, buf);
   }
   return str;
}

static adt_str_t* array_to_string(apx_dataElement_t const* self, bool normalized)
{
   adt_str_t* str = adt_str_new();
   char buf[TMP_BUF_SIZE];
   if (str != NULL)
   {
      adt_error_t result = ADT_NO_ERROR;      
      if (self->is_dynamic_array && normalized)
      {
         result = adt_str_append_cstr(str, "[*]");
      }
      else
      {
         char const* format = (self->is_dynamic_array) ? "[%u*]" : "[%u]";
         sprintf(buf, format, (unsigned int)self->array_len);
         result = adt_str_append_cstr(str, buf);
      }
      if (result != ADT_NO_ERROR)
      {
         adt_str_delete(str);
         return NULL;
      }
   }
   return str;
}
