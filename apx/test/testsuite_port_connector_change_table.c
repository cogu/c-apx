//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include "CuTest.h"
#include <stdio.h>
#include "apx/port_connector_change_table.h"
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
static void test_create_table(CuTest *tc);
static void test_connect_vehicle_speed_ports(CuTest *tc);
static void test_disconnect_require_ports(CuTest *tc);
static void test_disconnect_provide_ports(CuTest *tc);

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
CuSuite* testSuite_apx_portConnectorChangeTable(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_table);
   SUITE_ADD_TEST(suite, test_connect_vehicle_speed_ports);
   SUITE_ADD_TEST(suite, test_disconnect_require_ports);
   SUITE_ADD_TEST(suite, test_disconnect_provide_ports);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_table(CuTest *tc)
{
   apx_portConnectorChangeTable_t *table = apx_portConnectorChangeTable_new(3);

   CuAssertPtrNotNull(tc, table);
   CuAssertPtrNotNull(tc, table->entries);
   CuAssertIntEquals(tc, 3, table->num_ports);
   apx_portConnectorChangeTable_delete(table);
}

static void test_connect_vehicle_speed_ports(CuTest *tc)
{
   apx_portConnectorChangeTable_t *port_connections;
   apx_nodeManager_t *node_manager;
   apx_nodeInstance_t *node_instance1;
   apx_nodeInstance_t *node_instance2;
   apx_portInstance_t *local_port;
   apx_portInstance_t *remote_port;

   node_manager = apx_nodeManager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, node_manager);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text1));
   node_instance1 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(node_manager, m_node_text2));
   node_instance2 = apx_nodeManager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance2);

   port_connections = apx_portConnectorChangeTable_new(2);
   CuAssertPtrNotNull(tc, port_connections);

   //create a connection between nodeInstance1.P[1] and nodeInstance1.R[0]. nodeInstance1 is local node.
   local_port = apx_nodeInstance_get_provide_port(node_instance1, 1); // nodeInstance1.P[1]
   remote_port = apx_nodeInstance_get_require_port(node_instance2, 0); //nodeInstance1.R[0]
   CuAssertIntEquals(tc, 0, apx_portConnectorChangeTable_count(port_connections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectorChangeTable_count(port_connections, 1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectorChangeTable_connect(port_connections, local_port, remote_port));
   CuAssertIntEquals(tc, 0, apx_portConnectorChangeTable_count(port_connections, 0));
   CuAssertIntEquals(tc, 1, apx_portConnectorChangeTable_count(port_connections, 1));

   apx_nodeManager_delete(node_manager);
   apx_portConnectorChangeTable_delete(port_connections);
}

static void test_disconnect_require_ports(CuTest *tc)
{
   apx_portConnectorChangeTable_t *node_connections;
   apx_nodeManager_t *node_manager;
   apx_nodeInstance_t *node_instance1;
   apx_nodeInstance_t *node_instance2;
   apx_nodeInstance_t *node_instance3;
   apx_portInstance_t *provide_port;
   apx_portInstance_t *require_port_ref1;
   apx_portInstance_t *require_port_ref2;

   node_manager = apx_nodeManager_new(APX_SERVER_MODE);
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

   node_connections = apx_portConnectorChangeTable_new(2);
   CuAssertPtrNotNull(tc, node_connections);

   provide_port = apx_nodeInstance_get_provide_port(node_instance1, 1); // nodeInstance1.P[1]
   require_port_ref1 = apx_nodeInstance_get_require_port(node_instance2, 0); //nodeInstance2.R[0]
   require_port_ref2 = apx_nodeInstance_get_require_port(node_instance3, 0); //nodeInstance3.R[0]

   //disconnect nodeData2.R[0] and nodeData3.R[0] from nodeData1.P[1]
   CuAssertIntEquals(tc, 0, apx_portConnectorChangeTable_count(node_connections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectorChangeTable_count(node_connections, 1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectorChangeTable_disconnect(node_connections, provide_port, require_port_ref1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectorChangeTable_disconnect(node_connections, provide_port, require_port_ref2));
   CuAssertIntEquals(tc, 0, apx_portConnectorChangeTable_count(node_connections, 0));
   CuAssertIntEquals(tc, -2, apx_portConnectorChangeTable_count(node_connections, 1));

   apx_nodeManager_delete(node_manager);
   apx_portConnectorChangeTable_delete(node_connections);
}

static void test_disconnect_provide_ports(CuTest *tc)
{
   apx_portConnectorChangeTable_t *node2_require_connections;
   apx_portConnectorChangeTable_t *node3_require_connections;
   apx_nodeManager_t *node_manager;
   apx_nodeInstance_t *node_instance1;
   apx_nodeInstance_t *node_instance2;
   apx_nodeInstance_t *node_instance3;
   apx_portInstance_t *provide_port;
   apx_portInstance_t *require_port1;
   apx_portInstance_t *require_port2;
   node_manager = apx_nodeManager_new(APX_SERVER_MODE);
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

   node2_require_connections = apx_portConnectorChangeTable_new(1);
   node3_require_connections = apx_portConnectorChangeTable_new(1);
   CuAssertPtrNotNull(tc, node2_require_connections);
   CuAssertPtrNotNull(tc, node3_require_connections);

   provide_port = apx_nodeInstance_get_provide_port(node_instance1, 1);  //nodeInstance1.P[1]
   require_port1 = apx_nodeInstance_get_require_port(node_instance2, 0); //nodeInstance2.R[0]
   require_port2 = apx_nodeInstance_get_require_port(node_instance3, 0); //nodeInstance3.R[0]
   //disconnect nodeInstance2.R[0] and nodeInstance3.R[0] from nodeInstance1.P[1]
   CuAssertIntEquals(tc, 0, apx_portConnectorChangeTable_count(node2_require_connections, 0));
   CuAssertIntEquals(tc, 0, apx_portConnectorChangeTable_count(node3_require_connections, 0));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectorChangeTable_disconnect(node2_require_connections, require_port1, provide_port));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_portConnectorChangeTable_disconnect(node3_require_connections, require_port2, provide_port));
   CuAssertIntEquals(tc, -1, apx_portConnectorChangeTable_count(node2_require_connections, 0));
   CuAssertIntEquals(tc, -1, apx_portConnectorChangeTable_count(node3_require_connections, 0));

   apx_nodeManager_delete(node_manager);
   apx_portConnectorChangeTable_delete(node2_require_connections);
   apx_portConnectorChangeTable_delete(node3_require_connections);
}