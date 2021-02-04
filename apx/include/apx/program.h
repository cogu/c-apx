/*****************************************************************************
* \file      program.h
* \author    Conny Gustafsson
* \date      2020-11-30
* \brief     Utility functions for handling byte-code programs
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
void apx_program_decode_instruction(uint8_t instruction, uint8_t* opcode, uint8_t* variant, bool* flag);

#define APX_PROGRAM_CREATE(p) adt_bytearray_create(p, APX_PROGRAM_GROW_SIZE)
#define APX_PROGRAM_NEW() adt_bytearray_new(APX_PROGRAM_GROW_SIZE)
#define APX_PROGRAM_DESTROY(p) adt_bytearray_destroy(p)
#define APX_PROGRAM_DELETE(p) adt_bytearray_delete(p)

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_PROGRAM_H
