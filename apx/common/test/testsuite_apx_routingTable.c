/*****************************************************************************
* \file      testsuite_apx_routingTable.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Unit tests for apx_routingTable
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
#include "apx_routingTable.h"
#include "apx_parser.h"
#include "apx_nodeData.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char *m_node_text1 =
      "APX/1.2\n"
      "N\"LocalTestNode1\"\n"
      "R\"VehicleSpeed\"S:=65535\n"
      "R\"VehicleMode\"C(0,7):=7\n"
      "R\"SelectedGear\"C(0,15):=15\n";

static const char *m_node_text2 =
      "APX/1.2\n"
      "N\"LocalTestNode2\"\n"
      "P\"VehicleSpeed\"S:=65535\n"
      "P\"VehicleMode\"C(0,7):=7\n"
      "P\"SelectedGear\"C(0,15):=15\n";

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_routingTable_unconnectedRequirePortShallGetDefaultValues(CuTest* tc);
static void test_apx_routingTable_connectedRequirePortShallGetSenderValues(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_routingTable(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_routingTable_unconnectedRequirePortShallGetDefaultValues);
   SUITE_ADD_TEST(suite, test_apx_routingTable_connectedRequirePortShallGetSenderValues);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_routingTable_unconnectedRequirePortShallGetDefaultValues(CuTest* tc)
{
   uint8_t dataBuf[4] = {0,0,0,0};
   apx_routingTable_t routingTable;
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *nodeData = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   CuAssertPtrNotNull(tc, nodeData);
   apx_routingTable_create(&routingTable);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData, APX_SERVER_MODE));
   apx_routingTable_attachNodeData(&routingTable, nodeData);
   CuAssertIntEquals(tc, 4, apx_nodeData_getInPortDataLen(nodeData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_readInPortData(nodeData, dataBuf, 0, 4));
   CuAssertUIntEquals(tc, 255, dataBuf[0]);
   CuAssertUIntEquals(tc, 255, dataBuf[1]);
   CuAssertUIntEquals(tc, 7, dataBuf[2]);
   CuAssertUIntEquals(tc, 15, dataBuf[3]);
   apx_routingTable_destroy(&routingTable);
   apx_nodeData_delete(nodeData);
   apx_parser_delete(parser);
}

static void test_apx_routingTable_connectedRequirePortShallGetSenderValues(CuTest* tc)
{
   uint8_t provideDataBuf[4] = {0x12, 0x34, 4, 2};
   uint8_t requireDataBuf[4] = {0,0,0,0};
   apx_routingTable_t routingTable;
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *destNodeData = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   apx_nodeData_t *srcNodeData = apx_nodeData_makeFromString(parser, m_node_text2, NULL);
   CuAssertPtrNotNull(tc, destNodeData);
   CuAssertPtrNotNull(tc, srcNodeData);
   apx_routingTable_create(&routingTable);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(destNodeData, APX_SERVER_MODE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(srcNodeData, APX_SERVER_MODE));
   apx_routingTable_attachNodeData(&routingTable, srcNodeData); //attach node with provide ports first
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_writeOutPortData(srcNodeData, &provideDataBuf[0], 0, 4)); //update provide port data
   apx_routingTable_attachNodeData(&routingTable, destNodeData); //attach node with require ports second, init data should now be copied

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_readInPortData(destNodeData, &requireDataBuf[0], 0, 4));
   CuAssertUIntEquals(tc, provideDataBuf[0], requireDataBuf[0]);
   CuAssertUIntEquals(tc, provideDataBuf[1], requireDataBuf[1]);
   CuAssertUIntEquals(tc, provideDataBuf[2], requireDataBuf[2]);
   CuAssertUIntEquals(tc, provideDataBuf[3], requireDataBuf[3]);
   apx_routingTable_destroy(&routingTable);
   apx_nodeData_delete(destNodeData);
   apx_nodeData_delete(srcNodeData);
   apx_parser_delete(parser);
}

