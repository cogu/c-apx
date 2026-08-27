/*****************************************************************************
* \file      vm.h
* \author    Conny Gustafsson
* \date      2020-02-24
* \brief     APX virtual machine for APX VM 2.0 standard
*
* Copyright (c) 2019-2021 Conny Gustafsson
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
#ifndef APX_VM_H
#define APX_VM_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/program.h"
#include "apx/serializer.h"
#include "apx/deserializer.h"
#include "apx/decoder.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_vm_tag
{
   apx_vm_serializer_t serializer;
   apx_vm_deserializer_t deserializer;
   apx_vm_decoder_t decoder;
   apx_programHeader_t program_header;
} apx_vm_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_vm_create(apx_vm_t *self);
void apx_vm_destroy(apx_vm_t *self);
apx_vm_t* apx_vm_new(void);
void apx_vm_delete(apx_vm_t *self);
apx_error_t apx_vm_select_program(apx_vm_t *self, apx_program_t const* program);
apx_error_t apx_vm_set_write_buffer(apx_vm_t* self, uint8_t* data, uint32_t size);
apx_error_t apx_vm_set_read_buffer(apx_vm_t* self, uint8_t const* data, uint32_t size);
apx_error_t apx_vm_pack_value(apx_vm_t *self, dtl_dv_t const* dv);
apx_error_t apx_vm_unpack_value(apx_vm_t *self, dtl_dv_t **dv);
size_t apx_vm_get_bytes_written(apx_vm_t *self);
size_t apx_vm_get_bytes_read(apx_vm_t *self);

#endif //APX_VM_H
