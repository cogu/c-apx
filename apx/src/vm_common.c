/*****************************************************************************
* \file      vm_common.c
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/vm_common.h"
#include "pack.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

uint8_t const* apx_vm_parse_uint32_by_variant(uint8_t const* begin, uint8_t const* end, uint8_t variant, uint32_t* number)
{
   uint8_t const* next = begin;
   uint8_t const unpack_size = (uint8_t)apx_vm_variant_to_size(variant);
   if ((unpack_size == 0) || (next + unpack_size) > end)
   {
      return NULL;
   }
   *number = (uint32_t)unpackLE(next, unpack_size);
   return next + unpack_size;
}

uint8_t const* apx_vm_parse_uint64_by_variant(uint8_t const* begin, uint8_t const* end, uint8_t variant, uint64_t* number)
{
   uint8_t const* next = begin;
   uint8_t const unpack_size = (uint8_t)apx_vm_variant_to_size(variant);
   if ((unpack_size == 0) || (next + unpack_size) > end)
   {
      return NULL;
   }
   *number = unpackLE64(next, unpack_size);
   return next + unpack_size;
}

uint8_t const* apx_vm_parse_int32_by_variant(uint8_t const* begin, uint8_t const* end, uint8_t variant, int32_t* number)
{
   uint8_t const* next = begin;
   uint8_t const unpack_size = (uint8_t)apx_vm_variant_to_size(variant);
   if ((unpack_size == 0) || (next + unpack_size) > end)
   {
      return NULL;
   }
   switch (variant)
   {
   case APX_VM_VARIANT_INT8:
      *number = (int32_t)((int8_t)unpackLE(next, unpack_size));
      break;
   case APX_VM_VARIANT_INT16:
      *number = (int32_t)((int16_t)unpackLE(next, unpack_size));
      break;
   case APX_VM_VARIANT_INT32:
      *number = (int32_t)unpackLE(next, unpack_size);
      break;
   default:
      return NULL;
   }
   return next + unpack_size;
}

uint8_t const* apx_vm_parse_int64_by_variant(uint8_t const* begin, uint8_t const* end, uint8_t variant, int64_t* number)
{
   uint8_t const* next = begin;
   uint8_t const unpack_size = (uint8_t)apx_vm_variant_to_size(variant);
   if ((unpack_size == 0) || (next + unpack_size) > end)
   {
      return NULL;
   }
   *number = (int64_t)unpackLE64(next, unpack_size);
   return next + unpack_size;
}

uint8_t const* apx_vm_parse_uint32_by_size_type(uint8_t const* begin, uint8_t const* end, apx_sizeType_t size_type, uint32_t* number)
{
   uint8_t variant = apx_vm_size_type_to_variant(size_type);
   if (variant == APX_VM_VARIANT_INVALID)
   {
      return NULL;
   }
   return apx_vm_parse_uint32_by_variant(begin, end, variant, number);
}

apx_error_t apx_vm_value_in_range_i32(int32_t value, int32_t lower_limit, int32_t upper_limit)
{
   if (((lower_limit > INT32_MIN) && (value < lower_limit)) ||
      ((upper_limit < INT32_MAX) && (value > upper_limit)))
   {
      return APX_VALUE_RANGE_ERROR;
   }
   return APX_NO_ERROR;
}

apx_error_t apx_vm_value_in_range_u32(uint32_t value, uint32_t lower_limit, uint32_t upper_limit)
{
   if (((lower_limit > 0u) && (value < lower_limit)) ||
      ((upper_limit < UINT32_MAX) && (value > upper_limit)))
   {
      return APX_VALUE_RANGE_ERROR;
   }
   return APX_NO_ERROR;
}

apx_error_t apx_vm_value_in_range_i64(int64_t value, int64_t lower_limit, int64_t upper_limit)
{
   if (((lower_limit > INT64_MIN) && (value < lower_limit)) ||
      ((upper_limit < INT64_MAX) && (value > upper_limit)))
   {
      return APX_VALUE_RANGE_ERROR;
   }
   return APX_NO_ERROR;
}

apx_error_t apx_vm_value_in_range_u64(uint64_t value, uint64_t lower_limit, uint64_t upper_limit)
{
   if (((lower_limit > 0u) && (value < lower_limit)) ||
      ((upper_limit < UINT64_MAX) && (value > upper_limit)))
   {
      return APX_VALUE_RANGE_ERROR;
   }
   return APX_NO_ERROR;
}

uint32_t apx_vm_variant_to_size(uint8_t variant)
{

   uint32_t retval = 0u;
   switch (variant)
   {
   case APX_VM_VARIANT_UINT8:
   case APX_VM_VARIANT_BOOL:
   case APX_VM_VARIANT_BYTE:
   case APX_VM_VARIANT_CHAR:
   case APX_VM_VARIANT_CHAR8:
      retval = UINT8_SIZE;
      break;
   case APX_VM_VARIANT_UINT16:
   case APX_VM_VARIANT_CHAR16:
      retval = UINT16_SIZE;
      break;
   case APX_VM_VARIANT_UINT32:
   case APX_VM_VARIANT_CHAR32:
      retval = UINT32_SIZE;
      break;
   case APX_VM_VARIANT_UINT64:
      retval = UINT64_SIZE;
      break;
   case APX_VM_VARIANT_INT8:
      retval = INT8_SIZE;
      break;
   case APX_VM_VARIANT_INT16:
      retval = INT16_SIZE;
      break;
   case APX_VM_VARIANT_INT32:
      retval = INT32_SIZE;
      break;
   case APX_VM_VARIANT_INT64:
      retval = INT64_SIZE;
      break;
   }
   return retval;
}

apx_typeCode_t apx_vm_variant_to_type_code(uint8_t variant)
{
   apx_typeCode_t retval;
   switch (variant)
   {
   case APX_VM_VARIANT_UINT8:
      retval = APX_TYPE_CODE_UINT8;
      break;
   case APX_VM_VARIANT_UINT16:
      retval = APX_TYPE_CODE_UINT16;
      break;
   case APX_VM_VARIANT_UINT32:
      retval = APX_TYPE_CODE_UINT32;
      break;
   case APX_VM_VARIANT_UINT64:
      retval = APX_TYPE_CODE_UINT64;
      break;
   case APX_VM_VARIANT_INT8:
      retval = APX_TYPE_CODE_INT8;
      break;
   case APX_VM_VARIANT_INT16:
      retval = APX_TYPE_CODE_INT16;
      break;
   case APX_VM_VARIANT_INT32:
      retval = APX_TYPE_CODE_INT32;
      break;
   case APX_VM_VARIANT_INT64:
      retval = APX_TYPE_CODE_INT64;
      break;
   case APX_VM_VARIANT_BOOL:
      retval = APX_TYPE_CODE_BOOL;
      break;
   case APX_VM_VARIANT_BYTE:
      retval = APX_TYPE_CODE_BYTE;
      break;
   case APX_VM_VARIANT_RECORD:
      retval = APX_TYPE_CODE_RECORD;
      break;
   case APX_VM_VARIANT_CHAR:
      retval = APX_TYPE_CODE_CHAR;
      break;
   case APX_VM_VARIANT_CHAR8:
      retval = APX_TYPE_CODE_CHAR8;
      break;
   case APX_VM_VARIANT_CHAR16:
      retval = APX_TYPE_CODE_CHAR16;
      break;
   case APX_VM_VARIANT_CHAR32:
      retval = APX_TYPE_CODE_CHAR32;
      break;
   default:
      retval = APX_TYPE_CODE_NONE;
   }
   return retval;
}

uint32_t apx_vm_size_type_to_size(apx_sizeType_t size_type)
{
   uint32_t value_size;
   switch (size_type)
   {
   case APX_SIZE_TYPE_UINT8:
      value_size = UINT8_SIZE;
      break;
   case APX_SIZE_TYPE_UINT16:
      value_size = UINT16_SIZE;
      break;
   case APX_SIZE_TYPE_UINT32:
      value_size = UINT32_SIZE;
      break;
   default:
      value_size = 0u;
   }
   return value_size;
}

apx_sizeType_t apx_vm_size_to_size_type(uint32_t size)
{
   apx_sizeType_t size_type = APX_SIZE_TYPE_NONE;
   if (size > 0u)
   {
      if (size <= UINT8_MAX)
      {
         size_type = APX_SIZE_TYPE_UINT8;
      }
      else if (size <= UINT16_MAX)
      {
         size_type = APX_SIZE_TYPE_UINT16;
      }
      else
      {
         size_type = APX_SIZE_TYPE_UINT32;
      }
   }
   return size_type;

}

uint8_t apx_vm_size_type_to_variant(apx_sizeType_t size_type)
{
   uint8_t variant = APX_VM_VARIANT_INVALID;
   switch (size_type)
   {
   case APX_SIZE_TYPE_UINT8:
      variant = APX_VM_VARIANT_UINT8;
      break;
   case APX_SIZE_TYPE_UINT16:
      variant = APX_VM_VARIANT_UINT16;
      break;
   case APX_SIZE_TYPE_UINT32:
      variant = APX_VM_VARIANT_UINT32;
      break;
   }
   return variant;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
