/*****************************************************************************
* \file      parser_base.h
* \author    Conny Gustafsson
* \date      2020-12-04
* \brief     A set of shared functions that can be used by any APX parser
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
