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
static void test_connect_one(CuTest *tc);
static void test_connect_three(CuTest *tc);
static void test_apx_disconnect_one(CuTest *tc);
static void test_apx_disconnect_three(CuTest *tc);

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
   SUITE_ADD_TEST(suite, test_connect_one);
   SUITE_ADD_TEST(suite, test_connect_three);
   SUITE_ADD_TEST(suite, test_apx_disconnect_one);
   SUITE_ADD_TEST(suite, test_apx_disconnect_three);
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
   CuAssertPtrEquals(tc, NULL, entry.data.port_instance);
   apx_portConnectorChangeEntry_destroy(&entry);
}

static void test_connect_one(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_nodeManager_t *node_manager;
   apx_nodeInstance_t *node_instance1;
   apx_portInstance_t *port_instance1;

   node_manager = apx_nodeManager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, node_manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text1));
   node_instance1 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance1);
   apx_portConnectorChangeEntry_create(&entry);
   port_instance1 = apx_nodeInstance_get_require_port(node_instance1, 0);
   CuAssertPtrNotNull(tc, port_instance1);
   apx_portConnectorChangeEntry_add_connection(&entry, port_instance1);
   CuAssertIntEquals(tc, 1, entry.count);
   CuAssertPtrEquals(tc, port_instance1, apx_portConnectorChangeEntry_get(&entry, 0));

   apx_portConnectorChangeEntry_destroy(&entry);
   apx_nodeManager_delete(node_manager);

}

static void test_connect_three(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_nodeManager_t *node_manager;
   apx_nodeInstance_t *node_instance1;
   apx_nodeInstance_t *node_instance2;
   apx_nodeInstance_t *node_instance3;
   apx_portInstance_t *port_instance1;
   apx_portInstance_t *port_instance2;
   apx_portInstance_t *port_instance3;

   node_manager = apx_nodeManager_new(APX_SERVER_MODE);
   apx_portConnectorChangeEntry_create(&entry);
   CuAssertPtrNotNull(tc, node_manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text1));
   node_instance1 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text2));
   node_instance2 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text3));
   node_instance3 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance3);
   port_instance1 = apx_nodeInstance_get_require_port(node_instance1, 0);
   CuAssertPtrNotNull(tc, port_instance1);
   port_instance2 = apx_nodeInstance_get_require_port(node_instance2, 0);
   CuAssertPtrNotNull(tc, port_instance2);
   port_instance3 = apx_nodeInstance_get_require_port(node_instance3, 0);
   CuAssertPtrNotNull(tc, port_instance3);
   apx_portConnectorChangeEntry_add_connection(&entry, port_instance1);
   apx_portConnectorChangeEntry_add_connection(&entry, port_instance2);
   apx_portConnectorChangeEntry_add_connection(&entry, port_instance3);
   CuAssertIntEquals(tc, 3, entry.count);
   CuAssertPtrEquals(tc, port_instance1, apx_portConnectorChangeEntry_get(&entry, 0));
   CuAssertPtrEquals(tc, port_instance2, apx_portConnectorChangeEntry_get(&entry, 1));
   CuAssertPtrEquals(tc, port_instance3, apx_portConnectorChangeEntry_get(&entry, 2));
   apx_portConnectorChangeEntry_destroy(&entry);
   apx_nodeManager_delete(node_manager);
}

static void test_apx_disconnect_one(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_nodeManager_t *node_manager;
   apx_nodeInstance_t *node_instance1;
   apx_portInstance_t *port_instance1;

   node_manager = apx_nodeManager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, node_manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text1));
   node_instance1 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance1);
   apx_portConnectorChangeEntry_create(&entry);
   port_instance1 = apx_nodeInstance_get_require_port(node_instance1, 0);
   CuAssertPtrNotNull(tc, port_instance1);
   apx_portConnectorChangeEntry_remove_connection(&entry, port_instance1);
   CuAssertIntEquals(tc, -1, entry.count);
   CuAssertPtrEquals(tc, port_instance1, apx_portConnectorChangeEntry_get(&entry, 0));

   apx_portConnectorChangeEntry_destroy(&entry);
   apx_nodeManager_delete(node_manager);
}

static void test_apx_disconnect_three(CuTest *tc)
{
   apx_portConnectorChangeEntry_t entry;
   apx_nodeManager_t *node_manager;
   apx_nodeInstance_t *node_instance1;
   apx_nodeInstance_t *node_instance2;
   apx_nodeInstance_t *node_instance3;
   apx_portInstance_t *port_instance1;
   apx_portInstance_t *port_instance2;
   apx_portInstance_t *port_instance3;

   node_manager = apx_nodeManager_new(APX_SERVER_MODE);
   apx_portConnectorChangeEntry_create(&entry);
   CuAssertPtrNotNull(tc, node_manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text1));
   node_instance1 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text2));
   node_instance2 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text3));
   node_instance3 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance3);

   port_instance1 = apx_nodeInstance_get_require_port(node_instance1, 0);
   CuAssertPtrNotNull(tc, port_instance1);
   port_instance2 = apx_nodeInstance_get_require_port(node_instance2, 0);
   CuAssertPtrNotNull(tc, port_instance2);
   port_instance3 = apx_nodeInstance_get_require_port(node_instance3, 0);
   CuAssertPtrNotNull(tc, port_instance3);
   apx_portConnectorChangeEntry_remove_connection(&entry, port_instance1);
   apx_portConnectorChangeEntry_remove_connection(&entry, port_instance2);
   apx_portConnectorChangeEntry_remove_connection(&entry, port_instance3);
   CuAssertIntEquals(tc, -3, entry.count);
   CuAssertPtrEquals(tc, port_instance1, apx_portConnectorChangeEntry_get(&entry, 0));
   CuAssertPtrEquals(tc, port_instance2, apx_portConnectorChangeEntry_get(&entry, 1));
   CuAssertPtrEquals(tc, port_instance3, apx_portConnectorChangeEntry_get(&entry, 2));

   apx_portConnectorChangeEntry_destroy(&entry);
   apx_nodeManager_delete(node_manager);
}