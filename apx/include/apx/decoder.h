/*****************************************************************************
* \file      decoder.h
* \author    Conny Gustafsson
* \date      2021-01-15
* \brief     APX program decoder class
*
* Copyright (c) 2021 Conny Gustafsson
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
#ifndef APX_VM_DECODER_H
#define APX_VM_DECODER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/vm_defs.h"
#include "apx/vm_common.h"
#include "apx/program.h"
#include "apx/error.h"
#include "adt_str.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_vm_decoder_tag
{
   uint8_t const* program_begin;
   uint8_t const* program_next;
   uint8_t const* program_end;
   uint8_t const* program_mark;
   apx_operationType_t operation_type;
   apx_packUnpackOperationInfo_t pack_unpack_info;
   apx_rangeCheckUInt32OperationInfo_t range_check_uint32_info;
   apx_rangeCheckUInt64OperationInfo_t range_check_uint64_info;
   apx_rangeCheckInt32OperationInfo_t range_check_int32_info;
   apx_rangeCheckInt64OperationInfo_t range_check_int64_info;
   adt_str_t field_name;
   bool is_last_field;
} apx_vm_decoder_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_vm_decoder_create(apx_vm_decoder_t* self);
void apx_vm_decoder_destroy(apx_vm_decoder_t* self);
apx_error_t apx_vm_decoder_select_program(apx_vm_decoder_t* self, uint8_t const* data, uint32_t size);
apx_error_t apx_vm_decoder_parse_program_header(apx_vm_decoder_t* self, apx_programHeader_t* header);
apx_error_t apx_vm_decoder_parse_next_operation(apx_vm_decoder_t* self, apx_operationType_t* operation_type);
void apx_vm_decoder_get_pack_unpack_info(apx_vm_decoder_t const* self, apx_packUnpackOperationInfo_t* info);
void apx_vm_decoder_range_check_info_int32(apx_vm_decoder_t const* self, apx_rangeCheckInt32OperationInfo_t* info);
void apx_vm_decoder_range_check_info_uint32(apx_vm_decoder_t const* self, apx_rangeCheckUInt32OperationInfo_t* info);
void apx_vm_decoder_range_check_info_int64(apx_vm_decoder_t const* self, apx_rangeCheckInt64OperationInfo_t* info);
void apx_vm_decoder_range_check_info_uint64(apx_vm_decoder_t const* self, apx_rangeCheckUInt64OperationInfo_t* info);
char const* apx_vm_decoder_get_field_name(apx_vm_decoder_t* self);
void apx_vm_decoder_save_program_position(apx_vm_decoder_t* self);
void apx_vm_decoder_recall_program_position(apx_vm_decoder_t* self);
bool apx_vm_decoder_has_saved_program_position(apx_vm_decoder_t* self);
bool apx_vm_decoder_is_last_field(apx_vm_decoder_t* self);

#endif //APX_VM_DECODER_H
