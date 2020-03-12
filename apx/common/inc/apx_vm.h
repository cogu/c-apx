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
#include "adt_bytearray.h"
#include "apx_error.h"
#include "apx_vmSerializer.h"
#include "apx_vmDeserializer.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_vm_tag
{
   apx_vmSerializer_t serializer;
   apx_vmDeserializer_t deserializer;
   apx_size_t progDataSize; //maximum allowed data size (from program header)
   const uint8_t *progBegin; //weak reference
   const uint8_t *progEnd;   //weak reference
   const uint8_t *progNext;  //weak reference
   uint32_t maxArrayLen; //Maximum array len (read from program)
   uint32_t arrayLen; //Current array len (read from data). Only applies to dynamic array
   uint8_t progType; // APX_VM_HEADER_PACK_PROG or APX_VM_HEADER_UNPACK_PROG
   uint8_t expectedNext; //The opcode(s) to expect next
   bool isArray;
   apx_dynLenType_t dynLenType;
} apx_vm_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_vm_create(apx_vm_t *self);
void apx_vm_destroy(apx_vm_t *self);
apx_vm_t* apx_vm_new(void);
void apx_vm_delete(apx_vm_t *self);
apx_error_t apx_vm_selectProgram(apx_vm_t *self, const adt_bytes_t *program);
uint8_t apx_vm_getProgType(apx_vm_t *self);
apx_size_t apx_vm_getProgDataSize(apx_vm_t *self);
apx_error_t apx_vm_setWriteBuffer(apx_vm_t *self, uint8_t *buffer, uint32_t bufSize);
apx_error_t apx_vm_setReadBuffer(apx_vm_t *self, const uint8_t *buffer, uint32_t bufSize);
apx_error_t apx_vm_packValue(apx_vm_t *self, const dtl_dv_t *dv);
apx_error_t apx_vm_unpackValue(apx_vm_t *self, dtl_dv_t **dv);
apx_error_t apx_vm_writeNullValue(apx_vm_t *self);
#ifdef UNIT_TEST
apx_size_t apx_vm_getBytesWritten(apx_vm_t *self);
apx_size_t apx_vm_getBytesRead(apx_vm_t *self);
#endif

//state-less functions
apx_error_t apx_vm_decodeProgramHeader(const adt_bytes_t *program, uint8_t *majorVersion, uint8_t *minorVersion, uint8_t *progType, apx_size_t *maxDataSize);
apx_error_t apx_vm_decodeProgramDataProps(const adt_bytes_t *program, apx_size_t *dataSize, uint8_t *dataFlags);
apx_error_t apx_vm_decodeInstruction(uint8_t instruction, uint8_t *opcode, uint8_t *variant, uint8_t *flags);

#endif //APX_VM_H
