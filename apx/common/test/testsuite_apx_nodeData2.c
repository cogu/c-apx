/*****************************************************************************
* \file      testsuite_apx_nodeInstance.c
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Unit tests for apx_nodeInstance
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "apx_parser.h"
#include "apx_nodeInstance.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeData2_writeDefinitionBuffer(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeData2(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeData2_writeDefinitionBuffer);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeData2_writeDefinitionBuffer(CuTest *tc)
{
   const char *apx_text = "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"TestPort\"C:=255\n";

   apx_nodeData2_t *nodeData;
   apx_size_t apx_len = (apx_size_t) strlen(apx_text);

   nodeData =  apx_nodeData2_new();
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData2_createDefinitionBuffer(nodeData, apx_len ));
   CuAssertUIntEquals(tc, (apx_size_t) strlen(apx_text), nodeData->definitionDataLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData2_writeDefinitionData(nodeData, (const uint8_t*) apx_text, 0u, apx_len));
   apx_nodeData2_delete(nodeData);

}

