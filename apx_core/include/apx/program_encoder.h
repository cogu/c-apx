/*****************************************************************************
* \file      program_encoder.h
* \author    Conny Gustafsson
* \date      2021-01-15
* \brief     APX VM 2.1 bytecode program encoder
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PROGRAM_ENCODER_H
#define APX_PROGRAM_ENCODER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx/error.h"
#include "apx/types.h"
#include "apx/vm_defs.h"
#include "apx/program.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_program_encoder_tag
{
   apx_program_t header;
   apx_program_t buffer;
   apx_type_code_t last_type_code;
} apx_program_encoder_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_program_encoder_create(apx_program_encoder_t* self);
void apx_program_encoder_destroy(apx_program_encoder_t* self);
void apx_program_encoder_clear(apx_program_encoder_t* self);
apx_error_t apx_program_encoder_encode_instruction(apx_program_encoder_t* self, uint8_t opcode, uint8_t variant, bool flag);
apx_error_t apx_program_encoder_encode_header_instruction(apx_program_encoder_t* self, uint8_t opcode, uint8_t variant, bool flag);
apx_error_t apx_program_encoder_encode_array_size(apx_program_encoder_t* self, uint32_t array_size, bool is_dynamic);
apx_error_t apx_program_encoder_encode_limit_check_instruction(apx_program_encoder_t* self, uint8_t variant, int64_t lower_limit, int64_t upper_limit, bool is_array);
apx_error_t apx_program_encoder_encode_limit_values(apx_program_encoder_t* self, uint8_t variant, int64_t lower_limit, int64_t upper_limit);
apx_error_t apx_program_encoder_encode_program_header(apx_program_encoder_t* self, apx_program_type_t program_type, uint32_t elem_size, uint32_t queue_size, bool is_dynamic);
apx_error_t apx_program_encoder_encode_field_name(apx_program_encoder_t* self, const char* name);
apx_program_t const* apx_program_encoder_get_header(apx_program_encoder_t const* self);
apx_program_t const* apx_program_encoder_get_buffer(apx_program_encoder_t const* self);
apx_error_t apx_program_encoder_get_program(apx_program_encoder_t const* self, apx_program_t* out_program);

#endif //APX_PROGRAM_ENCODER_H
