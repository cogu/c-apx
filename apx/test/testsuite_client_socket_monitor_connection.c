//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "CuTest.h"
#include "pack.h"
#include "apx/socket_client_connection.h"
#include "testsocket_spy.h"
#include "apx/file_manager.h"
#include "apx/remotefile.h"
#include "sha256.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

#define CONNECTION_RUN(conn, sock) testsocket_run(sock); apx_clientSocketConnection_run(conn); testsocket_run(sock); apx_clientSocketConnection_run(conn)


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_connection_create(CuTest* tc);
static void test_send_greeting_on_connect(CuTest* tc);


//Helper functions
static void testsocket_helper_send_acknowledge(testsocket_t* sock);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_client_socket_monitor_connection(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_connection_create);
   SUITE_ADD_TEST(suite, test_send_greeting_on_connect);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_connection_create(CuTest* tc)
{
   apx_clientSocketConnection_t conn;
   testsocket_t* sock1;
   sock1 = testsocket_new(); //apx_clientSocketConnection_t takes ownership of this object. No need to manually delete it
   CuAssertIntEquals(tc, 0, apx_clientSocketConnection_create(&conn, sock1, APX_CONNECTION_TYPE_MONITOR));
   CuAssertUIntEquals(tc, APX_INVALID_CONNECTION_ID, conn.base.base.connection_id);
   CuAssertPtrEquals(tc, sock1, conn.socket_object);
   CuAssertIntEquals(tc, APX_CONNECTION_TYPE_MONITOR, apx_clientSocketConnection_get_connection_type(&conn));
   apx_clientSocketConnection_destroy(&conn);
}

static void test_send_greeting_on_connect(CuTest* tc)
{
   testsocket_t* sock;
   uint32_t len;
   adt_str_t* str;
   const char* expected_greeting = "RMFP/1.1\nMessage-Size: 32\nConnection-Type: Monitor\n\n";
   const char* data;
   apx_clientSocketConnection_t conn;

   testsocket_spy_create();
   sock = testsocket_spy_server();
   CuAssertPtrNotNull(tc, sock);
   CuAssertIntEquals(tc, 0, apx_clientSocketConnection_create(&conn, sock, APX_CONNECTION_TYPE_MONITOR));
   CuAssertIntEquals(tc, 0, testsocket_spy_getServerConnectedCount());
   testsocket_onConnect(sock);
   CONNECTION_RUN(&conn, sock);
   CuAssertIntEquals(tc, 1, testsocket_spy_getServerConnectedCount());
   data = (const char*)testsocket_spy_getReceivedData(&len);
   CuAssertIntEquals(tc, 53, len);
   CuAssertIntEquals(tc, 52, data[0]);
   str = adt_str_new_bstr((const uint8_t*)&data[1], (const uint8_t*)&data[1] + 52);
   CuAssertStrEquals(tc, expected_greeting, adt_str_cstr(str));
   adt_str_delete(str);

   apx_clientSocketConnection_destroy(&conn);
   testsocket_spy_destroy();

}


