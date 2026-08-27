/*****************************************************************************
* \file      signature_parser.c
* \author    Conny Gustafsson
* \date      2020-12-01
* \brief     Data signature parser
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "bstr.h"
#include "adt_str.h"
#include "apx/signature_parser.h"
#include "apx/parser_base.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_dataElement_t* apx_signatureParserState_take_data_element(apx_signatureParserState_t* self);
static void apx_signatureParser_reset(apx_signatureParser_t* self);
static void apx_signatureParser_set_error(apx_signatureParser_t* self, apx_error_t error_code, uint8_t const* error_pos);
static uint8_t const* apx_signatureParser_parse_data_element(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end);
static uint8_t const* apx_signatureParser_parse_type_reference(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end);
static uint8_t const* apx_signatureParser_parse_limits_i32(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end);
static uint8_t const* apx_signatureParser_parse_limits_u32(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end);
static uint8_t const* apx_signatureParser_parse_limits_i64(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end);
static uint8_t const* apx_signatureParser_parse_limits_u64(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end);
static uint8_t const* apx_signatureParser_parse_array(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end);



//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_signatureParserState_create(apx_signatureParserState_t* self, bool is_record)
{
   if (self != NULL)
   {
      self->is_record = is_record;
      self->data_element = NULL;
   }
}

void apx_signatureParserState_destroy(apx_signatureParserState_t* self)
{
   if (self != NULL)
   {
      if (self->data_element != NULL)
      {
         apx_dataElement_delete(self->data_element);
      }
   }
}

apx_signatureParserState_t* apx_signatureParserState_new(bool is_record)
{
   apx_signatureParserState_t* self = (apx_signatureParserState_t*)malloc(sizeof(apx_signatureParserState_t));
   if (self != 0)
   {
      apx_signatureParserState_create(self, is_record);
   }
   return self;
}

void apx_signatureParserState_delete(apx_signatureParserState_t* self)
{
   if (self != NULL)
   {
      apx_signatureParserState_destroy(self);
      free(self);
   }
}


apx_error_t apx_signatureParser_create(apx_signatureParser_t* self)
{
   if (self != NULL)
   {
      self->error_code = APX_NO_ERROR;
      self->error_pos = NULL;
      self->state = apx_signatureParserState_new(false);
      if (self->state == NULL)
      {
         return APX_MEM_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_signatureParser_destroy(apx_signatureParser_t* self)
{
   if (self != NULL)
   {
      apx_signatureParserState_delete(self->state);
   }
}

uint8_t const* apx_signatureParser_parse_data_signature(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   if ((self != NULL) && (begin != NULL) && (end != NULL) && (begin <= end))
   {
      apx_signatureParser_reset(self);
      self->state = apx_signatureParserState_new(false);
      if (self->state == NULL)
      {
         apx_signatureParser_set_error(self, APX_MEM_ERROR, begin);
      }
      return apx_signatureParser_parse_data_element(self, begin, end);
   }
   return NULL;
}

apx_error_t apx_signatureParser_get_last_error(apx_signatureParser_t* self, uint8_t const** error_pos)
{
   if (self != NULL)
   {
      if (error_pos != NULL)
      {
         *error_pos = self->error_pos;
      }
      return self->error_code;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_dataElement_t* apx_signatureParser_take_data_element(apx_signatureParser_t* self)
{
   if (self != NULL)
   {
      return apx_signatureParserState_take_data_element(self->state);
   }
   return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_dataElement_t* apx_signatureParserState_take_data_element(apx_signatureParserState_t* self)
{
   if (self != NULL)
   {
      apx_dataElement_t* retval = self->data_element;
      self->data_element = NULL;
      return retval;
   }
   return NULL;
}

static void apx_signatureParser_reset(apx_signatureParser_t* self)
{
   assert(self != NULL);
   self->error_code = APX_NO_ERROR;
   self->error_pos = NULL;
   apx_signatureParserState_delete(self->state);
}

static void apx_signatureParser_set_error(apx_signatureParser_t* self, apx_error_t error_code, uint8_t const* error_pos)
{
   assert(self != 0);
   self->error_code = error_code;
   self->error_pos = error_pos;
}

static uint8_t const* apx_signatureParser_parse_data_element(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* next = begin;
   uint8_t const* name_begin = NULL;
   uint8_t const* name_end = NULL;
   uint8_t const* result = NULL;
   assert( (self != NULL) && (self->state != NULL) );

   if (self->state->is_record)
   {
      result = bstr_match_pair(next, end, '"', '"', '\\');
      if (result > next)
      {
         name_begin = next + 1; //Don't include first '"' character
         name_end = result;
         next = result + 1; //Move past second '"' character
      }
      else
      {
         apx_signatureParser_set_error(self, APX_PARSE_ERROR, next);
         return NULL;
      }
   }
   if (next >= end)
   {
      return begin;
   }
   const char c = *next;
   bool is_signed_type = true;
   bool is_64_bit_type = false;
   bool check_limits = true;
   apx_typeCode_t type_code = APX_TYPE_CODE_NONE;
   apx_tokenClass_t token_class = APX_TOKEN_CLASS_DATA_ELEMENT;
   switch (c)
   {
   case '{':
      type_code = APX_TYPE_CODE_RECORD;
      check_limits = false;
      break;
   case '[':
      token_class = APX_TOKEN_GROUP_DECLARATION;
      check_limits = false;
      break;
   case '(':
      token_class = APX_TOKEN_FUNCTION_DECLARATION;
      check_limits = false;
      break;
   case 'a':
      type_code = APX_TYPE_CODE_CHAR;
      check_limits = false;
      break;
   case 'A':
      type_code = APX_TYPE_CODE_CHAR8;
      check_limits = false;
      break;
   case 'b':
      type_code = APX_TYPE_CODE_BOOL;
      check_limits = false;
      break;
   case 'B':
      type_code = APX_TYPE_CODE_BYTE;
      is_signed_type = false;
      break;
   case 'c':
      type_code = APX_TYPE_CODE_INT8;
      break;
   case 'C':
      type_code = APX_TYPE_CODE_UINT8;
      is_signed_type = false;
      break;
   case 'l':
      type_code = APX_TYPE_CODE_INT32;
      break;
   case 'L':
      type_code = APX_TYPE_CODE_UINT32;
      is_signed_type = false;
      break;
   case 'q':
      type_code = APX_TYPE_CODE_INT64;
      is_64_bit_type = true;
      break;
   case 'Q':
      type_code = APX_TYPE_CODE_UINT64;
      is_signed_type = false;
      is_64_bit_type = true;
      break;
   case 's':
      type_code = APX_TYPE_CODE_INT16;
      break;
   case 'S':
      type_code = APX_TYPE_CODE_UINT16;
      is_signed_type = false;
      break;
   case 'T':
      type_code = APX_TYPE_CODE_REF_ID; //Initial guess, might change while parsing continues
      check_limits = false;
      break;
   case 'u':
      type_code = APX_TYPE_CODE_CHAR16;
      check_limits = false;
      break;
   case 'U':
      type_code = APX_TYPE_CODE_CHAR32;
      check_limits = false;
      break;
   }
   if (token_class == APX_TOKEN_CLASS_DATA_ELEMENT)
   {
      if (type_code == APX_TYPE_CODE_NONE)
      {
         apx_signatureParser_set_error(self, APX_PARSE_ERROR, next);
         return NULL;
      }
      else
      {
         self->state->data_element = apx_dataElement_new(type_code);
         if (self->state->data_element == NULL)
         {
            apx_signatureParser_set_error(self, APX_MEM_ERROR, next);
            return NULL;
         }
         next++;
         if (name_begin != NULL)
         {
            assert((name_end != NULL) && (name_begin <= name_end));
            apx_error_t rc = apx_dataElement_set_name_bstr(self->state->data_element, name_begin, name_end);
            if (rc != APX_NO_ERROR)
            {
               apx_signatureParser_set_error(self, rc, next);
               return NULL;
            }
         }
      }
      if (type_code == APX_TYPE_CODE_REF_ID)
      {
         result = apx_signatureParser_parse_type_reference(self, next, end);
         if (result > next)
         {
            next = result;
         }
         else
         {
            apx_signatureParser_set_error(self, APX_PARSE_ERROR, next);
            return NULL;
         }
      }
      else if (type_code == APX_TYPE_CODE_RECORD)
      {
         apx_signatureParserState_t* parent = self->state;
         self->state = apx_signatureParserState_new(true);
         while (next < end)
         {
            result = apx_signatureParser_parse_data_element(self, next, end);
            if (result > next)
            {
               next = result;
               assert(parent->data_element != NULL);
               assert(self->state->data_element != NULL);
               apx_dataElement_append_child(parent->data_element, self->state->data_element);
               self->state->data_element = NULL; //Move ownership to parent
            }
            else
            {
               apx_signatureParser_set_error(self, APX_PARSE_ERROR, next);
               apx_signatureParserState_delete(self->state);
               self->state = parent;
               return NULL;
            }
            if (next >= end)
            {
               apx_signatureParser_set_error(self, APX_PARSE_ERROR, next);
               apx_signatureParserState_delete(self->state);
               self->state = parent;
               return NULL;
            }
            if (*next == '}')
            {
               next++;
               apx_signatureParserState_delete(self->state);
               self->state = parent;
               break;
            }
         }
      }
      if (check_limits)
      {
         if (is_64_bit_type)
         {
            result = is_signed_type ? apx_signatureParser_parse_limits_i64(self, next, end) : apx_signatureParser_parse_limits_u64(self, next, end);
         }
         else
         {
            result = is_signed_type ? apx_signatureParser_parse_limits_i32(self, next, end) : apx_signatureParser_parse_limits_u32(self, next, end);
         }
         if (result > next)
         {
            next = result;
         }
         else if (result == NULL)
         {
            apx_signatureParser_set_error(self, APX_PARSE_ERROR, next);
            return NULL;
         }
      }
      result = apx_signatureParser_parse_array(self, next, end);
      if (result > next)
      {
         next = result;
      }
      else if (result == NULL)
      {
         apx_signatureParser_set_error(self, APX_PARSE_ERROR, next);
         return NULL;
      }
   }
   else
   {
      apx_signatureParser_set_error(self, APX_NOT_IMPLEMENTED_ERROR, next);
      return NULL;
   }
   return next;
}

static uint8_t const* apx_signatureParser_parse_type_reference(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   const uint8_t* next = begin;
   if (next < end)
   {
      uint8_t c;
      if (*next++ != '[')
      {
         return begin;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      c = *next;
      if (c == '"')
      {
         uint8_t const* result = bstr_match_pair(next, end, '"', '"', '\\');
         if (result > next)
         {
            apx_error_t rc = apx_dataElement_set_type_ref_name_bstr(self->state->data_element, next + 1, result);
            if (rc != APX_NO_ERROR)
            {
               apx_signatureParser_set_error(self, rc, next);
               return NULL;
            }
            next = result + 1;
         }
         else
         {
            return NULL;
         }
      }
      else
      {
         uint32_t type_id;
         uint8_t const* result = apx_parserBase_parse_u32(next, end, &type_id);
         if (result > next)
         {
            apx_dataElement_set_type_ref_id(self->state->data_element, type_id);
            next = result;
         }
         else
         {
            return NULL;
         }
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ']')
      {
         return NULL;
      }
   }
   return next;
}

static uint8_t const* apx_signatureParser_parse_limits_i32(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      uint8_t const* result = NULL;
      if (*next++ != '(')
      {
         return begin; //This is not the beginning of a limit expression
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      int32_t lower_limit, upper_limit;
      result = apx_parserBase_parse_i32(next, end, &lower_limit);
      if (result > next)
      {
         next = result;
      }
      else
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ',')
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      result = apx_parserBase_parse_i32(next, end, &upper_limit);
      if (result > next)
      {
         next = result;
      }
      else
      {
         return NULL;
      }
      apx_dataElement_set_limits_int32(self->state->data_element, lower_limit, upper_limit);
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ')')
      {
         return NULL;
      }
      return next;
   }
   return begin;
}

static uint8_t const* apx_signatureParser_parse_limits_u32(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      uint8_t const* result = NULL;
      if (*next++ != '(')
      {
         return begin; //This is not the beginning of a limit expression
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      uint32_t lower_limit, upper_limit;
      result = apx_parserBase_parse_u32(next, end, &lower_limit);
      if (result > next)
      {
         next = result;
      }
      else
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ',')
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      result = apx_parserBase_parse_u32(next, end, &upper_limit);
      if (result > next)
      {
         next = result;
      }
      else
      {
         return NULL;
      }
      apx_dataElement_set_limits_uint32(self->state->data_element, lower_limit, upper_limit);
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ')')
      {
         return NULL;
      }
      return next;
   }
   return begin;
}

static uint8_t const* apx_signatureParser_parse_limits_i64(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      uint8_t const* result = NULL;
      if (*next++ != '(')
      {
         return begin;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      int64_t lower_limit, upper_limit;
      result = apx_parserBase_parse_i64(next, end, &lower_limit);
      if (result > next)
      {
         next = result;
      }
      else
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ',')
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      result = apx_parserBase_parse_i64(next, end, &upper_limit);
      if (result > next)
      {
         next = result;
      }
      else
      {
         return NULL;
      }
      apx_dataElement_set_limits_int64(self->state->data_element, lower_limit, upper_limit);
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ')')
      {
         return NULL;
      }
      return next;
   }
   return begin;
}

static uint8_t const* apx_signatureParser_parse_limits_u64(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      uint8_t const* result = NULL;
      if (*next++ != '(')
      {
         return begin;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      uint64_t lower_limit, upper_limit;
      result = apx_parserBase_parse_u64(next, end, &lower_limit);
      if (result > next)
      {
         next = result;
      }
      else
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ',')
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      result = apx_parserBase_parse_u64(next, end, &upper_limit);
      if (result > next)
      {
         next = result;
      }
      else
      {
         return NULL;
      }
      apx_dataElement_set_limits_uint64(self->state->data_element, lower_limit, upper_limit);
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next++ != ')')
      {
         return NULL;
      }
      return next;
   }
   return begin;
}

static uint8_t const* apx_signatureParser_parse_array(apx_signatureParser_t* self, uint8_t const* begin, uint8_t const* end)
{
   uint8_t const* next = begin;
   if (next < end)
   {
      uint8_t const* result = NULL;
      if (*next++ != '[')
      {
         return begin;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      uint32_t array_len;
      result = apx_parserBase_parse_u32(next, end, &array_len);
      if (result > next)
      {
         next = result;
         apx_dataElement_set_array_length(self->state->data_element, array_len);
      }
      else
      {
         return NULL;
      }
      next = bstr_lstrip(next, end);
      if (next >= end)
      {
         return NULL;
      }
      if (*next == '*')
      {
         apx_dataElement_set_dynamic_array(self->state->data_element);
         next = bstr_lstrip(next+1, end);
         if (next >= end)
         {
            return NULL;
         }
      }
      if (*next++ != ']')
      {
         return NULL;
      }
   }
   return next;
}

