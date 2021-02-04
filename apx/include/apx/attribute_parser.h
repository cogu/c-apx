/*****************************************************************************
* \file      attribute_parser.h
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
#ifndef APX_ATTRIBUTE_PARSER_H
#define APX_ATTRIBUTE_PARSER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_stack.h"
#include "apx/types.h"
#include "apx/port_attribute.h"
#include "apx/type_attribute.h"
#include "apx/error.h"
#include "adt_ary.h"
#include "dtl_type.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_attributeParseState_tag
{
   struct apx_attributeParseState_tag* parent;
   dtl_sv_t* sv;
   dtl_av_t* initializer_list;
} apx_attributeParseState_t;

typedef struct apx_range_tag
{
   union
   {
      int32_t i32;
      uint32_t u32;
   } lower;
   union
   {
      int32_t i32;
      uint32_t u32;
   } upper;
   bool is_signed_range;
} apx_range_t;

typedef struct apx_attributeParserValueTableState_tag
{
   apx_range_t range;
   adt_ary_t values; //strong references to adt_str_t
   int32_t num_count;
   bool last_was_string;
} apx_attributeParserValueTableState_t;

typedef struct apx_attributeParserRationalScalingState_tag
{
   apx_range_t range;
   double offset;
   int32_t numerator;
   int32_t denominator;
   uint32_t arg_index;
   char* unit;
} apx_attributeParserRationalScalingState_t;

typedef struct apx_attributeParser_tag
{
   adt_stack_t stack; //strong references to apx_attributeParseState_t
   apx_attributeParseState_t* state;
   apx_error_t last_error;
   const uint8_t* error_next;
} apx_attributeParser_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//apx_attributeParseState_t API
void apx_attributeParseState_create(apx_attributeParseState_t* self);
void apx_attributeParseState_destroy(apx_attributeParseState_t* self);
apx_attributeParseState_t* apx_attributeParseState_new(void);
void apx_attributeParseState_delete(apx_attributeParseState_t* self);
void apx_attributeParseState_vdelete(void* arg);
bool apx_attributeParseState_has_value(apx_attributeParseState_t* self);

//apx_range_t API
void apx_range_create(apx_range_t* self);

//apx_attributeParserValueTableState_t API
void apx_attributeParserValueTableState_create(apx_attributeParserValueTableState_t* self);
void apx_attributeParserValueTableState_destroy(apx_attributeParserValueTableState_t* self);
void apx_attributeParserValueTableState_append(apx_attributeParserValueTableState_t* self, adt_str_t* str);
int32_t apx_attributeParserValueTableState_length(apx_attributeParserValueTableState_t* self);

//apx_attributeParserRationalScalingState_t API
void apx_attributeParserRationalScalingState_create(apx_attributeParserRationalScalingState_t* self);
void apx_attributeParserRationalScalingState_destroy(apx_attributeParserRationalScalingState_t* self);

//apx_attributeParser_t API
void apx_attributeParser_create(apx_attributeParser_t *self);
void apx_attributeParser_destroy(apx_attributeParser_t *self);

uint8_t const* apx_attributeParser_parse_port_attributes(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_portAttributes_t* attr);
uint8_t const* apx_attributeParser_parse_type_attributes(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, apx_typeAttributes_t* attr);
uint8_t const* apx_attributeParser_parse_initializer(apx_attributeParser_t* self, uint8_t const* begin, uint8_t const* end, dtl_dv_t** dv);
apx_error_t apx_attributeParser_get_last_error(apx_attributeParser_t* self, uint8_t const** error_next);


#endif //APX_ATTRIBUTE_PARSER_H
