/*****************************************************************************
* \file      vm.h
* \author    Conny Gustafsson
* \date      2020-02-24
* \brief     APX virtual machine for APX VM 2.1 standard
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
#include "apx/program_decoder.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_vm_tag
{
   apx_vm_serializer_t serializer;
   apx_vm_deserializer_t deserializer;
   apx_programDecoder_t decoder;
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
