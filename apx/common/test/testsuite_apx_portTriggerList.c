/*****************************************************************************
* \file      testsuite_apx_portTriggerList.c
* \author    Conny Gustafsson
* \date      2018-12-07
* \brief     Unit tests for apx_portTriggerList
*
* Copyright (c) 2018 Conny Gustafsson
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
#include "apx_portTriggerList.h"
#include "apx_parser.h"
#include "apx_test_nodes.h"
#include "apx_nodeData.h"
#include "apx_portDataMap.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_create_apx_portTriggerListFromNode1(CuTest* tc);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_portTriggerList(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_apx_portTriggerListFromNode1);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_apx_portTriggerListFromNode1(CuTest* tc)
{
   apx_parser_t parser;
   apx_nodeData_t *nodeData1;
   apx_nodeData_t *nodeData3;
   apx_nodeData_t *nodeData4;
   apx_nodeData_t *nodeData5;
   apx_portRef_t *portData3;
   apx_portRef_t *portData4;
   apx_portRef_t *portData5;
   apx_portTriggerList_t triggerList; //a single trigger list for port 0 of nodeData1
   apx_parser_create(&parser);
   nodeData1 = apx_nodeData_makeFromString(&parser, g_apx_test_node1, NULL);
   CuAssertPtrNotNull(tc, nodeData1);
   nodeData3 = apx_nodeData_makeFromString(&parser, g_apx_test_node3, NULL);
   CuAssertPtrNotNull(tc, nodeData3);
   nodeData4 = apx_nodeData_makeFromString(&parser, g_apx_test_node4, NULL);
   CuAssertPtrNotNull(tc, nodeData4);
   nodeData5 = apx_nodeData_makeFromString(&parser, g_apx_test_node5, NULL);
   CuAssertPtrNotNull(tc, nodeData5);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData1, APX_SERVER_MODE ));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData3, APX_SERVER_MODE ));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData4, APX_SERVER_MODE ));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData5, APX_SERVER_MODE ));
   apx_parser_destroy(&parser);
   apx_portTriggerList_create(&triggerList);
   portData3 = apx_portDataMap_getRequirePortDataRef(apx_nodeData_getPortDataMap(nodeData3), 0);
   CuAssertPtrNotNull(tc, portData3);
   portData4 = apx_portDataMap_getRequirePortDataRef(apx_nodeData_getPortDataMap(nodeData4), 0);
   CuAssertPtrNotNull(tc, portData4);
   portData5 = apx_portDataMap_getRequirePortDataRef(apx_nodeData_getPortDataMap(nodeData5), 0);
   CuAssertPtrNotNull(tc, portData5);
   apx_portTriggerList_insert(&triggerList, portData3);
   apx_portTriggerList_insert(&triggerList, portData4);
   apx_portTriggerList_insert(&triggerList, portData5);
   CuAssertIntEquals(tc, 3, apx_portTriggerList_length(&triggerList));
   apx_portTriggerList_destroy(&triggerList);
   apx_nodeData_delete(nodeData1);
   apx_nodeData_delete(nodeData3);
   apx_nodeData_delete(nodeData4);
   apx_nodeData_delete(nodeData5);
}

