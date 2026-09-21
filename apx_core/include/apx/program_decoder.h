/*****************************************************************************
* \file      program_decoder.h
* \author    Conny Gustafsson
* \date      2021-01-15
* \brief     APX VM 2.1 bytecode program decoder
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PROGRAM_DECODER_H
#define APX_PROGRAM_DECODER_H

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
typedef struct apx_program_decoder_tag
{
   uint8_t const* program_begin;
   uint8_t const* program_next;
   uint8_t const* program_end;
   uint8_t const* program_mark;
   apx_operation_type_t operation_type;
   apx_type_code_t last_type_code;
   apx_pack_unpack_operation_info_t pack_unpack_info;
   apx_range_check_uint32_operation_info_t range_check_uint32_info;
   apx_range_check_uint64_operation_info_t range_check_uint64_info;
   apx_range_check_int32_operation_info_t range_check_int32_info;
   apx_range_check_int64_operation_info_t range_check_int64_info;
   adt_str_t field_name;
   bool is_first_field;
   bool is_array_limit;
} apx_program_decoder_t;

typedef apx_program_decoder_t apx_vm_decoder_t; // Backward compatibility

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_programDecoder_create(apx_program_decoder_t* self);
void apx_programDecoder_destroy(apx_program_decoder_t* self);
apx_error_t apx_programDecoder_select_program(apx_program_decoder_t* self, uint8_t const* data, uint32_t size);
apx_error_t apx_programDecoder_parse_program_header(apx_program_decoder_t* self, apx_program_header_t* header);
apx_error_t apx_programDecoder_parse_next_operation(apx_program_decoder_t* self, apx_operation_type_t* operation_type);
void apx_programDecoder_get_pack_unpack_info(apx_program_decoder_t const* self, apx_pack_unpack_operation_info_t* info);
void apx_programDecoder_range_check_info_int32(apx_program_decoder_t const* self, apx_range_check_int32_operation_info_t* info);
void apx_programDecoder_range_check_info_uint32(apx_program_decoder_t const* self, apx_range_check_uint32_operation_info_t* info);
void apx_programDecoder_range_check_info_int64(apx_program_decoder_t const* self, apx_range_check_int64_operation_info_t* info);
void apx_programDecoder_range_check_info_uint64(apx_program_decoder_t const* self, apx_range_check_uint64_operation_info_t* info);
char const* apx_programDecoder_get_field_name(apx_program_decoder_t* self);
void apx_programDecoder_save_program_position(apx_program_decoder_t* self);
void apx_programDecoder_recall_program_position(apx_program_decoder_t* self);
bool apx_programDecoder_has_saved_program_position(apx_program_decoder_t* self);
bool apx_programDecoder_is_first_field(apx_program_decoder_t* self);
bool apx_programDecoder_is_array_limit(apx_program_decoder_t* self);

// Backward compatibility macros
#define apx_vm_decoder_create apx_programDecoder_create
#define apx_vm_decoder_destroy apx_programDecoder_destroy
#define apx_vm_decoder_select_program apx_programDecoder_select_program
#define apx_vm_decoder_parse_program_header apx_programDecoder_parse_program_header
#define apx_vm_decoder_parse_next_operation apx_programDecoder_parse_next_operation
#define apx_vm_decoder_get_pack_unpack_info apx_programDecoder_get_pack_unpack_info
#define apx_vm_decoder_range_check_info_int32 apx_programDecoder_range_check_info_int32
#define apx_vm_decoder_range_check_info_uint32 apx_programDecoder_range_check_info_uint32
#define apx_vm_decoder_range_check_info_int64 apx_programDecoder_range_check_info_int64
#define apx_vm_decoder_range_check_info_uint64 apx_programDecoder_range_check_info_uint64
#define apx_vm_decoder_get_field_name apx_programDecoder_get_field_name
#define apx_vm_decoder_save_program_position apx_programDecoder_save_program_position
#define apx_vm_decoder_recall_program_position apx_programDecoder_recall_program_position
#define apx_vm_decoder_has_saved_program_position apx_programDecoder_has_saved_program_position
#define apx_vm_decoder_is_first_field apx_programDecoder_is_first_field

#endif //APX_PROGRAM_DECODER_H
