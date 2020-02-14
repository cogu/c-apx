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
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_fileMap_createLocalDefinitionFile(CuTest* tc);
static void test_apx_fileMap_createRemoteDefinitionFile(CuTest* tc);
static void test_apx_fileMap_autoInsert(CuTest* tc);
static void test_apx_fileMap_manualInsert(CuTest* tc);
static void test_apx_fileMap_makeFileInfoArray(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_fileMap(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_fileMap_createLocalDefinitionFile);
   SUITE_ADD_TEST(suite, test_apx_fileMap_createRemoteDefinitionFile);
   SUITE_ADD_TEST(suite, test_apx_fileMap_autoInsert);
   SUITE_ADD_TEST(suite, test_apx_fileMap_manualInsert);
   SUITE_ADD_TEST(suite, test_apx_fileMap_makeFileInfoArray);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileMap_createLocalDefinitionFile(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   apx_file_t *file = NULL;
   apx_fileInfo_t fileInfo;
   apx_fileManagerShared_create(&shared);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo, RMF_INVALID_ADDRESS, 5800, "MyNode.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   file = apx_fileManagerShared_createLocalFile(&shared, &fileInfo);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_ADDRESS_DEFINITION_START, file->fileInfo.address);
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_getNumRemoteFiles(&shared));
   CuAssertIntEquals(tc, 1, apx_fileManagerShared_getNumLocalFiles(&shared));
   apx_fileInfo_destroy(&fileInfo);
   apx_fileManagerShared_destroy(&shared);
}

static void test_apx_fileMap_createRemoteDefinitionFile(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   apx_file_t *file = NULL;
   apx_fileInfo_t fileInfo;
   apx_fileManagerShared_create(&shared);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo, APX_ADDRESS_DEFINITION_START , 5800, "MyNode.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   file = apx_fileManagerShared_createRemoteFile(&shared, &fileInfo);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_ADDRESS_DEFINITION_START | RMF_REMOTE_ADDRESS_BIT, file->fileInfo.address);
   CuAssertIntEquals(tc, 1, apx_fileManagerShared_getNumRemoteFiles(&shared));
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_getNumLocalFiles(&shared));
   apx_fileInfo_destroy(&fileInfo);
   apx_fileManagerShared_destroy(&shared);
}


/**
 * test that the apx_fileMap automatically assigns addresses correctly when inserted
 */
static void test_apx_fileMap_autoInsert(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   apx_fileInfo_t fileInfo1;
   apx_fileInfo_t fileInfo2;
   apx_fileInfo_t fileInfo3;
   apx_fileInfo_t fileInfo4;
   apx_fileInfo_t fileInfo5;
   apx_fileInfo_t fileInfo6;
   apx_file_t *file1;
   apx_file_t *file2;
   apx_file_t *file3;
   apx_file_t *file4;
   apx_file_t *file5;
   apx_file_t *file6;
   uint8_t out1[256];
   uint8_t out2[1328];
   uint8_t out3[256];
   uint8_t def1[256];
   uint8_t def2[256];
   uint8_t def3[256];
   adt_list_elem_t *iter;
   apx_file_t *pFile;

   apx_fileManagerShared_create(&shared);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo1, RMF_INVALID_ADDRESS, sizeof(out1), "TestNode1.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo2, RMF_INVALID_ADDRESS, sizeof(def1), "TestNode1.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo3, RMF_INVALID_ADDRESS, sizeof(out2), "TestNode2.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo4, RMF_INVALID_ADDRESS, sizeof(def2), "TestNode2.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo5, RMF_INVALID_ADDRESS, sizeof(out3), "TestNode3.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo6, RMF_INVALID_ADDRESS, sizeof(def3), "TestNode3.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   file1 = apx_fileManagerShared_createLocalFile(&shared, &fileInfo1);
   file2 = apx_fileManagerShared_createLocalFile(&shared, &fileInfo2);
   file3 = apx_fileManagerShared_createLocalFile(&shared, &fileInfo3);
   file4 = apx_fileManagerShared_createLocalFile(&shared, &fileInfo4);
   file5 = apx_fileManagerShared_createLocalFile(&shared, &fileInfo5);
   file6 = apx_fileManagerShared_createLocalFile(&shared, &fileInfo6);
   CuAssertPtrNotNull(tc, file1);
   CuAssertPtrNotNull(tc, file2);
   CuAssertPtrNotNull(tc, file3);
   CuAssertPtrNotNull(tc, file4);
   CuAssertPtrNotNull(tc, file5);
   CuAssertPtrNotNull(tc, file6);
   iter = adt_list_iter_first(apx_fileMap_getList(&shared.localFileMap));
   pFile = (apx_file_t*) iter->pItem;
   CuAssertPtrEquals(tc, iter->pItem, file1 );
   CuAssertStrEquals(tc, "TestNode1.out", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 0, pFile->fileInfo.address);
   iter = adt_list_iter_next(iter);
   pFile = (apx_file_t*) iter->pItem;
   CuAssertStrEquals(tc, "TestNode2.out", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 1024, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file3 );
   iter = adt_list_iter_next(iter);
   pFile = (apx_file_t*) iter->pItem;
   CuAssertStrEquals(tc, "TestNode3.out", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 1024*3, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file5 );
   iter = adt_list_iter_next(iter);
   pFile = (apx_file_t*) iter->pItem;
   CuAssertStrEquals(tc, "TestNode1.apx", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 64*1024*1024, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file2 );
   iter = adt_list_iter_next(iter);
   pFile = (apx_file_t*) iter->pItem;
   CuAssertStrEquals(tc, "TestNode2.apx", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 65*1024*1024, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file4 );
   iter = adt_list_iter_next(iter);
   pFile = (apx_file_t*) iter->pItem;
   CuAssertStrEquals(tc, "TestNode3.apx", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 66*1024*1024, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file6 );
   apx_fileManagerShared_destroy(&shared);
   apx_fileInfo_destroy(&fileInfo1);
   apx_fileInfo_destroy(&fileInfo2);
   apx_fileInfo_destroy(&fileInfo3);
   apx_fileInfo_destroy(&fileInfo4);
   apx_fileInfo_destroy(&fileInfo5);
   apx_fileInfo_destroy(&fileInfo6);
}


static void test_apx_fileMap_manualInsert(CuTest* tc)
{
   apx_fileMap_t fileMap;
   apx_file_t *file1;
   apx_file_t *file2;
   apx_fileInfo_t fileInfo1;
   apx_fileInfo_t fileInfo2;
   adt_list_elem_t *iter;
   apx_file_t *pFile;

   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo1, 10000, 10, "file1.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo2, 2000, 50, "file2.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));

   file1 = apx_file_new(&fileInfo1);
   apx_fileMap_create(&fileMap);
   apx_fileMap_insertFile(&fileMap, file1);
   iter = adt_list_iter_first(&fileMap.fileList);
   pFile = (apx_file_t*) iter->pItem;
   CuAssertPtrEquals(tc, iter->pItem, file1 );
   CuAssertUIntEquals(tc, 10000, pFile->fileInfo.address);
   iter = adt_list_iter_next(iter);
   CuAssertPtrEquals(tc, 0, iter);

   file2 = apx_file_new(&fileInfo2);
   apx_fileMap_insertFile(&fileMap, file2);
   iter = adt_list_iter_first(&fileMap.fileList);
   CuAssertPtrNotNull(tc, iter);
   CuAssertPtrEquals(tc, iter->pItem, file2 );
   pFile = (apx_file_t*) iter->pItem;
   CuAssertUIntEquals(tc, 2000, pFile->fileInfo.address);
   iter = adt_list_iter_next(iter);
   CuAssertPtrNotNull(tc, iter);
   CuAssertPtrEquals(tc, iter->pItem, file1 );
   iter = adt_list_iter_next(iter);
   CuAssertPtrEquals(tc, 0, iter);
   apx_fileMap_destroy(&fileMap);
   apx_fileInfo_destroy(&fileInfo1);
   apx_fileInfo_destroy(&fileInfo2);

}

static void test_apx_fileMap_makeFileInfoArray(CuTest* tc)
{
   apx_fileInfo_t fileInfo1;
   apx_fileInfo_t fileInfo2;
   apx_fileMap_t fileMap;
   apx_file_t *file;
   apx_fileInfo_t *fileInfo;

   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo1, 1000, 10, "file1.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_fileInfo_create(&fileInfo2, 2000, 50, "file2.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL));

   apx_fileMap_create(&fileMap);

   apx_fileMap_insertFile(&fileMap, apx_file_new(&fileInfo1));
   apx_fileMap_insertFile(&fileMap, apx_file_new(&fileInfo2));

   adt_ary_t *array = apx_fileMap_makeFileInfoArray(&fileMap);
   CuAssertPtrNotNull(tc, array);
   CuAssertIntEquals(tc, 2, adt_ary_length(array));
   fileInfo = (apx_fileInfo_t*) adt_ary_value(array, 0);
   CuAssertStrEquals(tc, "file1.out", fileInfo->name);
   fileInfo = (apx_fileInfo_t*) adt_ary_value(array, 1);
   CuAssertStrEquals(tc, "file2.out", fileInfo->name);

   adt_ary_delete(array);
   apx_fileInfo_destroy(&fileInfo1);
   apx_fileInfo_destroy(&fileInfo2);
   apx_fileMap_destroy(&fileMap);

}

