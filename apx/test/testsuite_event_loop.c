//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "CuTest.h"
#include "apx/event_loop.h"
#include "apx/file_manager.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_eventLoop_connected_event(CuTest* tc);
static void test_apx_eventLoop_disconnected_event(CuTest* tc);

/*
static void mockHandlerReset(void);
static void mock_onConnected(void *arg, struct apx_fileManager_tag *fileManager);
static void mock_onDisconnected(void *arg, struct apx_fileManager_tag *fileManager);
*/
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static uint32_t m_onConnectedCount;
static uint32_t m_onDisconnectedCount;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_eventLoop(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_eventLoop_connected_event);
   SUITE_ADD_TEST(suite, test_apx_eventLoop_disconnected_event);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_eventLoop_connected_event(CuTest* tc)
{
//   apx_fileManager_t fileManager;
   apx_eventLoop_t *loop = apx_eventLoop_new();
   CuAssertPtrNotNull(tc, loop);
/*
   CuAssertIntEquals(tc, 0, apx_fileManager_create(&fileManager, APX_FILEMANAGER_SERVER_MODE, 0, NULL));
   mockHandlerReset();
   apx_eventLoop_emitApxConnected(loop, mock_onConnected, 0, &fileManager);
   CuAssertUIntEquals(tc, 0, m_onConnectedCount);
   apx_eventLoop_run(loop);
   CuAssertUIntEquals(tc, 1, m_onConnectedCount);

   apx_fileManager_destroy(&fileManager);
*/
   apx_eventLoop_delete(loop);
}

static void test_apx_eventLoop_disconnected_event(CuTest* tc)
{
//   apx_fileManager_t fileManager;
   apx_eventLoop_t *loop = apx_eventLoop_new();
   CuAssertPtrNotNull(tc, loop);
/*
   CuAssertIntEquals(tc, 0, apx_fileManager_create(&fileManager, APX_FILEMANAGER_SERVER_MODE, 0, NULL));
   mockHandlerReset();
   apx_eventLoop_emitApxDisconnected(loop, mock_onDisconnected, 0, &fileManager);
   CuAssertUIntEquals(tc, 0, m_onDisconnectedCount);
   apx_eventLoop_run(loop);
   CuAssertUIntEquals(tc, 1, m_onDisconnectedCount);
   apx_fileManager_destroy(&fileManager);
   */
   apx_eventLoop_delete(loop);
}

/*
static void mockHandlerReset(void)
{
   m_onConnectedCount = 0;
   m_onDisconnectedCount = 0;
}

static void mock_onConnected(void *arg, struct apx_fileManager_tag *fileManager)
{
   m_onConnectedCount++;
}

static void mock_onDisconnected(void *arg, struct apx_fileManager_tag *fileManager)
{
   m_onDisconnectedCount++;
}
*/
