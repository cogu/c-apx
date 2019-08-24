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
static void test_apx_vm_pack_u8(CuTest* tc);

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
   SUITE_ADD_TEST(suite, test_apx_vm_pack_u8);

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

   adt_bytearray_t *program = adt_bytearray_new(APX_PROGRAM_GROW_SIZE);
   compiler =  apx_compiler_new();
   CuAssertPtrNotNull(tc, compiler);
   apx_compiler_setBuffer(compiler, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_compiler_encodePackHeader(compiler, APX_VM_MAJOR_VERSION, APX_VM_MINOR_VERSION, 0x12345678));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_parsePackHeader(program, &majorVersion, &minorVersion, &dataSize));
   CuAssertUIntEquals(tc, APX_VM_MAJOR_VERSION, majorVersion);
   CuAssertUIntEquals(tc, APX_VM_MINOR_VERSION, minorVersion);
   CuAssertUIntEquals(tc,  0x12345678, dataSize);

   apx_compiler_delete(compiler);
   adt_bytearray_delete(program);
}

static void test_apx_vm_pack_u8(CuTest* tc)
{
   apx_vm_t *vm = apx_vm_new();
   apx_vm_delete(vm);
}


