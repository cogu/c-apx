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
#include "apx_nodeManager.h"
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
static void test_apx_portConnectionTable_connectVehicleSpeedPorts(CuTest *tc);
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
      "N\"LocalTestNode2\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

static const char *m_node_text3 =
      "APX/1.2\n"
      "N\"LocalTestNode3\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_portConnectionTable(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_portConnectionTable_create);
   SUITE_ADD_TEST(suite, test_apx_portConnectionTable_connectVehicleSpeedPorts);
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

static void test_apx_portConnectionTable_connectVehicleSpeedPorts(CuTest *tc)
{
   apx_portConnectionTable_t *portConnections;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_portRef_t *localPortRef;
   apx_portRef_t *remotePortRef;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);

   portConnections = apx_portConnectionTable_new(2);
   CuAssertPtrNotNull(tc, portConnections);

   //create a connection between nodeInstance1.P[1] and nodeInstance1.R[0]. nodeInstance1 is local node.
   localPortRef = apx_nodeInstance_getProvidePortRef(nodeInstance1, 1); // nodeInstance1.P[1]
   remotePortRef = apx_nodeInstance_getRequirePortRef(nodeInstance2, 0); //nodeInstance1.R[0]
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(portConnections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(portConnections, 1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_connect(portConnections, localPortRef, remotePortRef));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(portConnections, 0));
   CuAssertIntEquals(tc, 1, apx_portConnectionTable_count(portConnections, 1));

   apx_nodeManager_delete(nodeManager);
   apx_portConnectionTable_delete(portConnections);
}

static void test_apx_portConnectionTable_disconnectRequirePorts(CuTest *tc)
{
   apx_portConnectionTable_t *nodeConnections;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portRef_t *providePortRef;
   apx_portRef_t *requirePortRef1;
   apx_portRef_t *requirePortRef2;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);

   nodeConnections = apx_portConnectionTable_new(2);
   CuAssertPtrNotNull(tc, nodeConnections);

   providePortRef = apx_nodeInstance_getProvidePortRef(nodeInstance1, 1); // nodeInstance1.P[1]
   requirePortRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance2, 0); //nodeInstance2.R[0]
   requirePortRef2 = apx_nodeInstance_getRequirePortRef(nodeInstance3, 0); //nodeInstance3.R[0]


   //disconnect nodeData2.R[0] and nodeData3.R[0] from nodeData1.P[1]
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(nodeConnections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(nodeConnections, 1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_disconnect(nodeConnections, providePortRef, requirePortRef1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_disconnect(nodeConnections, providePortRef, requirePortRef2));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(nodeConnections, 0));
   CuAssertIntEquals(tc, -2, apx_portConnectionTable_count(nodeConnections, 1));

   apx_nodeManager_delete(nodeManager);
   apx_portConnectionTable_delete(nodeConnections);
}


static void test_apx_portConnectionTable_disconnectProvidePort(CuTest *tc)
{
   apx_portConnectionTable_t *node2RequireConnections;
   apx_portConnectionTable_t *node3RequireConnections;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portRef_t *providePortRef;
   apx_portRef_t *requirePortRef1;
   apx_portRef_t *requirePortRef2;
   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);

   node2RequireConnections = apx_portConnectionTable_new(1);
   node3RequireConnections = apx_portConnectionTable_new(1);
   CuAssertPtrNotNull(tc, node2RequireConnections);
   CuAssertPtrNotNull(tc, node3RequireConnections);

   providePortRef = apx_nodeInstance_getProvidePortRef(nodeInstance1, 1);  //nodeInstance1.P[1]
   requirePortRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance2, 0); //nodeInstance2.R[0]
   requirePortRef2 = apx_nodeInstance_getRequirePortRef(nodeInstance3, 0); //nodeInstance3.R[0]
   //disconnect nodeInstance2.R[0] and nodeInstance3.R[0] from nodeInstance1.P[1]
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node2RequireConnections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectionTable_count(node3RequireConnections, 0));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_disconnect(node2RequireConnections, requirePortRef1, providePortRef));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectionTable_disconnect(node3RequireConnections, requirePortRef2, providePortRef));
   CuAssertIntEquals(tc, -1, apx_portConnectionTable_count(node2RequireConnections, 0));
   CuAssertIntEquals(tc, -1, apx_portConnectionTable_count(node3RequireConnections, 0));

   apx_nodeManager_delete(nodeManager);
   apx_portConnectionTable_delete(node2RequireConnections);
   apx_portConnectionTable_delete(node3RequireConnections);
}

