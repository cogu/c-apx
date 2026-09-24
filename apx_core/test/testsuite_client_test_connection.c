/*****************************************************************************
* \file      testsuite_client_test_connection.c
* \author    Conny Gustafsson
* \date      2019-08-04
* \brief     Unit tests for client test connection
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
static void test_client_requests_open_cout_cin_before_in_file(CuTest* tc);
static void test_client_requests_open_cout_cin_before_in_file_when_in_published_first(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_apx_client_test_connection(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_test_connection);
   SUITE_ADD_TEST(suite, test_local_files_are_published_when_greeting_is_accepted);
   SUITE_ADD_TEST(suite, test_definition_file_is_sent_when_file_open_is_requested);
   SUITE_ADD_TEST(suite, test_provide_port_file_is_sent_when_file_open_requested);
   SUITE_ADD_TEST(suite, test_require_port_file_is_requested_when_published_by_server);
   SUITE_ADD_TEST(suite, test_node_data_is_updated_when_require_port_is_written);
   SUITE_ADD_TEST(suite, test_client_requests_open_cout_cin_before_in_file);
   SUITE_ADD_TEST(suite, test_client_requests_open_cout_cin_before_in_file_when_in_published_first);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_create_test_connection(CuTest* tc)
{
   apx_client_test_connection_t* connection;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "P\"ProvidePort2\"C(0,7)\n";
   connection = apx_client_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_build_node(connection, apx_text));
   apx_client_test_connection_delete(connection);
}

static void test_local_files_are_published_when_greeting_is_accepted(CuTest* tc)
{
   apx_client_test_connection_t *connection;
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

   connection = apx_client_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_greeting_header_accepted_notification(connection);
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   packet = apx_client_test_connection_get_log_packet(connection, 0);
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

   apx_client_test_connection_delete(connection);
}

static void test_definition_file_is_sent_when_file_open_is_requested(CuTest* tc)
{
   apx_client_test_connection_t* connection;
   adt_bytearray_t* packet;
   uint8_t const* buffer;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "P\"ProvidePort2\"C(0,7)\n";

   connection = apx_client_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_greeting_header_accepted_notification(connection);
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   packet = apx_client_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, 67 * 2, adt_bytearray_length(packet)); //Should contain two published files
   apx_client_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_request_open_local_file(connection, "TestNode1.apx"));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   packet = apx_client_test_connection_get_log_packet(connection, 0);
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
   apx_client_test_connection_delete(connection);
}

static void test_provide_port_file_is_sent_when_file_open_requested(CuTest* tc)
{
   apx_client_test_connection_t* connection;
   adt_bytearray_t* packet;
   uint8_t const* buffer;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "P\"ProvidePort2\"C(0,7)\n";

   connection = apx_client_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_greeting_header_accepted_notification(connection);
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   packet = apx_client_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, 67 * 2, adt_bytearray_length(packet)); //Should contain two published files
   apx_client_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_request_open_local_file(connection, "TestNode1.out"));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   packet = apx_client_test_connection_get_log_packet(connection, 0);
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
   apx_client_test_connection_delete(connection);
}

static void test_require_port_file_is_requested_when_published_by_server(CuTest* tc)
{
   apx_client_test_connection_t* connection;
   adt_bytearray_t* packet;
   uint8_t const* buffer;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   connection = apx_client_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_greeting_header_accepted_notification(connection);
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   packet = apx_client_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, 67, adt_bytearray_length(packet)); //Should contain one published file
   apx_client_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode1.in", UINT8_SIZE*2));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   packet = apx_client_test_connection_get_log_packet(connection, 0);
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
   apx_client_test_connection_delete(connection);
}

static void test_node_data_is_updated_when_require_port_is_written(CuTest* tc)
{
   apx_client_test_connection_t* connection;
   adt_bytearray_t* packet;
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "R\"RequirePort1\"C(0,3):=3\n"
      "R\"RequirePort2\"C(0,7):=7\n";

   connection = apx_client_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_greeting_header_accepted_notification(connection);
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   packet = apx_client_test_connection_get_log_packet(connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, 67, adt_bytearray_length(packet)); //Should contain one published file
   apx_client_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode1.in", UINT8_SIZE * 2));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection)); //This should be the file open request
   apx_client_test_connection_clear_log(connection);
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_node_instance_t* node_instance = apx_client_test_connection_find_node(connection, "TestNode1");
   CuAssertPtrNotNull(tc, node_instance);
   CuAssertUIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_node_instance_get_require_port_data_state(node_instance));

   uint8_t data[UINT8_SIZE * 2] = { 1, 7 };
   uint8_t buffer[UINT8_SIZE * 2] = { 0, 0 };
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_write_remote_data(connection, APX_PORT_DATA_ADDRESS_START, &data[0], (apx_size_t)sizeof(data)));
   CuAssertUIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_require_port_data_state(node_instance));
   apx_node_data_t* node_data = apx_node_instance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_require_port_data(node_data, 0, &buffer[0], (apx_size_t) sizeof(buffer)));
   CuAssertUIntEquals(tc, data[0], buffer[0]);
   CuAssertUIntEquals(tc, data[1], buffer[1]);

   apx_client_test_connection_delete(connection);
}

static void test_client_requests_open_cout_cin_before_in_file(CuTest* tc)
{
   apx_client_test_connection_t* connection;
   adt_bytearray_t* packet;
   char const* apx_text =
      "APX/1.3\n"
      "N\"TestNode13\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "R\"RequirePort1\"C(0,7):=7\n";

   connection = apx_client_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_build_node(connection, apx_text));
   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_greeting_header_accepted_notification(connection);
   apx_client_test_connection_run(connection);
   CuAssertIntEquals(tc, 1u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_clear_log(connection);

   // Server publishes TestNode13.cout, TestNode13.cin, and TestNode13.in
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_publish_remote_file(connection, APX_PORT_COUNT_ADDRESS_START, "TestNode13.cout", 2u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_publish_remote_file(connection, APX_PORT_COUNT_ADDRESS_START + 0x400u, "TestNode13.cin", 2u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode13.in", 1u));

   CuAssertIntEquals(tc, 0u, apx_client_test_connection_log_length(connection));
   apx_client_test_connection_run(connection);

   // Verify that 3 open requests were queued in exact order: .cout, .cin, .in
   uint32_t opened_addresses[3] = {0};
   int msg_count = 0;
   for (int32_t i = 0; i < apx_client_test_connection_log_length(connection); ++i)
   {
      packet = apx_client_test_connection_get_log_packet(connection, i);
      CuAssertPtrNotNull(tc, packet);
      uint8_t const* p_data = adt_bytearray_const_data(packet);
      int p_len = (int)adt_bytearray_length(packet);
      while (p_len > 0)
      {
         uint32_t msg_len = 0;
         uint8_t const* next = numheader_decode32(p_data, p_data + p_len, &msg_len);
         CuAssertPtrNotNull(tc, next);
         p_len -= (int)(next - p_data);
         p_data = next;
         CuAssertTrue(tc, p_len >= (int)msg_len);

         uint32_t decoded_address = 0;
         bool more_bit = false;
         apx_size_t h_size = rmf_address_decode(p_data, p_data + msg_len, &decoded_address, &more_bit);
         CuAssertTrue(tc, h_size > 0);
         CuAssertUIntEquals(tc, RMF_CMD_AREA_START_ADDRESS, decoded_address);

         uint8_t const* cmd_data = p_data + h_size;
         uint32_t cmd_type = unpackLE(cmd_data, UINT32_SIZE);
         uint32_t file_address = unpackLE(cmd_data + UINT32_SIZE, UINT32_SIZE);
         CuAssertUIntEquals(tc, RMF_CMD_OPEN_FILE_MSG, cmd_type);

         if (msg_count < 3)
         {
            opened_addresses[msg_count] = file_address;
         }
         msg_count++;
         p_data += msg_len;
         p_len -= (int)msg_len;
      }
   }
   CuAssertIntEquals(tc, 3, msg_count);
   CuAssertUIntEquals(tc, APX_PORT_COUNT_ADDRESS_START, opened_addresses[0]);
   CuAssertUIntEquals(tc, APX_PORT_COUNT_ADDRESS_START + 0x400u, opened_addresses[1]);
   CuAssertUIntEquals(tc, APX_PORT_DATA_ADDRESS_START, opened_addresses[2]);

   apx_client_test_connection_delete(connection);
}

static void test_client_requests_open_cout_cin_before_in_file_when_in_published_first(CuTest* tc)
{
   apx_client_test_connection_t* connection;
   adt_bytearray_t* packet;
   char const* apx_text =
      "APX/1.3\n"
      "N\"TestNode13\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "R\"RequirePort1\"C(0,7):=7\n";

   connection = apx_client_test_connection_new();
   CuAssertPtrNotNull(tc, connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_build_node(connection, apx_text));
   apx_client_test_connection_greeting_header_accepted_notification(connection);
   apx_client_test_connection_run(connection);
   apx_client_test_connection_clear_log(connection);

   // Server has .cout and .cin published in file map, but sends publish .in first
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_publish_remote_file(connection, APX_PORT_COUNT_ADDRESS_START, "TestNode13.cout", 2u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_publish_remote_file(connection, APX_PORT_COUNT_ADDRESS_START + 0x400u, "TestNode13.cin", 2u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_test_connection_publish_remote_file(connection, APX_PORT_DATA_ADDRESS_START, "TestNode13.in", 1u));

   apx_client_test_connection_run(connection);

   // Verify all 3 are requested open, and .cout and .cin are opened before .in
   uint32_t opened_addresses[3] = {0};
   int msg_count = 0;
   for (int32_t i = 0; i < apx_client_test_connection_log_length(connection); ++i)
   {
      packet = apx_client_test_connection_get_log_packet(connection, i);
      CuAssertPtrNotNull(tc, packet);
      uint8_t const* p_data = adt_bytearray_const_data(packet);
      int p_len = (int)adt_bytearray_length(packet);
      while (p_len > 0)
      {
         uint32_t msg_len = 0;
         uint8_t const* next = numheader_decode32(p_data, p_data + p_len, &msg_len);
         CuAssertPtrNotNull(tc, next);
         p_len -= (int)(next - p_data);
         p_data = next;
         CuAssertTrue(tc, p_len >= (int)msg_len);

         uint32_t decoded_address = 0;
         bool more_bit = false;
         apx_size_t h_size = rmf_address_decode(p_data, p_data + msg_len, &decoded_address, &more_bit);
         CuAssertTrue(tc, h_size > 0);
         CuAssertUIntEquals(tc, RMF_CMD_AREA_START_ADDRESS, decoded_address);

         uint8_t const* cmd_data = p_data + h_size;
         uint32_t cmd_type = unpackLE(cmd_data, UINT32_SIZE);
         uint32_t file_address = unpackLE(cmd_data + UINT32_SIZE, UINT32_SIZE);
         CuAssertUIntEquals(tc, RMF_CMD_OPEN_FILE_MSG, cmd_type);

         if (msg_count < 3)
         {
            opened_addresses[msg_count] = file_address;
         }
         msg_count++;
         p_data += msg_len;
         p_len -= (int)msg_len;
      }
   }
   CuAssertIntEquals(tc, 3, msg_count);
   CuAssertUIntEquals(tc, APX_PORT_COUNT_ADDRESS_START, opened_addresses[0]);
   CuAssertUIntEquals(tc, APX_PORT_COUNT_ADDRESS_START + 0x400u, opened_addresses[1]);
   CuAssertUIntEquals(tc, APX_PORT_DATA_ADDRESS_START, opened_addresses[2]);

   apx_client_test_connection_delete(connection);
}