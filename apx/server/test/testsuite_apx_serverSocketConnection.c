//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "CuTest.h"
#include "pack.h"
#include "apx_serverSocketConnection.h"
#include "apx_server.h"
#include "testsocket_spy.h"
#include "apx_fileManager.h"
#include "numheader.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
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
static void test_apx_serverSocketConnection_transmitHandlerSetup(CuTest* tc);
static void test_apx_serverSocketConnection_sendAckAfterReceivingHeader(CuTest* tc);
static void test_apx_serverSocketConnection_sendFileOpenAfterPresentedApxDefinition(CuTest *tc);
static void test_apx_serverSocketConnection_processApxDefinitionAfterWrite(CuTest *tc);
static void sendHeader(testsocket_t *sock);
static void sendFileInfoNoCheckSum(CuTest* tc, testsocket_t *sock, const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType);
static void verifyAcknowledge(CuTest* tc, testsocket_t *sock);
static void verifyFileOpenRequest(CuTest* tc, testsocket_t *sock, uint32_t address);
static void sendFileContent(CuTest* tc, testsocket_t *sock, uint32_t address);
static void verifyFileInfoResponse(CuTest* tc, const char *name, uint32_t fileAddress, uint32_t fileLen);
static void sendFileOpenRequest(CuTest* tc, testsocket_t *sock, uint32_t address);
static void verifyFileWrite(CuTest *tc, testsocket_t *sock, uint32_t startAddress, const uint8_t *writeData, uint32_t writeLen);

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
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_transmitHandlerSetup);
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_sendAckAfterReceivingHeader);
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_sendFileOpenAfterPresentedApxDefinition);
   SUITE_ADD_TEST(suite, test_apx_serverSocketConnection_processApxDefinitionAfterWrite);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_serverSocketConnection_create(CuTest* tc)
{
   apx_serverSocketConnection_t conn;
   testsocket_t *sock1, *sock2;
   sock1 = testsocket_new(); //apx_serverSocketConnection_t takes ownership of this object. No need to manually delete it
   sock2 = testsocket_new();
   CuAssertIntEquals(tc, 0, apx_serverSocketConnection_create(&conn, sock1, NULL));
   CuAssertUIntEquals(tc, 0, conn.base.base.connectionId);
   CuAssertPtrEquals(tc, sock1, conn.socketObject);
   apx_serverSocketConnection_destroy(&conn);
   CuAssertIntEquals(tc, 0, apx_serverSocketConnection_create(&conn, sock2, NULL));
   CuAssertUIntEquals(tc, 0, conn.base.base.connectionId);
   CuAssertPtrEquals(tc, sock2, conn.socketObject);
   apx_serverSocketConnection_destroy(&conn);
}

static void test_apx_serverSocketConnection_transmitHandlerSetup(CuTest* tc)
{
   apx_serverSocketConnection_t *conn;
   testsocket_t *sock;
   apx_fileManager_t *fileManager;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   conn = apx_serverSocketConnection_new(sock, NULL);
   CuAssertPtrNotNull(tc, conn);
   fileManager = &conn->base.base.fileManager;
   CuAssertPtrNotNull(tc, fileManager->transmitHandler.send);
   apx_serverSocketConnection_delete(conn);
   testsocket_spy_destroy();
}

static void test_apx_serverSocketConnection_sendAckAfterReceivingHeader(CuTest* tc)
{
   apx_server_t server;
   testsocket_t *sock;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   apx_server_create(&server);
   apx_server_start(&server);
   apx_server_acceptTestSocket(&server, sock);
   testsocket_onConnect(sock);
   sendHeader(sock);
   SERVER_RUN(&server, sock);
   verifyAcknowledge(tc, sock);
   apx_server_destroy(&server);
   testsocket_spy_destroy();
}

static void test_apx_serverSocketConnection_sendFileOpenAfterPresentedApxDefinition(CuTest *tc)
{
   apx_server_t server;
   testsocket_t *sock;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   apx_server_create(&server);
   apx_server_start(&server);
   apx_server_acceptTestSocket(&server, sock);
   testsocket_onConnect(sock);
   sendHeader(sock);
   SERVER_RUN(&server, sock);
   verifyAcknowledge(tc, sock);
   sendFileInfoNoCheckSum(tc, sock, "TestNode.apx", 0x10000, (uint32_t) strlen(m_TestNodeDefinition), RMF_FILE_TYPE_FIXED);
   SERVER_RUN(&server, sock);
   verifyFileOpenRequest(tc, sock, 0x10000);
   apx_server_destroy(&server);
   testsocket_spy_destroy();
}

static void test_apx_serverSocketConnection_processApxDefinitionAfterWrite(CuTest *tc)
{
   apx_server_t server;
   testsocket_t *sock;
   const uint8_t expectedInPortDataMsg[NUMHEADER32_SHORT_SIZE+RMF_LOW_ADDRESS_SIZE+2] = {4, 0, 0, 0xFF, 0xFF};
   uint32_t definitionAddress = 0x10000;
   uint32_t inDataFileAddress = 0x0;
   uint32_t dummy;
   testsocket_spy_create();
   sock = testsocket_spy_client();
   apx_server_create(&server);
   apx_server_start(&server);
   apx_server_acceptTestSocket(&server, sock);
   testsocket_onConnect(sock);
   sendHeader(sock);
   SERVER_RUN(&server, sock);
   testsocket_spy_clearReceivedData();
   sendFileInfoNoCheckSum(tc, sock, "TestNode.apx", definitionAddress, (uint32_t) strlen(m_TestNodeDefinition), RMF_FILE_TYPE_FIXED);
   SERVER_RUN(&server, sock);
   verifyFileOpenRequest(tc, sock, definitionAddress);
   CuAssertPtrEquals(tc, NULL, (void*) testsocket_spy_getReceivedData(&dummy));
   sendFileContent(tc, sock, 0x10000);
   SERVER_RUN(&server, sock);
   verifyFileInfoResponse(tc, "TestNode.in", inDataFileAddress, 2);
   sendFileOpenRequest(tc, sock, inDataFileAddress);
   SERVER_RUN(&server, sock);
   verifyFileWrite(tc, sock, inDataFileAddress, &expectedInPortDataMsg[0], (uint32_t) sizeof(expectedInPortDataMsg));
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
   assert(msgLen <= NUMHEADER32_MAX_NUM_SHORT);
   bufData[0]=(uint8_t) msgLen;
   testsocket_clientSend(sock, &bufData[0], 1+msgLen);
}

static void verifyFileInfoResponse(CuTest* tc, const char *name, uint32_t fileAddress, uint32_t fileLen)
{
   int32_t i;
   const uint8_t *data;
   uint32_t dataLen;
   uint32_t expectedSize = 1+RMF_HIGH_ADDRESS_SIZE+RMF_CMD_FILE_INFO_BASE_SIZE+strlen(name)+1;
   uint8_t expected[65] = {64, 0xBF, 0xFF, 0xFC, 0x00, RMF_CMD_FILE_INFO, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   'T','e','s', 't', 'N', 'o', 'd', 'e', '.', 'i', 'n'};
   packLE(&expected[9], fileAddress, 4);
   packLE(&expected[13], fileLen, 4);
   data = testsocket_spy_getReceivedData(&dataLen);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, expectedSize, dataLen);

   for(i=0;i<(int32_t) sizeof(expected);i++)
   {
      char msg[ERROR_MSG_SIZE];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected[i], data[i]);
   }

   testsocket_spy_clearReceivedData();
}

static void sendFileOpenRequest(CuTest* tc, testsocket_t *sock, uint32_t address)
{
   rmf_cmdOpenFile_t cmd;
   int32_t msgLen = 0;
   uint8_t buf[RMF_MAX_CMD_BUF_SIZE];
   cmd.address = address;
   msgLen += rmf_packHeader(&buf[1+msgLen], sizeof(buf)-msgLen, RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdOpenFile(&buf[1+msgLen], sizeof(buf)-msgLen, &cmd);
   CuAssertIntEquals(tc, 12, msgLen);
   buf[0]=(uint8_t) msgLen;
   testsocket_clientSend(sock, &buf[0], 1+msgLen);

}

static void verifyFileWrite(CuTest *tc, testsocket_t *sock, uint32_t startAddress, const uint8_t *writeData, uint32_t writeLen)
{
   uint32_t receiveLen;
   int32_t i;
   const uint8_t *data;
   data = testsocket_spy_getReceivedData(&receiveLen);
   CuAssertUIntEquals(tc, writeLen, receiveLen);
   for(i=0; i < (int32_t) writeLen;i++)
   {
      char msg[ERROR_MSG_SIZE];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, writeData[i], data[i]);
   }
   testsocket_spy_clearReceivedData();
}
