/*****************************************************************************
 * \file      testsuite_monitor_extension.c
 * \author    Conny Gustafsson
 * \date      2026-08-26
 * \brief     Unit tests for monitor extension
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/connection_base.h"
#include "apx/extension/diag_dispatcher.h"
#include "apx/extension/log_streamer.h"
#include "apx/extension/monitor_extension.h"
#include "apx/extension/monitor_session.h"
#include "apx/extension/server_monitor.h"
#include "apx/file.h"
#include "apx/file_manager.h"
#include "apx/node_manager.h"
#include "apx/server.h"
#include "apx/server_test_connection.h"
#include "dtl_type.h"
#include "testsocket.h"
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_server_monitor_create_and_destroy(CuTest *tc);
static void test_server_monitor_node_ids_are_connection_scoped(CuTest *tc);
static void test_server_monitor_session_management(CuTest *tc);
static void test_server_monitor_configuration(CuTest *tc);
static void test_server_monitor_accept_testsocket(CuTest *tc);
static void test_monitor_session_publish_stream_files(CuTest *tc);
static void test_monitor_session_file_open_activates_streamer(CuTest *tc);
static void test_monitor_extension_register_lifecycle(CuTest *tc);
static void test_log_streamer_start_and_stop(CuTest *tc);
static void test_log_streamer_ringbuffer_overflow_and_drop_tracking(CuTest *tc);
static void test_server_monitor_fast_path_tap(CuTest *tc);
static void test_diag_dispatcher_get_server_info(CuTest *tc);
static void test_diag_dispatcher_list_commands(CuTest *tc);
static void test_diag_dispatcher_get_node_definition(CuTest *tc);
static void test_diag_dispatcher_subscribe_and_notify(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_monitor_extension(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_server_monitor_create_and_destroy);
  SUITE_ADD_TEST(suite, test_server_monitor_node_ids_are_connection_scoped);
  SUITE_ADD_TEST(suite, test_server_monitor_session_management);
  SUITE_ADD_TEST(suite, test_server_monitor_configuration);
  SUITE_ADD_TEST(suite, test_server_monitor_accept_testsocket);
  SUITE_ADD_TEST(suite, test_monitor_session_publish_stream_files);
  SUITE_ADD_TEST(suite, test_monitor_session_file_open_activates_streamer);
  SUITE_ADD_TEST(suite, test_monitor_extension_register_lifecycle);
  SUITE_ADD_TEST(suite, test_log_streamer_start_and_stop);
  SUITE_ADD_TEST(suite, test_log_streamer_ringbuffer_overflow_and_drop_tracking);
  SUITE_ADD_TEST(suite, test_server_monitor_fast_path_tap);
  SUITE_ADD_TEST(suite, test_diag_dispatcher_get_server_info);
  SUITE_ADD_TEST(suite, test_diag_dispatcher_list_commands);
  SUITE_ADD_TEST(suite, test_diag_dispatcher_get_node_definition);
  SUITE_ADD_TEST(suite, test_diag_dispatcher_subscribe_and_notify);
  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_server_monitor_create_and_destroy(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);
  CuAssertIntEquals(tc, 0, apx_server_monitor_num_sessions(monitor));

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_server_monitor_node_ids_are_connection_scoped(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);
  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  uint32_t const first = apx_server_monitor_get_or_create_node_id(monitor, 0u, "EngineController");
  uint32_t const second = apx_server_monitor_get_or_create_node_id(monitor, 1u, "EngineController");
  uint32_t const third = apx_server_monitor_get_or_create_node_id(monitor, 0u, "Dashboard");
  uint32_t const fourth = apx_server_monitor_get_or_create_node_id(monitor, UINT32_MAX - 1u, "EngineController");
  CuAssertTrue(tc, first != 0u && second != 0u && third != 0u && fourth != 0u);
  CuAssertTrue(tc, first != second && first != third && first != fourth);
  CuAssertTrue(tc, second != third && second != fourth && third != fourth);
  CuAssertUIntEquals(tc, first, apx_server_monitor_get_or_create_node_id(monitor, 0u, "EngineController"));
  CuAssertUIntEquals(tc, second, apx_server_monitor_get_node_id(monitor, 1u, "EngineController"));
  CuAssertUIntEquals(tc, third, apx_server_monitor_get_node_id(monitor, 0u, "Dashboard"));
  CuAssertUIntEquals(tc, fourth, apx_server_monitor_get_node_id(monitor, UINT32_MAX - 1u, "EngineController"));
  CuAssertUIntEquals(tc, 0u, apx_server_monitor_get_node_id(monitor, 2u, "EngineController"));
  CuAssertUIntEquals(tc, 0u, apx_server_monitor_get_node_id(monitor, 1u, "Dashboard"));

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_server_monitor_session_management(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  apx_server_test_connection_t *conn = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn);

  apx_monitor_session_t *session = apx_server_monitor_create_session(monitor, (apx_server_connection_t *)conn);
  CuAssertPtrNotNull(tc, session);
  CuAssertIntEquals(tc, 1, apx_server_monitor_num_sessions(monitor));

  apx_monitor_session_t *found = apx_server_monitor_find_session(monitor, (apx_server_connection_t *)conn);
  CuAssertPtrEquals(tc, session, found);
  CuAssertPtrNotNull(tc, apx_monitor_session_get_log_streamer(session));
  CuAssertPtrNotNull(tc, apx_monitor_session_get_diag_dispatcher(session));

  apx_server_monitor_remove_session(monitor, session);
  CuAssertIntEquals(tc, 0, apx_server_monitor_num_sessions(monitor));

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_server_monitor_configuration(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  dtl_hv_t *config = dtl_hv_new();
  dtl_hv_set_cstr(config, "enabled", (dtl_dv_t *)dtl_sv_make_bool(true), false);
  dtl_hv_set_cstr(config, "tcp-port", (dtl_dv_t *)dtl_sv_make_u32(5080), false);
  dtl_hv_set_cstr(config, "bind-address", (dtl_dv_t *)dtl_sv_make_cstr("127.0.0.1"), false);
  dtl_hv_set_cstr(config, "socket-file", (dtl_dv_t *)dtl_sv_make_cstr("/tmp/apx_monitor.socket"), false);

  dtl_hv_t *tls_hv = dtl_hv_new();
  dtl_hv_set_cstr(tls_hv, "server-cert", (dtl_dv_t *)dtl_sv_make_cstr("cert.pem"), false);
  dtl_hv_set_cstr(tls_hv, "server-key", (dtl_dv_t *)dtl_sv_make_cstr("key.pem"), false);
  dtl_hv_set_cstr(tls_hv, "ca-cert", (dtl_dv_t *)dtl_sv_make_cstr("ca.pem"), false);
  dtl_hv_set_cstr(tls_hv, "require-client-cert", (dtl_dv_t *)dtl_sv_make_bool(true), false);
  dtl_hv_set_cstr(config, "tls", (dtl_dv_t *)tls_hv, false);

  apx_error_t const err = apx_server_monitor_configure(monitor, config);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  CuAssertTrue(tc, monitor->is_enabled);
  CuAssertUIntEquals(tc, 5080, monitor->tcp_port);
  CuAssertStrEquals(tc, "127.0.0.1", monitor->bind_address);
  CuAssertStrEquals(tc, "/tmp/apx_monitor.socket", monitor->socket_file);
  CuAssertTrue(tc, monitor->tls_enabled);

  dtl_dec_ref((dtl_dv_t *)config);
  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_server_monitor_accept_testsocket(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_monitor_extension_register(server, NULL));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_start(server));

  testsocket_t *sock = testsocket_new();
  CuAssertPtrNotNull(tc, sock);

  apx_monitor_extension_accept_testsocket(sock);

  apx_server_connection_t *conn = apx_server_get_last_connection(server);
  CuAssertPtrNotNull(tc, conn);
  CuAssertIntEquals(tc, APX_CONNECTION_TYPE_MONITOR, apx_server_connection_get_connection_type(conn));

  apx_file_manager_t *file_manager = apx_connection_base_get_file_manager(&conn->base);
  CuAssertPtrNotNull(tc, file_manager);

  apx_file_t *log_file = apx_file_manager_find_local_file_by_name(file_manager, APX_SERVER_LOG_FILE_NAME);
  CuAssertPtrNotNull(tc, log_file);
  CuAssertTrue(tc, apx_file_is_stream(log_file));

  apx_file_t *diag_resp = apx_file_manager_find_local_file_by_name(file_manager, APX_DIAG_RESPONSE_FILE_NAME);
  CuAssertPtrNotNull(tc, diag_resp);
  CuAssertTrue(tc, apx_file_is_stream(diag_resp));

  apx_file_t *diag_notif = apx_file_manager_find_local_file_by_name(file_manager, APX_DIAG_NOTIFY_FILE_NAME);
  CuAssertPtrNotNull(tc, diag_notif);
  CuAssertTrue(tc, apx_file_is_stream(diag_notif));

  testsocket_on_disconnect(sock);

  apx_server_stop(server);
  apx_server_delete(server);
}

static void test_monitor_session_publish_stream_files(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  apx_server_test_connection_t *conn = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn);

  apx_monitor_session_t *session = apx_server_monitor_create_session(monitor, (apx_server_connection_t *)conn);
  CuAssertPtrNotNull(tc, session);

  apx_file_manager_t *file_manager = apx_connection_base_get_file_manager(&conn->base.base);
  CuAssertPtrNotNull(tc, file_manager);

  apx_file_t *log_file = apx_file_manager_find_local_file_by_name(file_manager, APX_SERVER_LOG_FILE_NAME);
  CuAssertPtrNotNull(tc, log_file);
  CuAssertTrue(tc, apx_file_is_stream(log_file));

  apx_file_t *diag_resp = apx_file_manager_find_local_file_by_name(file_manager, APX_DIAG_RESPONSE_FILE_NAME);
  CuAssertPtrNotNull(tc, diag_resp);
  CuAssertTrue(tc, apx_file_is_stream(diag_resp));

  apx_file_t *diag_notif = apx_file_manager_find_local_file_by_name(file_manager, APX_DIAG_NOTIFY_FILE_NAME);
  CuAssertPtrNotNull(tc, diag_notif);
  CuAssertTrue(tc, apx_file_is_stream(diag_notif));

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_monitor_session_file_open_activates_streamer(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  apx_server_test_connection_t *conn = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn);

  apx_monitor_session_t *session = apx_server_monitor_create_session(monitor, (apx_server_connection_t *)conn);
  CuAssertPtrNotNull(tc, session);

  apx_log_streamer_t *streamer = apx_monitor_session_get_log_streamer(session);
  CuAssertPtrNotNull(tc, streamer);
  CuAssertTrue(tc, !apx_log_streamer_is_active(streamer));

  apx_file_manager_t *file_manager = apx_connection_base_get_file_manager(&conn->base.base);
  apx_file_t *log_file = apx_file_manager_find_local_file_by_name(file_manager, APX_SERVER_LOG_FILE_NAME);
  CuAssertPtrNotNull(tc, log_file);

  apx_monitor_session_on_file_opened(session, log_file);
  CuAssertTrue(tc, apx_log_streamer_is_active(streamer));

  apx_monitor_session_on_file_closed(session, log_file);
  CuAssertTrue(tc, !apx_log_streamer_is_active(streamer));

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_monitor_extension_register_lifecycle(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_monitor_extension_register(server, NULL));
  apx_error_t const start_err = apx_server_start(server);
  CuAssertIntEquals(tc, APX_NO_ERROR, start_err);

  apx_server_stop(server);
  apx_server_delete(server);
}

static void test_log_streamer_start_and_stop(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  apx_server_test_connection_t *conn = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn);
  apx_server_connection_set_connection_type((apx_server_connection_t *)conn, APX_CONNECTION_TYPE_MONITOR);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn);

  apx_monitor_session_t *session = apx_server_monitor_create_session(monitor, (apx_server_connection_t *)conn);
  CuAssertPtrNotNull(tc, session);

  apx_log_streamer_t *streamer = apx_monitor_session_get_log_streamer(session);
  CuAssertPtrNotNull(tc, streamer);
  CuAssertTrue(tc, !apx_log_streamer_is_active(streamer));
  CuAssertIntEquals(tc, 0, monitor->num_active_log_streams);

  // Start streamer
  apx_file_manager_t *file_manager = apx_connection_base_get_file_manager(&conn->base.base);
  apx_file_t *log_file = apx_file_manager_find_local_file_by_name(file_manager, APX_SERVER_LOG_FILE_NAME);
  CuAssertPtrNotNull(tc, log_file);

  apx_file_open_stream(log_file, 4096u, 64u, 0u);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_streamer_start(streamer));
  CuAssertTrue(tc, apx_log_streamer_is_active(streamer));
  CuAssertIntEquals(tc, 1, monitor->num_active_log_streams);
  CuAssertUIntEquals(tc, 1, apx_log_streamer_get_sequence(streamer)); // STREAM_BEGIN (0) and SNAPSHOT_END (1)

  // Verify stream_id is non-zero
  bool has_non_zero_id = false;
  for (int i = 0; i < 16; ++i)
  {
    if (streamer->stream_id[i] != 0u)
    {
      has_non_zero_id = true;
      break;
    }
  }
  CuAssertTrue(tc, has_non_zero_id);

  // Stop streamer
  apx_log_streamer_stop(streamer);
  CuAssertTrue(tc, !apx_log_streamer_is_active(streamer));
  CuAssertIntEquals(tc, 0, monitor->num_active_log_streams);
  CuAssertUIntEquals(tc, 2, apx_log_streamer_get_sequence(streamer)); // STREAM_END (2)

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_log_streamer_ringbuffer_overflow_and_drop_tracking(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  apx_server_test_connection_t *conn = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn);

  apx_monitor_session_t *session = apx_server_monitor_create_session(monitor, (apx_server_connection_t *)conn);
  CuAssertPtrNotNull(tc, session);

  apx_log_streamer_t *streamer = apx_monitor_session_get_log_streamer(session);
  CuAssertPtrNotNull(tc, streamer);

  // Set tiny queue capacity of 3
  apx_log_streamer_set_queue_capacity(streamer, 3u);
  streamer->is_active = true;
  streamer->sequence = 0ULL;
  streamer->drop_count = 0ULL;

  // Insert 3 events while server.log is closed (so queue will not drain)
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_streamer_enqueue_node_detach(streamer, 1u, 10u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_streamer_enqueue_node_detach(streamer, 1u, 11u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_streamer_enqueue_node_detach(streamer, 1u, 12u));

  CuAssertUIntEquals(tc, 3, apx_log_streamer_get_queue_count(streamer));
  CuAssertUIntEquals(tc, 0, apx_log_streamer_get_drop_count(streamer));
  CuAssertUIntEquals(tc, 3, apx_log_streamer_get_sequence(streamer));

  // 4th and 5th events overflow and get dropped!
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_streamer_enqueue_node_detach(streamer, 1u, 13u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_streamer_enqueue_node_detach(streamer, 1u, 14u));

  CuAssertUIntEquals(tc, 3, apx_log_streamer_get_queue_count(streamer));
  CuAssertUIntEquals(tc, 2, apx_log_streamer_get_drop_count(streamer));
  CuAssertUIntEquals(tc, 5, apx_log_streamer_get_sequence(streamer));
  CuAssertUIntEquals(tc, 4, streamer->drop_first_sequence);

  // Free queue manually to simulate draining
  MUTEX_LOCK(streamer->lock);
  while (streamer->queue_count > 1)
  {
    uint32_t idx = streamer->read_idx;
    if (streamer->queue[idx].payload != NULL)
    {
      free(streamer->queue[idx].payload);
      streamer->queue[idx].payload = NULL;
    }
    streamer->read_idx = (streamer->read_idx + 1) % streamer->queue_capacity;
    streamer->queue_count--;
  }
  MUTEX_UNLOCK(streamer->lock);

  // Next event enqueued must trigger EVENTS_DROPPED first!
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_streamer_enqueue_node_detach(streamer, 1u, 15u));

  // Sequence should advance for EVENTS_DROPPED (seq 6) and the new event (seq 7)
  CuAssertUIntEquals(tc, 7, apx_log_streamer_get_sequence(streamer));
  CuAssertUIntEquals(tc, 0, apx_log_streamer_get_drop_count(streamer));

  streamer->is_active = false;
  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_server_monitor_fast_path_tap(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);
  CuAssertIntEquals(tc, 0, monitor->num_active_log_streams);

  // Connect operational client
  apx_server_test_connection_t *conn_op = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn_op);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn_op);
  CuAssertIntEquals(tc, 0, monitor->num_active_log_streams);

  // Connect monitor client
  apx_server_test_connection_t *conn_mon = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn_mon);
  apx_server_connection_set_connection_type((apx_server_connection_t *)conn_mon, APX_CONNECTION_TYPE_MONITOR);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn_mon);

  CuAssertIntEquals(tc, 1, apx_server_monitor_num_sessions(monitor));
  CuAssertIntEquals(tc, 0, monitor->num_active_log_streams);

  apx_monitor_session_t *session = apx_server_monitor_find_session(monitor, (apx_server_connection_t *)conn_mon);
  CuAssertPtrNotNull(tc, session);
  apx_log_streamer_t *streamer = apx_monitor_session_get_log_streamer(session);
  CuAssertPtrNotNull(tc, streamer);
  CuAssertTrue(tc, !apx_log_streamer_is_active(streamer));

  // Open server.log -> activates streamer
  apx_file_manager_t *fm_mon = apx_connection_base_get_file_manager(&conn_mon->base.base);
  apx_file_t *log_file = apx_file_manager_find_local_file_by_name(fm_mon, APX_SERVER_LOG_FILE_NAME);
  CuAssertPtrNotNull(tc, log_file);
  apx_file_open_stream(log_file, 4096u, 64u, 0u);

  apx_monitor_session_on_file_opened(session, log_file);
  CuAssertTrue(tc, apx_log_streamer_is_active(streamer));
  CuAssertIntEquals(tc, 1, monitor->num_active_log_streams);

  // Fast-path tap live event capture: publish a file on the operational node
  rmf_file_info_t test_file;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    rmf_file_info_create(&test_file, 0x1000u, 16u, "test.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
  apx_server_monitor_on_file_published(monitor, &conn_op->base.base, &test_file);
  rmf_file_info_destroy(&test_file);

  // Sequence must have advanced due to live event capture
  CuAssertTrue(tc, apx_log_streamer_get_sequence(streamer) > 1);

  // Close server.log -> deactivates streamer
  apx_monitor_session_on_file_closed(session, log_file);
  CuAssertTrue(tc, !apx_log_streamer_is_active(streamer));
  CuAssertIntEquals(tc, 0, monitor->num_active_log_streams);

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_diag_dispatcher_get_server_info(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_start(server));

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  apx_server_test_connection_t *conn = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn);
  apx_server_connection_set_connection_type((apx_server_connection_t *)conn, APX_CONNECTION_TYPE_MONITOR);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn);

  apx_monitor_session_t *session = apx_server_monitor_find_session(monitor, (apx_server_connection_t *)conn);
  CuAssertPtrNotNull(tc, session);

  apx_diag_dispatcher_t *dispatcher = apx_monitor_session_get_diag_dispatcher(session);
  CuAssertPtrNotNull(tc, dispatcher);

  // Open apx.diag.v1.response
  apx_file_manager_t *fm = apx_connection_base_get_file_manager(&conn->base.base);
  apx_file_t *resp_file = apx_file_manager_find_local_file_by_name(fm, APX_DIAG_RESPONSE_FILE_NAME);
  CuAssertPtrNotNull(tc, resp_file);
  apx_file_open_stream(resp_file, 4096u, 64u, 0u);
  apx_monitor_session_on_file_opened(session, resp_file);

  // Send GET_SERVER_INFO request
  uint8_t req[8];
  // request_id = 42, opcode = 0x0001, flags = 0
  req[0] = 42;
  req[1] = 0;
  req[2] = 0;
  req[3] = 0;
  req[4] = 1;
  req[5] = 0;
  req[6] = 0;
  req[7] = 0;

  apx_diag_dispatcher_on_request_written(dispatcher, 0u, req, sizeof(req));

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_diag_dispatcher_list_commands(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  // Operational client node
  apx_server_test_connection_t *conn_op = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn_op);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn_op);

  // Attach a node definition with 1 provide port and 1 require port
  const char *node_def = "APX/1.3\n"
                         "N\"TestNode\"\n"
                         "P\"WheelSpeed\"S:=0\n"
                         "R\"VehicleSpeed\"S:=0\n";
  apx_node_manager_t *node_mgr = apx_server_connection_get_node_manager((apx_server_connection_t *)conn_op);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_mgr, node_def));

  // Monitor client
  apx_server_test_connection_t *conn_mon = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn_mon);
  apx_server_connection_set_connection_type((apx_server_connection_t *)conn_mon, APX_CONNECTION_TYPE_MONITOR);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn_mon);

  apx_monitor_session_t *session = apx_server_monitor_find_session(monitor, (apx_server_connection_t *)conn_mon);
  CuAssertPtrNotNull(tc, session);

  apx_diag_dispatcher_t *dispatcher = apx_monitor_session_get_diag_dispatcher(session);
  CuAssertPtrNotNull(tc, dispatcher);

  apx_file_manager_t *fm = apx_connection_base_get_file_manager(&conn_mon->base.base);
  apx_file_t *resp_file = apx_file_manager_find_local_file_by_name(fm, APX_DIAG_RESPONSE_FILE_NAME);
  CuAssertPtrNotNull(tc, resp_file);
  apx_file_open_stream(resp_file, 4096u, 64u, 0u);
  apx_monitor_session_on_file_opened(session, resp_file);

  // 1. LIST_CONNECTIONS: snapshot_id = 0, cursor = 0, max_items = 10
  uint8_t req_conn[24];
  memset(req_conn, 0, sizeof(req_conn));
  req_conn[0] = 1; // request_id = 1
  req_conn[4] = 2; // opcode = 0x0002 (LIST_CONNECTIONS)
  req_conn[20] = 10; // max_items = 10
  apx_diag_dispatcher_on_request_written(dispatcher, 0u, req_conn, sizeof(req_conn));
  CuAssertTrue(tc, apx_diag_dispatcher_num_snapshots(dispatcher) >= 1);

  // 2. LIST_NODES: snapshot_id = 0, cursor = 0, max_items = 10
  uint8_t req_nodes[24];
  memset(req_nodes, 0, sizeof(req_nodes));
  req_nodes[0] = 2; // request_id = 2
  req_nodes[4] = 3; // opcode = 0x0003 (LIST_NODES)
  req_nodes[20] = 10;
  apx_diag_dispatcher_on_request_written(dispatcher, 0u, req_nodes, sizeof(req_nodes));

  // 3. LIST_ROUTES
  uint8_t req_routes[24];
  memset(req_routes, 0, sizeof(req_routes));
  req_routes[0] = 3;
  req_routes[4] = 4; // opcode = 0x0004 (LIST_ROUTES)
  req_routes[20] = 10;
  apx_diag_dispatcher_on_request_written(dispatcher, 0u, req_routes, sizeof(req_routes));

  // 4. LIST_UNCONNECTED_PORTS
  uint8_t req_unconn[24];
  memset(req_unconn, 0, sizeof(req_unconn));
  req_unconn[0] = 4;
  req_unconn[4] = 5; // opcode = 0x0005 (LIST_UNCONNECTED_PORTS)
  req_unconn[20] = 10;
  apx_diag_dispatcher_on_request_written(dispatcher, 0u, req_unconn, sizeof(req_unconn));

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_diag_dispatcher_get_node_definition(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);

  apx_server_test_connection_t *conn_op = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn_op);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn_op);

  const char *node_def = "APX/1.3\nN\"DefTestNode\"\nP\"Signal\"S:=0\n";
  apx_node_manager_t *node_mgr = apx_server_connection_get_node_manager((apx_server_connection_t *)conn_op);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_mgr, node_def));

  apx_server_test_connection_t *conn_mon = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn_mon);
  apx_server_connection_set_connection_type((apx_server_connection_t *)conn_mon, APX_CONNECTION_TYPE_MONITOR);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn_mon);

  apx_monitor_session_t *session = apx_server_monitor_find_session(monitor, (apx_server_connection_t *)conn_mon);
  CuAssertPtrNotNull(tc, session);
  apx_diag_dispatcher_t *dispatcher = apx_monitor_session_get_diag_dispatcher(session);
  CuAssertPtrNotNull(tc, dispatcher);

  apx_file_manager_t *fm = apx_connection_base_get_file_manager(&conn_mon->base.base);
  apx_file_t *resp_file = apx_file_manager_find_local_file_by_name(fm, APX_DIAG_RESPONSE_FILE_NAME);
  CuAssertPtrNotNull(tc, resp_file);
  apx_file_open_stream(resp_file, 4096u, 64u, 0u);
  apx_monitor_session_on_file_opened(session, resp_file);

  // Request definition of DefTestNode: request_id = 99, opcode = 0x0006
  const char *target = "DefTestNode";
  uint32_t const tlen = (uint32_t)strlen(target);
  uint32_t const req_size = 8u + 2u + tlen;
  uint8_t *req = (uint8_t *)malloc(req_size);
  CuAssertPtrNotNull(tc, req);
  memset(req, 0, req_size);
  req[0] = 99; // request_id = 99
  req[4] = 6; // opcode = 0x0006
  req[8] = (uint8_t)(tlen & 0xFF);
  req[9] = (uint8_t)((tlen >> 8) & 0xFF);
  memcpy(req + 10, target, tlen);

  apx_diag_dispatcher_on_request_written(dispatcher, 0u, req, req_size);
  free(req);

  // Verify temporary file was published
  apx_file_t *def_file = apx_file_manager_find_local_file_by_name(fm, "apx.diag.v1.def.99");
  CuAssertPtrNotNull(tc, def_file);

  // Simulate client opening definition file
  apx_file_open(def_file);
  apx_monitor_session_on_file_opened(session, def_file);

  // Simulate client closing definition file -> triggers revoke and cleanup
  apx_file_close(def_file);
  apx_monitor_session_on_file_closed(session, def_file);

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}

static void test_diag_dispatcher_subscribe_and_notify(CuTest *tc)
{
  apx_server_t *server = apx_server_new();
  CuAssertPtrNotNull(tc, server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_start(server));

  apx_server_monitor_t *monitor = apx_server_monitor_new(server);
  CuAssertPtrNotNull(tc, monitor);
  CuAssertIntEquals(tc, 0, monitor->num_active_port_watches);

  apx_server_test_connection_t *conn_op = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn_op);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn_op);

  const char *node_def = "APX/1.3\nN\"WatchNode\"\nP\"Counter\"S:=0\n";
  apx_node_manager_t *node_mgr = apx_server_connection_get_node_manager((apx_server_connection_t *)conn_op);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(node_mgr, node_def));

  apx_server_test_connection_t *conn_mon = apx_server_test_connection_new();
  CuAssertPtrNotNull(tc, conn_mon);
  apx_server_connection_set_connection_type((apx_server_connection_t *)conn_mon, APX_CONNECTION_TYPE_MONITOR);
  apx_server_accept_connection(server, (apx_server_connection_t *)conn_mon);

  apx_monitor_session_t *session = apx_server_monitor_find_session(monitor, (apx_server_connection_t *)conn_mon);
  CuAssertPtrNotNull(tc, session);
  apx_diag_dispatcher_t *dispatcher = apx_monitor_session_get_diag_dispatcher(session);
  CuAssertPtrNotNull(tc, dispatcher);

  apx_file_manager_t *fm = apx_connection_base_get_file_manager(&conn_mon->base.base);
  apx_file_t *resp_file = apx_file_manager_find_local_file_by_name(fm, APX_DIAG_RESPONSE_FILE_NAME);
  CuAssertPtrNotNull(tc, resp_file);
  apx_file_open_stream(resp_file, 4096u, 64u, 0u);
  apx_monitor_session_on_file_opened(session, resp_file);

  apx_file_t *notif_file = apx_file_manager_find_local_file_by_name(fm, APX_DIAG_NOTIFY_FILE_NAME);
  CuAssertPtrNotNull(tc, notif_file);
  apx_file_open_stream(notif_file, 4096u, 64u, 0u);
  apx_monitor_session_on_file_opened(session, notif_file);

  // Subscribe to WatchNode:Counter with SEND_INITIAL flag
  // Payload: selector_count = 1, reserved = 0, node_name = "WatchNode", port_name = "Counter"
  const char *nname = "WatchNode";
  const char *pname = "Counter";
  uint16_t nlen = (uint16_t)strlen(nname);
  uint16_t plen = (uint16_t)strlen(pname);
  uint32_t req_size = 8u + 4u + 2u + nlen + 2u + plen;
  uint8_t *req = (uint8_t *)malloc(req_size);
  CuAssertPtrNotNull(tc, req);
  memset(req, 0, req_size);
  req[0] = 50; // request_id = 50
  req[4] = 0x10; // opcode = 0x0010 (SUBSCRIBE_PORTS)
  req[6] = 0x01; // flags = SEND_INITIAL (0x0001)
  req[8] = 1;
  req[9] = 0; // selector_count = 1
  uint32_t off = 12u;
  req[off++] = (uint8_t)(nlen & 0xFF);
  req[off++] = (uint8_t)((nlen >> 8) & 0xFF);
  memcpy(req + off, nname, nlen);
  off += nlen;
  req[off++] = (uint8_t)(plen & 0xFF);
  req[off++] = (uint8_t)((plen >> 8) & 0xFF);
  memcpy(req + off, pname, plen);

  apx_diag_dispatcher_on_request_written(dispatcher, 0u, req, req_size);
  free(req);

  CuAssertIntEquals(tc, 1, apx_diag_dispatcher_num_subscriptions(dispatcher));
  CuAssertIntEquals(tc, 1, monitor->num_active_port_watches);

  // Fast-path live port write notification
  rmf_file_info_t out_file;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    rmf_file_info_create(&out_file, 0x1000u, 16u, "WatchNode.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
  apx_file_t test_out;
  memset(&test_out, 0, sizeof(test_out));
  memcpy(&test_out.file_info, &out_file, sizeof(out_file));

  uint8_t new_val[2] = {42, 0};
  apx_server_monitor_on_file_written(monitor, &conn_op->base.base, &test_out, 0u, new_val, sizeof(new_val));
  rmf_file_info_destroy(&out_file);

  // Unsubscribe (subscription_id = 1)
  uint8_t unsub_req[12];
  memset(unsub_req, 0, sizeof(unsub_req));
  unsub_req[0] = 51;
  unsub_req[4] = 0x11; // opcode = 0x0011 (UNSUBSCRIBE)
  unsub_req[8] = 1; // subscription_id = 1
  apx_diag_dispatcher_on_request_written(dispatcher, 0u, unsub_req, sizeof(unsub_req));

  CuAssertIntEquals(tc, 0, apx_diag_dispatcher_num_subscriptions(dispatcher));
  CuAssertIntEquals(tc, 0, monitor->num_active_port_watches);

  apx_server_monitor_delete(monitor);
  apx_server_delete(server);
}