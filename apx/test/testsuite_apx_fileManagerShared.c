//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_fileManagerShared.h"
#include "apx_eventListener.h"
#include "adt_bytearray.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerShared_create(CuTest* tc);
static void test_apx_fileManagerShared_findLocalFileByName(CuTest* tc);
static void test_apx_fileManagerShared_findRemoteFileByName(CuTest* tc);
static void test_apx_fileManagerShared_findLocalFileByAddress(CuTest* tc);
static void test_apx_fileManagerShared_findRemoteFileByAddress(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_fileManagerShared(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_fileManagerShared_create);
   SUITE_ADD_TEST(suite, test_apx_fileManagerShared_findLocalFileByName);
   SUITE_ADD_TEST(suite, test_apx_fileManagerShared_findRemoteFileByName);
   SUITE_ADD_TEST(suite, test_apx_fileManagerShared_findLocalFileByAddress);
   SUITE_ADD_TEST(suite, test_apx_fileManagerShared_findRemoteFileByAddress);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerShared_create(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_create(&shared));
   CuAssertUIntEquals(tc, APX_INVALID_CONNECTION_ID, shared.connectionId);

   apx_fileManagerShared_destroy(&shared);
}




static void test_apx_fileManagerShared_findLocalFileByName(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   apx_file_t *file = NULL;
   apx_fileInfo_t fileInfo1;
   apx_fileInfo_t fileInfo2;
   apx_fileInfo_t fileInfo3;
   apx_fileManagerShared_create(&shared);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo1, RMF_INVALID_ADDRESS, 100, "TestNode1.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo2, RMF_INVALID_ADDRESS, 200, "TestNode2.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo3, RMF_INVALID_ADDRESS, 300, "TestNode3.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   apx_fileManagerShared_createLocalFile(&shared, &fileInfo1);
   apx_fileManagerShared_createLocalFile(&shared, &fileInfo2);
   apx_fileManagerShared_createLocalFile(&shared, &fileInfo3);
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_getNumRemoteFiles(&shared));
   CuAssertIntEquals(tc, 3, apx_fileManagerShared_getNumLocalFiles(&shared));
   file = apx_fileManagerShared_findLocalFileByName(&shared, "TestNode2.apx");
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_ADDRESS_DEFINITION_START+0x100000, file->fileInfo.address);
   apx_fileManagerShared_destroy(&shared);
   apx_fileInfo_destroy(&fileInfo1);
   apx_fileInfo_destroy(&fileInfo2);
   apx_fileInfo_destroy(&fileInfo3);
}

static void test_apx_fileManagerShared_findRemoteFileByName(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   apx_file_t *file = NULL;
   apx_fileInfo_t fileInfo1;
   apx_fileInfo_t fileInfo2;
   apx_fileInfo_t fileInfo3;
   apx_fileManagerShared_create(&shared);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo1, APX_ADDRESS_DEFINITION_START, 100, "TestNode1.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo2, APX_ADDRESS_DEFINITION_START+0x100000, 200, "TestNode2.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo3, APX_ADDRESS_DEFINITION_START+0x200000, 300, "TestNode3.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   apx_fileManagerShared_createRemoteFile(&shared, &fileInfo1);
   apx_fileManagerShared_createRemoteFile(&shared, &fileInfo2);
   apx_fileManagerShared_createRemoteFile(&shared, &fileInfo3);
   CuAssertIntEquals(tc, 3, apx_fileManagerShared_getNumRemoteFiles(&shared));
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_getNumLocalFiles(&shared));
   file = apx_fileManagerShared_findRemoteFileByName(&shared, "TestNode2.apx");
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_ADDRESS_DEFINITION_START+0x100000, file->fileInfo.address & RMF_ADDRESS_MASK_INTERNAL);
   apx_fileManagerShared_destroy(&shared);
   apx_fileInfo_destroy(&fileInfo1);
   apx_fileInfo_destroy(&fileInfo2);
   apx_fileInfo_destroy(&fileInfo3);
}

static void test_apx_fileManagerShared_findLocalFileByAddress(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   apx_file_t *file = NULL;
   apx_fileInfo_t fileInfo1;
   apx_fileInfo_t fileInfo2;
   apx_fileInfo_t fileInfo3;
   apx_fileManagerShared_create(&shared);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo1, RMF_INVALID_ADDRESS, 100, "TestNode1.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo2, RMF_INVALID_ADDRESS, 200, "TestNode2.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo3, RMF_INVALID_ADDRESS, 300, "TestNode3.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   apx_fileManagerShared_createLocalFile(&shared, &fileInfo1);
   apx_fileManagerShared_createLocalFile(&shared, &fileInfo2);
   apx_fileManagerShared_createLocalFile(&shared, &fileInfo3);
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_getNumRemoteFiles(&shared));
   CuAssertIntEquals(tc, 3, apx_fileManagerShared_getNumLocalFiles(&shared));
   file = apx_fileManagerShared_findFileByAddress(&shared, APX_ADDRESS_DEFINITION_START+0x100000);
   CuAssertPtrNotNull(tc, file);
   CuAssertStrEquals(tc, "TestNode2.apx", file->fileInfo.name);
   apx_fileManagerShared_destroy(&shared);
   apx_fileInfo_destroy(&fileInfo1);
   apx_fileInfo_destroy(&fileInfo2);
   apx_fileInfo_destroy(&fileInfo3);
}

static void test_apx_fileManagerShared_findRemoteFileByAddress(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   apx_file_t *file = NULL;
   apx_fileInfo_t fileInfo1;
   apx_fileInfo_t fileInfo2;
   apx_fileInfo_t fileInfo3;
   apx_fileManagerShared_create(&shared);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo1, APX_ADDRESS_DEFINITION_START, 100, "TestNode1.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo2, APX_ADDRESS_DEFINITION_START+0x100000, 200, "TestNode2.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo3, APX_ADDRESS_DEFINITION_START+0x200000, 300, "TestNode3.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   apx_fileManagerShared_createRemoteFile(&shared, &fileInfo1);
   apx_fileManagerShared_createRemoteFile(&shared, &fileInfo2);
   apx_fileManagerShared_createRemoteFile(&shared, &fileInfo3);
   CuAssertIntEquals(tc, 3, apx_fileManagerShared_getNumRemoteFiles(&shared));
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_getNumLocalFiles(&shared));
   file = apx_fileManagerShared_findFileByAddress(&shared, (APX_ADDRESS_DEFINITION_START+0x100000) | RMF_REMOTE_ADDRESS_BIT);
   CuAssertPtrNotNull(tc, file);
   CuAssertStrEquals(tc, "TestNode2.apx", file->fileInfo.name);
   apx_fileManagerShared_destroy(&shared);
   apx_fileInfo_destroy(&fileInfo1);
   apx_fileInfo_destroy(&fileInfo2);
   apx_fileInfo_destroy(&fileInfo3);
}


