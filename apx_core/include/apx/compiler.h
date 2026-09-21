/*****************************************************************************
* \file      compiler.h
* \author    Conny Gustafsson
* \date      2019-01-03
* \brief     APX bytecode compiler
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
apx_program_t* apx_compiler_compile_port(apx_compiler_t* self, apx_port_t* port, apx_program_type_t program_type, apx_error_t* error_code);

#endif //APX_COMPILER_H
