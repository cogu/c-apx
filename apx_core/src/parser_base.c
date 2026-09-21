/*****************************************************************************
* \file      parser_base.c
* \author    Conny Gustafsson
* \date      2020-12-04
* \brief     A set of shared functions that can be used by any APX parser
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/parser_base.h"
#include "bstr.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

uint8_t const* apx_parserBase_parse_double(uint8_t const* begin, uint8_t const* end, double* v)
{
   return bstr_parse_double(begin, end, v);
}

uint8_t const* apx_parserBase_parse_i32(uint8_t const* begin, uint8_t const* end, int32_t* v)
{
   long tmp = 0;
   uint8_t const* result = bstr_parse_long(begin, end, &tmp);
   if ((result > begin) && (result <= end))
   {
      *v = (int32_t)tmp;
   }
   return result;
}

uint8_t const* apx_parserBase_parse_u32(uint8_t const* begin, uint8_t const* end, uint32_t* v)
{
   unsigned long tmp = 0;
   uint8_t const* result = bstr_parse_unsigned_long(begin, end, 10, &tmp);
   if ((result > begin) && (result <= end))
   {
      *v = (uint32_t)tmp;
   }
   return result;
}

uint8_t const* apx_parserBase_parse_i64(uint8_t const* begin, uint8_t const* end, int64_t* v)
{
   long long tmp = 0;
   uint8_t const* result = bstr_parse_long_long(begin, end, &tmp);
   if ((result > begin) && (result <= end))
   {
      *v = (int64_t)tmp;
   }
   return result;
}

uint8_t const* apx_parserBase_parse_u64(uint8_t const* begin, uint8_t const* end, uint64_t* v)
{
   unsigned long long tmp = 0;
   uint8_t const* result = bstr_parse_unsigned_long_long(begin, end, 10, &tmp);
   if ((result > begin) && (result <= end))
   {
      *v = (uint64_t)tmp;
   }
   return result;
}

uint8_t const* apx_parserBase_parse_string_literal(uint8_t const* begin, uint8_t const* end)
{
   return bstr_match_pair(begin, end, '"', '"', '\\');
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
