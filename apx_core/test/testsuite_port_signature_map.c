/*****************************************************************************
* \file      testsuite_port_signature_map.c
* \author    Conny Gustafsson
* \date      2020-02-19
* \brief     Unit tests for port signature map
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"
#include "apx/port_signature_map.h"
#include "apx/node_manager.h"
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
static void test_connecting_first_require_port_table(CuTest* tc);
static void test_connecting_provide_port_when_require_ports_are_waiting(CuTest* tc);
static void test_connecting_require_port_when_provide_port_is_available(CuTest* tc);
static void test_disconnecting_require_port_when_connected_to_provide_port(CuTest* tc);
static void test_disconnecting_provide_port_when_connected_to_require_port(CuTest* tc);
static void test_disconnecting_provide_port_when_not_connected_to_anything(CuTest* tc);



//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_port_signature_map(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_connecting_first_require_port_table);
   SUITE_ADD_TEST(suite, test_connecting_provide_port_when_require_ports_are_waiting);
   SUITE_ADD_TEST(suite, test_connecting_require_port_when_provide_port_is_available);
   SUITE_ADD_TEST(suite, test_disconnecting_require_port_when_connected_to_provide_port);
   SUITE_ADD_TEST(suite, test_disconnecting_provide_port_when_connected_to_require_port);
   SUITE_ADD_TEST(suite, test_disconnecting_provide_port_when_not_connected_to_anything);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_connecting_first_require_port_table(CuTest* tc)
{
   apx_node_manager_t *nodeManager;
   apx_node_instance_t *nodeInstance;
   apx_port_signature_map_t *map;
   apx_port_connector_change_table_t *requirePortChanges;

   nodeManager = apx_node_manager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, nodeManager);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(nodeManager, m_node_text1));
   nodeInstance = apx_node_manager_get_last_attached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance);

   map = apx_port_signature_map_new();
   CuAssertPtrNotNull(tc, map);
   CuAssertIntEquals(tc, 0, apx_port_signature_map_length(map));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, nodeInstance));
   CuAssertIntEquals(tc, 1, apx_port_signature_map_length(map));
   CuAssertPtrNotNull(tc, apx_port_signature_map_find(map, "\"VehicleSpeed\"S"));
   requirePortChanges = apx_node_instance_get_require_port_connector_changes(nodeInstance, false);
   CuAssertPtrEquals(tc, NULL, requirePortChanges);

   apx_port_signature_map_delete(map);
   apx_node_manager_delete(nodeManager);
}

static void test_connecting_provide_port_when_require_ports_are_waiting(CuTest* tc)
{
   apx_node_manager_t *node_manager;
   apx_node_instance_t *node_instance1;
   apx_node_instance_t *node_instance2;
   apx_node_instance_t *node_instance3;
   apx_port_signature_map_t *map;
   apx_port_connector_change_table_t *require_port_changes1; //associated with node_instance1
   apx_port_connector_change_table_t *require_port_changes2; //associated with node_instance2
   apx_port_connector_change_table_t *provide_port_changes3; //associated with node_instance3
   apx_port_connector_change_entry_t *entry;

   node_manager = apx_node_manager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, node_manager);
   map = apx_port_signature_map_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_manager, m_node_text1));
   node_instance1 = apx_node_manager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_manager, m_node_text2));
   node_instance2 = apx_node_manager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_manager, m_node_text3));
   node_instance3 = apx_node_manager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance3);

   CuAssertIntEquals(tc, 0, apx_port_signature_map_length(map));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance2));
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_require_port_connector_changes(node_instance1, false));
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_require_port_connector_changes(node_instance2, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_provide_ports(map, node_instance3));
   CuAssertIntEquals(tc, 1, apx_port_signature_map_length(map));
   require_port_changes1 = apx_node_instance_get_require_port_connector_changes(node_instance1, false);
   require_port_changes2 = apx_node_instance_get_require_port_connector_changes(node_instance2, false);
   provide_port_changes3 = apx_node_instance_get_provide_port_connector_changes(node_instance3, false);
   CuAssertPtrNotNull(tc, require_port_changes1);
   CuAssertPtrNotNull(tc, require_port_changes2);
   CuAssertPtrNotNull(tc, provide_port_changes3);
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_provide_port_connector_changes(node_instance1, false));
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_provide_port_connector_changes(node_instance2, false));
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_require_port_connector_changes(node_instance3, false));

   entry = apx_port_connector_change_table_get_entry(require_port_changes1, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_provide_port(node_instance3, 0), entry->data.port_instance);

   entry = apx_port_connector_change_table_get_entry(require_port_changes2, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_provide_port(node_instance3, 0), entry->data.port_instance);

   entry = apx_port_connector_change_table_get_entry(provide_port_changes3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 2, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_require_port(node_instance1, 0), adt_ary_value(entry->data.array, 0));
   CuAssertPtrEquals(tc, apx_node_instance_get_require_port(node_instance2, 0), adt_ary_value(entry->data.array, 1));

   apx_port_signature_map_delete(map);
   apx_node_manager_delete(node_manager);
}

static void test_connecting_require_port_when_provide_port_is_available(CuTest* tc)
{
   apx_node_manager_t *node_manager;
   apx_node_instance_t *node_instance1;
   apx_node_instance_t *node_instance2;
   apx_node_instance_t *node_instance3;
   apx_port_signature_map_t *map;
   apx_port_connector_change_table_t *require_port_changes1; //associated with node_instance1
   apx_port_connector_change_table_t *require_port_changes2; //associated with node_instance2
   apx_port_connector_change_table_t *provide_port_changes3; //associated with node_instance3
   apx_port_connector_change_entry_t *entry;

   node_manager = apx_node_manager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, node_manager);
   map = apx_port_signature_map_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_manager, m_node_text1));
   node_instance1 = apx_node_manager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_manager, m_node_text2));
   node_instance2 = apx_node_manager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_manager, m_node_text3));
   node_instance3 = apx_node_manager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance3);

   CuAssertIntEquals(tc, 0, apx_port_signature_map_length(map));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_provide_ports(map, node_instance3));
   CuAssertIntEquals(tc, 1, apx_port_signature_map_length(map));

   //Verify that no connectors has changed
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_require_port_connector_changes(node_instance3, false));
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_provide_port_connector_changes(node_instance3, false));

   //Now connect r-ports for node_instance1
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance1));

   require_port_changes1 = apx_node_instance_get_require_port_connector_changes(node_instance1, false);
   provide_port_changes3 = apx_node_instance_get_provide_port_connector_changes(node_instance3, false);
   CuAssertPtrNotNull(tc, require_port_changes1);
   CuAssertPtrNotNull(tc, provide_port_changes3);

   entry = apx_port_connector_change_table_get_entry(require_port_changes1, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_provide_port(node_instance3, 0), entry->data.port_instance);

   entry = apx_port_connector_change_table_get_entry(provide_port_changes3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_require_port(node_instance1, 0), entry->data.port_instance);

   //Clear connection changes
   apx_node_instance_clear_provide_port_connector_changes(node_instance3, true);
   apx_node_instance_clear_require_port_connector_changes(node_instance1, true);

   //Add second listener
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance2));
   require_port_changes2 = apx_node_instance_get_require_port_connector_changes(node_instance2, false);
   provide_port_changes3 = apx_node_instance_get_provide_port_connector_changes(node_instance3, false);
   CuAssertPtrNotNull(tc, require_port_changes1);
   CuAssertPtrNotNull(tc, require_port_changes2);

   entry = apx_port_connector_change_table_get_entry(require_port_changes2, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_provide_port(node_instance3, 0), entry->data.port_instance);

   entry = apx_port_connector_change_table_get_entry(provide_port_changes3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, 1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_require_port(node_instance2, 0), entry->data.port_instance);

   //Verify that node_instance1 was not affected by any changes
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_require_port_connector_changes(node_instance1, false));
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_provide_port_connector_changes(node_instance1, false));


   apx_port_signature_map_delete(map);
   apx_node_manager_delete(node_manager);

}

static void test_disconnecting_require_port_when_connected_to_provide_port(CuTest* tc)
{
   apx_node_manager_t *node_manager;
   apx_node_instance_t *node_instance1;
   apx_node_instance_t *node_instance3;
   apx_port_signature_map_t *map;
   apx_port_connector_change_table_t *require_port_changes1; //associated with node_instance1
   apx_port_connector_change_table_t *provide_port_changes3; //associated with node_instance3
   apx_port_connector_change_entry_t *entry;

   node_manager = apx_node_manager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, node_manager);
   map = apx_port_signature_map_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_manager, m_node_text1));
   node_instance1 = apx_node_manager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_manager, m_node_text3));
   node_instance3 = apx_node_manager_get_last_attached(node_manager);
   CuAssertPtrNotNull(tc, node_instance3);

   //Connect node_instance3, then connect node_instance1
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_provide_ports(map, node_instance3));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance1));

   CuAssertPtrNotNull(tc, apx_node_instance_get_require_port_connector_changes(node_instance1, false));
   CuAssertPtrNotNull(tc, apx_node_instance_get_provide_port_connector_changes(node_instance3, false));

   //Clear connection changes
   apx_node_instance_clear_provide_port_connector_changes(node_instance3, true);
   apx_node_instance_clear_require_port_connector_changes(node_instance1, true);

   //Now disconnect node_instance1
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_disconnect_require_ports(map, node_instance1));

   require_port_changes1 = apx_node_instance_get_require_port_connector_changes(node_instance1, false);
   provide_port_changes3 = apx_node_instance_get_provide_port_connector_changes(node_instance3, false);
   CuAssertPtrNotNull(tc, require_port_changes1);
   CuAssertPtrNotNull(tc, provide_port_changes3);

   entry = apx_port_connector_change_table_get_entry(require_port_changes1, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_provide_port(node_instance3, 0), entry->data.port_instance);

   entry = apx_port_connector_change_table_get_entry(provide_port_changes3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_require_port(node_instance1, 0), entry->data.port_instance);

   apx_port_signature_map_delete(map);
   apx_node_manager_delete(node_manager);

}

static void test_disconnecting_provide_port_when_connected_to_require_port(CuTest* tc)
{
   apx_node_manager_t *nodeManager;
   apx_node_instance_t *node_instance1;
   apx_node_instance_t *node_instance2;
   apx_node_instance_t *node_instance3;
   apx_port_signature_map_t *map;
   apx_port_connector_change_table_t *require_port_changes1; //associated with node_instance1
   apx_port_connector_change_table_t *require_port_changes2; //associated with node_instance2
   apx_port_connector_change_table_t *provide_port_changes3; //associated with node_instance3
   apx_port_connector_change_entry_t *entry;

   nodeManager = apx_node_manager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, nodeManager);
   map = apx_port_signature_map_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(nodeManager, m_node_text1));
   node_instance1 = apx_node_manager_get_last_attached(nodeManager);
   CuAssertPtrNotNull(tc, node_instance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(nodeManager, m_node_text2));
   node_instance2 = apx_node_manager_get_last_attached(nodeManager);
   CuAssertPtrNotNull(tc, node_instance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(nodeManager, m_node_text3));
   node_instance3 = apx_node_manager_get_last_attached(nodeManager);
   CuAssertPtrNotNull(tc, node_instance3);

   //Connect node_instance3, then connect node_instance1 and node_instance2
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_provide_ports(map, node_instance3));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance2));

   //Verify existence of and clear connector change
   CuAssertPtrNotNull(tc, apx_node_instance_get_require_port_connector_changes(node_instance1, false));
   CuAssertPtrNotNull(tc, apx_node_instance_get_require_port_connector_changes(node_instance2, false));
   CuAssertPtrNotNull(tc, apx_node_instance_get_provide_port_connector_changes(node_instance3, false));

   apx_node_instance_clear_require_port_connector_changes(node_instance1, true);
   apx_node_instance_clear_require_port_connector_changes(node_instance2, true);
   apx_node_instance_clear_provide_port_connector_changes(node_instance3, true);

   //Now disconnect node_instance3
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_disconnect_provide_ports(map, node_instance3));

   require_port_changes1 = apx_node_instance_get_require_port_connector_changes(node_instance1, false);
   require_port_changes2 = apx_node_instance_get_require_port_connector_changes(node_instance2, false);
   provide_port_changes3 = apx_node_instance_get_provide_port_connector_changes(node_instance3, false);
   CuAssertPtrNotNull(tc, require_port_changes1);
   CuAssertPtrNotNull(tc, require_port_changes2);
   CuAssertPtrNotNull(tc, provide_port_changes3);

   entry = apx_port_connector_change_table_get_entry(require_port_changes1, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_provide_port(node_instance3, 0), entry->data.port_instance);

   entry = apx_port_connector_change_table_get_entry(require_port_changes2, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -1, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_provide_port(node_instance3, 0), entry->data.port_instance);

   entry = apx_port_connector_change_table_get_entry(provide_port_changes3, 0);
   CuAssertPtrNotNull(tc, entry);
   CuAssertIntEquals(tc, -2, entry->count);
   CuAssertPtrEquals(tc, apx_node_instance_get_require_port(node_instance1, 0), adt_ary_value(entry->data.array, 0));
   CuAssertPtrEquals(tc, apx_node_instance_get_require_port(node_instance2, 0), adt_ary_value(entry->data.array, 1));

   //map entry must still exist (since the require-ports are still connected)
   CuAssertIntEquals(tc, 1, apx_port_signature_map_length(map));

   apx_port_signature_map_delete(map);
   apx_node_manager_delete(nodeManager);
}

static void test_disconnecting_provide_port_when_not_connected_to_anything(CuTest* tc)
{
   apx_node_manager_t *nodeManager;
   apx_node_instance_t *node_instance1;
   apx_node_instance_t *node_instance2;
   apx_node_instance_t *node_instance3;
   apx_port_signature_map_t *map;

   nodeManager = apx_node_manager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, nodeManager);
   map = apx_port_signature_map_new();
   CuAssertPtrNotNull(tc, map);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(nodeManager, m_node_text1));
   node_instance1 = apx_node_manager_get_last_attached(nodeManager);
   CuAssertPtrNotNull(tc, node_instance1);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(nodeManager, m_node_text2));
   node_instance2 = apx_node_manager_get_last_attached(nodeManager);
   CuAssertPtrNotNull(tc, node_instance2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(nodeManager, m_node_text3));
   node_instance3 = apx_node_manager_get_last_attached(nodeManager);
   CuAssertPtrNotNull(tc, node_instance3);

   //Connect node_instance3, then connect node_instance1 and node_instance2
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_provide_ports(map, node_instance3));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_connect_require_ports(map, node_instance2));

   //Verify existence of and clear connector change
   CuAssertPtrNotNull(tc, apx_node_instance_get_require_port_connector_changes(node_instance1, false));
   CuAssertPtrNotNull(tc, apx_node_instance_get_require_port_connector_changes(node_instance2, false));
   CuAssertPtrNotNull(tc, apx_node_instance_get_provide_port_connector_changes(node_instance3, false));
   apx_node_instance_clear_require_port_connector_changes(node_instance1, true);
   apx_node_instance_clear_require_port_connector_changes(node_instance2, true);
   apx_node_instance_clear_provide_port_connector_changes(node_instance3, true);

   //Disconnect node_instance1 followed by node_instance2
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_disconnect_require_ports(map, node_instance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_disconnect_require_ports(map, node_instance2));

   //Verify existence of and clear connector change
   CuAssertPtrNotNull(tc, apx_node_instance_get_require_port_connector_changes(node_instance1, false));
   CuAssertPtrNotNull(tc, apx_node_instance_get_require_port_connector_changes(node_instance2, false));
   CuAssertPtrNotNull(tc, apx_node_instance_get_provide_port_connector_changes(node_instance3, false));
   apx_node_instance_clear_require_port_connector_changes(node_instance1, true);
   apx_node_instance_clear_require_port_connector_changes(node_instance2, true);
   apx_node_instance_clear_provide_port_connector_changes(node_instance3, true);

   //Now disconnect node_instance3
   CuAssertIntEquals(tc, 1, apx_port_signature_map_length(map));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_signature_map_disconnect_provide_ports(map, node_instance3));

   //Verify nothing changed in connectors
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_require_port_connector_changes(node_instance3, false));
   CuAssertPtrEquals(tc, NULL, apx_node_instance_get_provide_port_connector_changes(node_instance3, false));

   //map should now be empty since all nodes are disconnected
   CuAssertIntEquals(tc, 0, apx_port_signature_map_length(map));

   apx_port_signature_map_delete(map);
   apx_node_manager_delete(nodeManager);
}
