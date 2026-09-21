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
typedef struct apx_programEncoder_tag
{
   apx_program_t header;
   apx_program_t buffer;
   apx_typeCode_t last_type_code;
} apx_programEncoder_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_programEncoder_create(apx_programEncoder_t* self);
void apx_programEncoder_destroy(apx_programEncoder_t* self);
void apx_programEncoder_clear(apx_programEncoder_t* self);
apx_error_t apx_programEncoder_encode_instruction(apx_programEncoder_t* self, uint8_t opcode, uint8_t variant, bool flag);
apx_error_t apx_programEncoder_encode_header_instruction(apx_programEncoder_t* self, uint8_t opcode, uint8_t variant, bool flag);
apx_error_t apx_programEncoder_encode_array_size(apx_programEncoder_t* self, uint32_t array_size, bool is_dynamic);
apx_error_t apx_programEncoder_encode_limit_check_instruction(apx_programEncoder_t* self, uint8_t variant, int64_t lower_limit, int64_t upper_limit, bool is_array);
apx_error_t apx_programEncoder_encode_limit_values(apx_programEncoder_t* self, uint8_t variant, int64_t lower_limit, int64_t upper_limit);
apx_error_t apx_programEncoder_encode_program_header(apx_programEncoder_t* self, apx_programType_t program_type, uint32_t elem_size, uint32_t queue_size, bool is_dynamic);
apx_error_t apx_programEncoder_encode_field_name(apx_programEncoder_t* self, const char* name);
apx_program_t const* apx_programEncoder_get_header(apx_programEncoder_t const* self);
apx_program_t const* apx_programEncoder_get_buffer(apx_programEncoder_t const* self);
apx_error_t apx_programEncoder_get_program(apx_programEncoder_t const* self, apx_program_t* out_program);

#endif //APX_PROGRAM_ENCODER_H
