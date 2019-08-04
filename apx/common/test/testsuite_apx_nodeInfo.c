/*****************************************************************************
* \file      testsuite_apx_nodeInfo.c
* \author    Conny Gustafsson
* \date      2019-01-04
* \brief     Unit test for apx_nodeInfo_t
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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"
#include "apx_nodeInfo.h"
#include "apx_parser.h"
#include "apx_test_nodes.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeInfo_updateFromString(CuTest *tc);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeInfo(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeInfo_updateFromString);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeInfo_updateFromString(CuTest *tc)
{
   apx_parser_t parser;
   apx_nodeInfo_t *nodeInfo;

   apx_parser_create(&parser);
   nodeInfo = apx_nodeInfo_new();
   CuAssertPtrNotNull(tc, nodeInfo);

   CuAssertPtrEquals(tc, 0, nodeInfo->node);
   CuAssertPtrEquals(tc, 0, nodeInfo->text);
   CuAssertUIntEquals(tc, 0u, nodeInfo->textLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInfo_updateFromString(nodeInfo, &parser, g_apx_test_node1));
   CuAssertPtrNotNull(tc, nodeInfo->node);
   CuAssertPtrNotNull(tc, nodeInfo->text);
   CuAssertTrue(tc, nodeInfo->textLen > 0);

   apx_parser_destroy(&parser);
   apx_nodeInfo_delete(nodeInfo);
}

