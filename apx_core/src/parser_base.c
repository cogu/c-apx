/*****************************************************************************
* \file      parser_base.c
* \author    Conny Gustafsson
* \date      2020-12-04
* \brief     A set of shared functions that can be used by any APX parser
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
   return bstr_to_double(begin, end, v);
}

uint8_t const* apx_parserBase_parse_i32(uint8_t const* begin, uint8_t const* end, int32_t* v)
{
   long tmp = 0;
   uint8_t const* result = bstr_to_long(begin, end, &tmp);
   if ((result > begin) && (result <= end))
   {
      *v = (int32_t)tmp;
   }
   return result;
}

uint8_t const* apx_parserBase_parse_u32(uint8_t const* begin, uint8_t const* end, uint32_t* v)
{
   unsigned long tmp = 0;
   uint8_t const* result = bstr_to_unsigned_long(begin, end, 10, &tmp);
   if ((result > begin) && (result <= end))
   {
      *v = (uint32_t)tmp;
   }
   return result;
}

uint8_t const* apx_parserBase_parse_i64(uint8_t const* begin, uint8_t const* end, int64_t* v)
{
   long long tmp = 0;
   uint8_t const* result = bstr_to_long_long(begin, end, &tmp);
   if ((result > begin) && (result <= end))
   {
      *v = (int64_t)tmp;
   }
   return result;
}

uint8_t const* apx_parserBase_parse_u64(uint8_t const* begin, uint8_t const* end, uint64_t* v)
{
   unsigned long long tmp = 0;
   uint8_t const* result = bstr_to_unsigned_long_long(begin, end, 10, &tmp);
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
