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
#include "apx_serverSocketConnection.h"
#include "apx_server.h"
#include "apx_socketServerExtension.h"
#include "apx_serverConnectionBase.h"
#include "testsocket_spy.h"
#include "apx_fileManager.h"
#include "numheader.h"
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
static void test_apx_serverSocketConnection_create(CuTest* tc);
static void test_apx_serverSocketConnection_eachConnectionGetUniqueID(CuTest* tc);
static void test_apx_serverSocketConnection_transmitHandlerSetup(CuTest* tc);
static void test_apx_serverSocketConnection_serverSendsAckAfterAcceptingHeaderFromClient(CuTest* tc);
static void test_apx_serverSocketConnection_serverOpensFileAfterApxFileInfoReceived(CuTest *tc);
static void test_apx_serverSocketConnection_serverProcessesApxDefinitionAfterWrite(CuTest *tc);
static void sendHeader(testsocket_t *sock);
static void sendFileInfoNoCheckSum(CuTest* tc, testsocket_t *sock, const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType);
static void verifyAcknowledge(CuTest* tc, testsocket_t *sock);
static void verifyFileOpenRequest(CuTest* tc, testsocket_t *sock, uint32_t address);
static void sendFileContent(CuTest* tc, testsocket_t *sock, uint32_t address);

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


CuSuite* testSuite_apx_serverSocketConnection(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_create);
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_eachConnectionGetUniqueID);
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_transmitHandlerSetup);
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_serverSendsAckAfterAcceptingHeaderFromClient);
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_serverOpensFileAfterApxFileInfoReceived);
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_serverProcessesApxDefinitionAfterWrite);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_serverSocketConnection_create(CuTest* tc)
{
   apx_serverSocketConnection_t conn;
   testsocket_t *sock1;
   sock1 = testsocket_new(); //apx_serverSocketConnection_t takes ownership of this object. No need to manually delete it
   CuAssertIntEquals(tc, 0, apx_serverSocketConnection_create(&conn, sock1));
   CuAssertUIntEquals(tc, 0, conn.base.base.connectionId);
   CuAssertPtrEquals(tc, sock1, conn.socketObject);
   apx_serverSocketConnection_destroy(&conn);
}

static void test_apx_serverSocketConnection_eachConnectionGetUniqueID(CuTest* tc)
{
   apx_server_t server;
   testsocket_t *sockets[11];
   apx_serverConnectionBase_t *lastConnection;
   uint32_t connectionIdExpected = 0;
   int i;
   apx_server_create(&server);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&server, NULL));
   apx_server_start(&server);

   for (i=0;i<10;i++)
   {
      char msg[15];
      sockets[i] = testsocket_new();
      apx_socketServerExtension_acceptTestSocket(sockets[i]);
      lastConnection = apx_server_getLastConnection(&server);
      CuAssertPtrNotNull(tc, lastConnection);
      sprintf(msg, "i=%d",i);
      CuAssertUIntEquals_Msg(tc, &msg[0], connectionIdExpected++, apx_serverConnectionBase_getConnectionId(lastConnection));
   }
   //resetting internal variable nextConnectionId to 0 shall still yield next generated ID to be unique
   server.connectionManager.nextConnectionId=0;
   sockets[10] = testsocket_new();
   apx_socketServerExtension_acceptTestSocket(sockets[10]);
   lastConnection = apx_server_getLastConnection(&server);
   CuAssertPtrNotNull(tc, lastConnection);
   CuAssertUIntEquals(tc, 10, apx_serverConnectionBase_getConnectionId(lastConnection));
   apx_server_destroy(&server);
}

static void test_apx_serverSocketConnection_transmitHandlerSetup(CuTest* tc)
{
   apx_serverSocketConnection_t *conn;
   testsocket_t *sock;
   apx_fileManager_t *fileManager;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   conn = apx_serverSocketConnection_new(sock);
   CuAssertPtrNotNull(tc, conn);
   fileManager = &conn->base.base.fileManager;
   CuAssertPtrNotNull(tc, fileManager->worker.transmitHandler.arg);
   CuAssertPtrNotNull(tc, fileManager->worker.transmitHandler.send);
   CuAssertPtrNotNull(tc, fileManager->worker.transmitHandler.getSendBuffer);
   apx_serverSocketConnection_delete(conn);
   testsocket_spy_destroy();
}

static void test_apx_serverSocketConnection_serverSendsAckAfterAcceptingHeaderFromClient(CuTest* tc)
{
   apx_server_t server;
   testsocket_t *sock;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   apx_server_create(&server);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&server, NULL));
   apx_server_start(&server);
   apx_socketServerExtension_acceptTestSocket(sock);
   testsocket_onConnect(sock);
   sendHeader(sock);
   SERVER_RUN(&server, sock);
   verifyAcknowledge(tc, sock);
   apx_server_destroy(&server);
   testsocket_spy_destroy();
}

static void test_apx_serverSocketConnection_serverOpensFileAfterApxFileInfoReceived(CuTest *tc)
{
   apx_server_t server;
   testsocket_t *sock;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   apx_server_create(&server);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&server, NULL));
   apx_server_start(&server);
   apx_socketServerExtension_acceptTestSocket(sock);
   testsocket_onConnect(sock);
   sendHeader(sock);
   SERVER_RUN(&server, sock);
   verifyAcknowledge(tc, sock);
   sendFileInfoNoCheckSum(tc, sock, "TestNode.apx", APX_ADDRESS_DEFINITION_START, (uint32_t) strlen(m_TestNodeDefinition), RMF_FILE_TYPE_FIXED);
   SERVER_RUN(&server, sock);
   verifyFileOpenRequest(tc, sock, APX_ADDRESS_DEFINITION_START);
   apx_server_destroy(&server);
   testsocket_spy_destroy();
}

static void test_apx_serverSocketConnection_serverProcessesApxDefinitionAfterWrite(CuTest *tc)
{
   apx_server_t server;
   testsocket_t *sock;
   //const uint8_t expectedInPortDataMsg[NUMHEADER32_SHORT_SIZE+RMF_LOW_ADDRESS_SIZE+2] = {4, 0, 0, 0xFF, 0xFF};
   uint32_t definitionAddress = APX_ADDRESS_DEFINITION_START;
   //uint32_t inDataFileAddress = 0x0;
   uint32_t dummy;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   apx_server_create(&server);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_socketServerExtension_register(&server, NULL));
   apx_server_start(&server);
   apx_socketServerExtension_acceptTestSocket(sock);
   testsocket_onConnect(sock);
   sendHeader(sock);
   SERVER_RUN(&server, sock);
   testsocket_spy_clearReceivedData();
   sendFileInfoNoCheckSum(tc, sock, "TestNode.apx", definitionAddress, (uint32_t) strlen(m_TestNodeDefinition), RMF_FILE_TYPE_FIXED);
   SERVER_RUN(&server, sock);
   verifyFileOpenRequest(tc, sock, definitionAddress);
   CuAssertPtrEquals(tc, NULL, (void*) testsocket_spy_getReceivedData(&dummy));
   apx_serverConnectionBase_t *connection = apx_server_getLastConnection(&server);
   CuAssertPtrNotNull(tc, connection);
   apx_nodeInstance_t *nodeInstance = apx_nodeManager_find(&connection->base.nodeManager, "TestNode");
   CuAssertPtrNotNull(tc, nodeInstance);
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getNodeInfo(nodeInstance));
   sendFileContent(tc, sock, definitionAddress);
   SERVER_RUN(&server, sock);
   CuAssertPtrNotNull(tc, apx_nodeInstance_getNodeInfo(nodeInstance));

   apx_server_destroy(&server);
   testsocket_spy_destroy();
}

static void sendHeader(testsocket_t *sock)
{
   const char *greeting = "RMFP/1.0\nNumHeader-Format:32\n\n";
   int32_t msgLen;
   uint8_t msg[RMF_GREETING_MAX_LEN];
   msgLen = (int32_t) strlen(greeting);
   msg[0] = (uint8_t) msgLen;
   memcpy(&msg[1], greeting, msgLen);
   testsocket_clientSend(sock, &msg[0], 1+msgLen);
}

static void sendFileInfoNoCheckSum(CuTest* tc, testsocket_t *sock, const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType)
{
   rmf_fileInfo_t info;
   int32_t msgLen = 0;
   uint8_t buf[RMF_MAX_CMD_BUF_SIZE];
   CuAssertIntEquals(tc, 0, rmf_fileInfo_create(&info, name, startAddress, length, fileType));
   msgLen += rmf_packHeader(&buf[1+msgLen], sizeof(buf)-msgLen, RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdFileInfo(&buf[1+msgLen], sizeof(buf)-msgLen, &info);
   CuAssertIntEquals(tc, 65, msgLen);
   buf[0]=(uint8_t) msgLen;
   testsocket_clientSend(sock, &buf[0], 1+msgLen);
}

static void verifyAcknowledge(CuTest* tc, testsocket_t *sock)
{
   uint32_t len;
   int32_t i;
   const uint8_t expected[9] = {8, 0xBF, 0xFF, 0xFC, 0x00, 0, 0, 0, 0};
   const uint8_t *data;
   data = testsocket_spy_getReceivedData(&len);
   CuAssertUIntEquals(tc, RMF_CMD_ADDRESS_LEN+RMF_CMD_ACK_LEN+1, len);
   for(i=0;i<(int32_t) sizeof(expected);i++)
   {
      char msg[14];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected[i], data[i]);
   }
   testsocket_spy_clearReceivedData();
}

static void verifyFileOpenRequest(CuTest* tc, testsocket_t *sock, uint32_t address)
{
   uint32_t len;
   int32_t i;
   uint8_t expected[12+1] = {12, 0xBF, 0xFF, 0xFC, 0x00, RMF_CMD_FILE_OPEN, 0, 0, 0, 0, 0, 0, 0};
   const uint8_t *data;
   packLE(&expected[9], address, 4);
   data = testsocket_spy_getReceivedData(&len);
   CuAssertUIntEquals(tc, RMF_CMD_ADDRESS_LEN+RMF_CMD_FILE_OPEN_LEN+1, len);
   for(i=0;i<(int32_t) sizeof(expected);i++)
   {
      char msg[ERROR_MSG_SIZE];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected[i], data[i]);
   }
   testsocket_spy_clearReceivedData();
}

static void sendFileContent(CuTest* tc, testsocket_t *sock, uint32_t address)
{
   uint8_t bufData[200];
   int32_t msgLen = 0;
   uint32_t dataLen = (uint32_t) strlen(m_TestNodeDefinition);
   uint32_t bufLen=(uint32_t) sizeof(bufData);

   msgLen += rmf_packHeader(&bufData[1+msgLen], bufLen, address, false);
   memcpy(&bufData[1+msgLen], &m_TestNodeDefinition[0], dataLen);
   msgLen+=dataLen;
   assert((uint32_t) msgLen <= NUMHEADER32_MAX_NUM_SHORT);
   bufData[0]=(uint8_t) msgLen;
   testsocket_clientSend(sock, &bufData[0], 1+msgLen);
}
