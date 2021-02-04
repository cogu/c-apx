//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include "pack.h"
#include "apx/numheader.h"
#include "apx/client_test_connection.h"
#include "sha256.h"
//#include "client_event_listener_spy.h"
#include "CuTest.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_create_test_connection(CuTest* tc);
static void test_local_files_are_published_when_greeting_is_accepted(CuTest* tc);
static void test_definition_file_is_sent_when_file_open_is_requested(CuTest* tc);
static void test_provide_port_file_is_sent_when_file_open_requested(CuTest* tc);
static void test_require_port_file_is_requested_when_published_by_server(CuTest* tc);
static void test_node_data_is_updated_when_require_port_is_written(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_clientTestConnection(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_test_connection);
   SUITE_ADD_TEST(suite, test_local_files_are_published_when_greeting_is_accepted);
   SUITE_ADD_TEST(suite, test_definition_file_is_sent_when_file_open_is_requested);
   SUITE_ADD_TEST(suite, test_provide_port_file_is_sent_when_file_open_requested);
   SUITE_ADD_TEST(suite, test_require_port_file_is_requested_when_published_by_server);
   SUITE_ADD_TEST(suite, test_node_data_is_updated_when_require_port_is_written);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_create_test_connection(CuTest* tc)
{
   apx_clientTestConnection_t* connection;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "P\"ProvidePort2\"C(0,7)\n";
   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_build_node(connection, apx_text));
   apx_clientTestConnection_delete(connection);
}

static void test_local_files_are_published_when_greeting_is_accepted(CuTest* tc)
{
   apx_clientTestConnection_t *connection;
   adt_bytearray_t *packet;
   int const message_size = 67;
   uint8_t actual[67];
   uint8_t expected[67] = {
      66, //(NumHeader short)
      //RemoteFile Header (High Address)
      0xBFu,
      0xFFu,
      0xFCu,
      0x00u,
      //CmdType (U32LE)
      (uint8_t)RMF_CMD_PUBLISH_FILE_MSG,
      0u,
      0u,
      0u,
      //StartAddress (U32LE)
      0u,
      0u,
      0u,
      0u,
      //FileSize (U32LE)
      2u,
      0u,
      0u,
      0u,
      //FileType (U16LE)
      (uint8_t)RMF_FILE_TYPE_FIXED,
      0u,
      //DigestType (U16LE)
      (uint8_t) RMF_DIGEST_TYPE_NONE,
      0u,
      //DigestData U8[32]
      0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
      0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
      //FileName (Null-terminated string)
      'T', 'e', 's', 't', 'N','o','d','e', '1', '.','o','u','t','\0'
   };
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "P\"ProvidePort2\"C(0,7)\n";

   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_greeting_header_accepted_notification(connection);
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection));
   packet = apx_clientTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, message_size * 2, adt_bytearray_length(packet)); //Should contain two published files
   //Verify first message
   memcpy(actual, adt_bytearray_data(packet), message_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, message_size));
   //Verify second message
   expected[12] = 0x04; //StartAddress[0]
   expected[13] = (uint8_t)strlen(apx_text); //FileSize[0]
   expected[19] = (uint8_t)RMF_DIGEST_TYPE_SHA256;
   sha256_calc(&expected[21], apx_text, strlen(apx_text));
   expected[63] = 'a'; //FileName extension
   expected[64] = 'p'; //FileName extension
   expected[65] = 'x'; //FileName extension
   memcpy(actual, adt_bytearray_data(packet) + message_size, message_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, message_size));

   apx_clientTestConnection_delete(connection);
}

static void test_definition_file_is_sent_when_file_open_is_requested(CuTest* tc)
{
   apx_clientTestConnection_t* connection;
   adt_bytearray_t* packet;
   uint8_t const* buffer;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "P\"ProvidePort2\"C(0,7)\n";

   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_greeting_header_accepted_notification(connection);
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection));
   packet = apx_clientTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, 67 * 2, adt_bytearray_length(packet)); //Should contain two published files
   apx_clientTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_request_open_local_file(connection, "TestNode1.apx"));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection));
   packet = apx_clientTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, NUMHEADER32_SHORT_SIZE + RMF_HIGH_ADDR_SIZE + ((int)strlen(apx_text)), adt_bytearray_length(packet));
   buffer = adt_bytearray_const_data(packet);
   CuAssertUIntEquals(tc, 69, buffer[0]);
   bool more_bit = false;
   uint32_t address = RMF_INVALID_ADDRESS;
   CuAssertUIntEquals(tc, RMF_HIGH_ADDR_SIZE, rmf_address_decode(&buffer[1], &buffer[1] + RMF_HIGH_ADDR_SIZE, &address, &more_bit));
   CuAssertUIntEquals(tc, APX_DEFINITION_ADDRESS_START, address);
   CuAssertFalse(tc, more_bit);
   CuAssertIntEquals(tc, 0, memcmp(apx_text, &buffer[5], strlen(apx_text)));
   apx_clientTestConnection_delete(connection);
}

static void test_provide_port_file_is_sent_when_file_open_requested(CuTest* tc)
{
   apx_clientTestConnection_t* connection;
   adt_bytearray_t* packet;
   uint8_t const* buffer;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "P\"ProvidePort2\"C(0,7)\n";

   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_greeting_header_accepted_notification(connection);
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection));
   packet = apx_clientTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, 67 * 2, adt_bytearray_length(packet)); //Should contain two published files
   apx_clientTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_request_open_local_file(connection, "TestNode1.out"));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection));
   packet = apx_clientTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, NUMHEADER32_SHORT_SIZE + RMF_LOW_ADDR_SIZE + UINT8_SIZE*2, adt_bytearray_length(packet));
   buffer = adt_bytearray_const_data(packet);
   CuAssertUIntEquals(tc, 4, buffer[0]);
   bool more_bit = false;
   uint32_t address = RMF_INVALID_ADDRESS;
   CuAssertUIntEquals(tc, RMF_LOW_ADDR_SIZE, rmf_address_decode(&buffer[1], &buffer[1] + RMF_LOW_ADDR_SIZE, &address, &more_bit));
   CuAssertUIntEquals(tc, APX_PORT_DATA_ADDRESS_START, address);
   CuAssertFalse(tc, more_bit);
   CuAssertUIntEquals(tc, 0, buffer[3]);
   CuAssertUIntEquals(tc, 0, buffer[4]);
   apx_clientTestConnection_delete(connection);
}

static void test_require_port_file_is_requested_when_published_by_server(CuTest* tc)
{
   apx_clientTestConnection_t* connection;
   adt_bytearray_t* packet;
   uint8_t const* buffer;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_greeting_header_accepted_notification(connection);
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection));
   packet = apx_clientTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, 67, adt_bytearray_length(packet)); //Should contain one published file
   apx_clientTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode1.in", UINT8_SIZE*2));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection));
   packet = apx_clientTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, NUMHEADER32_SHORT_SIZE + RMF_HIGH_ADDR_SIZE + RMF_CMD_TYPE_SIZE + UINT32_SIZE, adt_bytearray_length(packet));
   buffer = adt_bytearray_const_data(packet);
   CuAssertUIntEquals(tc, UINT32_SIZE*3, buffer[0]);
   bool more_bit = false;
   uint32_t write_address = RMF_INVALID_ADDRESS;
   CuAssertUIntEquals(tc, RMF_HIGH_ADDR_SIZE, rmf_address_decode(&buffer[1], &buffer[1] + RMF_HIGH_ADDR_SIZE, &write_address, &more_bit));
   CuAssertUIntEquals(tc, RMF_CMD_AREA_START_ADDRESS, write_address);
   CuAssertFalse(tc, more_bit);
   uint32_t cmd_type;
   uint32_t file_address;
   cmd_type = unpackLE(&buffer[5], UINT32_SIZE);
   file_address = unpackLE(&buffer[9], UINT32_SIZE);
   CuAssertUIntEquals(tc, RMF_CMD_OPEN_FILE_MSG, cmd_type);
   CuAssertUIntEquals(tc, APX_PORT_DATA_ADDRESS_START, file_address);
   apx_clientTestConnection_delete(connection);
}

static void test_node_data_is_updated_when_require_port_is_written(CuTest* tc)
{
   apx_clientTestConnection_t* connection;
   adt_bytearray_t* packet;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_greeting_header_accepted_notification(connection);
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection));
   packet = apx_clientTestConnection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, 67, adt_bytearray_length(packet)); //Should contain one published file
   apx_clientTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode1.in", UINT8_SIZE * 2));
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_clientTestConnection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_clientTestConnection_log_length(connection)); //This should be the file open request
   apx_clientTestConnection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_clientTestConnection_log_length(connection));
   apx_nodeInstance_t* node_instance = apx_clientTestConnection_find_node(connection, "TestNode1");
   CuAssertPtrNotNull(tc, node_instance);
   CuAssertUIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_get_require_port_data_state(node_instance));

   uint8_t data[UINT8_SIZE * 2] = { 1, 7 };
   uint8_t buffer[UINT8_SIZE * 2] = { 0, 0 };
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_write_remote_data(connection, APX_PORT_DATA_ADDRESS_START, &data[0], (apx_size_t)sizeof(data)));
   CuAssertUIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_require_port_data_state(node_instance));
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_require_port_data(node_data, 0, &buffer[0], (apx_size_t) sizeof(buffer)));
   CuAssertUIntEquals(tc, data[0], buffer[0]);
   CuAssertUIntEquals(tc, data[1], buffer[1]);

   apx_clientTestConnection_delete(connection);
}