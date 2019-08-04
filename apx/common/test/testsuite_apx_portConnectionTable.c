/*****************************************************************************
* \file      testsuite_apx_portConnectionTable.c
* \author    Conny Gustafsson
* \date      2019-01-31
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
#include "apx_portConnectionTable.h"
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
static void test_apx_portConnectionTable_create(CuTest *tc);
static void test_apx_portConnectionTable_connectPorts(CuTest *tc);
static void test_apx_portConnectionTable_disconnectRequirePorts(CuTest *tc);
static void test_apx_portConnectionTable_disconnectProvidePort(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const char *m_node_text1 =
      "APX/1.2\n"
      "N\"LocalTestNode1\"\n"
      "P\"EngineSpeed\"S:=65535\n"
      "P\"VehicleSpeed\"S:=65535\n";


static const char *m_node_text2 =
      "APX/1.2\n"
      "N\"LocalTestNode1\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

static const char *m_node_text3 =
      "APX/1.2\n"
      "N\"LocalTestNode1\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_portConnectionTable(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_portConnectionTable_create);
   SUITE_ADD_TEST(suite, test_apx_portConnectionTable_connectPorts);
   SUITE_ADD_TEST(suite, test_apx_portConnectionTable_disconnectRequirePorts);
   SUITE_ADD_TEST(suite, test_apx_portConnectionTable_disconnectProvidePort);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portConnectionTable_create(CuTest *tc)
{
   apx_portConnectionTable_t *table = apx_portConnectionTable_new(3);

   CuAssertPtrNotNull(tc, table);
   CuAssertPtrNotNull(tc, table->connections);
   CuAssertIntEquals(tc, 3, table->numPorts);
   apx_portConnectionTable_delete(table);
}

static void test_apx_portConnectionTable_connectPorts(CuTest *tc)
{
   apx_portConnectionTable_t *node1Connections;
   apx_parser_t *parser;
   apx_nodeData_t *nodeData1;
   apx_nodeData_t *nodeData2;
   apx_portDataRef_t *providePortRef;
   apx_portDataRef_t *requirePortRef;
   parser = apx_parser_new();
   nodeData1 = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   CuAssertPtrNotNull(tc, nodeData1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData1, APX_SERVER_MODE));
   nodeData2 = apx_nodeData_makeFromString(parser, m_node_text2, NULL);
   CuAssertPtrNotNull(tc, nodeData2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData2, APX_SERVER_MODE));

   node1Connections = apx_portConnectionTable_new(2);
   CuAssertPtrNotNull(tc, node1Connections);

   //create a connection between nodeData1.P[1] and nodeData2.R[0]
   providePortRef = apx_nodeData_getProvidePortDataRef(nodeData1, 1); // nodeData1.P[1]
   requirePortRef = apx_nodeData_getRequirePortDataRef(nodeData2, 0); //nodeData2.R[0]
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node1Connections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node1Connections, 1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_connect(node1Connections, providePortRef, requirePortRef));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node1Connections, 0));
   CuAssertIntEquals(tc, 1, apx_portConnectionTable_count(node1Connections, 1));

   apx_parser_delete(parser);
   apx_nodeData_delete(nodeData1);
   apx_nodeData_delete(nodeData2);
   apx_portConnectionTable_delete(node1Connections);
}

static void test_apx_portConnectionTable_disconnectRequirePorts(CuTest *tc)
{
   apx_portConnectionTable_t *node1Connections;
   apx_parser_t *parser;
   apx_nodeData_t *nodeData1;
   apx_nodeData_t *nodeData2;
   apx_nodeData_t *nodeData3;
   apx_portDataRef_t *providePortRef;
   apx_portDataRef_t *requirePortRef1;
   apx_portDataRef_t *requirePortRef2;
   parser = apx_parser_new();
   nodeData1 = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   CuAssertPtrNotNull(tc, nodeData1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData1, APX_SERVER_MODE));
   nodeData2 = apx_nodeData_makeFromString(parser, m_node_text2, NULL);
   CuAssertPtrNotNull(tc, nodeData2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData2, APX_SERVER_MODE));
   nodeData3 = apx_nodeData_makeFromString(parser, m_node_text3, NULL);
   CuAssertPtrNotNull(tc, nodeData3);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData3, APX_SERVER_MODE));

   node1Connections = apx_portConnectionTable_new(2);
   CuAssertPtrNotNull(tc, node1Connections);

   providePortRef = apx_nodeData_getProvidePortDataRef(nodeData1, 1);  //nodeData1.P[1]
   requirePortRef1 = apx_nodeData_getRequirePortDataRef(nodeData2, 0); //nodeData2.R[0]
   requirePortRef2 = apx_nodeData_getRequirePortDataRef(nodeData3, 0); //nodeData3.R[0]
   //disconnect nodeData2.R[0] and nodeData3.R[0] from nodeData1.P[1]
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node1Connections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node1Connections, 1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_disconnect(node1Connections, providePortRef, requirePortRef1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_disconnect(node1Connections, providePortRef, requirePortRef2));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node1Connections, 0));
   CuAssertIntEquals(tc, -2, apx_portConnectionTable_count(node1Connections, 1));

   apx_parser_delete(parser);
   apx_nodeData_delete(nodeData1);
   apx_nodeData_delete(nodeData2);
   apx_nodeData_delete(nodeData3);
   apx_portConnectionTable_delete(node1Connections);
}

static void test_apx_portConnectionTable_disconnectProvidePort(CuTest *tc)
{
   apx_portConnectionTable_t *node2RequireConnections;
   apx_portConnectionTable_t *node3RequireConnections;
   apx_parser_t *parser;
   apx_nodeData_t *nodeData1;
   apx_nodeData_t *nodeData2;
   apx_nodeData_t *nodeData3;
   apx_portDataRef_t *providePortRef;
   apx_portDataRef_t *requirePortRef1;
   apx_portDataRef_t *requirePortRef2;
   parser = apx_parser_new();
   nodeData1 = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   CuAssertPtrNotNull(tc, nodeData1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData1, APX_SERVER_MODE));
   nodeData2 = apx_nodeData_makeFromString(parser, m_node_text2, NULL);
   CuAssertPtrNotNull(tc, nodeData2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData2, APX_SERVER_MODE));
   nodeData3 = apx_nodeData_makeFromString(parser, m_node_text3, NULL);
   CuAssertPtrNotNull(tc, nodeData3);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData3, APX_SERVER_MODE));

   node2RequireConnections = apx_portConnectionTable_new(1);
   node3RequireConnections = apx_portConnectionTable_new(1);
   CuAssertPtrNotNull(tc, node2RequireConnections);
   CuAssertPtrNotNull(tc, node3RequireConnections);

   providePortRef = apx_nodeData_getProvidePortDataRef(nodeData1, 1);  //nodeData1.P[1]
   requirePortRef1 = apx_nodeData_getRequirePortDataRef(nodeData2, 0); //nodeData2.R[0]
   requirePortRef2 = apx_nodeData_getRequirePortDataRef(nodeData3, 0); //nodeData3.R[0]
   //disconnect nodeData2.R[0] and nodeData3.R[0] from nodeData1.P[1]
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node2RequireConnections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node3RequireConnections, 0));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_disconnect(node2RequireConnections, requirePortRef1, providePortRef));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_disconnect(node3RequireConnections, requirePortRef2, providePortRef));
   CuAssertIntEquals(tc, -1, apx_portConnectionTable_count(node2RequireConnections, 0));
   CuAssertIntEquals(tc, -1, apx_portConnectionTable_count(node3RequireConnections, 0));

   apx_parser_delete(parser);
   apx_nodeData_delete(nodeData1);
   apx_nodeData_delete(nodeData2);
   apx_nodeData_delete(nodeData3);
   apx_portConnectionTable_delete(node2RequireConnections);
   apx_portConnectionTable_delete(node3RequireConnections);
}

