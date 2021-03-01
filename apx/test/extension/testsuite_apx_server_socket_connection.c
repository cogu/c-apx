//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "CuTest.h"
#include "pack.h"
#include "apx/extension/socket_server_connection.h"
#include "apx/server.h"
#include "apx/extension/socket_server_extension.h"
#include "apx/server_connection.h"
#include "testsocket_spy.h"
#include "apx/file_manager.h"
#include "apx/numheader.h"
#include "osmacro.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define DEFAULT_CONNECTION_ID 0
#define ERROR_MSG_SIZE 150

#define SERVER_RUN(srv, sock) testsocket_run(sock); apx_server_run(srv); SLEEP(50); testsocket_run(sock); apx_server_run(srv)


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_connection_create(CuTest* tc);
static void test_each_connection_get_unique_id(CuTest* tc);
static void test_server_sends_acknowledge_after_accepting_header(CuTest* tc);
static void test_server_opens_definition_file_after_publication(CuTest *tc);
static void test_server_parses_definition_data_after_transmission(CuTest *tc);
static void send_header(testsocket_t *sock);
static void send_file_info_no_checksum(CuTest* tc, testsocket_t *sock, const char *name, uint32_t startAddress, uint32_t length);
static void verify_acknowledge(CuTest* tc, testsocket_t *sock);
static void verify_file_open_request(CuTest* tc, testsocket_t *sock, uint32_t address);
static void send_file_content(CuTest* tc, testsocket_t *sock, uint32_t address);

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


CuSuite* testSuite_apx_socketServerConnection(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_connection_create);
   SUITE_ADD_TEST(suite, test_each_connection_get_unique_id);
   SUITE_ADD_TEST(suite, test_server_sends_acknowledge_after_accepting_header);
   SUITE_ADD_TEST(suite, test_server_opens_definition_file_after_publication);
   SUITE_ADD_TEST(suite, test_server_parses_definition_data_after_transmission);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_connection_create(CuTest* tc)
{
   apx_socketServerConnection_t conn;
   testsocket_t *sock1;
   sock1 = testsocket_new(); //apx_socketServerConnection_t takes ownership of this object. No need to manually delete it
   CuAssertIntEquals(tc, 0, apx_socketServerConnection_create(&conn, sock1));
   CuAssertUIntEquals(tc, APX_INVALID_CONNECTION_ID, conn.base.base.connection_id);
   CuAssertPtrEquals(tc, sock1, conn.socket_object);
   apx_socketServerConnection_destroy(&conn);
}

static void test_each_connection_get_unique_id(CuTest* tc)
{
   apx_server_t server;
   testsocket_t *sockets[11];
   apx_serverConnection_t *lastConnection;
   uint32_t connectionIdExpected = 0;
   int i;
   apx_server_create(&server);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&server, NULL));
   apx_server_start(&server);

   for (i=0;i<10;i++)
   {
      char msg[15];
      sockets[i] = testsocket_new();
      apx_socketServerExtension_accept_testsocket(sockets[i]);
      lastConnection = apx_server_get_last_connection(&server);
      CuAssertPtrNotNull(tc, lastConnection);
      sprintf(msg, "i=%d",i);
      CuAssertUIntEquals_Msg(tc, &msg[0], connectionIdExpected++, apx_serverConnection_get_connection_id(lastConnection));
   }
   //resetting internal variable nextConnectionId to 0 shall still yield next generated ID to be unique
   server.connection_manager.next_connection_id = 0;
   sockets[10] = testsocket_new();
   apx_socketServerExtension_accept_testsocket(sockets[10]);
   lastConnection = apx_server_get_last_connection(&server);
   CuAssertPtrNotNull(tc, lastConnection);
   CuAssertUIntEquals(tc, 10, apx_serverConnection_get_connection_id(lastConnection));
   apx_server_destroy(&server);
}

static void test_server_sends_acknowledge_after_accepting_header(CuTest* tc)
{
   apx_server_t server;
   testsocket_t *sock;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   apx_server_create(&server);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&server, NULL));
   apx_server_start(&server);
   apx_socketServerExtension_accept_testsocket(sock);
   testsocket_onConnect(sock);
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&server, NULL));
   apx_server_start(&server);
   apx_socketServerExtension_accept_testsocket(sock);
   testsocket_onConnect(sock);
   send_header(sock);
   SERVER_RUN(&server, sock);
   verify_acknowledge(tc, sock);
   send_file_info_no_checksum(tc, sock, "TestNode.apx", APX_DEFINITION_ADDRESS_START, (uint32_t) strlen(m_TestNodeDefinition));
   SERVER_RUN(&server, sock);
   verify_file_open_request(tc, sock, APX_DEFINITION_ADDRESS_START);
   apx_server_destroy(&server);
   testsocket_spy_destroy();
}

static void test_server_parses_definition_data_after_transmission(CuTest *tc)
{
   apx_server_t server;
   testsocket_t *sock;
   unsigned int const definition_size = (unsigned int) strlen(m_TestNodeDefinition);
   uint32_t definitionAddress = APX_DEFINITION_ADDRESS_START;
   uint32_t dummy;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   apx_server_create(&server);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&server, NULL));
   apx_server_start(&server);
   apx_socketServerExtension_accept_testsocket(sock);
   testsocket_onConnect(sock);
   send_header(sock);
   SERVER_RUN(&server, sock);
   testsocket_spy_clearReceivedData();
   send_file_info_no_checksum(tc, sock, "TestNode.apx", definitionAddress, (uint32_t)definition_size);
   SERVER_RUN(&server, sock);
   verify_file_open_request(tc, sock, definitionAddress);
   CuAssertPtrEquals(tc, NULL, (void*) testsocket_spy_getReceivedData(&dummy));
   apx_serverConnection_t *connection = apx_server_get_last_connection(&server);
   CuAssertPtrNotNull(tc, connection);
   apx_nodeInstance_t *node_instance = apx_nodeManager_find(connection->base.node_manager, "TestNode");
   CuAssertPtrNotNull(tc, node_instance);
   send_file_content(tc, sock, definitionAddress);
   SERVER_RUN(&server, sock);
   apx_nodeData_t const* node_data = apx_nodeInstance_get_const_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_nodeData_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_DATA_STATE_CONNECTED, apx_nodeInstance_get_definition_data_state(node_instance));

   apx_server_destroy(&server);
   testsocket_spy_destroy();
}

static void send_header(testsocket_t *sock)
{
   const char *greeting = "RMFP/1.0\nMessage-Size:32\n\n";
   int32_t msgLen;
   uint8_t msg[RMF_GREETING_MAX_LEN];
   msgLen = (int32_t) strlen(greeting);
   msg[0] = (uint8_t) msgLen;
   memcpy(&msg[1], greeting, msgLen);
   testsocket_clientSend(sock, &msg[0], 1+msgLen);
}

static void send_file_info_no_checksum(CuTest* tc, testsocket_t *sock, const char *name, uint32_t startAddress, uint32_t length)
{
   int32_t msgLen = 0;
   uint8_t buf[RMF_CMD_AREA_SIZE];
   rmf_fileInfo_t* file_info = rmf_fileInfo_make_fixed(name, length, startAddress);
   CuAssertPtrNotNull(tc, file_info);

   msgLen += rmf_address_encode(&buf[1+msgLen], sizeof(buf)-msgLen, RMF_CMD_AREA_START_ADDRESS, false);
   msgLen += rmf_encode_publish_file_cmd(&buf[1+msgLen], sizeof(buf)-msgLen, file_info);
   rmf_fileInfo_delete(file_info);
   CuAssertIntEquals(tc, 65, msgLen);
   buf[0]=(uint8_t) msgLen;
   testsocket_clientSend(sock, &buf[0], 1+msgLen);

}

static void verify_acknowledge(CuTest* tc, testsocket_t *sock)
{
   uint32_t len;
   int32_t i;
   const uint8_t expected[9] = {8, 0xBF, 0xFF, 0xFC, 0x00, 0, 0, 0, 0};
   const uint8_t *data;
   (void)sock;
   data = testsocket_spy_getReceivedData(&len);
   CuAssertUIntEquals(tc, RMF_HIGH_ADDR_SIZE + RMF_CMD_TYPE_SIZE + 1, len);
   for(i=0;i<(int32_t) sizeof(expected);i++)
   {
      char msg[14];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected[i], data[i]);
   }
   testsocket_spy_clearReceivedData();
}

static void verify_file_open_request(CuTest* tc, testsocket_t *sock, uint32_t address)
{
   uint32_t len;
   int32_t i;
   uint8_t expected[12+1] = {12, 0xBF, 0xFF, 0xFC, 0x00, RMF_CMD_OPEN_FILE_MSG, 0, 0, 0, 0, 0, 0, 0};
   const uint8_t *data;
   (void)sock;
   packLE(&expected[9], address, 4);
   data = testsocket_spy_getReceivedData(&len);
   CuAssertUIntEquals(tc, RMF_HIGH_ADDR_SIZE + RMF_CMD_TYPE_SIZE + UINT32_SIZE+ 1, len);
   for(i=0;i<(int32_t) sizeof(expected);i++)
   {
      char msg[ERROR_MSG_SIZE];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected[i], data[i]);
   }
   testsocket_spy_clearReceivedData();
}

static void send_file_content(CuTest* tc, testsocket_t *sock, uint32_t address)
{
   uint8_t bufData[200];
   int32_t msgLen = 0;
   uint32_t dataLen = (uint32_t) strlen(m_TestNodeDefinition);
   uint32_t bufLen=(uint32_t) sizeof(bufData);
   (void)tc;
   msgLen += rmf_address_encode(&bufData[1+msgLen], bufLen, address, false);
   memcpy(&bufData[1+msgLen], &m_TestNodeDefinition[0], dataLen);
   msgLen+=dataLen;
   assert((uint32_t) msgLen <= NUMHEADER32_MAX_NUM_SHORT);
   bufData[0]=(uint8_t) msgLen;
   testsocket_clientSend(sock, &bufData[0], 1+msgLen);
}
