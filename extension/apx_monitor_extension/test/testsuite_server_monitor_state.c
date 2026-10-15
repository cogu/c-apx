/*****************************************************************************
 * \file      testsuite_server_monitor_state.c
 * \author    Conny Gustafsson
 * \date      2026-08-26
 * \brief     Unit tests for server monitor state
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/extension/server_monitor.h"
#include "apx/extension/socket_server_connection.h"
#include "apx/file.h"
#include "apx/file_info.h"
#include "apx/server.h"
#include "apx/server_test_connection.h"
#include "testsocket.h"
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_new_connection_event_creates_connection_observer(CuTest *tc);
static void test_connection_observer_destroyed_on_disconnect_event(CuTest *tc);
static void test_file_published_creates_observed_file(CuTest *tc);
static void test_file_lifecycle_state_transitions(CuTest *tc);
static void test_signed_file_observation(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_server_monitor_state(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_new_connection_event_creates_connection_observer);
  SUITE_ADD_TEST(suite, test_connection_observer_destroyed_on_disconnect_event);
  SUITE_ADD_TEST(suite, test_file_published_creates_observed_file);
  SUITE_ADD_TEST(suite, test_file_lifecycle_state_transitions);
  SUITE_ADD_TEST(suite, test_signed_file_observation);
  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_new_connection_event_creates_connection_observer(CuTest *tc)
{
  apx_connection_id_t const connection_id = 0u;
  const char *connection_tag = "TCP";
  testsocket_t *socket = testsocket_new();
  apx_socket_server_connection_t *socket_connection = apx_socket_server_connection_new(socket);
  apx_server_connection_t *server_connection = (apx_server_connection_t *)socket_connection;
  apx_server_monitor_t *monitor = apx_server_monitor_new(NULL);
  apx_observed_connection_t *observed_connection = NULL;
  CuAssertPtrNotNull(tc, socket_connection);
  CuAssertPtrNotNull(tc, monitor);
  CuAssertIntEquals(tc, 0, apx_server_monitor_num_connections(monitor));
  apx_server_connection_set_connection_id(server_connection, connection_id);
  apx_server_connection_set_tag(server_connection, connection_tag);
  apx_server_monitor_virtual_on_new_connection((void *)monitor, server_connection);
  CuAssertIntEquals(tc, 1, apx_server_monitor_num_connections(monitor));
  observed_connection = apx_server_monitor_get_last_observed_connection(monitor);
  CuAssertPtrNotNull(tc, observed_connection);
  CuAssertUIntEquals(tc, connection_id, apx_observed_connection_connection_id(observed_connection));
  CuAssertStrEquals(tc, connection_tag, apx_observed_connection_tag(observed_connection));
  apx_server_monitor_delete(monitor);
  apx_socket_server_connection_delete(socket_connection);
}

static void test_connection_observer_destroyed_on_disconnect_event(CuTest *tc)
{
  apx_connection_id_t const connection_id = 0u;
  testsocket_t *socket = testsocket_new();
  apx_socket_server_connection_t *socket_connection = apx_socket_server_connection_new(socket);
  apx_server_connection_t *server_connection = (apx_server_connection_t *)socket_connection;
  apx_server_monitor_t *monitor = apx_server_monitor_new(NULL);
  CuAssertPtrNotNull(tc, socket_connection);
  CuAssertPtrNotNull(tc, monitor);
  CuAssertIntEquals(tc, 0, apx_server_monitor_num_connections(monitor));
  apx_server_connection_set_connection_id(server_connection, connection_id);
  apx_server_monitor_virtual_on_new_connection((void *)monitor, server_connection);
  CuAssertIntEquals(tc, 1, apx_server_monitor_num_connections(monitor));
  apx_server_monitor_virtual_on_connection_closed((void *)monitor, server_connection);
  CuAssertIntEquals(tc, 0, apx_server_monitor_num_connections(monitor));
  apx_socket_server_connection_delete(socket_connection);
  apx_server_monitor_delete(monitor);
}

static void test_file_published_creates_observed_file(CuTest *tc)
{
  testsocket_t *socket = testsocket_new();
  apx_socket_server_connection_t *socket_connection = apx_socket_server_connection_new(socket);
  apx_server_connection_t *server_connection = (apx_server_connection_t *)socket_connection;
  apx_server_monitor_t *monitor = apx_server_monitor_new(NULL);
  rmf_file_info_t *file_info =
    rmf_file_info_new(0x1000u, 64u, "test_node.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
  apx_observed_connection_t *observed_connection = NULL;
  apx_observed_file_t *observed_file = NULL;

  CuAssertPtrNotNull(tc, socket_connection);
  CuAssertPtrNotNull(tc, monitor);
  CuAssertPtrNotNull(tc, file_info);

  apx_server_connection_set_connection_id(server_connection, 1u);
  apx_server_monitor_virtual_on_new_connection((void *)monitor, server_connection);

  observed_connection = apx_server_monitor_find_observed_connection(monitor, server_connection);
  CuAssertPtrNotNull(tc, observed_connection);
  CuAssertIntEquals(tc, 0, apx_observed_connection_num_files(observed_connection));

  apx_server_monitor_virtual_on_file_published((void *)monitor, (apx_connection_base_t *)server_connection, file_info);

  CuAssertIntEquals(tc, 1, apx_observed_connection_num_files(observed_connection));
  observed_file = apx_observed_connection_find_file_by_name(observed_connection, "test_node.out");
  CuAssertPtrNotNull(tc, observed_file);
  CuAssertStrEquals(tc, "test_node.out", apx_observed_file_get_name(observed_file));
  CuAssertUIntEquals(tc, 0x1000u, apx_observed_file_get_address(observed_file));
  CuAssertUIntEquals(tc, 64u, apx_observed_file_get_size(observed_file));
  CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_observed_file_get_data_state(observed_file));

  rmf_file_info_delete(file_info);
  apx_server_monitor_delete(monitor);
  apx_socket_server_connection_delete(socket_connection);
}

static void test_file_lifecycle_state_transitions(CuTest *tc)
{
  testsocket_t *socket = testsocket_new();
  apx_socket_server_connection_t *socket_connection = apx_socket_server_connection_new(socket);
  apx_server_connection_t *server_connection = (apx_server_connection_t *)socket_connection;
  apx_server_monitor_t *monitor = apx_server_monitor_new(NULL);
  rmf_file_info_t *file_info =
    rmf_file_info_new(0x2000u, 128u, "test_node.in", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
  apx_file_t *file = apx_file_new(file_info);
  apx_observed_connection_t *observed_connection = NULL;
  apx_observed_file_t *observed_file = NULL;
  uint8_t write_data[4] = {1, 2, 3, 4};

  CuAssertPtrNotNull(tc, socket_connection);
  CuAssertPtrNotNull(tc, monitor);
  CuAssertPtrNotNull(tc, file_info);
  CuAssertPtrNotNull(tc, file);

  apx_server_connection_set_connection_id(server_connection, 2u);
  apx_server_monitor_virtual_on_new_connection((void *)monitor, server_connection);
  observed_connection = apx_server_monitor_find_observed_connection(monitor, server_connection);
  CuAssertPtrNotNull(tc, observed_connection);

  // 1. Publish -> WAITING_FOR_FILE_OPEN_REQUEST
  apx_server_monitor_virtual_on_file_published((void *)monitor, (apx_connection_base_t *)server_connection, file_info);
  observed_file = apx_observed_connection_find_file_by_name(observed_connection, "test_node.in");
  CuAssertPtrNotNull(tc, observed_file);
  CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_observed_file_get_data_state(observed_file));

  // 2. Open -> WAITING_FOR_FILE_DATA
  apx_server_monitor_virtual_on_file_opened((void *)monitor, (apx_connection_base_t *)server_connection, file_info);
  CuAssertIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, apx_observed_file_get_data_state(observed_file));

  // 3. Write -> SYNCHRONIZED
  apx_server_monitor_virtual_on_file_written(
    (void *)monitor, (apx_connection_base_t *)server_connection, file, 0u, write_data, sizeof(write_data));
  CuAssertIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, apx_observed_file_get_data_state(observed_file));

  // 4. Find by address within range
  CuAssertPtrEquals(tc, observed_file, apx_observed_connection_find_file_by_address(observed_connection, 0x2000u));
  CuAssertPtrEquals(tc, observed_file, apx_observed_connection_find_file_by_address(observed_connection, 0x2050u));
  CuAssertPtrEquals(tc, NULL, apx_observed_connection_find_file_by_address(observed_connection, 0x2080u));

  apx_file_delete(file);
  rmf_file_info_delete(file_info);
  apx_server_monitor_delete(monitor);
  apx_socket_server_connection_delete(socket_connection);
}

static void test_signed_file_observation(CuTest *tc)
{
  testsocket_t *socket = testsocket_new();
  apx_socket_server_connection_t *socket_connection = apx_socket_server_connection_new(socket);
  apx_server_connection_t *server_connection = (apx_server_connection_t *)socket_connection;
  apx_server_monitor_t *monitor = apx_server_monitor_new(NULL);
  uint8_t dummy_sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  for (size_t i = 0; i < sizeof(dummy_sig); i++)
  {
    dummy_sig[i] = (uint8_t)(0x55 ^ (uint8_t)i);
  }

  rmf_file_info_t *file_info = rmf_file_info_make_fixed_with_signature(
    "SecurityNode.apx", 256u, 0x1000u, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_sig);
  CuAssertPtrNotNull(tc, socket_connection);
  CuAssertPtrNotNull(tc, monitor);
  CuAssertPtrNotNull(tc, file_info);

  apx_server_connection_set_connection_id(server_connection, 42u);
  apx_server_monitor_virtual_on_new_connection((void *)monitor, server_connection);

  apx_server_monitor_virtual_on_file_published((void *)monitor, (apx_connection_base_t *)server_connection, file_info);

  apx_observed_connection_t *observed_connection =
    apx_server_monitor_find_observed_connection(monitor, server_connection);
  CuAssertPtrNotNull(tc, observed_connection);

  apx_observed_file_t *observed_file =
    apx_observed_connection_find_file_by_name(observed_connection, "SecurityNode.apx");
  CuAssertPtrNotNull(tc, observed_file);
  CuAssertTrue(tc, apx_observed_file_is_signed(observed_file));
  CuAssertIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, apx_observed_file_get_signature_type(observed_file));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, apx_observed_file_get_signature_data(observed_file), sizeof(dummy_sig)));

  rmf_file_info_delete(file_info);
  apx_server_monitor_delete(monitor);
  apx_socket_server_connection_delete(socket_connection);
}