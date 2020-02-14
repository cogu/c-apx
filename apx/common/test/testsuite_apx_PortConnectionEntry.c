/*****************************************************************************
* \file      testsuite_apx_PortConnectionEntry.c
* \author    Conny Gustafsson
* \date      2019-01-28
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
#include <stdlib.h>
#include "CuTest.h"
#include <stdio.h>
#include "apx_portConnectionEntry.h"
#include "apx_nodeData.h"
#include "apx_parser.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portConnectionEntry_create(CuTest *tc);
static void test_apx_portConnectionEntry_connectOne(CuTest *tc);
static void test_apx_portConnectionEntry_connectThree(CuTest *tc);
static void test_apx_portConnectionEntry_disconnectOne(CuTest *tc);
static void test_apx_portConnectionEntry_disconnectThree(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const char *m_node_text1 =
      "APX/1.2\n"
      "N\"LocalTestNode1\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

static const char *m_node_text2 =
      "APX/1.2\n"
      "N\"LocalTestNode2\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

static const char *m_node_text3 =
      "APX/1.2\n"
      "N\"LocalTestNode3\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_portConnectionEntry(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_create);
   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_connectOne);
   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_connectThree);
   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_disconnectOne);
   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_disconnectThree);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portConnectionEntry_create(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_portConnectionEntry_create(&entry);
   CuAssertIntEquals(tc, 0, entry.count);
   CuAssertPtrEquals(tc, NULL, entry.pAny);
   apx_portConnectionEntry_destroy(&entry);
}

static void test_apx_portConnectionEntry_connectOne(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_parser_t *parser;
   apx_nodeData_t *nodeData1;
   apx_portDataRef_t *portRef1;
   parser = apx_parser_new();
   nodeData1 = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   CuAssertPtrNotNull(tc, nodeData1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData1, APX_SERVER_MODE));

   apx_portConnectionEntry_create(&entry);
   portRef1 = apx_nodeData_getRequirePortDataRef(nodeData1, 0);
   apx_portConnectionEntry_addConnection(&entry, portRef1);
   CuAssertIntEquals(tc, 1, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectionEntry_get(&entry, 0));
   apx_portConnectionEntry_destroy(&entry);

   apx_parser_delete(parser);
   apx_nodeData_delete(nodeData1);
}

static void test_apx_portConnectionEntry_connectThree(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_parser_t *parser;
   apx_nodeData_t *nodeData1;
   apx_nodeData_t *nodeData2;
   apx_nodeData_t *nodeData3;
   apx_portDataRef_t *portRef1;
   apx_portDataRef_t *portRef2;
   apx_portDataRef_t *portRef3;

   parser = apx_parser_new();
   nodeData1 = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   nodeData2 = apx_nodeData_makeFromString(parser, m_node_text2, NULL);
   nodeData3 = apx_nodeData_makeFromString(parser, m_node_text3, NULL);
   CuAssertPtrNotNull(tc, nodeData1);
   CuAssertPtrNotNull(tc, nodeData2);
   CuAssertPtrNotNull(tc, nodeData3);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData1, APX_SERVER_MODE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData2, APX_SERVER_MODE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData3, APX_SERVER_MODE));

   apx_portConnectionEntry_create(&entry);
   portRef1 = apx_nodeData_getRequirePortDataRef(nodeData1, 0);
   portRef2 = apx_nodeData_getRequirePortDataRef(nodeData2, 0);
   portRef3 = apx_nodeData_getRequirePortDataRef(nodeData3, 0);
   apx_portConnectionEntry_addConnection(&entry, portRef1);
   apx_portConnectionEntry_addConnection(&entry, portRef2);
   apx_portConnectionEntry_addConnection(&entry, portRef3);
   CuAssertIntEquals(tc, 3, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectionEntry_get(&entry, 0));
   CuAssertPtrEquals(tc, portRef2, apx_portConnectionEntry_get(&entry, 1));
   CuAssertPtrEquals(tc, portRef3, apx_portConnectionEntry_get(&entry, 2));
   apx_portConnectionEntry_destroy(&entry);

   apx_parser_delete(parser);
   apx_nodeData_delete(nodeData1);
   apx_nodeData_delete(nodeData2);
   apx_nodeData_delete(nodeData3);
}

static void test_apx_portConnectionEntry_disconnectOne(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_parser_t *parser;
   apx_nodeData_t *nodeData1;
   apx_portDataRef_t *portRef1;
   parser = apx_parser_new();
   nodeData1 = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   CuAssertPtrNotNull(tc, nodeData1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData1, APX_SERVER_MODE));

   apx_portConnectionEntry_create(&entry);
   portRef1 = apx_nodeData_getRequirePortDataRef(nodeData1, 0);
   apx_portConnectionEntry_removeConnection(&entry, portRef1);
   CuAssertIntEquals(tc, -1, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectionEntry_get(&entry, 0));
   apx_portConnectionEntry_destroy(&entry);

   apx_parser_delete(parser);
   apx_nodeData_delete(nodeData1);
}

static void test_apx_portConnectionEntry_disconnectThree(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_parser_t *parser;
   apx_nodeData_t *nodeData1;
   apx_nodeData_t *nodeData2;
   apx_nodeData_t *nodeData3;
   apx_portDataRef_t *portRef1;
   apx_portDataRef_t *portRef2;
   apx_portDataRef_t *portRef3;

   parser = apx_parser_new();
   nodeData1 = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   nodeData2 = apx_nodeData_makeFromString(parser, m_node_text2, NULL);
   nodeData3 = apx_nodeData_makeFromString(parser, m_node_text3, NULL);
   CuAssertPtrNotNull(tc, nodeData1);
   CuAssertPtrNotNull(tc, nodeData2);
   CuAssertPtrNotNull(tc, nodeData3);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData1, APX_SERVER_MODE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData2, APX_SERVER_MODE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData3, APX_SERVER_MODE));

   apx_portConnectionEntry_create(&entry);
   portRef1 = apx_nodeData_getRequirePortDataRef(nodeData1, 0);
   portRef2 = apx_nodeData_getRequirePortDataRef(nodeData2, 0);
   portRef3 = apx_nodeData_getRequirePortDataRef(nodeData3, 0);
   apx_portConnectionEntry_removeConnection(&entry, portRef1);
   apx_portConnectionEntry_removeConnection(&entry, portRef2);
   apx_portConnectionEntry_removeConnection(&entry, portRef3);
   CuAssertIntEquals(tc, -3, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectionEntry_get(&entry, 0));
   CuAssertPtrEquals(tc, portRef2, apx_portConnectionEntry_get(&entry, 1));
   CuAssertPtrEquals(tc, portRef3, apx_portConnectionEntry_get(&entry, 2));
   apx_portConnectionEntry_destroy(&entry);

   apx_parser_delete(parser);
   apx_nodeData_delete(nodeData1);
   apx_nodeData_delete(nodeData2);
   apx_nodeData_delete(nodeData3);
}
