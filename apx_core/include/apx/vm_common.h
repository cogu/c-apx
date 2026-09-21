/*****************************************************************************
* \file      vm_common.h
* \author    Conny Gustafsson
* \date      2020-12-01
* \brief     Common functions shared between VM subcomponents
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
uint8_t const* apx_vm_parse_uint32_by_size_type(uint8_t const* begin, uint8_t const* end, apx_size_type_t size_type, uint32_t* number);
apx_error_t apx_vm_value_in_range_i32(int32_t value, int32_t lower_limit, int32_t upper_limit);
apx_error_t apx_vm_value_in_range_u32(uint32_t value, uint32_t lower_limit, uint32_t upper_limit);
apx_error_t apx_vm_value_in_range_i64(int64_t value, int64_t lower_limit, int64_t upper_limit);
apx_error_t apx_vm_value_in_range_u64(uint64_t value, uint64_t lower_limit, uint64_t upper_limit);
uint32_t apx_vm_variant_to_size(uint8_t variant);
apx_type_code_t apx_vm_variant_to_type_code(uint8_t variant);
uint32_t apx_vm_size_type_to_size(apx_size_type_t size_type);
apx_size_type_t apx_vm_size_to_size_type(uint32_t size);
uint8_t apx_vm_size_type_to_variant(apx_size_type_t size_type);


#endif //APX_VM_COMMON_H
