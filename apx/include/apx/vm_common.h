/*****************************************************************************
* \file      vm_common.h
* \author    Conny Gustafsson
* \date      2020-12-01
* \brief     Common functions shared between VM subcomponents
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
#ifndef APX_VM_COMMON_H
#define APX_VM_COMMON_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/vm_defs.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
uint8_t const* apx_vm_parse_uint32_by_variant(uint8_t const* begin, uint8_t const* end, uint8_t variant, uint32_t* number);
uint8_t const* apx_vm_parse_uint64_by_variant(uint8_t const* begin, uint8_t const* end, uint8_t variant, uint64_t* number);
uint8_t const* apx_vm_parse_int32_by_variant(uint8_t const* begin, uint8_t const* end, uint8_t variant, int32_t* number);
uint8_t const* apx_vm_parse_int64_by_variant(uint8_t const* begin, uint8_t const* end, uint8_t variant, int64_t* number);
uint8_t const* apx_vm_parse_uint32_by_size_type(uint8_t const* begin, uint8_t const* end, apx_sizeType_t size_type, uint32_t* number);
apx_error_t apx_vm_value_in_range_i32(int32_t value, int32_t lower_limit, int32_t upper_limit);
apx_error_t apx_vm_value_in_range_u32(uint32_t value, uint32_t lower_limit, uint32_t upper_limit);
apx_error_t apx_vm_value_in_range_i64(int64_t value, int64_t lower_limit, int64_t upper_limit);
apx_error_t apx_vm_value_in_range_u64(uint64_t value, uint64_t lower_limit, uint64_t upper_limit);
uint32_t apx_vm_variant_to_size(uint8_t variant);
apx_typeCode_t apx_vm_variant_to_type_code(uint8_t variant);
uint32_t apx_vm_size_type_to_size(apx_sizeType_t size_type);
apx_sizeType_t apx_vm_size_to_size_type(uint32_t size);
uint8_t apx_vm_size_type_to_variant(apx_sizeType_t size_type);


#endif //APX_VM_COMMON_H
