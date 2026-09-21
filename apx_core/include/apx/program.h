/*****************************************************************************
* \file      program.h
* \author    Conny Gustafsson
* \date      2020-11-30
* \brief     Utility functions for handling byte-code programs
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PROGRAM_H
#define APX_PROGRAM_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/vm_defs.h"
#include "adt_bytearray.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_PROGRAM_GROW_SIZE  64

typedef adt_bytearray_t apx_program_t;


typedef struct apx_programHeader_tag
{
   apx_programType_t program_type;
   uint32_t data_size;
   uint32_t element_size;
   uint32_t queue_length;
   bool has_dynamic_data;
} apx_programHeader_t;

apx_error_t apx_program_encode_header(apx_program_t *program, apx_programType_t program_type, uint32_t element_size, uint32_t queue_size, bool is_dynamic);
apx_error_t apx_program_decode_header(uint8_t const* begin, uint8_t const* end, uint8_t const** next, apx_programHeader_t *header);
uint8_t apx_program_encode_instruction(uint8_t opcode, uint8_t variant, bool flag);
void apx_program_decode_instruction(uint8_t const instruction, uint8_t* opcode, uint8_t* variant, bool* flag);
void apx_program_dump(apx_program_t const* program);

#define APX_PROGRAM_CREATE(p) adt_bytearray_create(p)
#define APX_PROGRAM_NEW() adt_bytearray_new()
#define APX_PROGRAM_DESTROY(p) adt_bytearray_destroy(p)
#define APX_PROGRAM_DELETE(p) adt_bytearray_delete(p)

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_PROGRAM_H
