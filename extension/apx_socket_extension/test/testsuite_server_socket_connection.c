/*****************************************************************************
 * \file      testsuite_server_socket_connection.c
 * \author    Conny Gustafsson
 * \date      2019-08-04
 * \brief     Unit tests for server socket connection
 *
 * Copyright (c) 2019-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <unistd.h>
#endif
#include "CuTest.h"
#include "apx/extension/socket_server_connection.h"
#include "apx/extension/socket_server_extension.h"
#include "apx/file_manager.h"
#include "apx/numheader.h"
#include "apx/server.h"
#include "apx/server_connection.h"
#include "apx/socket_client_connection.h"
#include "osmacro.h"
#include "pack.h"
#include "testsocket_spy.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define DEFAULT_CONNECTION_ID 0
#define ERROR_MSG_SIZE 150

#define SERVER_RUN(srv, sock)                                                                                          \
  testsocket_run(sock);                                                                                                \
  apx_server_run(srv);                                                                                                 \
  SLEEP(50);                                                                                                           \
  testsocket_run(sock);                                                                                                \
  apx_server_run(srv)


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_connection_create(CuTest *tc);
static void test_socket_transmit_buffer(CuTest *tc);
static void verify_transmit_buffer(
  CuTest *tc, apx_connection_interface_t const *connection, adt_bytearray_t *send_buffer, adt_bytearray_t *wire);
static void test_each_connection_get_unique_id(CuTest *tc);
static void test_server_sends_acknowledge_after_accepting_header(CuTest *tc);
static void test_server_opens_definition_file_after_publication(CuTest *tc);
static void test_server_parses_definition_data_after_transmission(CuTest *tc);
static void send_header(testsocket_t *sock);
static void send_file_info_no_checksum(
  CuTest *tc, testsocket_t *sock, const char *name, uint32_t startAddress, uint32_t length);
static void verify_acknowledge(CuTest *tc, testsocket_t *sock);
static void verify_file_open_request(CuTest *tc, testsocket_t *sock, uint32_t address);
static void send_file_content(CuTest *tc, testsocket_t *sock, uint32_t address);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const char *m_TestNodeDefinition = "APX/1.2\n"
                                          "N\"TestNode\"\n"
                                          "T\"VehicleSpeed_T\"S\n"
                                          "R\"VehicleSpeed\"T[0]:=65535\n";


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite *testsuite_apx_socket_server_connection(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_connection_create);
  SUITE_ADD_TEST(suite, test_socket_transmit_buffer);
  SUITE_ADD_TEST(suite, test_each_connection_get_unique_id);
  SUITE_ADD_TEST(suite, test_server_sends_acknowledge_after_accepting_header);
  SUITE_ADD_TEST(suite, test_server_opens_definition_file_after_publication);
  SUITE_ADD_TEST(suite, test_server_parses_definition_data_after_transmission);
  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_connection_create(CuTest *tc)
{
  apx_socket_server_connection_t conn;
  testsocket_t *sock1;
  sock1 =
    testsocket_new(); // apx_socket_server_connection_t takes ownership of this object. No need to manually delete it
  CuAssertIntEquals(tc, 0, apx_socket_server_connection_create(&conn, sock1));
  CuAssertUIntEquals(tc, APX_INVALID_CONNECTION_ID, conn.base.base.connection_id);
  CuAssertPtrEquals(tc, sock1, conn.socket_object);
  apx_socket_server_connection_destroy(&conn);
}

static void test_socket_transmit_buffer(CuTest *tc)
{
  apx_socket_server_connection_t server;
  testsocket_t *server_socket = testsocket_new();
  CuAssertPtrNotNull(tc, server_socket);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_socket_server_connection_create(&server, server_socket));
  verify_transmit_buffer(
    tc, apx_connection_base_get_connection(&server.base.base), &server.send_buffer, &server_socket->pending_client);
  apx_socket_server_connection_destroy(&server);

  apx_client_socket_connection_t client;
  testsocket_t *client_socket = testsocket_new();
  CuAssertPtrNotNull(tc, client_socket);
  CuAssertIntEquals(
    tc, APX_NO_ERROR, apx_client_socket_connection_create(&client, client_socket, APX_CONNECTION_TYPE_MONITOR));
  verify_transmit_buffer(
    tc, apx_connection_base_get_connection(&client.base.base), &client.send_buffer, &client_socket->pending_server);
  apx_client_socket_connection_destroy(&client);
}

static void verify_transmit_buffer(
  CuTest *tc, apx_connection_interface_t const *connection, adt_bytearray_t *send_buffer, adt_bytearray_t *wire)
{
  CuAssertPtrNotNull(tc, connection);
  int32_t const max_size = connection->transmit_max_buffer_size(connection->arg);
  CuAssertIntEquals(tc, 4096, max_size);
  uint8_t const small_data[] = {0xAA, 0xBB};
  uint8_t const expected_small[] = {4, 0x10, 0x00, 0xAA, 0xBB, 2, 0xAA, 0xBB, 6, 0xC8, 0x00, 0x00, 0x00, 0xAA, 0xBB};
  int32_t available = -1;
  connection->transmit_begin(connection->arg);
  CuAssertUIntEquals(tc, 0u, adt_bytearray_length(send_buffer));
  CuAssertIntEquals(tc, max_size, connection->transmit_current_bytes_avaiable(connection->arg));
  CuAssertIntEquals(tc, APX_NO_ERROR,
    connection->transmit_data_message(connection->arg, 0x1000u, false, small_data, sizeof(small_data), &available));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, connection->transmit_direct_message(connection->arg, small_data, sizeof(small_data), &available));
  CuAssertIntEquals(tc, APX_NO_ERROR,
    connection->transmit_data_message(connection->arg, 0x08000000u, true, small_data, sizeof(small_data), &available));
  CuAssertUIntEquals(tc, sizeof(expected_small), adt_bytearray_length(send_buffer));
  CuAssertIntEquals(tc, max_size - (int32_t)sizeof(expected_small), available);
  CuAssertIntEquals(tc, 0, memcmp(expected_small, adt_bytearray_const_data(send_buffer), sizeof(expected_small)));
  CuAssertUIntEquals(tc, 0u, adt_bytearray_length(wire));
  connection->transmit_end(connection->arg);
  CuAssertUIntEquals(tc, 0u, adt_bytearray_length(send_buffer));
  CuAssertUIntEquals(tc, sizeof(expected_small), adt_bytearray_length(wire));
  CuAssertIntEquals(tc, 0, memcmp(expected_small, adt_bytearray_const_data(wire), sizeof(expected_small)));
  connection->transmit_begin(connection->arg);
  connection->transmit_end(connection->arg);
  CuAssertUIntEquals(tc, sizeof(expected_small), adt_bytearray_length(wire));

  for (uint32_t direct = 0u; direct < 2u; ++direct)
  {
    uint8_t data[64];
    uint8_t expected[150u * (NUMHEADER32_LONG_SIZE + RMF_HIGH_ADDR_SIZE + sizeof(data))];
    uint32_t expected_size = 0u;
    uint32_t buffered_size = 0u;
    uint32_t flushed_size = 0u;
    uint32_t flush_count = 0u;
    adt_bytearray_clear(wire);
    connection->transmit_begin(connection->arg);
    for (uint32_t i = 0u; i < 150u; ++i)
    {
      memset(data, (int)i, sizeof(data));
      uint8_t header[NUMHEADER32_LONG_SIZE + RMF_HIGH_ADDR_SIZE];
      uint32_t const address = 0x08000000u + i * sizeof(data);
      uint32_t const address_size = direct ? 0u : RMF_HIGH_ADDR_SIZE;
      uint32_t header_size = (uint32_t)numheader_encode32(header, sizeof(header), sizeof(data) + address_size);
      if (!direct)
      {
        header_size += rmf_address_encode(header + header_size, sizeof(header) - header_size, address, i != 149u);
      }
      uint32_t const frame_size = header_size + sizeof(data);
      if (frame_size > (uint32_t)max_size - buffered_size)
      {
        flushed_size += buffered_size;
        buffered_size = 0u;
        ++flush_count;
      }
      apx_error_t const result = direct ?
        connection->transmit_direct_message(connection->arg, data, sizeof(data), &available) :
        connection->transmit_data_message(connection->arg, address, i != 149u, data, sizeof(data), &available);
      CuAssertIntEquals(tc, APX_NO_ERROR, result);
      buffered_size += frame_size;
      CuAssertUIntEquals(tc, buffered_size, adt_bytearray_length(send_buffer));
      CuAssertUIntEquals(tc, flushed_size, adt_bytearray_length(wire));
      CuAssertUIntEquals(tc, (uint32_t)max_size, send_buffer->u32AllocLen);
      CuAssertIntEquals(tc, max_size - (int32_t)buffered_size, available);
      CuAssertIntEquals(tc, available, connection->transmit_current_bytes_avaiable(connection->arg));
      memcpy(expected + expected_size, header, header_size);
      expected_size += header_size;
      memcpy(expected + expected_size, data, sizeof(data));
      expected_size += sizeof(data);
    }
    CuAssertTrue(tc, flush_count >= 2u);
    connection->transmit_end(connection->arg);
    CuAssertUIntEquals(tc, 0u, adt_bytearray_length(send_buffer));
    CuAssertUIntEquals(tc, expected_size, adt_bytearray_length(wire));
    CuAssertIntEquals(tc, 0, memcmp(expected, adt_bytearray_const_data(wire), expected_size));
  }

  for (uint32_t kind = 0u; kind < 3u; ++kind)
  {
    uint8_t data[4096];
    memset(data, 0x5A, sizeof(data));
    bool const direct = kind == 2u;
    uint32_t const address = kind == 0u ? 0x1000u : 0x08000000u;
    uint32_t const address_size = direct ? 0u : rmf_needed_encoding_size(address);
    int32_t const data_size = max_size - (int32_t)(NUMHEADER32_LONG_SIZE + address_size);
    adt_bytearray_clear(wire);
    connection->transmit_begin(connection->arg);
    apx_error_t result = direct ?
      connection->transmit_direct_message(connection->arg, data, data_size, &available) :
      connection->transmit_data_message(connection->arg, address, false, data, data_size, &available);
    CuAssertIntEquals(tc, APX_NO_ERROR, result);
    CuAssertIntEquals(tc, 0, available);
    CuAssertUIntEquals(tc, (uint32_t)max_size, adt_bytearray_length(send_buffer));
    CuAssertUIntEquals(tc, 0u, adt_bytearray_length(wire));

    available = -17;
    result = direct ?
      connection->transmit_direct_message(connection->arg, data, data_size + 1, &available) :
      connection->transmit_data_message(connection->arg, address, false, data, data_size + 1, &available);
    CuAssertIntEquals(tc, APX_MSG_TOO_LARGE_ERROR, result);
    CuAssertIntEquals(
      tc, APX_MSG_TOO_LARGE_ERROR, connection->transmit_direct_message(connection->arg, data, max_size, &available));
    CuAssertIntEquals(
      tc, APX_INVALID_ARGUMENT_ERROR, connection->transmit_direct_message(connection->arg, NULL, 1, &available));
    CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR,
      connection->transmit_data_message(connection->arg, address, false, data, -1, &available));
    CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR,
      connection->transmit_data_message(connection->arg, UINT32_MAX, false, data, 1, &available));
    CuAssertIntEquals(
      tc, APX_INVALID_ARGUMENT_ERROR, connection->transmit_direct_message(connection->arg, data, 1, NULL));
    CuAssertIntEquals(tc, -17, available);
    CuAssertUIntEquals(tc, (uint32_t)max_size, adt_bytearray_length(send_buffer));
    CuAssertUIntEquals(tc, 0u, adt_bytearray_length(wire));

    CuAssertIntEquals(tc, APX_NO_ERROR, connection->transmit_direct_message(connection->arg, NULL, 0, &available));
    CuAssertUIntEquals(tc, (uint32_t)max_size, adt_bytearray_length(wire));
    CuAssertUIntEquals(tc, 1u, adt_bytearray_length(send_buffer));
    CuAssertIntEquals(tc, max_size - 1, available);
    connection->transmit_end(connection->arg);
    CuAssertUIntEquals(tc, (uint32_t)max_size + 1u, adt_bytearray_length(wire));
    uint8_t header[NUMHEADER32_LONG_SIZE + RMF_HIGH_ADDR_SIZE];
    uint32_t header_size = (uint32_t)numheader_encode32(header, sizeof(header), (uint32_t)data_size + address_size);
    if (!direct)
    {
      header_size += rmf_address_encode(header + header_size, sizeof(header) - header_size, address, false);
    }
    uint8_t const *actual = adt_bytearray_const_data(wire);
    CuAssertIntEquals(tc, 0, memcmp(header, actual, header_size));
    CuAssertIntEquals(tc, 0, memcmp(data, actual + header_size, (size_t)data_size));
    CuAssertIntEquals(tc, 0, actual[max_size]);
  }

  adt_bytearray_clear(wire);
  uint8_t const expected_empty[] = {0, 2, 0x10, 0x00};
  connection->transmit_begin(connection->arg);
  CuAssertIntEquals(tc, APX_NO_ERROR, connection->transmit_direct_message(connection->arg, NULL, 0, &available));
  CuAssertIntEquals(
    tc, APX_NO_ERROR, connection->transmit_data_message(connection->arg, 0x1000u, false, NULL, 0, &available));
  connection->transmit_end(connection->arg);
  CuAssertUIntEquals(tc, sizeof(expected_empty), adt_bytearray_length(wire));
  CuAssertIntEquals(tc, 0, memcmp(expected_empty, adt_bytearray_const_data(wire), sizeof(expected_empty)));
}

static void test_each_connection_get_unique_id(CuTest *tc)
{
  apx_server_t server;
  testsocket_t *sockets[11];
  apx_server_connection_t *lastConnection;
  uint32_t connectionIdExpected = 0;
  int i;
  apx_server_create(&server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_socket_server_extension_register(&server, NULL));
  apx_server_start(&server);

  for (i = 0; i < 10; i++)
  {
    char msg[15];
    sockets[i] = testsocket_new();
    apx_socket_server_extension_accept_testsocket(sockets[i]);
    lastConnection = apx_server_get_last_connection(&server);
    CuAssertPtrNotNull(tc, lastConnection);
    sprintf(msg, "i=%d", i);
    CuAssertUIntEquals_Msg(
      tc, &msg[0], connectionIdExpected++, apx_server_connection_get_connection_id(lastConnection));
  }
   // resetting internal variable nextConnectionId to 0 shall still yield next generated ID to be unique
  server.connection_manager.next_connection_id = 0;
  sockets[10] = testsocket_new();
  apx_socket_server_extension_accept_testsocket(sockets[10]);
  lastConnection = apx_server_get_last_connection(&server);
  CuAssertPtrNotNull(tc, lastConnection);
  CuAssertUIntEquals(tc, 10, apx_server_connection_get_connection_id(lastConnection));
  apx_server_destroy(&server);
}

static void test_server_sends_acknowledge_after_accepting_header(CuTest *tc)
{
  apx_server_t server;
  testsocket_t *sock;
  testsocket_spy_create();
  sock = testsocket_spy_client();
  apx_server_create(&server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_socket_server_extension_register(&server, NULL));
  apx_server_start(&server);
  apx_socket_server_extension_accept_testsocket(sock);
  testsocket_on_connect(sock);
  send_header(sock);
  SERVER_RUN(&server, sock);
  verify_acknowledge(tc, sock);
  apx_server_destroy(&server);
  testsocket_spy_destroy();
}

static void test_server_opens_definition_file_after_publication(CuTest *tc)
{
  apx_server_t server;
  testsocket_t *sock;
  testsocket_spy_create();
  sock = testsocket_spy_client();
  apx_server_create(&server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_socket_server_extension_register(&server, NULL));
  apx_server_start(&server);
  apx_socket_server_extension_accept_testsocket(sock);
  testsocket_on_connect(sock);
  send_header(sock);
  SERVER_RUN(&server, sock);
  verify_acknowledge(tc, sock);
  send_file_info_no_checksum(
    tc, sock, "TestNode.apx", APX_DEFINITION_ADDRESS_START, (uint32_t)strlen(m_TestNodeDefinition));
  SERVER_RUN(&server, sock);
  verify_file_open_request(tc, sock, APX_DEFINITION_ADDRESS_START);
  apx_server_destroy(&server);
  testsocket_spy_destroy();
}

static void test_server_parses_definition_data_after_transmission(CuTest *tc)
{
  apx_server_t server;
  testsocket_t *sock;
  unsigned int const definition_size = (unsigned int)strlen(m_TestNodeDefinition);
  uint32_t definitionAddress = APX_DEFINITION_ADDRESS_START;
  uint32_t dummy;
  testsocket_spy_create();
  sock = testsocket_spy_client();
  apx_server_create(&server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_socket_server_extension_register(&server, NULL));
  apx_server_start(&server);
  apx_socket_server_extension_accept_testsocket(sock);
  testsocket_on_connect(sock);
  send_header(sock);
  SERVER_RUN(&server, sock);
  testsocket_spy_clear_received_data();
  send_file_info_no_checksum(tc, sock, "TestNode.apx", definitionAddress, (uint32_t)definition_size);
  SERVER_RUN(&server, sock);
  verify_file_open_request(tc, sock, definitionAddress);
  CuAssertPtrEquals(tc, NULL, (void *)testsocket_spy_get_received_data(&dummy));
  apx_server_connection_t *connection = apx_server_get_last_connection(&server);
  CuAssertPtrNotNull(tc, connection);
  apx_node_instance_t *node_instance = apx_node_manager_find(connection->base.node_manager, "TestNode");
  CuAssertPtrNotNull(tc, node_instance);
  send_file_content(tc, sock, definitionAddress);
  SERVER_RUN(&server, sock);
  apx_node_data_t const *node_data = apx_node_instance_get_const_node_data(node_instance);
  CuAssertPtrNotNull(tc, node_data);
  CuAssertUIntEquals(tc, definition_size, apx_node_data_definition_data_size(node_data));
  CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_node_instance_get_definition_data_state(node_instance));

  apx_server_destroy(&server);
  testsocket_spy_destroy();
}

static void send_header(testsocket_t *sock)
{
  const char *greeting = "RMFP/1.1\nMessage-Size:32\n\n";
  int32_t msgLen;
  uint8_t msg[RMF_GREETING_MAX_LEN];
  msgLen = (int32_t)strlen(greeting);
  msg[0] = (uint8_t)msgLen;
  memcpy(&msg[1], greeting, msgLen);
  testsocket_client_send(sock, &msg[0], 1 + msgLen);
}

static void send_file_info_no_checksum(
  CuTest *tc, testsocket_t *sock, const char *name, uint32_t startAddress, uint32_t length)
{
  apx_size_t msgLen = 0;
  uint8_t buf[RMF_CMD_AREA_SIZE];
  rmf_file_info_t *file_info = rmf_file_info_make_fixed(name, length, startAddress);
  CuAssertPtrNotNull(tc, file_info);

  msgLen += rmf_address_encode(&buf[1 + msgLen], (apx_size_t)(sizeof(buf) - msgLen), RMF_CMD_AREA_START_ADDRESS, false);
  msgLen += rmf_encode_publish_file_cmd(&buf[1 + msgLen], (apx_size_t)(sizeof(buf) - msgLen), file_info);
  rmf_file_info_delete(file_info);
  CuAssertUIntEquals(tc, 65, msgLen);
  buf[0] = (uint8_t)msgLen;
  testsocket_client_send(sock, &buf[0], (uint32_t)(1 + msgLen));
}

static void verify_acknowledge(CuTest *tc, testsocket_t *sock)
{
  uint32_t len;
  int32_t i;
  const uint8_t expected[13] = {12, 0xBF, 0xFF, 0xFC, 0x00, RMF_CMD_ACCEPT_HEADER, 0, 0, 0, 0, 0, 0, 0};
  const uint8_t *data;
  (void)sock;
  data = testsocket_spy_get_received_data(&len);
  CuAssertUIntEquals(tc, sizeof(expected), len);
  for (i = 0; i < (int32_t)sizeof(expected); i++)
  {
    char msg[14];
    sprintf(msg, "i=%d", i);
    CuAssertIntEquals_Msg(tc, msg, expected[i], data[i]);
  }
  testsocket_spy_clear_received_data();
}

static void verify_file_open_request(CuTest *tc, testsocket_t *sock, uint32_t address)
{
  uint32_t len;
  int32_t i;
  uint8_t expected[12 + 1] = {12, 0xBF, 0xFF, 0xFC, 0x00, RMF_CMD_OPEN_FILE_MSG, 0, 0, 0, 0, 0, 0, 0};
  const uint8_t *data;
  (void)sock;
  packLE(&expected[9], address, 4);
  data = testsocket_spy_get_received_data(&len);
  CuAssertUIntEquals(tc, RMF_HIGH_ADDR_SIZE + RMF_CMD_TYPE_SIZE + UINT32_SIZE + 1, len);
  for (i = 0; i < (int32_t)sizeof(expected); i++)
  {
    char msg[ERROR_MSG_SIZE];
    sprintf(msg, "i=%d", i);
    CuAssertIntEquals_Msg(tc, msg, expected[i], data[i]);
  }
  testsocket_spy_clear_received_data();
}

static void send_file_content(CuTest *tc, testsocket_t *sock, uint32_t address)
{
  uint8_t bufData[200];
  apx_size_t msgLen = 0;
  uint32_t dataLen = (uint32_t)strlen(m_TestNodeDefinition);
  uint32_t bufLen = (uint32_t)sizeof(bufData);
  (void)tc;
  msgLen += rmf_address_encode(&bufData[1 + msgLen], bufLen, address, false);
  memcpy(&bufData[1 + msgLen], &m_TestNodeDefinition[0], dataLen);
  msgLen += dataLen;
  bufData[1 + msgLen] = '\0';
  assert((uint32_t)msgLen <= NUMHEADER32_MAX_NUM_SHORT);
  bufData[0] = (uint8_t)msgLen;
  testsocket_client_send(sock, &bufData[0], (uint32_t)(1 + msgLen));
}
