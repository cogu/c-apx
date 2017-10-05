//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_sessionCmd.h"
//#include "apx_testServer.h"
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
static void test_apx_sessionCmd_TcpSock(CuTest* tc);
static void test_apx_sessionCmd_LocalSock(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_sessionCmd(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_sessionCmd_TcpSock);
   SUITE_ADD_TEST(suite, test_apx_sessionCmd_LocalSock);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_sessionCmd_TcpSock(CuTest* tc)
{
   apx_connectCmd_t connectCmd;
   apx_connectCmd_createTcpSock(&connectCmd, "hostname", 1234);
   CuAssertStrEquals(tc, "hostname", connectCmd.hostname_or_path);
   CuAssertUIntEquals(tc, 1234, connectCmd.tcp_port);
   CuAssertUIntEquals(tc, APX_CONNECTION_TYPE_TCP_SOCKET, connectCmd.connectionType);
   apx_connectCmd_destroy(&connectCmd);
}

static void test_apx_sessionCmd_LocalSock(CuTest* tc)
{
   apx_connectCmd_t connectCmd;
   apx_connectCmd_createLocalSock(&connectCmd, "/path/to/socket");
   CuAssertStrEquals(tc, "/path/to/socket", connectCmd.hostname_or_path);
   CuAssertUIntEquals(tc, 0, connectCmd.tcp_port);
   CuAssertUIntEquals(tc, APX_CONNECTION_TYPE_LOCAL_SOCKET, connectCmd.connectionType);
   apx_connectCmd_destroy(&connectCmd);
}

