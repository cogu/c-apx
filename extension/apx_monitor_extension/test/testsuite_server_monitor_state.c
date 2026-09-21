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
#include "apx/server.h"
#include "apx/extension/server_monitor.h"
#include "apx/server_test_connection.h"
#include "testsocket.h"
#include "apx/extension/socket_server_connection.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_new_connection_event_creates_connection_observer(CuTest* tc);
static void test_connection_observer_destroyed_on_disconnect_event(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_serverMonitorState(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_new_connection_event_creates_connection_observer);
   SUITE_ADD_TEST(suite, test_connection_observer_destroyed_on_disconnect_event);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_new_connection_event_creates_connection_observer(CuTest* tc)
{
   apx_connection_id_t const connection_id = 0u;
   const char* connection_tag = "TCP";
   testsocket_t* socket = testsocket_new();
   apx_socket_server_connection_t* socket_connection = apx_socketServerConnection_new(socket);
   apx_server_connection_t* server_connection = (apx_server_connection_t*)socket_connection;
   apx_server_monitor_t* monitor = apx_serverMonitor_new(NULL);
   apx_observed_connection_t* observed_connection = NULL;
   CuAssertPtrNotNull(tc, socket_connection);
   CuAssertPtrNotNull(tc, monitor);
   CuAssertIntEquals(tc, 0, apx_serverMonitor_num_connections(monitor));
   apx_serverConnection_set_connection_id(server_connection, connection_id);
   apx_serverConnection_set_tag(server_connection, connection_tag);
   apx_serverMonitor_virtual_on_new_connection((void*)monitor, server_connection);
   CuAssertIntEquals(tc, 1, apx_serverMonitor_num_connections(monitor));
   observed_connection = apx_serverMonitor_get_last_observed_connection(monitor);
   CuAssertPtrNotNull(tc, observed_connection);
   CuAssertUIntEquals(tc, connection_id, apx_observedConnection_connection_id(observed_connection));
   CuAssertStrEquals(tc, connection_tag, apx_observed_connection_tag(observed_connection));
   apx_serverMonitor_delete(monitor);
   apx_socketServerConnection_delete(socket_connection);
}

static void test_connection_observer_destroyed_on_disconnect_event(CuTest* tc)
{
   apx_connection_id_t const connection_id = 0u;
   testsocket_t* socket = testsocket_new();
   apx_socket_server_connection_t* socket_connection = apx_socketServerConnection_new(socket);
   apx_server_connection_t* server_connection = (apx_server_connection_t*)socket_connection;
   apx_server_monitor_t* monitor = apx_serverMonitor_new(NULL);
   CuAssertPtrNotNull(tc, socket_connection);
   CuAssertPtrNotNull(tc, monitor);
   CuAssertIntEquals(tc, 0, apx_serverMonitor_num_connections(monitor));
   apx_serverConnection_set_connection_id(server_connection, connection_id);
   apx_serverMonitor_virtual_on_new_connection((void*)monitor, server_connection);
   CuAssertIntEquals(tc, 1, apx_serverMonitor_num_connections(monitor));
   apx_serverMonitor_virtual_on_connection_closed((void*)monitor, server_connection);
   CuAssertIntEquals(tc, 0, apx_serverMonitor_num_connections(monitor));
   apx_socketServerConnection_delete(socket_connection);
   apx_serverMonitor_delete(monitor);
}