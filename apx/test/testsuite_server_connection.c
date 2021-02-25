//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include "pack.h"
#include "apx/numheader.h"
#include "apx/server_test_connection.h"
#include "sha256.h"
#include "CuTest.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_acknowledge_is_sent_when_greeting_is_seen(CuTest* tc);
static void test_node_instance_is_created_when_definition_file_is_seen(CuTest* tc);
static void test_file_open_request_is_sent_after_definition_file_is_seen(CuTest* tc);
static void test_file_open_request_is_sent_after_definition_file_is_seen_in_compatibility_mode(CuTest* tc);
static void test_definition_is_parsed_after_file_has_been_sent(CuTest* tc);
static void test_provide_port_data_is_requested_after_definition_file_has_been_parsed(CuTest* tc);
static void test_provide_port_data_is_received_after_request(CuTest* tc);
static void test_require_port_data_is_published_after_definition_has_been_parsed(CuTest* tc);
static void test_require_port_data_is_sent_after_file_open_request_received(CuTest* tc);



//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_serverConnection(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_acknowledge_is_sent_when_greeting_is_seen);
   SUITE_ADD_TEST(suite, test_node_instance_is_created_when_definition_file_is_seen);
   SUITE_ADD_TEST(suite, test_file_open_request_is_sent_after_definition_file_is_seen);
   SUITE_ADD_TEST(suite, test_file_open_request_is_sent_after_definition_file_is_seen_in_compatibility_mode);
   SUITE_ADD_TEST(suite, test_definition_is_parsed_after_file_has_been_sent);
   SUITE_ADD_TEST(suite, test_provide_port_data_is_requested_after_definition_file_has_been_parsed);
   SUITE_ADD_TEST(suite, test_provide_port_data_is_received_after_request);
   SUITE_ADD_TEST(suite, test_require_port_data_is_published_after_definition_has_been_parsed);
   SUITE_ADD_TEST(suite, test_require_port_data_is_sent_after_file_open_request_received);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_acknowledge_is_sent_when_greeting_is_seen(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet;
   uint8_t actual[9];
   uint8_t expected[9] = {
      //message size
      8,
      //write address
      0xBFu,
      0xFFu,
      0xFCu,
      0x00u,
      //command type
      (uint8_t)RMF_CMD_ACK_MSG,
      0u,
      0u,
      0u,
   };
   memset(actual, 0, sizeof(actual));
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(connection));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, (int)sizeof(actual), adt_bytearray_length(packet)); //Should contain acknowledge message
   memcpy(actual, adt_bytearray_data(packet), sizeof(actual));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(actual)));

   apx_serverTestConnection_delete(connection);
}

static void test_node_instance_is_created_when_definition_file_is_seen(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   apx_nodeManager_t* node_manager;
   adt_bytearray_t* packet;
   int const acknowledge_size = 9;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(connection));
   apx_serverTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   node_manager = apx_serverTestConnection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   CuAssertIntEquals(tc, 0, apx_nodeManager_length(node_manager));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   CuAssertIntEquals(tc, 1, apx_nodeManager_length(node_manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_find(node_manager, "TestNode1");
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t const* node_data = apx_nodeInstance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_nodeData_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_definition_data_state(node_instance));
   CuAssertPtrNotNull(tc, apx_nodeData_get_definition_data(node_data));
   CuAssertIntEquals(tc, RMF_DIGEST_TYPE_NONE, apx_nodeData_get_checksum_type(node_data));
   apx_serverTestConnection_delete(connection);
}

static void test_file_open_request_is_sent_after_definition_file_is_seen(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   uint8_t actual[13];
   uint8_t expected[13] = {
         //message size
         12,
         //write address
         0xBFu,
         0xFFu,
         0xFCu,
         0x00u,
         //command type
         (uint8_t)RMF_CMD_OPEN_FILE_MSG,
         0u,
         0u,
         0u,
         //address
         0u,
         0u,
         0u,
         4u,
   };
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
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
   memcpy(actual, adt_bytearray_data(packet), open_request_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, open_request_size));
   apx_serverTestConnection_delete(connection);
}

static void test_file_open_request_is_sent_after_definition_file_is_seen_in_compatibility_mode(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   uint8_t actual[13];
   uint8_t expected[13] = {
      //message size
      12,
      //write address
      0xBFu,
      0xFFu,
      0xFCu,
      0x00u,
      //command type
      (uint8_t)RMF_CMD_OPEN_FILE_MSG,
      0u,
      0u,
      0u,
      //address
      0u,
      0u,
      0u,
      4u,
   };
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_serverTestConnection_enable_compatibility_mode(connection);
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
   memcpy(actual, adt_bytearray_data(packet), open_request_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, open_request_size));
   apx_serverTestConnection_delete(connection);
}

static void test_definition_is_parsed_after_file_has_been_sent(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet;
   apx_nodeManager_t* node_manager;
   int const acknowledge_size = 9;
   int const open_request_size = 13;

   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*) apx_text, definition_size));
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(node_instance));
   apx_serverTestConnection_run(connection);
   apx_serverTestConnection_delete(connection);
}

static void test_provide_port_data_is_requested_after_definition_file_has_been_parsed(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet;
   apx_nodeManager_t* node_manager;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   int const provide_port_data_size = 2u;
   uint8_t actual[13];
   uint8_t expected[13] = {
      //message size
      12,
      //write address
      0xBFu,
      0xFFu,
      0xFCu,
      0x00u,
      //command type
      (uint8_t)RMF_CMD_OPEN_FILE_MSG,
      0u,
      0u,
      0u,
      //address
      0u,
      0u,
      0u,
      0u,
   };


   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"RequirePort1\"C(0,3):=3\n"
      "P\"RequirePort2\"C(0,7):=7\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
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
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   memcpy(actual, adt_bytearray_data(packet), open_request_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, open_request_size));
   apx_serverTestConnection_delete(connection);
}

static void test_provide_port_data_is_received_after_request(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet;
   apx_nodeManager_t* node_manager;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   int const provide_port_data_size = 2u;
   uint8_t provide_port_data[2] = { 1u, 2u };

   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"RequirePort1\"C(0,3):=3\n"
      "P\"RequirePort2\"C(0,7):=7\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
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
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_serverTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_provide_port_data_state(node_instance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_write_remote_data(connection, APX_PORT_DATA_ADDRESS_START, provide_port_data, provide_port_data_size));
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_provide_port_data_state(node_instance));
   apx_serverTestConnection_delete(connection);
}

static void test_require_port_data_is_published_after_definition_has_been_parsed(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet;
   apx_nodeManager_t* node_manager;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   int const require_port_data_size = 2u;
   int const file_info_publish_size = 66;
   uint8_t actual[66];
   uint8_t expected[66] = {
      //message size
      65,
      //write address
      0xBFu,
      0xFFu,
      0xFCu,
      0x00u,
      //command type
      (uint8_t)RMF_CMD_PUBLISH_FILE_MSG,
      0u,
      0u,
      0u,
      //address
      0u,
      0u,
      0u,
      0u,
      //FileSize (U32LE)
      (uint8_t)require_port_data_size,
      0u,
      0u,
      0u,
      //FileType (U16LE)
      (uint8_t)RMF_FILE_TYPE_FIXED,
      0u,
      //DigestType (U16LE)
      (uint8_t)RMF_DIGEST_TYPE_NONE,
      0u,
      //DigestData U8[32]
      0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
      0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
      //FileName (Null-terminated string)
      'T', 'e', 's', 't', 'N','o','d','e', '1', '.','i','n','\0'
   };


   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";
   memset(actual, 0, sizeof(actual));
   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
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
   memcpy(actual, adt_bytearray_data(packet), file_info_publish_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, file_info_publish_size));
   apx_serverTestConnection_delete(connection);
}

static void test_require_port_data_is_sent_after_file_open_request_received(CuTest* tc)
{
   apx_serverTestConnection_t* connection;
   adt_bytearray_t* packet;
   apx_nodeManager_t* node_manager;
   int const acknowledge_size = 9;
   int const open_request_size = 13;
   int const file_info_publish_size = 66;
   int const data_write_size = 5;
   uint8_t actual[5];
   uint8_t expected[5] =
   {
   //Size
   4u,
   //Address
   0u,
   0u,
   //Data
   3u,
   7u
   };


   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   memset(actual, 0, sizeof(actual));
   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
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
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_require_port_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(connection));
   packet = apx_serverTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, data_write_size, adt_bytearray_length(packet)); //This is the require port data
   memcpy(actual, adt_bytearray_data(packet), data_write_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, data_write_size));
   apx_serverTestConnection_delete(connection);
}
