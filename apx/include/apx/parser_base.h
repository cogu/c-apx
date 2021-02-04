/*****************************************************************************
* \file      parser_base.h
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
#ifndef APX_PARSER_BASE_H
#define APX_PARSER_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

uint8_t const* apx_parserBase_parse_double(uint8_t const* begin, uint8_t const* end, double* v);
uint8_t const* apx_parserBase_parse_i32(uint8_t const* begin, uint8_t const* end, int32_t* v);
uint8_t const* apx_parserBase_parse_u32(uint8_t const* begin, uint8_t const* end, uint32_t* v);
uint8_t const* apx_parserBase_parse_i64(uint8_t const* begin, uint8_t const* end, int64_t* v);
uint8_t const* apx_parserBase_parse_u64(uint8_t const* begin, uint8_t const* end, uint64_t* v);
uint8_t const* apx_parserBase_parse_string_literal(uint8_t const* begin, uint8_t const* end);

#endif //APX_PARSER_BASE_H
