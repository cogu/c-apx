//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include "CuTest.h"
#include <stdio.h>
#include "apx/port_connector_change_entry.h"
#include "apx/node_manager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portConnectorChangeEntry_create(CuTest *tc);
static void test_apx_portConnectorChangeEntry_connectOne(CuTest *tc);
static void test_apx_portConnectorChangeEntry_connectThree(CuTest *tc);
static void test_apx_portConnectorChangeEntry_disconnectOne(CuTest *tc);
static void test_apx_portConnectorChangeEntry_disconnectThree(CuTest *tc);

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
CuSuite* testSuite_apx_portConnectorChangeEntry(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_portConnectorChangeEntry_create);
   SUITE_ADD_TEST(suite, test_apx_portConnectorChangeEntry_connectOne);
   SUITE_ADD_TEST(suite, test_apx_portConnectorChangeEntry_connectThree);
   SUITE_ADD_TEST(suite, test_apx_portConnectorChangeEntry_disconnectOne);
   SUITE_ADD_TEST(suite, test_apx_portConnectorChangeEntry_disconnectThree);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portConnectorChangeEntry_create(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_portConnectorChangeEntry_create(&entry);
   CuAssertIntEquals(tc, 0, entry.count);
   CuAssertPtrEquals(tc, NULL, entry.data.portRef);
   apx_portConnectorChangeEntry_destroy(&entry);
}

static void test_apx_portConnectorChangeEntry_connectOne(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_portRef_t *portRef1;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);
   apx_portConnectorChangeEntry_create(&entry);
   portRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, portRef1);
   apx_portConnectorChangeEntry_addConnection(&entry, portRef1);
   CuAssertIntEquals(tc, 1, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectorChangeEntry_get(&entry, 0));


   apx_portConnectorChangeEntry_destroy(&entry);
   apx_nodeManager_delete(nodeManager);

}

static void test_apx_portConnectorChangeEntry_connectThree(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portRef_t *portRef1;
   apx_portRef_t *portRef2;
   apx_portRef_t *portRef3;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   apx_portConnectorChangeEntry_create(&entry);
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


   portRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, portRef1);
   portRef2 = apx_nodeInstance_getRequirePortRef(nodeInstance2, 0);
   CuAssertPtrNotNull(tc, portRef2);
   portRef3 = apx_nodeInstance_getRequirePortRef(nodeInstance3, 0);
   CuAssertPtrNotNull(tc, portRef3);
   apx_portConnectorChangeEntry_addConnection(&entry, portRef1);
   apx_portConnectorChangeEntry_addConnection(&entry, portRef2);
   apx_portConnectorChangeEntry_addConnection(&entry, portRef3);
   CuAssertIntEquals(tc, 3, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectorChangeEntry_get(&entry, 0));
   CuAssertPtrEquals(tc, portRef2, apx_portConnectorChangeEntry_get(&entry, 1));
   CuAssertPtrEquals(tc, portRef3, apx_portConnectorChangeEntry_get(&entry, 2));

   apx_portConnectorChangeEntry_destroy(&entry);
   apx_nodeManager_delete(nodeManager);
}


static void test_apx_portConnectorChangeEntry_disconnectOne(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_portRef_t *portRef1;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);
   apx_portConnectorChangeEntry_create(&entry);
   portRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, portRef1);
   apx_portConnectorChangeEntry_removeConnection(&entry, portRef1);
   CuAssertIntEquals(tc, -1, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectorChangeEntry_get(&entry, 0));


   apx_portConnectorChangeEntry_destroy(&entry);
   apx_nodeManager_delete(nodeManager);
}


static void test_apx_portConnectorChangeEntry_disconnectThree(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portRef_t *portRef1;
   apx_portRef_t *portRef2;
   apx_portRef_t *portRef3;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   apx_portConnectorChangeEntry_create(&entry);
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

   portRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, portRef1);
   portRef2 = apx_nodeInstance_getRequirePortRef(nodeInstance2, 0);
   CuAssertPtrNotNull(tc, portRef2);
   portRef3 = apx_nodeInstance_getRequirePortRef(nodeInstance3, 0);
   CuAssertPtrNotNull(tc, portRef3);
   apx_portConnectorChangeEntry_removeConnection(&entry, portRef1);
   apx_portConnectorChangeEntry_removeConnection(&entry, portRef2);
   apx_portConnectorChangeEntry_removeConnection(&entry, portRef3);
   CuAssertIntEquals(tc, -3, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectorChangeEntry_get(&entry, 0));
   CuAssertPtrEquals(tc, portRef2, apx_portConnectorChangeEntry_get(&entry, 1));
   CuAssertPtrEquals(tc, portRef3, apx_portConnectorChangeEntry_get(&entry, 2));

   apx_portConnectorChangeEntry_destroy(&entry);
   apx_nodeManager_delete(nodeManager);
}
