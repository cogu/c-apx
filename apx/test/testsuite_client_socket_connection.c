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
//#define DEFAULT_CONNECTION_ID 0
//#define ERROR_MSG_SIZE 150
//#define FILE_INFO_MAX_SIZE 256

#define CONNECTION_RUN(conn, sock) testsocket_run(sock); apx_clientSocketConnection_run(conn); testsocket_run(sock); apx_clientSocketConnection_run(conn)


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_connection_create(CuTest* tc);
static void test_send_greeting_on_connect(CuTest* tc);
static void test_send_file_info_after_acknowledge_from_single_node(CuTest* tc);

//Helper functions
static void testsocket_helper_send_acknowledge(testsocket_t *sock);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_client_socketConnection(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_connection_create);
   SUITE_ADD_TEST(suite, test_send_greeting_on_connect);
   SUITE_ADD_TEST(suite, test_send_file_info_after_acknowledge_from_single_node);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_connection_create(CuTest* tc)
{
   apx_clientSocketConnection_t conn;
   testsocket_t *sock1;
   sock1 = testsocket_new(); //apx_clientSocketConnection_t takes ownership of this object. No need to manually delete it
   CuAssertIntEquals(tc, 0, apx_clientSocketConnection_create(&conn, sock1, APX_CONNECTION_TYPE_DEFAULT));
   CuAssertUIntEquals(tc, APX_INVALID_CONNECTION_ID, conn.base.base.connection_id);
   CuAssertPtrEquals(tc, sock1, conn.socket_object);
   apx_clientSocketConnection_destroy(&conn);
}

static void test_send_greeting_on_connect(CuTest* tc)
{
   testsocket_t *sock;
   uint32_t len;
   adt_str_t *str;
   const char *expected_greeting = "RMFP/1.0\nMessage-Format: 32\n\n";
   const char *data;
   apx_clientSocketConnection_t conn;

   testsocket_spy_create();
   sock = testsocket_spy_server();
   CuAssertPtrNotNull(tc, sock);
   CuAssertIntEquals(tc, 0, apx_clientSocketConnection_create(&conn, sock, APX_CONNECTION_TYPE_DEFAULT));
   CuAssertIntEquals(tc, 0, testsocket_spy_getServerConnectedCount());
   testsocket_onConnect(sock);
   CONNECTION_RUN(&conn, sock);
   CuAssertIntEquals(tc, 1, testsocket_spy_getServerConnectedCount());
   data = (const char*) testsocket_spy_getReceivedData(&len);
   CuAssertIntEquals(tc, 30, len);
   CuAssertIntEquals(tc, 29, data[0]);
   str = adt_str_new_bstr((const uint8_t*) &data[1], (const uint8_t*) &data[1]+29);
   CuAssertStrEquals(tc, expected_greeting, adt_str_cstr(str));
   adt_str_delete(str);

   apx_clientSocketConnection_destroy(&conn);
   testsocket_spy_destroy();

}


/**
 * Test client with a single attached node
 */
static void test_send_file_info_after_acknowledge_from_single_node(CuTest* tc)
{
   char const* apx_text =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvidePort1\"C(0,3)\n"
      "P\"ProvidePort2\"C(0,7)\n";

   testsocket_t* sock;
   uint32_t len;
   const char* data;
   apx_clientSocketConnection_t conn;
   apx_nodeManager_t node_manager;
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
      (uint8_t)RMF_DIGEST_TYPE_NONE,
      0u,
      //DigestData U8[32]
      0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
      0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
      //FileName (Null-terminated string)
      'T', 'e', 's', 't', 'N','o','d','e', '1', '.','o','u','t','\0'
   };


   testsocket_spy_create();
   sock = testsocket_spy_server();
   apx_nodeManager_create(&node_manager, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, sock);
   CuAssertIntEquals(tc, 0, apx_clientSocketConnection_create(&conn, sock, APX_CONNECTION_TYPE_DEFAULT));
   apx_clientSocketConnection_attach_node_manager(&conn, &node_manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientSocketConnection_build_node(&conn, apx_text));
   CuAssertIntEquals(tc, 0, testsocket_spy_getServerConnectedCount());
   testsocket_onConnect(sock);
   CONNECTION_RUN(&conn, sock);
   CuAssertIntEquals(tc, 1, testsocket_spy_getServerConnectedCount());
   data = (const char*)testsocket_spy_getReceivedData(&len);
   CuAssertIntEquals(tc, 30, len); //This is the greeting message
   testsocket_spy_clearReceivedData();

   testsocket_helper_send_acknowledge(sock);
   CONNECTION_RUN(&conn, sock);
   data = (const char*) testsocket_spy_getReceivedData(&len);
   CuAssertIntEquals(tc, message_size *2, len); //Expect 2 messages in buffer
   //Verify first message
   memcpy(actual, data, message_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, data, message_size));
   //Verify second message
   expected[12] = 0x04; //StartAddress[0]
   expected[13] = (uint8_t)strlen(apx_text); //FileSize[0]
   expected[19] = (uint8_t)RMF_DIGEST_TYPE_SHA256;
   sha256_calc(&expected[21], apx_text, strlen(apx_text));
   expected[63] = 'a'; //FileName extension
   expected[64] = 'p'; //FileName extension
   expected[65] = 'x'; //FileName extension
   memcpy(actual, data + message_size, message_size);
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, message_size));

   //clean
   apx_clientSocketConnection_destroy(&conn);
   testsocket_spy_destroy();
   apx_nodeManager_destroy(&node_manager);
}

static void testsocket_helper_send_acknowledge(testsocket_t *sock)
{
   uint8_t buffer[1+8];
   apx_size_t msg_size;
   int32_t buf_size = (int32_t) sizeof(buffer) - 1;
   msg_size = rmf_address_encode(&buffer[1], buf_size, RMF_CMD_AREA_START_ADDRESS, false);
   msg_size += rmf_encode_acknowledge_cmd(&buffer[1+ msg_size], buf_size - msg_size);
   assert(msg_size == 8);
   buffer[0]= (uint8_t)msg_size;
   testsocket_serverSend(sock, &buffer[0], 1+ msg_size);
}