//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "apx/file.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_file_create_local(CuTest* tc);
static void test_apx_file_create_remote(CuTest* tc);
static void test_apx_file_newLocal(CuTest* tc);
static void test_apx_file_newRemote(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_file2(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_file_create_local);
   SUITE_ADD_TEST(suite, test_apx_file_create_remote);
   SUITE_ADD_TEST(suite, test_apx_file_newLocal);
   SUITE_ADD_TEST(suite, test_apx_file_newRemote);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_file_create_local(CuTest* tc)
{
   apx_file_t file;
   apx_fileInfo_t info;
   apx_fileInfo_create(&info, APX_ADDRESS_DEFINITION_START, 100, "test.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);

   apx_file_create(&file, &info);

   CuAssertTrue(tc, apx_file_isOpen(&file)==false);
   CuAssertTrue(tc, apx_file_isRemoteFile(&file)==false);
   CuAssertStrEquals(tc, "test.apx", file.fileInfo.name);
   CuAssertIntEquals(tc, 100, file.fileInfo.length);
   CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, file.fileInfo.fileType);
   CuAssertIntEquals(tc, APX_DEFINITION_FILE_TYPE, apx_file_getApxFileType(&file));
   CuAssertPtrEquals(tc, 0, file.notificationHandler.arg);
   CuAssertPtrEquals(tc, 0, file.notificationHandler.openNotify);
   CuAssertPtrEquals(tc, 0, file.notificationHandler.writeNotify);
   apx_file_destroy(&file);
   apx_fileInfo_destroy(&info);
}

static void test_apx_file_create_remote(CuTest* tc)
{
   apx_file_t file;
   apx_fileInfo_t info;
   apx_fileInfo_create(&info, APX_ADDRESS_DEFINITION_START | RMF_REMOTE_ADDRESS_BIT, 100, "test.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);

   apx_file_create(&file, &info);

   CuAssertTrue(tc, apx_file_isOpen(&file)==false);
   CuAssertTrue(tc, apx_file_isRemoteFile(&file)==true);
   CuAssertStrEquals(tc, "test.apx", file.fileInfo.name);
   CuAssertIntEquals(tc, 100, file.fileInfo.length);
   CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, file.fileInfo.fileType);
   CuAssertIntEquals(tc, APX_DEFINITION_FILE_TYPE, apx_file_getApxFileType(&file));
   CuAssertPtrEquals(tc, 0, file.notificationHandler.arg);
   CuAssertPtrEquals(tc, 0, file.notificationHandler.openNotify);
   CuAssertPtrEquals(tc, 0, file.notificationHandler.writeNotify);
   apx_file_destroy(&file);
   apx_fileInfo_destroy(&info);
}

static void test_apx_file_newLocal(CuTest* tc)
{
   apx_file_t *file1;
   rmf_fileInfo_t info1;
   rmf_fileInfo_create(&info1, "test.apx", 0, 100, RMF_FILE_TYPE_FIXED);

   file1 = apx_file_newLocal(&info1);

   CuAssertTrue(tc, apx_file_isOpen(file1)==false);
   CuAssertTrue(tc, apx_file_isRemoteFile(file1)==false);


   apx_file_delete(file1);
}

static void test_apx_file_newRemote(CuTest* tc)
{
   apx_file_t *file1;
   rmf_fileInfo_t info1;
   rmf_fileInfo_create(&info1, "test.apx", 0, 100, RMF_FILE_TYPE_FIXED);

   file1 = apx_file_newRemote(&info1);

   CuAssertTrue(tc, apx_file_isOpen(file1)==false);
   CuAssertTrue(tc, apx_file_isRemoteFile(file1)==true);

   apx_file_delete(file1);
}
