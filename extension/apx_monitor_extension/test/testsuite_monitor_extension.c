//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "CuTest.h"
#include "apx/server.h"
#include "apx/extension/server_monitor.h"
#include "apx/extension/monitor_extension.h"
#include "apx/server_test_connection.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_new_monitor_connection_receives_connection_id_in_accept_header(CuTest* tc);
static void test_monitor_connection_transmits_existing_connection_info_on_connect(CuTest* tc);

//Helper functions
static void register_extension(CuTest* tc, apx_server_t* server);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_monitor_extension(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_new_monitor_connection_receives_connection_id_in_accept_header);
   SUITE_ADD_TEST(suite, test_monitor_connection_transmits_existing_connection_info_on_connect);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_new_monitor_connection_receives_connection_id_in_accept_header(CuTest* tc)
{
   apx_server_t* server;
   apx_serverTestConnection_t* monitor_connection = NULL;
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

   server = apx_server_new();
   CuAssertPtrNotNull(tc, server);
   register_extension(tc, server);
   apx_server_start(server);
   monitor_connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, monitor_connection);
   apx_server_accept_connection(server, (apx_serverConnection_t*)monitor_connection);
   apx_serverTestConnection_set_tester_connection_type(monitor_connection, APX_CONNECTION_TYPE_MONITOR);   
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(monitor_connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(monitor_connection));
   apx_serverTestConnection_run(monitor_connection);
   apx_server_run(server);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(monitor_connection));
   packet = apx_serverTestConnection_get_log_packet(monitor_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, (int)sizeof(actual), adt_bytearray_length(packet)); //Should contain acknowledge message
   memcpy(actual, adt_bytearray_data(packet), sizeof(actual));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(actual)));   
   apx_server_delete(server);
}

static void test_monitor_connection_transmits_existing_connection_info_on_connect(CuTest* tc)
{
   apx_server_t* server;
   apx_serverTestConnection_t* default_connection = NULL;
   apx_serverTestConnection_t* monitor_connection = NULL;
   adt_bytearray_t* packet;
   int32_t const default_greeting_accepted_size = 9;
   int32_t const new_greeting_accepted_size = 13;
   uint8_t actual[15];
   uint8_t expected[15] = {
      //message size
      UINT32_SIZE+ RMF_CMD_TYPE_SIZE + UINT32_SIZE + UINT8_SIZE + CHAR_SIZE, //last byte is the null-terminator of the connection tag
      //write address
      0xBFu,
      0xFFu,
      0xFCu,
      0x00u,
      //command type
      (uint8_t)RMF_CMD_CONNECTION_CREATE,
      0u,
      0u,
      0u,
      //connection id
      0x00u,
      0x00u,
      0x00u,
      0x00u,
      APX_CONNECTION_STATE_ACCEPTED,
      0x00u,
   };

   server = apx_server_new();
   CuAssertPtrNotNull(tc, server);
   register_extension(tc, server);
   apx_server_start(server);
   default_connection = apx_serverTestConnection_new();
   monitor_connection = apx_serverTestConnection_new();
   CuAssertPtrNotNull(tc, default_connection);
   CuAssertPtrNotNull(tc, monitor_connection);

   //Attach default connection
   apx_server_accept_connection(server, (apx_serverConnection_t*)default_connection);
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(default_connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(default_connection));
   apx_serverTestConnection_run(default_connection);
   apx_server_run(server);
   CuAssertIntEquals(tc, 1u, apx_serverTestConnection_log_length(default_connection));
   packet = apx_serverTestConnection_get_log_packet(default_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, default_greeting_accepted_size, adt_bytearray_length(packet)); //Should contain acknowledge message

   //Attach monitor Connection
   apx_serverTestConnection_set_tester_connection_type(monitor_connection, APX_CONNECTION_TYPE_MONITOR);
   apx_server_accept_connection(server, (apx_serverConnection_t*)monitor_connection);   
   CuAssertIntEquals(tc, 0u, apx_serverTestConnection_log_length(monitor_connection));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_send_greeting_header(monitor_connection));
   apx_serverTestConnection_run(monitor_connection);
   apx_server_run(server);
   CuAssertIntEquals(tc, 2u, apx_serverTestConnection_log_length(monitor_connection));
   packet = apx_serverTestConnection_get_log_packet(monitor_connection, 0);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, new_greeting_accepted_size, adt_bytearray_length(packet)); //Should contain acknowledge message
   packet = apx_serverTestConnection_get_log_packet(monitor_connection, 1);
   CuAssertPtrNotNull(tc, packet);
   CuAssertIntEquals(tc, (int) sizeof(actual), adt_bytearray_length(packet));
   memcpy(actual, adt_bytearray_data(packet), sizeof(actual));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(actual)));
   apx_server_delete(server);
}


//Helper functions
static void register_extension(CuTest* tc, apx_server_t* server)
{   
   dtl_hv_t* cfg = dtl_hv_new();
   CuAssertPtrNotNull(tc, cfg);
   dtl_hv_set_cstr(cfg, "extension-enabled", (dtl_dv_t*) dtl_sv_make_bool(true), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_monitorExtension_register(server, (dtl_dv_t*)cfg));
   dtl_dec_ref(cfg);
}