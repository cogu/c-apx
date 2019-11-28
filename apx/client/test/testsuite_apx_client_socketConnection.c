//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "CuTest.h"
#include "pack.h"
#include "apx_clientSocketConnection.h"
#include "apx_client.h"
#include "testsocket_spy.h"
#include "apx_fileManager2.h"
#include "apx_test_nodes.h"
#include "rmf.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define DEFAULT_CONNECTION_ID 0
#define ERROR_MSG_SIZE 150
#define FILE_INFO_MAX_SIZE 256

#define PORT_DATA_START      0x0
#define DEFINITION_START     0x4000000
#define PORT_DATA_BOUNDARY   0x400u //1KB, this must be a power of 2
#define DEFINITION_BOUNDARY  0x100000u //1MB, this must be a power of 2

#define CLIENT_RUN(cli, sock) testsocket_run(sock); apx_client_run(cli); testsocket_run(sock); apx_client_run(cli)


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_clientSocketConnection_create(CuTest* tc);
static void test_apx_clientSocketConnection_transmitHandlerSetup(CuTest* tc);
static void test_apx_clientSocketConnection_sendGreetingOnConnect(CuTest* tc);
static void test_apx_clientSocketConnection_sendApxFileAfterAcknowledge1(CuTest* tc);
static void test_apx_clientSocketConnection_sendApxFileAfterAcknowledge2(CuTest* tc);
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
   SUITE_ADD_TEST(suite, test_apx_clientSocketConnection_create);
   SUITE_ADD_TEST(suite, test_apx_clientSocketConnection_transmitHandlerSetup);
   SUITE_ADD_TEST(suite, test_apx_clientSocketConnection_sendGreetingOnConnect);
   SUITE_ADD_TEST(suite, test_apx_clientSocketConnection_sendApxFileAfterAcknowledge1);
   SUITE_ADD_TEST(suite, test_apx_clientSocketConnection_sendApxFileAfterAcknowledge2);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_clientSocketConnection_create(CuTest* tc)
{
   apx_clientSocketConnection_t conn;
   testsocket_t *sock1, *sock2;
   sock1 = testsocket_new(); //apx_clientSocketConnection_t takes ownership of this object. No need to manually delete it
   sock2 = testsocket_new();
   CuAssertIntEquals(tc, 0, apx_clientSocketConnection_create(&conn, sock1));
   CuAssertUIntEquals(tc, 0, conn.base.base.connectionId);
   CuAssertPtrEquals(tc, sock1, conn.socketObject);
   apx_clientSocketConnection_destroy(&conn);
   CuAssertIntEquals(tc, 0, apx_clientSocketConnection_create(&conn, sock2));
   CuAssertUIntEquals(tc, 0, conn.base.base.connectionId);
   CuAssertPtrEquals(tc, sock2, conn.socketObject);
   apx_clientSocketConnection_destroy(&conn);
}

static void test_apx_clientSocketConnection_transmitHandlerSetup(CuTest* tc)
{
   apx_clientSocketConnection_t *conn;
   testsocket_t *sock;
   apx_fileManager2_t *fileManager;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   conn = apx_clientSocketConnection_new(sock);
   CuAssertPtrNotNull(tc, conn);
   fileManager = &conn->base.base.fileManager;
   CuAssertPtrNotNull(tc, fileManager->worker.transmitHandler.send);
   apx_connectionBase_delete(&conn->base.base); //always delete using the base pointer
   testsocket_spy_destroy();
}

static void test_apx_clientSocketConnection_sendGreetingOnConnect(CuTest* tc)
{
   apx_client_t *client;
   testsocket_t *sock;
   uint32_t len;
   adt_str_t *str;
   const char *expectedGreeting = "RMFP/1.0\nNumHeader-Format:32\n\n";
   const char *data;
   testsocket_spy_create();
   client = apx_client_new();
   sock = testsocket_spy_server();
   CuAssertPtrNotNull(tc, sock);

   CuAssertIntEquals(tc, 0, testsocket_spy_getServerConnectedCount());
   apx_client_socketConnect(client, sock);
   CLIENT_RUN(client, sock);
   CuAssertIntEquals(tc, 1, testsocket_spy_getServerConnectedCount());
   data = (const char*) testsocket_spy_getReceivedData(&len);
   CuAssertIntEquals(tc, 31, len);
   CuAssertIntEquals(tc, 30, data[0]);
   str = adt_str_new_bstr((const uint8_t*) &data[1], (const uint8_t*) &data[1]+30);
   CuAssertStrEquals(tc, expectedGreeting, adt_str_cstr(str));
   adt_str_delete(str);

   apx_client_delete(client);
   testsocket_spy_destroy();
}

/**
 * Test client with a single attached node
 */
static void test_apx_clientSocketConnection_sendApxFileAfterAcknowledge1(CuTest* tc)
{
   apx_client_t *client;
   testsocket_t *sock;
   uint32_t len;
   const char *data;
   uint8_t msgBuf[FILE_INFO_MAX_SIZE];

   rmf_fileInfo_t fileInfo;

   //setup
   testsocket_spy_create();
   client = apx_client_new();
   CuAssertIntEquals(tc, 0, apx_client_getNumAttachedNodes(client));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, g_apx_test_node1));
   CuAssertIntEquals(tc, 1, apx_client_getNumAttachedNodes(client));
   sock = testsocket_spy_server();
   CuAssertPtrNotNull(tc, sock);
   CuAssertIntEquals(tc, 0, testsocket_spy_getServerConnectedCount());
   apx_client_socketConnect(client, sock);
   CLIENT_RUN(client, sock);
   testsocket_spy_clearReceivedData();

   //act
   testsocket_helper_send_acknowledge(sock);
   CLIENT_RUN(client, sock);
   data = (const char*) testsocket_spy_getReceivedData(&len);
   CuAssertIntEquals(tc, 134, len);
   CuAssertIntEquals(tc, 66, data[0]);
   rmf_fileInfo_create(&fileInfo, "TestNode1.out", PORT_DATA_START, (uint32_t) APX_TESTNODE1_OUT_DATA_LEN, RMF_FILE_TYPE_FIXED);
   CuAssertIntEquals(tc, 4, rmf_packHeader(msgBuf, sizeof(msgBuf), RMF_CMD_START_ADDR, false));
   CuAssertIntEquals(tc, 62, rmf_serialize_cmdFileInfo(msgBuf+4, sizeof(msgBuf)-4, &fileInfo));
   CuAssertIntEquals(tc, 0, memcmp(&data[1], &msgBuf[0], 66));
   CuAssertIntEquals(tc, 66, data[67]);
   rmf_fileInfo_create(&fileInfo, "TestNode1.apx", DEFINITION_START, (uint32_t) strlen(g_apx_test_node1), RMF_FILE_TYPE_FIXED);
   CuAssertIntEquals(tc, 4, rmf_packHeader(msgBuf, sizeof(msgBuf), RMF_CMD_START_ADDR, false));
   CuAssertIntEquals(tc, 62, rmf_serialize_cmdFileInfo(msgBuf+4, sizeof(msgBuf)-4, &fileInfo));
   CuAssertIntEquals(tc, 0, memcmp(&data[68], &msgBuf[0], 66));

   //clean
   apx_client_delete(client);
   testsocket_spy_destroy();
}

/**
 * Test client with 2 attached nodes
 */
static void test_apx_clientSocketConnection_sendApxFileAfterAcknowledge2(CuTest* tc)
{
   apx_client_t *client;
   testsocket_t *sock;
   uint32_t len;
   const char *data;
   uint8_t msgBuf[FILE_INFO_MAX_SIZE];
   rmf_fileInfo_t fileInfo;

   //init
   testsocket_spy_create();
   client = apx_client_new();
   CuAssertIntEquals(tc, 0, apx_client_getNumAttachedNodes(client));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, g_apx_test_node1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, g_apx_test_node2));
   CuAssertIntEquals(tc, 2, apx_client_getNumAttachedNodes(client));
   sock = testsocket_spy_server();
   CuAssertPtrNotNull(tc, sock);
   CuAssertIntEquals(tc, 0, testsocket_spy_getServerConnectedCount());
   apx_client_socketConnect(client, sock);
   CLIENT_RUN(client, sock);
   testsocket_spy_clearReceivedData();

   //act
   testsocket_helper_send_acknowledge(sock);
   CLIENT_RUN(client, sock);
   data = (const char*) testsocket_spy_getReceivedData(&len);
   CuAssertIntEquals(tc, 268, len);
   CuAssertIntEquals(tc, 66, data[0]);
   rmf_fileInfo_create(&fileInfo, "TestNode1.out", PORT_DATA_START, (uint32_t) APX_TESTNODE1_OUT_DATA_LEN, RMF_FILE_TYPE_FIXED);
   CuAssertIntEquals(tc, 4, rmf_packHeader(msgBuf, sizeof(msgBuf), RMF_CMD_START_ADDR, false));
   CuAssertIntEquals(tc, 62, rmf_serialize_cmdFileInfo(msgBuf+4, sizeof(msgBuf)-4, &fileInfo));
   CuAssertIntEquals(tc, 0, memcmp(&data[1], &msgBuf[0], 66));
   CuAssertIntEquals(tc, 66, data[67]);
   rmf_fileInfo_create(&fileInfo, "TestNode2.out", PORT_DATA_START+PORT_DATA_BOUNDARY, (uint32_t) APX_TESTNODE2_OUT_DATA_LEN, RMF_FILE_TYPE_FIXED);
   //rmf_fileInfo_create(&fileInfo, "TestNode1.apx", DEFINITION_START, (uint32_t) strlen(g_apx_test_node1), RMF_FILE_TYPE_FIXED);
   CuAssertIntEquals(tc, 4, rmf_packHeader(msgBuf, sizeof(msgBuf), RMF_CMD_START_ADDR, false));
   CuAssertIntEquals(tc, 62, rmf_serialize_cmdFileInfo(msgBuf+4, sizeof(msgBuf)-4, &fileInfo));
   CuAssertIntEquals(tc, 0, memcmp(&data[68], &msgBuf[0], 66));
   CuAssertIntEquals(tc, 66, data[134]);
   rmf_fileInfo_create(&fileInfo, "TestNode1.apx", DEFINITION_START, (uint32_t) strlen(g_apx_test_node1), RMF_FILE_TYPE_FIXED);
   CuAssertIntEquals(tc, 4, rmf_packHeader(msgBuf, sizeof(msgBuf), RMF_CMD_START_ADDR, false));
   CuAssertIntEquals(tc, 62, rmf_serialize_cmdFileInfo(msgBuf+4, sizeof(msgBuf)-4, &fileInfo));
   CuAssertIntEquals(tc, 0, memcmp(&data[135], &msgBuf[0], 66));
   CuAssertIntEquals(tc, 66, data[201]);
   rmf_fileInfo_create(&fileInfo, "TestNode2.apx", DEFINITION_START+DEFINITION_BOUNDARY, (uint32_t) strlen(g_apx_test_node2), RMF_FILE_TYPE_FIXED);
   CuAssertIntEquals(tc, 4, rmf_packHeader(msgBuf, sizeof(msgBuf), RMF_CMD_START_ADDR, false));
   CuAssertIntEquals(tc, 62, rmf_serialize_cmdFileInfo(msgBuf+4, sizeof(msgBuf)-4, &fileInfo));
   CuAssertIntEquals(tc, 0, memcmp(&data[202], &msgBuf[0], 66));

   //clean
   apx_client_delete(client);
   testsocket_spy_destroy();
}

static void testsocket_helper_send_acknowledge(testsocket_t *sock)
{
   uint8_t buffer[1+8];
   int32_t dataLen;
   int32_t bufSize = (int32_t) sizeof(buffer) - 1;
   dataLen = rmf_packHeader(&buffer[1], bufSize, RMF_CMD_START_ADDR, false);
   dataLen +=rmf_serialize_acknowledge(&buffer[1+dataLen], bufSize - dataLen);
   assert(dataLen == 8);
   buffer[0]=dataLen;
   testsocket_serverSend(sock, &buffer[0], 1+dataLen);
}

