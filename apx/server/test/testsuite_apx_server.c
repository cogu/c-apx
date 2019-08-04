//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_server.h"
#include "apx_serverSocketConnection.h"
#include "apx_eventFile.h"
#include "rmf.h"
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

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_server_create(CuTest* tc);
static void test_apx_server_eachConnectionGetNewId(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_server(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_server_create);
   SUITE_ADD_TEST(suite, test_apx_server_eachConnectionGetNewId);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_server_create(CuTest* tc)
{
   apx_server_t server;
   apx_server_create(&server);
   apx_server_destroy(&server);
}

#if 0
static void test_apx_server_sendsEventFileAfterGreeting(CuTest* tc)
{
   apx_server_t server;
   testsocket_t *socket;
   uint8_t *sendBuffer;
   uint32_t greetingLen;
   uint32_t u32HeaderLen;
   int32_t i32HeaderLen;
   const uint8_t *data;
   int32_t dataLen;
   uint8_t expectedArray[8];
   int32_t i32Result;
   int fileNameLen;
   char greeting[RMF_GREETING_MAX_LEN];
   socket = testsocket_new();
   apx_server_create(&server);
   CuAssertIntEquals(tc, 0, adt_bytearray_length(&socket->pendingServer));
   apx_server_accept_test_socket(&server, socket);
   strcpy(greeting, RMF_GREETING_START);
   //headers end with an additional newline
   strcat(greeting, "\n");
   CuAssertIntEquals(tc, 0, adt_bytearray_length(&socket->pendingServer));
   greetingLen = (uint32_t) strlen(greeting);
   CuAssertIntEquals(tc, 10, greetingLen);
   u32HeaderLen = 1;
   sendBuffer = (uint8_t*) malloc(u32HeaderLen+greetingLen);
   sendBuffer[0]=greetingLen;
   memcpy(&sendBuffer[u32HeaderLen], greeting, greetingLen);
   testsocket_clientSend(socket, sendBuffer, u32HeaderLen+greetingLen);
   CuAssertIntEquals(tc, u32HeaderLen+greetingLen, adt_bytearray_length(&socket->pendingServer));
   testsocket_run(socket);
   SLEEP(10);
   CuAssertIntEquals(tc, 0, adt_bytearray_length(&socket->pendingServer));
   dataLen = adt_bytearray_length(&socket->pendingClient);
   fileNameLen = strlen(APX_EVENT_SRV_FILE_NAME);
   CuAssertIntEquals(tc, 9+5+CMD_FILE_INFO_BASE_SIZE+fileNameLen+1, dataLen);
   data = adt_bytearray_data(&socket->pendingClient);
   CuAssertIntEquals(tc, 8, data[0]);
   i32HeaderLen = rmf_packHeader(&expectedArray[0], sizeof(expectedArray), RMF_CMD_START_ADDR, false);
   CuAssertIntEquals(tc, 4, i32HeaderLen);
   i32Result = rmf_serialize_acknowledge(&expectedArray[4], sizeof(expectedArray));
   CuAssertIntEquals(tc, 4, i32Result);
   CuAssertIntEquals(tc, 0, memcmp(&expectedArray[0], &data[1], 8));
   apx_server_destroy(&server);
   free(sendBuffer);
}
#endif
static void test_apx_server_eachConnectionGetNewId(CuTest* tc)
{
   apx_server_t server;
   testsocket_t *sockets[10];
   testsocket_t *lastSocket;
   apx_serverSocketConnection_t *conn;
   uint32_t connectionIdExpected = 0;
   int i;
   apx_server_create(&server);
   for (i=0;i<10;i++)
   {
      char msg[15];
      sockets[i] = testsocket_new();
      apx_server_acceptTestSocket(&server, sockets[i]);
      conn = (apx_serverSocketConnection_t*) apx_server_getLastConnection(&server);
      CuAssertPtrNotNull(tc, conn);
      sprintf(msg, "i=%d",i);
      CuAssertUIntEquals_Msg(tc, &msg[0], connectionIdExpected++, conn->base.base.connectionId);
   }
   //resetting internal variable nextConnectionId to 0 shall still yield next generated ID to be unique
   server.connectionManager.nextConnectionId=0;
   lastSocket = testsocket_new();
   apx_server_acceptTestSocket(&server, lastSocket);
   conn = (apx_serverSocketConnection_t*) apx_server_getLastConnection(&server);
   CuAssertPtrNotNull(tc, conn);
   CuAssertUIntEquals(tc, 10, conn->base.base.connectionId);
   apx_server_destroy(&server);

}
