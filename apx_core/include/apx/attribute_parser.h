/*****************************************************************************
* \file      attribute_parser.h
* \author    Conny Gustafsson
* \date      2017-07-30
* \brief     Port attribute parser
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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

typedef struct apx_attribute_parse_state_tag
{
   struct apx_attribute_parse_state_tag* parent;
   dtl_sv_t* sv;
   dtl_av_t* initializer_list;
} apx_attribute_parse_state_t;

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

typedef struct apx_attribute_parser_value_table_state_tag
{
   apx_range_t range;
   adt_ary_t values; //strong references to adt_str_t
   int32_t num_count;
   bool last_was_string;
} apx_attribute_parser_value_table_state_t;

typedef struct apx_attribute_parser_rational_scaling_state_tag
{
   apx_range_t range;
   double offset;
   int32_t numerator;
   int32_t denominator;
   uint32_t arg_index;
   char* unit;
} apx_attribute_parser_rational_scaling_state_t;

typedef struct apx_attribute_parser_tag
{
   adt_stack_t stack; //strong references to apx_attribute_parse_state_t
   apx_attribute_parse_state_t* state;
   apx_error_t last_error;
   const uint8_t* error_next;
} apx_attribute_parser_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//apx_attribute_parse_state_t API
void apx_attribute_parse_state_create(apx_attribute_parse_state_t* self);
void apx_attribute_parse_state_destroy(apx_attribute_parse_state_t* self);
apx_attribute_parse_state_t* apx_attribute_parse_state_new(void);
void apx_attribute_parse_state_delete(apx_attribute_parse_state_t* self);
void apx_attribute_parse_state_vdelete(void* arg);
bool apx_attribute_parse_state_has_value(apx_attribute_parse_state_t* self);

//apx_range_t API
void apx_range_create(apx_range_t* self);

//apx_attribute_parser_value_table_state_t API
void apx_attribute_parser_value_table_state_create(apx_attribute_parser_value_table_state_t* self);
void apx_attribute_parser_value_table_state_destroy(apx_attribute_parser_value_table_state_t* self);
void apx_attribute_parser_value_table_state_append(apx_attribute_parser_value_table_state_t* self, adt_str_t* str);
int32_t apx_attribute_parser_value_table_state_length(apx_attribute_parser_value_table_state_t* self);

//apx_attribute_parser_rational_scaling_state_t API
void apx_attribute_parser_rational_scaling_state_create(apx_attribute_parser_rational_scaling_state_t* self);
void apx_attribute_parser_rational_scaling_state_destroy(apx_attribute_parser_rational_scaling_state_t* self);

//apx_attribute_parser_t API
void apx_attribute_parser_create(apx_attribute_parser_t *self);
void apx_attribute_parser_destroy(apx_attribute_parser_t *self);

uint8_t const* apx_attribute_parser_parse_port_attributes(apx_attribute_parser_t* self, uint8_t const* begin, uint8_t const* end, apx_port_attributes_t* attr);
uint8_t const* apx_attribute_parser_parse_type_attributes(apx_attribute_parser_t* self, uint8_t const* begin, uint8_t const* end, apx_type_attributes_t* attr);
uint8_t const* apx_attribute_parser_parse_initializer(apx_attribute_parser_t* self, uint8_t const* begin, uint8_t const* end, dtl_dv_t** dv);
apx_error_t apx_attribute_parser_get_last_error(apx_attribute_parser_t* self, uint8_t const** error_next);


#endif //APX_ATTRIBUTE_PARSER_H
