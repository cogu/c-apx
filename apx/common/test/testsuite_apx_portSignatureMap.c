/*****************************************************************************
* \file      testsuite_apx_portSignatureMap.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Unit tests for apx_portSignatureMap
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
#include "apx_portSignatureMap.h"
#include "apx_nodeManager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

static const char *m_node_text1 =
      "APX/1.2\n"
      "N\"Requester1\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

static const char *m_node_text2 =
      "APX/1.2\n"
      "N\"Requester2\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

static const char *m_node_text3 =
      "APX/1.2\n"
      "N\"Provider1\"\n"
      "P\"VehicleSpeed\"S:=65535\n";


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portSignatureMap_connectingFirstRequirePortTable(CuTest* tc);
static void test_apx_portSignatureMap_connectingProvidePortWhenRequirePortsAreWaiting(CuTest* tc);
static void test_apx_portSignatureMap_connectingRequrePortWhenProvidePortsIsAvailable(CuTest* tc);
static void test_apx_portSignatureMap_disconnectingRequirePortWhenConnectedToProvidePort(CuTest* tc);
static void test_apx_portSignatureMap_disconnectingProvidePortWhenConnectedToRequireProvidePort(CuTest* tc);
static void test_apx_portSignatureMap_disconnectingProvidePortWhenNotConnectedToAnything(CuTest* tc);



//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_portSignatureMap(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_portSignatureMap_connectingFirstRequirePortTable);
   SUITE_ADD_TEST(suite, test_apx_portSignatureMap_connectingProvidePortWhenRequirePortsAreWaiting);
   SUITE_ADD_TEST(suite, test_apx_portSignatureMap_connectingRequrePortWhenProvidePortsIsAvailable);
   SUITE_ADD_TEST(suite, test_apx_portSignatureMap_disconnectingRequirePortWhenConnectedToProvidePort);
   SUITE_ADD_TEST(suite, test_apx_portSignatureMap_disconnectingProvidePortWhenConnectedToRequireProvidePort);
   SUITE_ADD_TEST(suite, test_apx_portSignatureMap_disconnectingProvidePortWhenNotConnectedToAnything);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portSignatureMap_connectingFirstRequirePortTable(CuTest* tc)
{
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance;
   apx_portSignatureMap_t *map;
   apx_portConnectorChangeTable_t *requirePortChanges;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance);

   map = apx_portSignatureMap_new();
   CuAssertPtrNotNull(tc, map);
   CuAssertIntEquals(tc, 0, apx_portSignatureMap_length(map));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance));
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(map));
   CuAssertPtrNotNull(tc, apx_portSignatureMap_find(map, "\"VehicleSpeed\"S"));
   requirePortChanges = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance, false);
   CuAssertPtrEquals(tc, NULL, requirePortChanges);

   apx_portSignatureMap_delete(map);
   apx_nodeManager_delete(nodeManager);
}

static void test_apx_portSignatureMap_connectingProvidePortWhenRequirePortsAreWaiting(CuTest* tc)
{
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portSignatureMap_t *map;
   apx_portConnectorChangeTable_t *requirePortChanges1; //associated with nodeInstance1
   apx_portConnectorChangeTable_t *requirePortChanges2; //associated with nodeInstance2
   apx_portConnectorChangeTable_t *providePortChanges3; //associated with nodeInstance3
   apx_portConnectorChangeEntry_t *entry;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   map = apx_portSignatureMap_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);

   CuAssertIntEquals(tc, 0, apx_portSignatureMap_length(map));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance2));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectProvidePorts(map, nodeInstance3));
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(map));
   requirePortChanges1 = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false);
   requirePortChanges2 = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false);
   providePortChanges3 = apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false);
   CuAssertPtrNotNull(tc, requirePortChanges1);
   CuAssertPtrNotNull(tc, requirePortChanges2);
   CuAssertPtrNotNull(tc, providePortChanges3);
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance2, false));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance3, false));

   entry = apx_portConnectorChangeTable_getEntry(requirePortChanges1, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(nodeInstance3, 0), entry->data.portRef);

   entry = apx_portConnectorChangeTable_getEntry(requirePortChanges2, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(nodeInstance3, 0), entry->data.portRef);

   entry = apx_portConnectorChangeTable_getEntry(providePortChanges3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 2, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(nodeInstance1, 0), adt_ary_value(entry->data.array, 0));
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(nodeInstance2, 0), adt_ary_value(entry->data.array, 1));

   apx_portSignatureMap_delete(map);
   apx_nodeManager_delete(nodeManager);
}

static void test_apx_portSignatureMap_connectingRequrePortWhenProvidePortsIsAvailable(CuTest* tc)
{
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portSignatureMap_t *map;
   apx_portConnectorChangeTable_t *requirePortChanges1; //associated with nodeInstance1
   apx_portConnectorChangeTable_t *requirePortChanges2; //associated with nodeInstance2
   apx_portConnectorChangeTable_t *providePortChanges3; //associated with nodeInstance3
   apx_portConnectorChangeEntry_t *entry;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   map = apx_portSignatureMap_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);

   CuAssertIntEquals(tc, 0, apx_portSignatureMap_length(map));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectProvidePorts(map, nodeInstance3));
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(map));

   //Verify that no connectors has changed
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance3, false));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false));

   //Now connect r-ports for nodeInstance1
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance1));

   requirePortChanges1 = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false);
   providePortChanges3 = apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false);
   CuAssertPtrNotNull(tc, requirePortChanges1);
   CuAssertPtrNotNull(tc, providePortChanges3);

   entry = apx_portConnectorChangeTable_getEntry(requirePortChanges1, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(nodeInstance3, 0), entry->data.portRef);

   entry = apx_portConnectorChangeTable_getEntry(providePortChanges3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(nodeInstance1, 0), entry->data.portRef);

   //Clear connection changes
   apx_nodeInstance_clearProvidePortConnectorChanges(nodeInstance3, true);
   apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance1, true);

   //Add second listener
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance2));
   requirePortChanges2 = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false);
   providePortChanges3 = apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false);
   CuAssertPtrNotNull(tc, requirePortChanges1);
   CuAssertPtrNotNull(tc, requirePortChanges2);

   entry = apx_portConnectorChangeTable_getEntry(requirePortChanges2, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(nodeInstance3, 0), entry->data.portRef);

   entry = apx_portConnectorChangeTable_getEntry(providePortChanges3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(nodeInstance2, 0), entry->data.portRef);

   //Verify that nodeInstance1 was not affected by any changes
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance1, false));


   apx_portSignatureMap_delete(map);
   apx_nodeManager_delete(nodeManager);

}

static void test_apx_portSignatureMap_disconnectingRequirePortWhenConnectedToProvidePort(CuTest* tc)
{
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance3;
   apx_portSignatureMap_t *map;
   apx_portConnectorChangeTable_t *requirePortChanges1; //associated with nodeInstance1
   apx_portConnectorChangeTable_t *providePortChanges3; //associated with nodeInstance3
   apx_portConnectorChangeEntry_t *entry;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   map = apx_portSignatureMap_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);

   //Connect nodeInstance3, then connect nodeInstance1
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectProvidePorts(map, nodeInstance3));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance1));

   CuAssertPtrNotNull(tc, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false));

   //Clear connection changes
   apx_nodeInstance_clearProvidePortConnectorChanges(nodeInstance3, true);
   apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance1, true);

   //Now disconnect nodeInstance1
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_disconnectRequirePorts(map, nodeInstance1));

   requirePortChanges1 = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false);
   providePortChanges3 = apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false);
   CuAssertPtrNotNull(tc, requirePortChanges1);
   CuAssertPtrNotNull(tc, providePortChanges3);

   entry = apx_portConnectorChangeTable_getEntry(requirePortChanges1, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(nodeInstance3, 0), entry->data.portRef);

   entry = apx_portConnectorChangeTable_getEntry(providePortChanges3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(nodeInstance1, 0), entry->data.portRef);

   apx_portSignatureMap_delete(map);
   apx_nodeManager_delete(nodeManager);

}

static void test_apx_portSignatureMap_disconnectingProvidePortWhenConnectedToRequireProvidePort(CuTest* tc)
{
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portSignatureMap_t *map;
   apx_portConnectorChangeTable_t *requirePortChanges1; //associated with nodeInstance1
   apx_portConnectorChangeTable_t *requirePortChanges2; //associated with nodeInstance2
   apx_portConnectorChangeTable_t *providePortChanges3; //associated with nodeInstance3
   apx_portConnectorChangeEntry_t *entry;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   map = apx_portSignatureMap_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);

   //Connect nodeInstance3, then connect nodeInstance1 and nodeInstance2
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectProvidePorts(map, nodeInstance3));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance2));

   //Verify existence of and clear connector change
   CuAssertPtrNotNull(tc, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false));

   apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance1, true);
   apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance2, true);
   apx_nodeInstance_clearProvidePortConnectorChanges(nodeInstance3, true);

   //Now disconnect nodeInstance3
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_disconnectProvidePorts(map, nodeInstance3));

   requirePortChanges1 = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false);
   requirePortChanges2 = apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false);
   providePortChanges3 = apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false);
   CuAssertPtrNotNull(tc, requirePortChanges1);
   CuAssertPtrNotNull(tc, requirePortChanges2);
   CuAssertPtrNotNull(tc, providePortChanges3);

   entry = apx_portConnectorChangeTable_getEntry(requirePortChanges1, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(nodeInstance3, 0), entry->data.portRef);

   entry = apx_portConnectorChangeTable_getEntry(requirePortChanges2, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -1, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(nodeInstance3, 0), entry->data.portRef);

   entry = apx_portConnectorChangeTable_getEntry(providePortChanges3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -2, entry->count);
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(nodeInstance1, 0), adt_ary_value(entry->data.array, 0));
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(nodeInstance2, 0), adt_ary_value(entry->data.array, 1));

   //map entry must still exist (since the require-ports are still connected)
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(map));

   apx_portSignatureMap_delete(map);
   apx_nodeManager_delete(nodeManager);
}

static void test_apx_portSignatureMap_disconnectingProvidePortWhenNotConnectedToAnything(CuTest* tc)
{
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portSignatureMap_t *map;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   map = apx_portSignatureMap_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);

   //Connect nodeInstance3, then connect nodeInstance1 and nodeInstance2
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectProvidePorts(map, nodeInstance3));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_connectRequirePorts(map, nodeInstance2));

   //Verify existence of and clear connector change
   CuAssertPtrNotNull(tc, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false));
   apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance1, true);
   apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance2, true);
   apx_nodeInstance_clearProvidePortConnectorChanges(nodeInstance3, true);

   //Disconnect nodeInstance1 followed by nodeInstance2
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_disconnectRequirePorts(map, nodeInstance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_disconnectRequirePorts(map, nodeInstance2));

   //Verify existence of and clear connector change
   CuAssertPtrNotNull(tc, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false));
   apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance1, true);
   apx_nodeInstance_clearRequirePortConnectorChanges(nodeInstance2, true);
   apx_nodeInstance_clearProvidePortConnectorChanges(nodeInstance3, true);

   //Now disconnect nodeInstance3
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(map));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portSignatureMap_disconnectProvidePorts(map, nodeInstance3));

   //Verify nothing changed in connectors
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance3, false));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance3, false));

   //map should now be empty since all nodes are disconnected
   CuAssertIntEquals(tc, 0, apx_portSignatureMap_length(map));

   apx_portSignatureMap_delete(map);
   apx_nodeManager_delete(nodeManager);
}
