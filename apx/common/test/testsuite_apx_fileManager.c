//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_fileManager.h"
#include "apx_event.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_fileManager_server_create(CuTest* tc);
static void test_apx_fileManager_client_create(CuTest* tc);
//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_fileManager(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_fileManager_server_create);
   SUITE_ADD_TEST(suite, test_apx_fileManager_client_create);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManager_server_create(CuTest* tc)
{
   apx_fileManager_t manager;
   apx_fileMap_t *fileMap;
   apx_file_t *file1;
   apx_file_t *file2;
   apx_fileManager_create(&manager, APX_FILEMANAGER_SERVER_MODE);
   fileMap = &manager.localFileMap;
   CuAssertIntEquals(tc, 1, apx_fileMap_length(fileMap));
   file1 = apx_fileMap_findByName(fileMap, APX_EVENT_SRV_FILE_NAME);
   CuAssertPtrNotNull(tc, file1);
   file2 = apx_fileMap_findByAddress(fileMap, APX_EVENT_FILE_ADDRESS);
   CuAssertPtrNotNull(tc, file2);
   CuAssertPtrEquals(tc, file1, file2);
   apx_fileManager_destroy(&manager);
}

static void test_apx_fileManager_client_create(CuTest* tc)
{
   apx_fileManager_t manager;
   apx_fileMap_t *fileMap;
   apx_fileManager_create(&manager, APX_FILEMANAGER_CLIENT_MODE);
   fileMap = &manager.localFileMap;
   CuAssertIntEquals(tc, 0, apx_fileMap_length(fileMap));
   apx_fileManager_destroy(&manager);
}

