/*****************************************************************************
* \file      testsuite_apx_vm.c
* \author    Conny Gustafsson
* \date      2019-02-24
* \brief     Unit tests for APX Virtual Machine
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_compiler.h"
#include "apx_parser.h"
#include "apx_vm.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_vm_create(CuTest* tc);
static void test_apx_vm_parsePackHeader(CuTest* tc);
static void test_apx_vm_selectPackProgram(CuTest* tc);
static void test_apx_vm_packU8(CuTest* tc);
static void test_apx_vm_packU8FixArray(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_vm(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_vm_create);
   SUITE_ADD_TEST(suite, test_apx_vm_parsePackHeader);
   SUITE_ADD_TEST(suite, test_apx_vm_selectPackProgram);
   SUITE_ADD_TEST(suite, test_apx_vm_packU8);
   SUITE_ADD_TEST(suite, test_apx_vm_packU8FixArray);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_vm_create(CuTest* tc)
{
   apx_vm_t *vm = apx_vm_new();
   CuAssertPtrNotNull(tc, vm);
   apx_vm_delete(vm);
}

static void test_apx_vm_parsePackHeader(CuTest* tc)
{
   apx_compiler_t *compiler;
   uint8_t majorVersion;
   uint8_t minorVersion;
   uint32_t dataSize;
   uint8_t progType;

   adt_bytearray_t *program = adt_bytearray_new(APX_PROGRAM_GROW_SIZE);
   compiler =  apx_compiler_new();
   CuAssertPtrNotNull(tc, compiler);
   apx_compiler_begin(compiler, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_compiler_encodePackHeader(compiler, APX_VM_MAJOR_VERSION, APX_VM_MINOR_VERSION, 0x12345678));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_parsePackHeader(program, &majorVersion, &minorVersion, &progType, &dataSize));
   CuAssertUIntEquals(tc, APX_VM_MAJOR_VERSION, majorVersion);
   CuAssertUIntEquals(tc, APX_VM_MINOR_VERSION, minorVersion);
   CuAssertUIntEquals(tc, APX_VM_HEADER_PACK_PROG, progType);
   CuAssertUIntEquals(tc,  0x12345678, dataSize);

   apx_compiler_delete(compiler);
   adt_bytearray_delete(program);
}

static void test_apx_vm_selectPackProgram(CuTest* tc)
{
   apx_vm_t *vm = apx_vm_new();
   adt_bytearray_t *program = adt_bytearray_new(APX_PROGRAM_GROW_SIZE);
   apx_dataElement_t *element = apx_dataElement_new(APX_BASE_TYPE_UINT8, NULL);
   apx_compiler_t *compiler = apx_compiler_new();
   CuAssertPtrNotNull(tc, compiler);

   apx_compiler_begin(compiler, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_compiler_encodePackHeader(compiler, APX_VM_MAJOR_VERSION, APX_VM_MINOR_VERSION, 0u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_compiler_compilePackDataElement(compiler, element));
   apx_compiler_end(compiler);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_vm_setProgram(vm, program));
   CuAssertUIntEquals(tc, APX_VM_HEADER_PACK_PROG, apx_vm_getProgType(vm));
   CuAssertUIntEquals(tc, UINT8_SIZE, apx_vm_getDataSize(vm));

   apx_compiler_delete(compiler);
   apx_vm_delete(vm);
   adt_bytearray_delete(program);
   apx_dataElement_delete(element);
}

static void test_apx_vm_packU8(CuTest* tc)
{
   apx_vm_t *vm = apx_vm_new();
   adt_bytearray_t *program = adt_bytearray_new(APX_PROGRAM_GROW_SIZE);
   apx_dataElement_t *element = apx_dataElement_new(APX_BASE_TYPE_UINT8, NULL);
   apx_compiler_t *compiler = apx_compiler_new();
   dtl_sv_t *sv = dtl_sv_new();
   uint8_t dataBuffer[UINT8_SIZE];

   apx_compiler_begin(compiler, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_compiler_encodePackHeader(compiler, APX_VM_MAJOR_VERSION, APX_VM_MINOR_VERSION, 0u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_compiler_compilePackDataElement(compiler, element));
   apx_compiler_end(compiler);
   apx_compiler_delete(compiler);
   dataBuffer[0]=0xff;
   dtl_sv_set_u32(sv, 0u);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_vm_setProgram(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_setWriteBuffer(vm, dataBuffer, (apx_size_t) sizeof(dataBuffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serialize(vm, (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 1u, apx_vm_getBytesWritten(vm));
   CuAssertUIntEquals(tc, 0u, dataBuffer[0]);

   dtl_sv_set_u32(sv, 255u);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_vm_setProgram(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_setWriteBuffer(vm, dataBuffer, (apx_size_t) sizeof(dataBuffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serialize(vm, (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 1u, apx_vm_getBytesWritten(vm));
   CuAssertUIntEquals(tc, 255u, dataBuffer[0]);

   apx_vm_delete(vm);
   adt_bytearray_delete(program);
   apx_dataElement_delete(element);
   dtl_dec_ref(sv);
}

static void test_apx_vm_packU8FixArray(CuTest* tc)
{
   apx_vm_t *vm = apx_vm_new();
   adt_bytearray_t *program = adt_bytearray_new(APX_PROGRAM_GROW_SIZE);
   apx_dataElement_t *element;
   apx_compiler_t *compiler = apx_compiler_new();
   dtl_av_t *av = dtl_av_new();
   uint8_t dataBuffer[UINT8_SIZE*4];

   element = apx_dataElement_new(APX_BASE_TYPE_UINT8, NULL);
   apx_dataElement_setArrayLen(element, 4);

   apx_compiler_begin(compiler, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_compiler_encodePackHeader(compiler, APX_VM_MAJOR_VERSION, APX_VM_MINOR_VERSION, 0u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_compiler_compilePackDataElement(compiler, element));
   apx_compiler_end(compiler);
   apx_compiler_delete(compiler);
   memset(&dataBuffer[0], 0xff, sizeof(dataBuffer));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(1u), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(2u), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(3u), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(4u), false);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_vm_setProgram(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_setWriteBuffer(vm, dataBuffer, (apx_size_t) sizeof(dataBuffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serialize(vm, (dtl_dv_t*) av));
   CuAssertUIntEquals(tc, 4u, apx_vm_getBytesWritten(vm));
   CuAssertUIntEquals(tc, 1u, dataBuffer[0]);
   CuAssertUIntEquals(tc, 2u, dataBuffer[1]);
   CuAssertUIntEquals(tc, 3u, dataBuffer[2]);
   CuAssertUIntEquals(tc, 4u, dataBuffer[3]);

   apx_vm_delete(vm);
   adt_bytearray_delete(program);
   apx_dataElement_delete(element);
   dtl_dec_ref(av);
}
