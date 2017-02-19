//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_file.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_file_remote(CuTest* tc);
static void test_apx_file_basename(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_file(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_file_remote);
   SUITE_ADD_TEST(suite, test_apx_file_basename);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_file_remote(CuTest* tc)
{
   apx_file_t *file1;
   rmf_cmdFileInfo_t info1;
   info1.address = 0x10000;
   strcpy(info1.name,"hello.txt");
   info1.digestType = RMF_DIGEST_TYPE_NONE;
   info1.fileType = RMF_FILE_TYPE_FIXED;
   info1.length = 2542;
   file1 = apx_file_newRemoteFile(&info1);
   CuAssertPtrNotNull(tc, file1);
   CuAssertIntEquals(tc, 1, (int) file1->isRemoteFile);
   CuAssertStrEquals(tc, "hello.txt", file1->fileInfo.name);
   CuAssertIntEquals(tc, 0x10000, (int) file1->fileInfo.address);
   CuAssertIntEquals(tc, 2542, (int) file1->fileInfo.length);
   CuAssertIntEquals(tc, RMF_DIGEST_TYPE_NONE, (int) file1->fileInfo.digestType);
   CuAssertPtrEquals(tc, (void*) file1, (void*) file1->fileInfo.userData);
   CuAssertPtrEquals(tc, 0, file1->nodeData);
   CuAssertIntEquals(tc, APX_USER_DATA_FILE, file1->fileType);
   apx_file_delete(file1);
}

static void test_apx_file_basename(CuTest* tc)
{
   apx_file_t *file1;
   rmf_cmdFileInfo_t info1;
   char *str;
   info1.address = 0x10000;
   strcpy(info1.name,"hello.txt");
   info1.digestType = RMF_DIGEST_TYPE_NONE;
   info1.fileType = RMF_FILE_TYPE_FIXED;
   info1.length = 2542;
   file1 = apx_file_newRemoteFile(&info1);
   CuAssertPtrNotNull(tc, file1);
   str = apx_file_basename(file1);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "hello", (char*) str);
   free(str);
   apx_file_delete(file1);
}
