/*****************************************************************************
* \file      testsuite_server_connection.c
* \author    Conny Gustafsson
* \date      2019-11-29
* \brief     Unit tests for server connection
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
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
static void test_remotefile_protocol_version_is_parsed_from_greeting_header(CuTest* tc);
static void test_message_format_is_parsed_from_greeting_header(CuTest* tc);
static void test_default_connection_type_is_parsed_from_greeting_header(CuTest* tc);
static void test_monitor_connection_type_is_parsed_from_greeting_header(CuTest* tc);
static void test_event_connection_type_is_parsed_from_greeting_header(CuTest* tc);
static void test_accept_header_is_sent_when_new_greeting_format_is_seen(CuTest* tc);
static void test_msg_size_hint_returned_for_partial_message(CuTest* tc);



//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_server_connection(void)
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
   SUITE_ADD_TEST(suite, test_remotefile_protocol_version_is_parsed_from_greeting_header);
   SUITE_ADD_TEST(suite, test_message_format_is_parsed_from_greeting_header);
   SUITE_ADD_TEST(suite, test_default_connection_type_is_parsed_from_greeting_header);
   SUITE_ADD_TEST(suite, test_monitor_connection_type_is_parsed_from_greeting_header);
   SUITE_ADD_TEST(suite, test_event_connection_type_is_parsed_from_greeting_header);
   SUITE_ADD_TEST(suite, test_accept_header_is_sent_when_new_greeting_format_is_seen);
   SUITE_ADD_TEST(suite, test_msg_size_hint_returned_for_partial_message);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_acknowledge_is_sent_when_greeting_is_seen(CuTest* tc)
{
   apx_server_test_connection_t* connection;
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
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, (int)sizeof(actual), adt_bytearray_length(packet)); //Should contain acknowledge message
   memcpy(actual, adt_bytearray_data(packet), sizeof(actual));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(actual)));

   apx_server_test_connection_delete(connection);
}

static void test_node_instance_is_created_when_definition_file_is_seen(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   apx_node_manager_t* node_manager;
   adt_bytearray_t* packet;
   int const acknowledge_size = 9;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   node_manager = apx_server_test_connection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   CuAssertIntEquals(tc, 0, apx_node_manager_length(node_manager));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   CuAssertIntEquals(tc, 1, apx_node_manager_length(node_manager));
   apx_node_instance_t* node_instance = apx_node_manager_find(node_manager, "TestNode1");
   CuAssertPtrNotNull(tc, node_instance);
   apx_node_data_t const* node_data = apx_node_instance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_node_data_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertPtrNotNull(tc, apx_node_data_get_definition_data(node_data));
   CuAssertIntEquals(tc, RMF_DIGEST_TYPE_NONE, apx_node_data_get_checksum_type(node_data));
   apx_server_test_connection_delete(connection);
}

static void test_file_open_request_is_sent_after_definition_file_is_seen(CuTest* tc)
{
   apx_server_test_connection_t* connection;
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
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   memcpy(actual, adt_bytearray_data(packet), open_request_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, open_request_size));
   apx_server_test_connection_delete(connection);
}

static void test_file_open_request_is_sent_after_definition_file_is_seen_in_compatibility_mode(CuTest* tc)
{
   apx_server_test_connection_t* connection;
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
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_server_test_connection_enable_compatibility_mode(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   memcpy(actual, adt_bytearray_data(packet), open_request_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, open_request_size));
   apx_server_test_connection_delete(connection);
}

static void test_definition_is_parsed_after_file_has_been_sent(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   adt_bytearray_t* packet;
   apx_node_manager_t* node_manager;
   int const acknowledge_size = 9;
   int const open_request_size = 13;

   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_server_test_connection_clear_log(connection);
   node_manager = apx_server_test_connection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_node_instance_t* node_instance = apx_node_manager_find(node_manager, "TestNode1");
   apx_node_data_t const* node_data = apx_node_instance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_node_data_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_write_remote_data(connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*) apx_text, definition_size));
   CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_definition_data_state(node_instance));
   apx_server_test_connection_run(connection);
   apx_server_test_connection_delete(connection);
}

static void test_provide_port_data_is_requested_after_definition_file_has_been_parsed(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   adt_bytearray_t* packet;
   apx_node_manager_t* node_manager;
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
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode1.out", provide_port_data_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_server_test_connection_clear_log(connection);
   node_manager = apx_server_test_connection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_node_instance_t* node_instance = apx_node_manager_find(node_manager, "TestNode1");
   apx_node_data_t const* node_data = apx_node_instance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_node_data_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_write_remote_data(connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)apx_text, definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   memcpy(actual, adt_bytearray_data(packet), open_request_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, open_request_size));
   apx_server_test_connection_delete(connection);
}

static void test_provide_port_data_is_received_after_request(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   adt_bytearray_t* packet;
   apx_node_manager_t* node_manager;
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
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode1.out", provide_port_data_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_server_test_connection_clear_log(connection);
   node_manager = apx_server_test_connection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_node_instance_t* node_instance = apx_node_manager_find(node_manager, "TestNode1");
   apx_node_data_t const* node_data = apx_node_instance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_node_data_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_write_remote_data(connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)apx_text, definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_node_instance_get_provide_port_data_state(node_instance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_write_remote_data(connection, APX_PORT_DATA_ADDRESS_START, provide_port_data, provide_port_data_size));
   CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_provide_port_data_state(node_instance));
   apx_server_test_connection_delete(connection);
}

static void test_require_port_data_is_published_after_definition_has_been_parsed(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   adt_bytearray_t* packet;
   apx_node_manager_t* node_manager;
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
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_server_test_connection_clear_log(connection);
   node_manager = apx_server_test_connection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_node_instance_t* node_instance = apx_node_manager_find(node_manager, "TestNode1");
   apx_node_data_t const* node_data = apx_node_instance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_node_data_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_write_remote_data(connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)apx_text, definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, file_info_publish_size, adt_bytearray_length(packet)); //Should be a file info struct
   memcpy(actual, adt_bytearray_data(packet), file_info_publish_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, file_info_publish_size));
   apx_server_test_connection_delete(connection);
}

static void test_require_port_data_is_sent_after_file_open_request_received(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   adt_bytearray_t* packet;
   apx_node_manager_t* node_manager;
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
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, acknowledge_size, adt_bytearray_length(packet)); //This is the acknowledge message
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_publish_remote_file(connection, APX_DEFINITION_ADDRESS_START, "TestNode1.apx", definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, open_request_size, adt_bytearray_length(packet)); //Should be a file open request message
   apx_server_test_connection_clear_log(connection);
   node_manager = apx_server_test_connection_get_node_manager(connection);
   CuAssertPtrNotNull(tc, node_manager);
   apx_node_instance_t* node_instance = apx_node_manager_find(node_manager, "TestNode1");
   apx_node_data_t const* node_data = apx_node_instance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_node_data_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_write_remote_data(connection, APX_DEFINITION_ADDRESS_START, (uint8_t const*)apx_text, definition_size));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_definition_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, file_info_publish_size, adt_bytearray_length(packet)); //Should be a file info struct
   apx_server_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_node_instance_get_require_port_data_state(node_instance));
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_request_open_local_file(connection, "TestNode1.in"));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_require_port_data_state(node_instance));
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, data_write_size, adt_bytearray_length(packet)); //This is the require port data
   memcpy(actual, adt_bytearray_data(packet), data_write_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, data_write_size));
   apx_server_test_connection_delete(connection);
}

static void test_remotefile_protocol_version_is_parsed_from_greeting_header(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   char const* greeting = "RMFP/1.1\n\n";
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_custom_greeting_header(connection, greeting));
   apx_server_test_connection_run(connection);
   CuAssertUIntEquals(tc, RMF_PROTOCOL_VERSION_ID_1_1, apx_server_test_connection_get_rmf_proto_id(connection));

   apx_server_test_connection_delete(connection);
}

static void test_message_format_is_parsed_from_greeting_header(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   char const* greeting = "RMFP/1.1\n"
      "Message-Size: 16\n"
      "\n";
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_custom_greeting_header(connection, greeting));
   apx_server_test_connection_run(connection);
   CuAssertUIntEquals(tc, UINT16_SIZE, apx_server_test_connection_get_num_header_size(connection));

   apx_server_test_connection_delete(connection);
}

static void test_default_connection_type_is_parsed_from_greeting_header(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   char const* greeting = "RMFP/1.1\n"
      "Connection-Type: Default\n"
      "\n";
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_custom_greeting_header(connection, greeting));
   apx_server_test_connection_run(connection);
   CuAssertUIntEquals(tc, APX_CONNECTION_TYPE_DEFAULT, apx_server_test_connection_get_connection_type(connection));

   apx_server_test_connection_delete(connection);
}

static void test_monitor_connection_type_is_parsed_from_greeting_header(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   char const* greeting = "RMFP/1.1\n"
      "Connection-Type: Monitor\n"
      "\n";
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_custom_greeting_header(connection, greeting));
   apx_server_test_connection_run(connection);
   CuAssertUIntEquals(tc, APX_CONNECTION_TYPE_MONITOR, apx_server_test_connection_get_connection_type(connection));

   apx_server_test_connection_delete(connection);
}

static void test_event_connection_type_is_parsed_from_greeting_header(CuTest* tc)
{
   apx_server_test_connection_t* connection;
   char const* greeting = "RMFP/1.1\n"
      "Connection-Type: Event\n"
      "\n";
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_custom_greeting_header(connection, greeting));
   apx_server_test_connection_run(connection);
   CuAssertUIntEquals(tc, APX_CONNECTION_TYPE_EVENT, apx_server_test_connection_get_connection_type(connection));

   apx_server_test_connection_delete(connection);
}

static void test_accept_header_is_sent_when_new_greeting_format_is_seen(CuTest* tc)
{
   uint32_t const connection_id = 0u;
   apx_server_test_connection_t* connection;
   adt_bytearray_t* packet;
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
      (uint8_t)RMF_CMD_ACCEPT_HEADER,
      0u,
      0u,
      0u,
      //connection id
      0x00u,
      0x00u,
      0x00u,
      0x00u,
   };
   memset(actual, 0, sizeof(actual));
   connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_server_test_connection_set_tester_protocol_version(connection, RMF_PROTOCOL_VERSION_ID_1_1);
   apx_server_test_connection_set_tester_connection_type(connection, APX_CONNECTION_TYPE_MONITOR);
   apx_server_test_connection_set_connection_id(connection, connection_id);
   CuAssertIntEquals(tc, 0u, apx_server_test_connection_log_length(connection));
   apx_connection_base_start((apx_connection_base_t*)connection);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_server_test_connection_send_greeting_header(connection));
   apx_server_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_server_test_connection_log_length(connection));
   packet = apx_server_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, (int)sizeof(actual), adt_bytearray_length(packet)); //Should contain acknowledge message
   memcpy(actual, adt_bytearray_data(packet), sizeof(actual));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(actual)));

   apx_server_test_connection_delete(connection);
}

static void test_msg_size_hint_returned_for_partial_message(CuTest* tc)
{
   apx_server_test_connection_t* connection = apx_server_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_connection_base_start((apx_connection_base_t*)connection);

   /* Encode a message header declaring payload size of 1000 bytes, but only provide a partial buffer */
   uint8_t buffer[16];
   memset(buffer, 0, sizeof(buffer));
   uint32_t expected_payload_size = 1000u;
   int32_t header_size = numheader_encode32(buffer, (int32_t)sizeof(buffer), expected_payload_size);
   CuAssertTrue(tc, header_size > 0);
   apx_size_t total_expected = (apx_size_t)header_size + expected_payload_size;

   apx_size_t parse_len = 0u;
   apx_size_t msg_size_hint = 0u;

   /* Provide only 10 bytes (header + partial payload) */
   int result = apx_server_connection_on_data_received(&connection->base, buffer, 10u, &parse_len, &msg_size_hint);
   CuAssertIntEquals(tc, 0, result);
   CuAssertUIntEquals(tc, 0u, parse_len);
   CuAssertUIntEquals(tc, total_expected, msg_size_hint);

   apx_server_test_connection_delete(connection);
}