/*****************************************************************************
* \file      compiler.h
* \author    Conny Gustafsson
* \date      2019-01-03
* \brief     APX bytecode compiler
*
* Copyright (c) 2019-2020 Conny Gustafsson
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
#ifndef APX_COMPILER_H
#define APX_COMPILER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/port.h"
#include "apx/program.h"
#include "apx/vm_defs.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


typedef struct apx_compiler_tag
{
   apx_program_t* program; //Strong reference
   apx_error_t last_error;
   bool has_dynamic_data;
}apx_compiler_t;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_compiler_create(apx_compiler_t *self);
void apx_compiler_destroy(apx_compiler_t *self);
apx_compiler_t* apx_compiler_new(void);
void apx_compiler_delete(apx_compiler_t *self);
apx_program_t* apx_compiler_compile_port(apx_compiler_t* self, apx_port_t* port, apx_programType_t program_type, apx_error_t* error_code);

#endif //APX_COMPILER_H
