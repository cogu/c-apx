//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/server.h"
#include "apx/server_test_connection.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_create_destroy(CuTest* tc);
static void test_connectors_connect_disconnect_node_with_only_require_ports(CuTest* tc);
static void test_connectors_connect_disconnect_node_with_only_provide_ports(CuTest* tc);
static void test_connectors_node_with_require_port_is_connected_after_node_with_provide_port(CuTest* tc);
static void test_connectors_node_with_provide_port_is_connected_when_multiple_nodes_with_require_ports_are_waiting(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const char* m_provider1_definition = "APX/1.2\n"
"N\"Provider1\"\n"
"P\"VehicleSpeed\"S:=65535\n"
"\n";

static const char* m_requester1_definition = "APX/1.2\n"
"N\"Requester1\"\n"
"R\"VehicleSpeed\"S:=65535\n"
"\n";

static const char* m_requester2_definition = "APX/1.2\n"
"N\"Requester2\"\n"
"R\"EngineSpeed\"S:=65535\n"
"R\"VehicleSpeed\"S:=65535\n"
"\n";

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_server(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_destroy);
   SUITE_ADD_TEST(suite, test_connectors_connect_disconnect_node_with_only_require_ports);
   SUITE_ADD_TEST(suite, test_connectors_connect_disconnect_node_with_only_provide_ports);
   SUITE_ADD_TEST(suite, test_connectors_node_with_require_port_is_connected_after_node_with_provide_port);
   SUITE_ADD_TEST(suite, test_connectors_node_with_provide_port_is_connected_when_multiple_nodes_with_require_ports_are_waiting);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_destroy(CuTest* tc)
{
   apx_server_t* server;
   server = apx_server_new();
   CuAssertPtrNotNull(tc, server);
   apx_server_delete(server);
}

static void test_connectors_connect_disconnect_node_with_only_require_ports(CuTest* tc)
{
   apx_server_t* server;
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet = NULL;
   apx_nodeManager_t* node_manager;
   apx_portSignatureMap_t *port_signature_map = NULL;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   int const file_info_publish_size = 66;
   int const data_write_size = 4;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"BreakAlertStatus\"C(0,3):=3\n";

   apx_size_t const definition_size = (apx_size_t)strlen(apx_text);
   server = apx_server_new();
   CuAssertPtrNotNull(tc, server);
   port_signature_map = apx_server_get_port_signature_map(server);
   CuAssertPtrNotNull(tc, port_signature_map);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_server_accept_connection(server, (apx_serverConnection_t*)connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(connection));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_serverTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_serverTestConnection_clear_log(connection);
   node_manager = apx_serverTestConnection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_nodeInstance_t* node_instance = apx_nodeManager_find(node_manager, "TestNode1");
   apx_nodeData_t const* node_data = apx_nodeInstance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_nodeData_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)apx_text, definition_size));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, file_info_publish_size, adt_bytearray_length(packet)); //Should be a file info struct
   apx_serverTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_nodeInstance_get_require_port_data_state(node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_request_open_local_file(connection, "TestNode1.in"));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(port_signature_map));
   apx_portSignatureMapEntry_t* map_entry = apx_portSignatureMap_find(port_signature_map, "\"BreakAlertStatus\"C(0,3)");
   CuAssertPtrNotNull(tc, map_entry);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_require_port_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, data_write_size, adt_bytearray_length(packet)); //port data
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_detach_connection(server, (apx_serverConnection_t*)connection));
   CuAssertIntEquals(tc, 0, apx_portSignatureMap_length(port_signature_map));
   apx_server_delete(server);
}

static void test_connectors_connect_disconnect_node_with_only_provide_ports(CuTest* tc)
{
   apx_server_t* server;
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet = NULL;
   apx_nodeManager_t* node_manager;
   apx_portSignatureMap_t* port_signature_map = NULL;
   int const provide_port_data_size = 1;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   uint8_t provide_port_data[1] = { 0u };
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"BreakAlertStatus\"C(0,3):=3\n";

   apx_size_t const definition_size = (apx_size_t)strlen(apx_text);
   server = apx_server_new();
   CuAssertPtrNotNull(tc, server);
   port_signature_map = apx_server_get_port_signature_map(server);
   CuAssertPtrNotNull(tc, port_signature_map);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_server_accept_connection(server, (apx_serverConnection_t*)connection);
   port_signature_map = apx_server_get_port_signature_map(server);
   CuAssertPtrNotNull(tc, port_signature_map);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(connection));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_serverTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode1.out", provide_port_data_size));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message (TestNode1.apx)
   apx_serverTestConnection_clear_log(connection);
   node_manager = apx_serverTestConnection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_nodeInstance_t* node_instance = apx_nodeManager_find(node_manager, "TestNode1");
   apx_nodeData_t const* node_data = apx_nodeInstance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_nodeData_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)apx_text, definition_size));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message (TestNode1.out)
   apx_serverTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_provide_port_data_state(node_instance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(connection, APX_PORT_DATA_ADDRESS_START, provide_port_data, provide_port_data_size));
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_provide_port_data_state(node_instance));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(port_signature_map));
   apx_portSignatureMapEntry_t* map_entry = apx_portSignatureMap_find(port_signature_map, "\"BreakAlertStatus\"C(0,3)");
   CuAssertPtrNotNull(tc, map_entry);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_detach_connection(server, (apx_serverConnection_t*)connection));
   CuAssertIntEquals(tc, 0, apx_portSignatureMap_length(port_signature_map));
   apx_server_delete(server);
}

static void test_connectors_node_with_require_port_is_connected_after_node_with_provide_port(CuTest* tc)
{
   apx_server_t* server;
   apx_serverTestConnection_t* provider_connection = NULL;
   apx_serverTestConnection_t* requester_connection = NULL;
   adt_bytearray_t* packet = NULL;
   apx_nodeManager_t* node_manager;
   apx_portSignatureMap_t* port_signature_map = NULL;
   int const provide_port_data_size = UINT16_SIZE;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   int const file_info_publish_size = 67;
   int const requester1_data_size = 5;
   uint8_t provide_port_data[UINT16_SIZE] = { 0x34u, 0x12u };
   uint8_t data_message[5];
   apx_size_t const provider_definition_size = (apx_size_t)strlen(m_provider1_definition);
   apx_size_t const requester_definition_size = (apx_size_t)strlen(m_requester1_definition);

   memset(data_message, 0, sizeof(data_message));
   server = apx_server_new();
   CuAssertPtrNotNull(tc, server);
   port_signature_map = apx_server_get_port_signature_map(server);
   CuAssertPtrNotNull(tc, port_signature_map);
   provider_connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, provider_connection);
   apx_server_accept_connection(server, (apx_serverConnection_t*)provider_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(provider_connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(provider_connection));
   apx_serverTestConnection_run(provider_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(provider_connection));
   packet = apx_serverTestConnection_get_log_packet(provider_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_serverTestConnection_clear_log(provider_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(provider_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(provider_connection, APX_PORT_DATA_ADDRESS_START, "Provider1.out", provide_port_data_size));
   apx_serverTestConnection_run(provider_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(provider_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(provider_connection, APX_DEFINITION_ADDRESS_START, "Provider1.apx", provider_definition_size));
   apx_serverTestConnection_run(provider_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(provider_connection));
   packet = apx_serverTestConnection_get_log_packet(provider_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message (TestNode1.apx)
   apx_serverTestConnection_clear_log(provider_connection);
   node_manager = apx_serverTestConnection_get_node_manager(provider_connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_nodeInstance_t* provider_node_instance = apx_nodeManager_find(node_manager, "Provider1");
   apx_nodeData_t* provider_node_data = apx_nodeInstance_get_node_data(provider_node_instance);
   CuAssertPtrNotNull(tc, provider_node_data);
   CuAssertUIntEquals(tc, provider_definition_size, apx_nodeData_definition_data_size(provider_node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_definition_data_state(provider_node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(provider_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(provider_connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)m_provider1_definition, provider_definition_size));
   apx_serverTestConnection_run(provider_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(provider_node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(provider_connection));
   packet = apx_serverTestConnection_get_log_packet(provider_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message (TestNode1.out)
   apx_serverTestConnection_clear_log(provider_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_provide_port_data_state(provider_node_instance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(provider_connection, APX_PORT_DATA_ADDRESS_START, provide_port_data, provide_port_data_size));
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_provide_port_data_state(provider_node_instance));
   apx_serverTestConnection_run(provider_connection);
   apx_portSignatureMapEntry_t* map_entry = apx_portSignatureMap_find(port_signature_map, "\"VehicleSpeed\"S");
   CuAssertPtrNotNull(tc, map_entry);
   uint8_t* snapshot = apx_nodeData_take_provide_port_data_snapshot(provider_node_data);
   CuAssertPtrNotNull(tc, snapshot);
   CuAssertUIntEquals(tc, 0x34, snapshot[0]);
   CuAssertUIntEquals(tc, 0x12, snapshot[1]);
   free(snapshot);

   requester_connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, requester_connection);
   apx_server_accept_connection(server, (apx_serverConnection_t*)requester_connection);
   port_signature_map = apx_server_get_port_signature_map(server);
   CuAssertPtrNotNull(tc, port_signature_map);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester_connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(requester_connection));
   apx_serverTestConnection_run(requester_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester_connection));
   packet = apx_serverTestConnection_get_log_packet(requester_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_serverTestConnection_clear_log(requester_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(requester_connection, APX_DEFINITION_ADDRESS_START, "Requester1.apx", requester_definition_size));
   apx_serverTestConnection_run(requester_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester_connection));
   packet = apx_serverTestConnection_get_log_packet(requester_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_serverTestConnection_clear_log(requester_connection);
   node_manager = apx_serverTestConnection_get_node_manager(requester_connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_nodeInstance_t* requester_node_instance = apx_nodeManager_find(node_manager, "Requester1");
   apx_nodeData_t* requester_node_data = apx_nodeInstance_get_node_data(requester_node_instance);
   CuAssertPtrNotNull(tc, requester_node_data);
   CuAssertUIntEquals(tc, requester_definition_size, apx_nodeData_definition_data_size(requester_node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_definition_data_state(requester_node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(requester_connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)m_requester1_definition, requester_definition_size));
   apx_serverTestConnection_run(requester_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(requester_node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester_connection));
   packet = apx_serverTestConnection_get_log_packet(requester_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, file_info_publish_size, adt_bytearray_length(packet)); //Should be a file info struct
   apx_serverTestConnection_clear_log(requester_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_nodeInstance_get_require_port_data_state(requester_node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_request_open_local_file(requester_connection, "Requester1.in"));
   apx_serverTestConnection_run(requester_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_require_port_data_state(requester_node_instance));
   snapshot = apx_nodeData_take_require_port_data_snapshot(requester_node_data);
   CuAssertPtrNotNull(tc, snapshot);
   CuAssertUIntEquals(tc, 0x34, snapshot[0]);
   CuAssertUIntEquals(tc, 0x12, snapshot[1]);
   free(snapshot);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester_connection));
   packet = apx_serverTestConnection_get_log_packet(requester_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, requester1_data_size, adt_bytearray_length(packet)); //Should be contents of "Requester1.in"
   memcpy(data_message, adt_bytearray_data(packet), requester1_data_size);
   CuAssertUIntEquals(tc, 4u, data_message[0]);
   CuAssertUIntEquals(tc, 0u, data_message[1]);
   CuAssertUIntEquals(tc, 0u, data_message[2]);
   CuAssertUIntEquals(tc, 0x34, data_message[3]);
   CuAssertUIntEquals(tc, 0x12, data_message[4]);
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(port_signature_map));
   map_entry = apx_portSignatureMap_find(port_signature_map, "\"VehicleSpeed\"S");
   CuAssertPtrNotNull(tc, map_entry);
   CuAssertIntEquals(tc, 1, apx_portSignatureMapEntry_get_num_providers(map_entry));
   CuAssertIntEquals(tc, 1, apx_portSignatureMapEntry_get_num_requesters(map_entry));
   apx_server_delete(server);
}

static void test_connectors_node_with_provide_port_is_connected_when_multiple_nodes_with_require_ports_are_waiting(CuTest* tc)
{
   apx_server_t* server;
   apx_serverTestConnection_t* provider_connection = NULL;
   apx_serverTestConnection_t* requester1_connection = NULL;
   apx_serverTestConnection_t* requester2_connection = NULL;
   adt_bytearray_t* packet = NULL;
   apx_nodeManager_t* node_manager;
   apx_portSignatureMap_t* port_signature_map = NULL;
   apx_portSignatureMapEntry_t* map_entry = NULL;
   int const provide_port_data_size = UINT16_SIZE;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   int const file_info_publish_size = 67;
   int const requester1_data_size = 5;
   int const requester2_data_size = 7;
   int const single_port_update_size = 5;
   uint8_t provide_port_data[UINT16_SIZE] = { 0x34u, 0x12u };
   uint8_t data_message[7];
   uint8_t* snapshot;
   apx_size_t const provider_definition_size = (apx_size_t)strlen(m_provider1_definition);
   apx_size_t const requester1_definition_size = (apx_size_t)strlen(m_requester1_definition);
   apx_size_t const requester2_definition_size = (apx_size_t)strlen(m_requester2_definition);

   server = apx_server_new();
   CuAssertPtrNotNull(tc, server);
   port_signature_map = apx_server_get_port_signature_map(server);
   CuAssertPtrNotNull(tc, port_signature_map);
   //First connection
   requester1_connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, requester1_connection);
   apx_server_accept_connection(server, (apx_serverConnection_t*)requester1_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester1_connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(requester1_connection));
   apx_serverTestConnection_run(requester1_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester1_connection));
   packet = apx_serverTestConnection_get_log_packet(requester1_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_serverTestConnection_clear_log(requester1_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester1_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(requester1_connection, APX_DEFINITION_ADDRESS_START, "Requester1.apx", requester1_definition_size));
   apx_serverTestConnection_run(requester1_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester1_connection));
   packet = apx_serverTestConnection_get_log_packet(requester1_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_serverTestConnection_clear_log(requester1_connection);
   node_manager = apx_serverTestConnection_get_node_manager(requester1_connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_nodeInstance_t* requester1_node_instance = apx_nodeManager_find(node_manager, "Requester1");
   apx_nodeData_t* requester1_node_data = apx_nodeInstance_get_node_data(requester1_node_instance);
   CuAssertPtrNotNull(tc, requester1_node_data);
   CuAssertUIntEquals(tc, requester1_definition_size, apx_nodeData_definition_data_size(requester1_node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_definition_data_state(requester1_node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester1_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(requester1_connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)m_requester1_definition, requester1_definition_size));
   apx_serverTestConnection_run(requester1_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(requester1_node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester1_connection));
   packet = apx_serverTestConnection_get_log_packet(requester1_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, file_info_publish_size, adt_bytearray_length(packet)); //Should be a file info struct
   apx_serverTestConnection_clear_log(requester1_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_nodeInstance_get_require_port_data_state(requester1_node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester1_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_request_open_local_file(requester1_connection, "Requester1.in"));
   apx_serverTestConnection_run(requester1_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_require_port_data_state(requester1_node_instance));
   //Verify that Requester1.in has default init values since no provider is available yet
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester1_connection));
   packet = apx_serverTestConnection_get_log_packet(requester1_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, requester1_data_size, adt_bytearray_length(packet)); //Should be contents of "Requester1.in"
   memcpy(data_message, adt_bytearray_data(packet), requester1_data_size);
   CuAssertUIntEquals(tc, 4u, data_message[0]);
   CuAssertUIntEquals(tc, 0u, data_message[1]);
   CuAssertUIntEquals(tc, 0u, data_message[2]);
   CuAssertUIntEquals(tc, 0xFF, data_message[3]);
   CuAssertUIntEquals(tc, 0xFF, data_message[4]);
   apx_serverTestConnection_clear_log(requester1_connection);
   //Analyze port map changes
   CuAssertIntEquals(tc, 1, apx_portSignatureMap_length(port_signature_map));
   map_entry = apx_portSignatureMap_find(port_signature_map, "\"VehicleSpeed\"S");
   CuAssertPtrNotNull(tc, map_entry);
   CuAssertIntEquals(tc, 0, apx_portSignatureMapEntry_get_num_providers(map_entry));
   CuAssertIntEquals(tc, 1, apx_portSignatureMapEntry_get_num_requesters(map_entry));

   //Second connection
   requester2_connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, requester2_connection);
   apx_server_accept_connection(server, (apx_serverConnection_t*)requester2_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester2_connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(requester2_connection));
   apx_serverTestConnection_run(requester2_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester2_connection));
   packet = apx_serverTestConnection_get_log_packet(requester2_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_serverTestConnection_clear_log(requester2_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester2_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(requester2_connection, APX_DEFINITION_ADDRESS_START, "Requester2.apx", requester2_definition_size));
   apx_serverTestConnection_run(requester2_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester2_connection));
   packet = apx_serverTestConnection_get_log_packet(requester2_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_serverTestConnection_clear_log(requester2_connection);
   node_manager = apx_serverTestConnection_get_node_manager(requester2_connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_nodeInstance_t* requester2_node_instance = apx_nodeManager_find(node_manager, "Requester2");
   apx_nodeData_t* requester2_node_data = apx_nodeInstance_get_node_data(requester2_node_instance);
   CuAssertPtrNotNull(tc, requester2_node_data);
   CuAssertUIntEquals(tc, requester2_definition_size, apx_nodeData_definition_data_size(requester2_node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_definition_data_state(requester2_node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester2_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(requester2_connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)m_requester2_definition, requester2_definition_size));
   apx_serverTestConnection_run(requester2_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(requester2_node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester2_connection));
   packet = apx_serverTestConnection_get_log_packet(requester2_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, file_info_publish_size, adt_bytearray_length(packet)); //Should be a file info struct
   apx_serverTestConnection_clear_log(requester2_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_nodeInstance_get_require_port_data_state(requester2_node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(requester2_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_request_open_local_file(requester2_connection, "Requester2.in"));
   apx_serverTestConnection_run(requester2_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_require_port_data_state(requester1_node_instance));
   //Verify that Requester1.in has default init values since no provider is available yet
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester2_connection));
   packet = apx_serverTestConnection_get_log_packet(requester2_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, requester2_data_size, adt_bytearray_length(packet)); //Should be contents of "Requester2.in"
   memcpy(data_message, adt_bytearray_data(packet), requester2_data_size);
   CuAssertUIntEquals(tc, 6u, data_message[0]);
   CuAssertUIntEquals(tc, 0u, data_message[1]);
   CuAssertUIntEquals(tc, 0u, data_message[2]);
   CuAssertUIntEquals(tc, 0xFF, data_message[3]);
   CuAssertUIntEquals(tc, 0xFF, data_message[4]);
   CuAssertUIntEquals(tc, 0xFF, data_message[5]);
   CuAssertUIntEquals(tc, 0xFF, data_message[6]);
   apx_serverTestConnection_clear_log(requester2_connection);
   //Analyze port map changes
   CuAssertIntEquals(tc, 2, apx_portSignatureMap_length(port_signature_map));
   map_entry = apx_portSignatureMap_find(port_signature_map, "\"VehicleSpeed\"S");
   CuAssertPtrNotNull(tc, map_entry);
   CuAssertIntEquals(tc, 0, apx_portSignatureMapEntry_get_num_providers(map_entry));
   CuAssertIntEquals(tc, 2, apx_portSignatureMapEntry_get_num_requesters(map_entry));

   //Third connection
   provider_connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, provider_connection);
   apx_server_accept_connection(server, (apx_serverConnection_t*)provider_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(provider_connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(provider_connection));
   apx_serverTestConnection_run(provider_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(provider_connection));
   packet = apx_serverTestConnection_get_log_packet(provider_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_serverTestConnection_clear_log(provider_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(provider_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(provider_connection, APX_PORT_DATA_ADDRESS_START, "Provider1.out", provide_port_data_size));
   apx_serverTestConnection_run(provider_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(provider_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(provider_connection, APX_DEFINITION_ADDRESS_START, "Provider1.apx", provider_definition_size));
   apx_serverTestConnection_run(provider_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(provider_connection));
   packet = apx_serverTestConnection_get_log_packet(provider_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message (TestNode1.apx)
   apx_serverTestConnection_clear_log(provider_connection);
   node_manager = apx_serverTestConnection_get_node_manager(provider_connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_nodeInstance_t* provider_node_instance = apx_nodeManager_find(node_manager, "Provider1");
   apx_nodeData_t* provider_node_data = apx_nodeInstance_get_node_data(provider_node_instance);
   CuAssertPtrNotNull(tc, provider_node_data);
   CuAssertUIntEquals(tc, provider_definition_size, apx_nodeData_definition_data_size(provider_node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_definition_data_state(provider_node_instance));
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(provider_connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(provider_connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)m_provider1_definition, provider_definition_size));
   apx_serverTestConnection_run(provider_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(provider_node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(provider_connection));
   packet = apx_serverTestConnection_get_log_packet(provider_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message (TestNode1.out)
   apx_serverTestConnection_clear_log(provider_connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_provide_port_data_state(provider_node_instance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(provider_connection, APX_PORT_DATA_ADDRESS_START, provide_port_data, provide_port_data_size));
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_provide_port_data_state(provider_node_instance));
   apx_serverTestConnection_run(provider_connection);

   //Analyze port map changes
   CuAssertIntEquals(tc, 2, apx_portSignatureMap_length(port_signature_map));
   map_entry = apx_portSignatureMap_find(port_signature_map, "\"VehicleSpeed\"S");
   CuAssertPtrNotNull(tc, map_entry);
   CuAssertIntEquals(tc, 1, apx_portSignatureMapEntry_get_num_providers(map_entry));
   CuAssertIntEquals(tc, 2, apx_portSignatureMapEntry_get_num_requesters(map_entry));

   //Verify that require ports have been locally updated in server
   snapshot = apx_nodeData_take_require_port_data_snapshot(requester1_node_data);
   CuAssertPtrNotNull(tc, snapshot);
   CuAssertUIntEquals(tc, 0x34, snapshot[0]);
   CuAssertUIntEquals(tc, 0x12, snapshot[1]);
   free(snapshot);
   snapshot = apx_nodeData_take_require_port_data_snapshot(requester2_node_data);
   CuAssertPtrNotNull(tc, snapshot);
   CuAssertUIntEquals(tc, 0xff, snapshot[0]);
   CuAssertUIntEquals(tc, 0xff, snapshot[1]);
   CuAssertUIntEquals(tc, 0x34, snapshot[2]);
   CuAssertUIntEquals(tc, 0x12, snapshot[3]);
   free(snapshot);

   //Check for data updates in each connections
   apx_serverTestConnection_run(provider_connection);
   apx_serverTestConnection_run(requester1_connection);
   apx_serverTestConnection_run(requester2_connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester1_connection));
   packet = apx_serverTestConnection_get_log_packet(requester1_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, single_port_update_size, adt_bytearray_length(packet));
   memcpy(data_message, adt_bytearray_data(packet), single_port_update_size);
   CuAssertUIntEquals(tc, 4u, data_message[0]);
   CuAssertUIntEquals(tc, 0u, data_message[1]);
   CuAssertUIntEquals(tc, 0u, data_message[2]); //offset: 0
   CuAssertUIntEquals(tc, 0x34, data_message[3]);
   CuAssertUIntEquals(tc, 0x12, data_message[4]);
   apx_serverTestConnection_clear_log(requester1_connection);

   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(requester2_connection));
   packet = apx_serverTestConnection_get_log_packet(requester2_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, single_port_update_size, adt_bytearray_length(packet));
   memcpy(data_message, adt_bytearray_data(packet), single_port_update_size);
   CuAssertUIntEquals(tc, 4u, data_message[0]);
   CuAssertUIntEquals(tc, 0u, data_message[1]);
   CuAssertUIntEquals(tc, 2u, data_message[2]); //offset: 2
   CuAssertUIntEquals(tc, 0x34, data_message[3]);
   CuAssertUIntEquals(tc, 0x12, data_message[4]);
   apx_serverTestConnection_clear_log(requester2_connection);

   apx_server_delete(server);
}