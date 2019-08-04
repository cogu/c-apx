/*****************************************************************************
* \file      testsuite_apx_compiler.c
* \author    Conny Gustafsson
* \date      2019-01-03
* \brief     Description
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
#include "apx_vmdefs.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_compiler_pack_u8(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_compiler(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_compiler_pack_u8);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_compiler_pack_u8(CuTest* tc)
{
   const char *apx_text = "APX/1.2\n"
   "N\"TestNode\"\n"
   "P\"CurrentGear\"C(0,15)\n";
   apx_parser_t parser;
   apx_node_t *node;
   apx_compiler_t *compiler;
   adt_bytearray_t *program = adt_bytearray_new(APX_PROGRAM_GROW_DEFAULT);
   CuAssertPtrNotNull(tc, program);
   apx_parser_create(&parser);
   node = apx_parser_parseString(&parser, apx_text);
   CuAssertPtrNotNull(tc, node);
   compiler = apx_compiler_new();
   CuAssertPtrNotNull(tc, compiler);
   CuAssertIntEquals(tc, 0, adt_bytearray_length(program));
   CuAssertIntEquals(tc, APX_NO_ERROR,apx_compiler_compileProvidePort(compiler, node, 0, program));
   CuAssertIntEquals(tc, APX_INST_PACK_PROG_SIZE+APX_INST_PACK_U8_SIZE, adt_bytearray_length(program));
   apx_compiler_delete(compiler);
   adt_bytearray_delete(program);
   apx_parser_destroy(&parser);
}

