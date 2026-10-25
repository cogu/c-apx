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
#include "apx/server.h"
#include "apx/server_test_connection.h"
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
static void test_server_monitor_session_management(CuTest *tc);
static void test_monitor_session_publish_stream_files(CuTest *tc);
static void test_monitor_session_file_open_activates_streamer(CuTest *tc);
static void test_monitor_extension_register_lifecycle(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_monitor_extension(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_server_monitor_create_and_destroy);
  SUITE_ADD_TEST(suite, test_server_monitor_session_management);
  SUITE_ADD_TEST(suite, test_monitor_session_publish_stream_files);
  SUITE_ADD_TEST(suite, test_monitor_session_file_open_activates_streamer);
  SUITE_ADD_TEST(suite, test_monitor_extension_register_lifecycle);
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