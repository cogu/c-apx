/*****************************************************************************
* \file      attribute_parser.c
* \author    Conny Gustafsson
* \date      2017-07-30
* \brief     Port attribute parser
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
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "apx/attribute_parser.h"
#include "apx/parser_base.h"
#include "apx/computation.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_attributeParser_set_error(apx_attributeParser_t* self, apx_error_t error, uint8_t const* error_next);
static void apx_attributeParser_reset(apx_attributeParser_t* self);
apx_valueTable_t* apx_attributeParser_create_value_table_from_state(apx_attributeParser_t* self, apx_attributeParserValueTableState_t* vts);
apx_rationalScaling_t* apx_attributeParser_create_rational_scaling_from_state(apx_attributeParserRationalScalingState_t* vts);
static uint8_t const* apx_attributeParser_parse_single_port_attribute(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_portAttributes_t* attr);
static uint8_t const* apx_attributeParser_parse_single_type_attribute(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_typeAttributes_t* attr);
static uint8_t const* apx_attributeParser_parse_initializer_list(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end);
static bool apx_attributeParser_push_value_to_parent(apx_attributeParser_t* self);
static uint8_t const* apx_attributeParser_parse_scalar(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end);
static uint8_t const* apx_attributeParser_parse_integer_literal(uint8_t const* next, uint8_t const* end, bool unary_minus, dtl_sv_t* sv);
static uint8_t const* apx_attributeParser_parse_string_literal(uint8_t const* begin, uint8_t const* end, dtl_sv_t* sv);
static uint8_t const* apx_attributeParser_parse_array_length(uint8_t const* begin, uint8_t const* end, uint32_t* length);
static uint8_t const* apx_attributeParser_parse_value_table(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_valueTable_t** vt);
static uint8_t const* apx_attributeParser_parse_rational_scaling(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_rationalScaling_t** rs);
static uint8_t const* apx_attributeParser_parse_value_table_arg(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_attributeParserValueTableState_t* vts);
static uint8_t const* apx_attributeParser_parse_rational_scaling_arg(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_attributeParserRationalScalingState_t* rss);
static uint8_t const* apx_attributeParser_parse_lower_limit(uint8_t const* begin, uint8_t const* end, bool unary_minus, apx_range_t* range);
static uint8_t const* apx_attributeParser_parse_upper_limit(uint8_t const* begin, uint8_t const* end, bool unary_minus, apx_range_t* range);
static bool apx_attributeParser_is_initializer_list(apx_attributeParser_t* self);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//apx_attributeParseState_t API
void apx_attributeParseState_create(apx_attributeParseState_t* self)
{
   if (self != NULL)
   {
      self->parent = NULL;
      self->initializer_list = NULL;
      self->sv = NULL;
   }
}

void apx_attributeParseState_destroy(apx_attributeParseState_t* self)
{
   if (self != NULL)
   {
      if (self->sv != NULL)
      {
         dtl_dec_ref(self->sv);
      }
      if (self->initializer_list != NULL)
      {
         dtl_dec_ref(self->initializer_list);
      }
   }
}

apx_attributeParseState_t* apx_attributeParseState_new(void)
{
   apx_attributeParseState_t* self = (apx_attributeParseState_t*)malloc(sizeof(apx_attributeParseState_t));
   if (self != NULL)
   {
      apx_attributeParseState_create(self);
   }
   return self;
}

void apx_attributeParseState_delete(apx_attributeParseState_t* self)
{
   if (self != NULL)
   {
      apx_attributeParseState_destroy(self);
      free(self);
   }
}

void apx_attributeParseState_vdelete(void* arg)
{
   apx_attributeParseState_delete((apx_attributeParseState_t*)arg);
}


bool apx_attributeParseState_has_value(apx_attributeParseState_t* self)
{
   if (self != NULL)
   {
      return ((self->sv != NULL) || (self->initializer_list != NULL)) ? true : false;
   }
   return false;
}

//apx_range_t API
void apx_range_create(apx_range_t* self)
{
   if (self != NULL)
   {
      self->is_signed_range = false;
      self->lower.i32 = 0u;
      self->upper.i32 = 0u;
   }
}

//apx_attributeParserValueTableState_t API
void apx_attributeParserValueTableState_create(apx_attributeParserValueTableState_t* self)
{
   if (self != NULL)
   {
      apx_range_create(&self->range);
      self->num_count = 0u;
      self->last_was_string = false;
      adt_ary_create(&self->values, adt_str_vdelete);
   }
}

void apx_attributeParserValueTableState_destroy(apx_attributeParserValueTableState_t* self)
{
   if (self != NULL)
   {
      adt_ary_destroy(&self->values);
   }
}

void apx_attributeParserValueTableState_append(apx_attributeParserValueTableState_t* self, adt_str_t* str)
{
   if ((self != NULL) && (str != NULL))
   {
      adt_error_t result = adt_ary_push(&self->values, (void*)str);
      if (result != ADT_NO_ERROR)
      {
         //TODO: error handling
      }
   }
}

int32_t apx_attributeParserValueTableState_length(apx_attributeParserValueTableState_t* self)
{
   if (self != NULL)
   {
      return adt_ary_length(&self->values);
   }
   return -1;
}

//apx_attributeParserRationalScalingState_t API
void apx_attributeParserRationalScalingState_create(apx_attributeParserRationalScalingState_t* self)
{
   if (self != NULL)
   {
      apx_range_create(&self->range);
      self->offset = 0.0;
      self->numerator = 0;
      self->denominator = 0;
      self->arg_index = 0u;
      self->unit = NULL;
   }
}

void apx_attributeParserRationalScalingState_destroy(apx_attributeParserRationalScalingState_t* self)
{
   if (self != NULL)
   {
      if (self->unit != NULL)
      {
         free(self->unit);
      }
   }
}

//apx_attributeParser_t API
void apx_attributeParser_create(apx_attributeParser_t* self)
{
   if (self != NULL)
   {
      adt_stack_create(&self->stack, apx_attributeParseState_vdelete);
      self->state = NULL;
      self->last_error = APX_NO_ERROR;
      self->error_next = NULL;
   }
}

void apx_attributeParser_destroy(apx_attributeParser_t* self)
{
   if (self != NULL)
   {
      apx_attributeParser_reset(self);
   }
}

uint8_t const* apx_attributeParser_parse_port_attributes(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_portAttributes_t* attr)
{
   uint8_t const* next = begin;
   if ((end < begin) || (begin == NULL) || (end == NULL))
   {
      apx_attributeParser_set_error(self, APX_INVALID_ARGUMENT_ERROR, NULL);
      return NULL;
   }
   while (next < end)
   {
      next = bstr_lstrip(next, end);
      if (next == end)
      {
         break;
      }
      uint8_t const* result = apx_attributeParser_parse_single_port_attribute(self, next, end, attr);
      if (result > next)
      {
         next = bstr_lstrip(result, end);
         if (next == end)
         {
            break;
         }
         if (*next == ',')
         {
            next++;
         }
         else
         {
            apx_attributeParser_set_error(self, APX_PARSE_ERROR, next);
            next = NULL;
            break;
         }
      }
      else
      {
         break;
      }
   }
   return next;

}

uint8_t const* apx_attributeParser_parse_type_attributes(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_typeAttributes_t* attr)
{
   uint8_t const* next = begin;
   if ((end < begin) || (begin == NULL) || (end == NULL))
   {
      apx_attributeParser_set_error(self, APX_INVALID_ARGUMENT_ERROR, begin);
      return NULL;
   }
   while (next < end)
   {
      next = bstr_lstrip(next, end);
      if (next == end)
      {
         break;
      }
      uint8_t const* result = apx_attributeParser_parse_single_type_attribute(self, next, end, attr);
      if (result > next)
      {
         next = bstr_lstrip(result, end);
         if (next == end)
         {
            break;
         }
         if (*next == ',')
         {
            next++;
         }
         else
         {
            apx_attributeParser_set_error(self, APX_PARSE_ERROR, next);
            next = NULL;
            break;
         }
      }
      else
      {
         break;
      }
   }
   return next;
}

uint8_t const* apx_attributeParser_parse_initializer(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, dtl_dv_t** dv)
{
   if ((self != NULL) && (begin != NULL) && (end != NULL) && (begin <= end) && (dv != NULL))
   {
      uint8_t const* next;
      apx_attributeParser_reset(self);
      self->state = apx_attributeParseState_new();
      if (self->state == NULL)
      {
         apx_attributeParser_set_error(self, APX_MEM_ERROR, begin);
      }
      next = apx_attributeParser_parse_initializer_list(self, begin, end);
      if (next > begin)
      {
         assert(next <= end);
         if (self->state->initializer_list != NULL)
         {
            *dv = (dtl_dv_t*)self->state->initializer_list;
            self->state->initializer_list = NULL;
         }
         else if (self->state->sv != NULL)
         {
            *dv = (dtl_dv_t*)self->state->sv;
            self->state->sv = NULL;
         }
      }
      if (next == NULL)
      {
         apx_attributeParser_set_error(self, APX_PARSE_ERROR, begin);
      }
      apx_attributeParseState_delete(self->state);
      self->state = NULL;
      return next;
   }
   return NULL;
}

apx_error_t apx_attributeParser_get_last_error(apx_attributeParser_t* self, uint8_t const** error_next)
{
   if (self != NULL)
   {
      if (error_next != NULL)
      {
         *error_next = self->error_next;
      }
      return self->last_error;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_attributeParser_set_error(apx_attributeParser_t* self, apx_error_t error, uint8_t const* error_next)
{
   assert(self != NULL);
   self->last_error = error;
   self->error_next = error_next;
}

static void apx_attributeParser_reset(apx_attributeParser_t* self)
{
   assert(self != NULL);
   self->last_error = APX_NO_ERROR;
   self->error_next = NULL;
   adt_stack_clear(&self->stack);
   if (self->state != NULL)
   {
      apx_attributeParseState_delete(self->state);
      self->state = NULL;
   }
}


apx_valueTable_t* apx_attributeParser_create_value_table_from_state(apx_attributeParser_t* self, apx_attributeParserValueTableState_t* vts)
{
   bool auto_upper_limit = false;
   apx_valueTable_t* vt = apx_valueTable_new();
   int32_t num_values = apx_attributeParserValueTableState_length(vts);
   apx_error_t result;
   if (vt == NULL)
   {
      return NULL;
   }
   if (vts->num_count > 0)
   {
      if (vts->num_count == 1)
      {
         auto_upper_limit = true;
      }
      if (vts->range.is_signed_range)
      {
         apx_valueTable_set_range_signed(vt, vts->range.lower.i32, auto_upper_limit ? vts->range.lower.i32 : vts->range.upper.i32);
      }
      else
      {
         apx_valueTable_set_range_unsigned(vt, vts->range.lower.u32, auto_upper_limit ? vts->range.lower.u32 : vts->range.upper.u32);
      }
   }
   else
   {
      auto_upper_limit = true;
   }
   if (auto_upper_limit)
   {
      int32_t i32_index = vt->base.lower_limit.i32;
      uint32_t u32_index = vt->base.lower_limit.u32;
      int32_t i;
      for (i = 0; i < num_values; i++)
      {
         if (vts->range.is_signed_range)
         {
            if (vt->base.upper_limit.i32 < i32_index)
            {
               vt->base.upper_limit.i32 = i32_index;
            }
         }
         else
         {
            if (vt->base.upper_limit.u32 < u32_index)
            {
               vt->base.upper_limit.u32 = u32_index;
            }
         }
         i32_index++;
         u32_index++;
      }
   }
   result = apx_valueTable_move_values(vt, &vts->values);
   if (result != APX_NO_ERROR)
   {
      apx_attributeParser_set_error(self, result, NULL);
      apx_valueTable_delete(vt);
      vt = NULL;
   }
   return vt;
}

apx_rationalScaling_t* apx_attributeParser_create_rational_scaling_from_state(apx_attributeParserRationalScalingState_t* vts)
{
   apx_rationalScaling_t* rs = apx_rationalScaling_new(vts->offset, vts->numerator, vts->denominator, vts->unit);
   if (rs != NULL)
   {
      if (vts->range.is_signed_range)
      {
         apx_rationalScaling_set_range_signed(rs, vts->range.lower.i32, vts->range.upper.i32);
      }
      else
      {
         apx_rationalScaling_set_range_unsigned(rs, vts->range.lower.u32, vts->range.upper.u32);
      }
   }
   return rs;
}

static uint8_t const* apx_attributeParser_parse_single_port_attribute(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_portAttributes_t* attr)
{
   uint8_t const* next = begin;
   char c = *next;
   apx_attributeParseType_t attribute_type = APX_ATTRIBUTE_PARSE_TYPE_NONE;
   switch (c)
   {
   case '=':
      attribute_type = APX_ATTRIBUTE_PARSE_TYPE_INIT_VALUE;
      break;
   case 'P':
      attribute_type = APX_ATTRIBUTE_PARSE_TYPE_PARAMETER;
      break;
   case 'Q':
      attribute_type = APX_ATTRIBUTE_PARSE_TYPE_QUEUE_LENGTH;
      break;
   default:
      attribute_type = APX_ATTRIBUTE_PARSE_TYPE_NONE;
   }
   if (attribute_type != APX_ATTRIBUTE_PARSE_TYPE_NONE)
   {
      uint8_t const* result = NULL;
      switch (attribute_type)
      {
      case APX_ATTRIBUTE_PARSE_TYPE_INIT_VALUE:
         result = apx_attributeParser_parse_initializer(self, next + 1, end, &attr->init_value);
         break;
      case APX_ATTRIBUTE_PARSE_TYPE_PARAMETER:
         result = next + 1;
         attr->is_parameter = true;
         break;
      case APX_ATTRIBUTE_PARSE_TYPE_QUEUE_LENGTH:
         result = apx_attributeParser_parse_array_length(next + 1, end, &attr->queue_length);
         break;
      default:
         apx_attributeParser_set_error(self, APX_INTERNAL_ERROR, NULL);
         return NULL;
      }
      if (result > next)
      {
         next = result;
      }
      else
      {
         apx_attributeParser_set_error(self, APX_PARSE_ERROR, next);
         next = NULL;
      }
   }
   else
   {
      apx_attributeParser_set_error(self, APX_PARSE_ERROR, next);
      next = NULL;
   }
   return next;

}

static uint8_t const* apx_attributeParser_parse_single_type_attribute(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_typeAttributes_t* attr)
{
   {
      uint8_t const* next = begin;
      uint8_t const* result;
      apx_valueTable_t* vt = NULL;
      apx_rationalScaling_t* rs = NULL;
      apx_computation_t* computation = NULL;
      apx_attributeParseType_t attr_type = APX_ATTRIBUTE_PARSE_TYPE_NONE;
      result = bstr_match_cstr(next, end, "VT");
      if (result > begin)
      {
         attr_type = APX_ATTRIBUTE_PARSE_TYPE_VALUE_TABLE;
         next += 2;
      }
      else
      {
         result = bstr_match_cstr(next, end, "RS");
         if (result > begin)
         {
            attr_type = APX_ATTRIBUTE_PARSE_TYPE_RATIONAL_SCALING;
            next += 2;
         }
      }
      switch (attr_type)
      {
      case APX_ATTRIBUTE_PARSE_TYPE_NONE:
         apx_attributeParser_set_error(self, APX_PARSE_ERROR, next);
         result = NULL;
         break;
      case APX_ATTRIBUTE_PARSE_TYPE_VALUE_TABLE:
         result = apx_attributeParser_parse_value_table(self, next, end, &vt);
         if (result > next)
         {
            computation = (apx_computation_t*)vt;
         }
         break;
      case APX_ATTRIBUTE_PARSE_TYPE_RATIONAL_SCALING:
         result = apx_attributeParser_parse_rational_scaling(self, next, end, &rs);
         if (result > next)
         {
            computation = (apx_computation_t*)rs;
         }
         break;
      }
      if (result > next)
      {
         assert(result <= end);
         next = result;
         if (computation != NULL)
         {
            apx_typeAttributes_append_computation(attr, computation);
         }
      }
      else
      {
         if (computation != NULL)
         {
            apx_computation_vdelete((void*)computation);
         }
         apx_attributeParser_set_error(self, APX_PARSE_ERROR, next);
         next = NULL;
      }
      return next;
   }
}

static uint8_t const* apx_attributeParser_parse_initializer_list(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* next = begin;
   while (next < end)
   {
      uint8_t c;
      next = bstr_lstrip(next, end);
      c = *next;

      if (c == '{')
      {
         apx_attributeParseState_t* child_state;
         next++;
         self->state->initializer_list = dtl_av_new();
         adt_stack_push(&self->stack, self->state);
         child_state = apx_attributeParseState_new();
         if (child_state == NULL)
         {
            apx_attributeParser_set_error(self, APX_MEM_ERROR, next);
            return NULL;
         }
         else
         {
            child_state->parent = self->state;
            self->state = child_state;
            continue;
         }
      }
      else
      {
         uint8_t const* result = apx_attributeParser_parse_scalar(self, next, end);
         if (result > next)
         {
            next = result;
         }
         else if (result == NULL)
         {
            return result;
         }
      }
   POST_VALUE_HANDLER:
      if (apx_attributeParser_is_initializer_list(self))
      {
         next = bstr_lstrip(next, end);
         c = *next;
         if (apx_attributeParseState_has_value(self->state))
         {
            if (!apx_attributeParser_push_value_to_parent(self))
            {
               return NULL;
            }
         }
         if (!dtl_av_is_empty(self->state->parent->initializer_list) && (c == ','))
         {
            next++;
         }
         else if (c == '}')
         {
            next++;
            if (adt_stack_size(&self->stack) > 0)
            {
               apx_attributeParseState_delete(self->state);
               self->state = adt_stack_top(&self->stack);
               adt_stack_pop(&self->stack);
               if (adt_stack_size(&self->stack) == 0u )
               {
                  break; //Reached top of stack
               }
               else
               {
                  goto POST_VALUE_HANDLER;
               }
            }
            else
            {
               return NULL;
            }
         }
         else
         {
            //unexpected character
            return NULL;
         }
      }
      else
      {
         break;
      }
   }
   next = bstr_lstrip(next, end);
   return next;
}

static bool apx_attributeParser_push_value_to_parent(apx_attributeParser_t* self)
{
   assert(self != NULL);
   if (self->state->parent != NULL)
   {
      assert(self->state->parent->initializer_list != NULL);
      if (self->state->sv != NULL)
      {
         dtl_av_push(self->state->parent->initializer_list, (dtl_dv_t*) self->state->sv, false);
         self->state->sv = NULL;
      }
      else if (self->state->initializer_list != NULL)
      {
         dtl_av_push(self->state->parent->initializer_list, (dtl_dv_t*)self->state->initializer_list, false);
         self->state->initializer_list = NULL;
      }
      else
      {
         return false;
      }
   }
   return true;
}

static uint8_t const* apx_attributeParser_parse_scalar(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* next = begin;
   uint8_t c = *next;
   assert(self != NULL);
   if (c > 0)
   {
      if (isdigit(c))
      {
         uint8_t const* result;
         self->state->sv = dtl_sv_new();
         result = apx_attributeParser_parse_integer_literal(next, end, false, self->state->sv);
         if (result > next)
         {
            assert(result <= end);
            next = result;
         }
      }
      else if (c == '-')
      {
         next++;
         if (next < end)
         {
            uint8_t const* result;
            self->state->sv = dtl_sv_new();
            result = apx_attributeParser_parse_integer_literal(next, end, true, self->state->sv);
            if (result > next)
            {
               assert(result <= end);
               next = result;
            }
         }
         else
         {
            return NULL; //Nothing after the minus sign?
         }
      }
      else if (c == '"')
      {
         uint8_t const* result;
         self->state->sv = dtl_sv_new();
         result = apx_attributeParser_parse_string_literal(next, end, self->state->sv);
         if (result > next)
         {
            assert(result <= end);
            next = result;
         }
      }
   }
   return next;
}

static uint8_t const* apx_attributeParser_parse_integer_literal(uint8_t const* next, uint8_t const* end, bool unary_minus, dtl_sv_t* sv)
{
   uint8_t* end_ptr = NULL;
   unsigned long value;
   value = strtoul((const char*) next,(char**) &end_ptr, 0);
   if ((end_ptr > next) && (end_ptr <= end))
   {
      next = end_ptr;
      if (unary_minus && (value == 0x80000000UL))
      {
         dtl_sv_set_i32(sv, INT32_MIN);
      }
      else if (value > INT32_MAX)
      {
         dtl_sv_set_u32(sv, (uint32_t)value);
      }
      else
      {
         int32_t tmp = (int32_t) value;
         dtl_sv_set_i32(sv, unary_minus ? -tmp : tmp);
      }
   }
   else
   {
      return NULL;
   }
   return next;
}

static uint8_t const* apx_attributeParser_parse_string_literal(uint8_t const* begin, uint8_t const* end, dtl_sv_t* sv)
{
   assert(begin < end);
   uint8_t const* next = bstr_match_pair(begin, end, '"', '"', '\\');
   if (next > begin)
   {
      assert(next < end);
      dtl_sv_set_bstr(sv, begin + 1, next);
      next++; //skip past ending '"' character
   }
   return next;
}

static uint8_t const* apx_attributeParser_parse_array_length(uint8_t const* begin, uint8_t const* end, uint32_t* length)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      if (*next++ == '[')
      {
         next = bstr_lstrip(next, end);
         if (next < end)
         {
            uint8_t const* result = apx_parserBase_parse_u32(next, end, length);
            if (result > next)
            {
               next = bstr_lstrip(result, end);
               if (next < end)
               {
                  if (*next++ == ']')
                  {
                     return next;
                  }
                  else
                  {
                     return NULL;
                  }
               }
            }
            else
            {
               return NULL;
            }
         }
      }
      else
      {
         return NULL;
      }
   }
   return begin;
}

static uint8_t const* apx_attributeParser_parse_value_table(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_valueTable_t** vt)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      apx_attributeParserValueTableState_t state;
      if (*next++ != '(')
      {
         return NULL;
      }
      apx_attributeParserValueTableState_create(&state);

      while (next < end)
      {
         next = bstr_lstrip(next, end);
         uint8_t const* result = apx_attributeParser_parse_value_table_arg(self, next, end, &state);
         if (result > next)
         {
            assert(result <= end);
            next = result;
         }
         else
         {
            next = NULL;
            break;
         }
         next = bstr_lstrip(next, end);
         const char c = *next;
         if (c == ',')
         {
            //prepare for parsing next argument
            next++;
         }
         else if (c == ')')
         {
            //that was the last argument
            *vt = apx_attributeParser_create_value_table_from_state(self, &state);
            if (*vt == NULL)
            {
               self->error_next = next;
            }
            return next + 1;
         }
      }
   }
   return begin; //not enough characters in stream
}

static uint8_t const* apx_attributeParser_parse_rational_scaling(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_rationalScaling_t** rs)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      apx_attributeParserRationalScalingState_t state;
      if (*next++ != '(')
      {
         return NULL;
      }
      apx_attributeParserRationalScalingState_create(&state);
      while (next < end)
      {
         uint8_t const* result;
         next = bstr_lstrip(next, end);
         result = apx_attributeParser_parse_rational_scaling_arg(self, next, end, &state);
         if (result > next)
         {
            assert(result <= end);
            next = result;
         }
         else
         {
            next = NULL;
            break;
         }
         next = bstr_lstrip(next, end);
         const char c = *next;
         if (c == ',')
         {
            //prepare for parsing next argument
            next++;
         }
         else if (c == ')')
         {
            //that was the last argument
            *rs = apx_attributeParser_create_rational_scaling_from_state(&state);
            apx_attributeParserRationalScalingState_destroy(&state);
            if (*rs == NULL)
            {
               apx_attributeParser_set_error(self, APX_MEM_ERROR, next);
               return NULL;
            }
            return next + 1;
         }
      }
      //This is the error path, we should not normally end up here.
      if (self->error_next == APX_NO_ERROR)
      {
         apx_attributeParser_set_error(self, APX_PARSE_ERROR, next);
      }
      apx_attributeParserRationalScalingState_destroy(&state);
   }
   return begin; //not enough characters in stream
}

static uint8_t const* apx_attributeParser_parse_value_table_arg(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_attributeParserValueTableState_t* vts)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      apx_argumentType_t arg_type = APX_ARGUMENT_TYPE_INVALID;
      uint8_t const* result = NULL;
      bool unary_minus = false;

      if (!vts->last_was_string)
      {
         //Determing if next character is a string or integer literal
         uint8_t c = *next;
         if (c == '"')
         {
            arg_type = APX_ARGUMENT_TYPE_STRING_LITERAL;
         }
         else if (c == '-')
         {
            unary_minus = true;
            next++;
            arg_type = APX_ARGUMENT_TYPE_INTEGER_LITERAL;
         }
         else if (isdigit((int)c))
         {
            arg_type = APX_ARGUMENT_TYPE_INTEGER_LITERAL;
         }
      }
      else
      {
         if (*next == '"')
         {
            arg_type = APX_ARGUMENT_TYPE_STRING_LITERAL;
         }
      }
      switch (arg_type)
      {
      case APX_ARGUMENT_TYPE_INVALID:
         next = NULL;
         break;
      case APX_ARGUMENT_TYPE_INTEGER_LITERAL:
         if (vts->num_count == 0)
         {
            result = apx_attributeParser_parse_lower_limit(next, end, unary_minus, &vts->range);
         }
         else if (vts->num_count == 1)
         {
            result = apx_attributeParser_parse_upper_limit(next, end, unary_minus, &vts->range);
         }
         else
         {
            result = NULL; //Three consecutive numbers not allowed
         }
         if (result > next)
         {
            next = result;
         }
         vts->num_count++;
         break;
      case APX_ARGUMENT_TYPE_STRING_LITERAL:
         vts->last_was_string = true;
         result = bstr_match_pair(next, end, '"', '"', '\\');
         if (result > next)
         {
            adt_str_t* str = adt_str_new_bstr(next + 1, result);
            if (str == NULL)
            {
               apx_attributeParser_set_error(self, APX_MEM_ERROR, next);
               return NULL; //TODO: clean memory
            }
            apx_attributeParserValueTableState_append(vts, str);
            next = result + 1; //skip past ending '"' character
         }
         break;
      }
      if (result == NULL)
      {
         apx_attributeParser_set_error(self, APX_PARSE_ERROR, next);
         return result;
      }
   }
   return next;

}

static uint8_t const* apx_attributeParser_parse_rational_scaling_arg(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_attributeParserRationalScalingState_t* rss)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      uint8_t const* result = NULL;
      bool unary_minus = false;
      double d = 0.0;
      int32_t i32 = 0;

      switch (rss->arg_index)
      {
      case APX_RATIONAL_ARG_INDEX_0: //LOWER RANGE LIMIT (integer)
         if (*next == '-')
         {
            unary_minus = true;
            next++;
         }
         result = apx_attributeParser_parse_lower_limit(next, end, unary_minus, &rss->range);
         break;
      case APX_RATIONAL_ARG_INDEX_1: //UPPER RANGE LIMIT (integer)
         if (*next == '-')
         {
            unary_minus = true;
            next++;
         }
         result = apx_attributeParser_parse_upper_limit(next, end, unary_minus, &rss->range);
         break;
      case APX_RATIONAL_ARG_INDEX_2: //OFFSET (double)
         result = apx_parserBase_parse_double(next, end, &d);
         if (result > next)
         {
            rss->offset = d;
         }
         break;
      case APX_RATIONAL_ARG_INDEX_3: //NUMERATOR (integer)
         result = apx_parserBase_parse_i32(next, end, &i32);
         if (result > next)
         {
            rss->numerator = i32;
         }
         break;
      case APX_RATIONAL_ARG_INDEX_4: //DENOMINATOR (integer)
         result = apx_parserBase_parse_i32(next, end, &i32);
         if (result > next)
         {
            rss->denominator = i32;
         }
         break;
      case APX_RATIONAL_ARG_INDEX_5: //Unit (string literal)
         result = bstr_match_pair(next, end, '"', '"', '\\');
         if (result > next)
         {
            rss->unit = bstr_make_cstr(next + 1, result);
            if (rss->unit == NULL)
            {
               apx_attributeParser_set_error(self, APX_MEM_ERROR, next);
               return NULL;
            }
            result++;
         }
         break;
      default:
         result = NULL;
      }
      next = result;
      rss->arg_index++;
   }
   return next;
}

static uint8_t const* apx_attributeParser_parse_lower_limit(uint8_t const* begin, uint8_t const* end, bool unary_minus, apx_range_t* range)
{
   const char* next = (const char*) begin;
   char* end_ptr;
   uint32_t tmp = (uint32_t) strtoul(next, &end_ptr, 0);
   if ((end_ptr > next) && (end_ptr <= (const char*)end))
   {
      next = end_ptr;
      if (unary_minus)
      {
         if (tmp > 0x80000000UL)
         {
            return NULL; //value out of range
         }
         range->lower.i32 = -((int32_t)tmp);
         range->is_signed_range = true;
      }
      else
      {
         range->lower.u32 = tmp;
      }
   }
   else
   {
      next = NULL;
   }
   return (uint8_t const*) next;
}

static uint8_t const* apx_attributeParser_parse_upper_limit(uint8_t const* begin, uint8_t const* end, bool unary_minus, apx_range_t* range)
{
   const char* next = (const char*)begin;
   char* end_ptr;
   uint32_t tmp = (uint32_t)strtoul(next, &end_ptr, 0);
   if ((end_ptr > next) && (end_ptr <= (const char*)end))
   {
      next = end_ptr;
      if (unary_minus)
      {
         if (!range->is_signed_range)
         {
            next = NULL; //lower range is positive while upper range is negative?
         }
         else
         {
            if (tmp > 0x80000000UL)
            {
               return NULL; //value out of range
            }
            range->upper.i32 = -((int32_t)tmp);
         }
      }
      else
      {
         if (range->is_signed_range)
         {
            if (tmp >= 0x80000000UL)
            {
               return NULL; //value out of range
            }
            range->upper.i32 = (int32_t)tmp;
         }
         else
         {
            range->upper.u32 = tmp;
         }
      }
   }
   else
   {
      next = NULL;
   }
   return (uint8_t const*)next;
}

static bool apx_attributeParser_is_initializer_list(apx_attributeParser_t* self)
{
   assert(self != NULL);
   return self->state->parent != NULL? true : false;
}
