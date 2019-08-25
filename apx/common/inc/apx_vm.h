/*****************************************************************************
* \file      apx_vm.h
* \author    Conny Gustafsson
* \date      2019-02-24
* \brief     APX virtual machine (implements v2 of APX byte code language)
*
* Copyright (c) 2019 Conny Gustafsson
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
#include "apx_types.h"
#include "apx_vmdefs.h"
#include "apx_portDataElement.h"
#include "adt_bytearray.h"
#include "apx_error.h"
#include "apx_vmSerializer.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_vm_tag
{
   apx_vmSerializer_t serializer;
   apx_size_t dataSize;
   const uint8_t *codeBegin; //weak reference
   const uint8_t *codeEnd;   //weak reference
   const uint8_t *codeNext;  //weak reference
   uint8_t progType;
   uint8_t expectedCode;
   uint32_t arrayLen;
   bool isArray;
   bool isDynamicArray;
} apx_vm_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_vm_create(apx_vm_t *self);
void apx_vm_destroy(apx_vm_t *self);
apx_vm_t* apx_vm_new(void);
void apx_vm_delete(apx_vm_t *self);
apx_error_t apx_vm_setProgram(apx_vm_t *self, apx_program_t *program);
uint8_t apx_vm_getProgType(apx_vm_t *self);
apx_size_t apx_vm_getDataSize(apx_vm_t *self);
apx_error_t apx_vm_setWriteBuffer(apx_vm_t *self, uint8_t *buffer, uint32_t bufSize);
apx_error_t apx_vm_serialize(apx_vm_t *self, const dtl_dv_t *dv);
apx_size_t apx_vm_getBytesWritten(apx_vm_t *self);

//stateless functions
apx_error_t apx_vm_parsePackHeader(adt_bytearray_t *program, uint8_t *majorVersion, uint8_t *minorVersion, uint8_t *progType, apx_size_t *dataSize);
apx_error_t apx_vm_decodeInstruction(uint8_t instruction, uint8_t *opCode, uint8_t *variant, uint8_t *flags);

#endif //APX_VM_H
