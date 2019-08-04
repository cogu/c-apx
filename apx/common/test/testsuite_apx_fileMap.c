//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_fileMap.h"
#include "apx_nodeData.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_fileMap_create(CuTest* tc);
static void test_apx_fileMap_autoInsert(CuTest* tc);
static void test_apx_fileMap_manualInsert(CuTest* tc);
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

   SUITE_ADD_TEST(suite, test_apx_fileMap_create);
   SUITE_ADD_TEST(suite, test_apx_fileMap_autoInsert);
   SUITE_ADD_TEST(suite, test_apx_fileMap_manualInsert);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileMap_create(CuTest* tc)
{
   apx_fileMap_t fileMap;
   apx_fileMap_create(&fileMap);
   apx_fileMap_destroy(&fileMap);
}

/**
 * test that the apx_fileMap automatically assigns addresses correctly when inserted
 */
static void test_apx_fileMap_autoInsert(CuTest* tc)
{
   apx_fileMap_t fileMap;
   apx_nodeData_t nodeData1;
   apx_nodeData_t nodeData2;
   apx_nodeData_t nodeData3;
   apx_file2_t *file1;
   apx_file2_t *file2;
   apx_file2_t *file3;
   apx_file2_t *file4;
   apx_file2_t *file5;
   apx_file2_t *file6;
   uint8_t out1[256];
   uint8_t out2[1328];
   uint8_t out3[256];
   uint8_t def1[256];
   uint8_t def2[256];
   uint8_t def3[256];
   adt_list_elem_t *iter;
   apx_file2_t *pFile;

   apx_fileMap_create(&fileMap);
   apx_nodeData_create(&nodeData1, "testnode1", def1, sizeof(def1) , 0, 0, 0, out1, 0, sizeof(out1));
   apx_nodeData_create(&nodeData2, "testnode2", def2, sizeof(def2) , 0, 0, 0, out2, 0, sizeof(out2));
   apx_nodeData_create(&nodeData3, "testnode3", def3, sizeof(def3) , 0, 0, 0, out3, 0, sizeof(out3));
   file1 = apx_nodeData_newLocalOutPortDataFile(&nodeData1);
   file2 = apx_nodeData_newLocalDefinitionFile(&nodeData1);
   file3 = apx_nodeData_newLocalOutPortDataFile(&nodeData2);
   file4 = apx_nodeData_newLocalDefinitionFile(&nodeData2);
   file5 = apx_nodeData_newLocalOutPortDataFile(&nodeData3);
   file6 = apx_nodeData_newLocalDefinitionFile(&nodeData3);
   CuAssertPtrNotNull(tc, file1);
   CuAssertPtrNotNull(tc, file2);
   CuAssertPtrNotNull(tc, file3);
   CuAssertPtrNotNull(tc, file4);
   CuAssertPtrNotNull(tc, file5);
   CuAssertPtrNotNull(tc, file6);
   apx_fileMap_insertFile(&fileMap, file2);
   apx_fileMap_insertFile(&fileMap, file1);
   apx_fileMap_insertFile(&fileMap, file4);
   apx_fileMap_insertFile(&fileMap, file3);
   apx_fileMap_insertFile(&fileMap, file6);
   apx_fileMap_insertFile(&fileMap, file5);
   iter = adt_list_iter_first(&fileMap.fileList);
   pFile = (apx_file2_t*) iter->pItem;
   CuAssertPtrEquals(tc, iter->pItem, file1 );
   CuAssertStrEquals(tc, "testnode1.out", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 0, pFile->fileInfo.address);
   iter = adt_list_iter_next(iter);
   pFile = (apx_file2_t*) iter->pItem;
   CuAssertStrEquals(tc, "testnode2.out", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 1024, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file3 );
   iter = adt_list_iter_next(iter);
   pFile = (apx_file2_t*) iter->pItem;
   CuAssertStrEquals(tc, "testnode3.out", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 1024*3, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file5 );
   iter = adt_list_iter_next(iter);
   pFile = (apx_file2_t*) iter->pItem;
   CuAssertStrEquals(tc, "testnode1.apx", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 64*1024*1024, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file2 );
   iter = adt_list_iter_next(iter);
   pFile = (apx_file2_t*) iter->pItem;
   CuAssertStrEquals(tc, "testnode2.apx", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 65*1024*1024, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file4 );
   iter = adt_list_iter_next(iter);
   pFile = (apx_file2_t*) iter->pItem;
   CuAssertStrEquals(tc, "testnode3.apx", pFile->fileInfo.name);
   CuAssertUIntEquals(tc, 66*1024*1024, pFile->fileInfo.address);
   CuAssertPtrEquals(tc, iter->pItem, file6 );
   apx_fileMap_destroy(&fileMap);

}

static void test_apx_fileMap_manualInsert(CuTest* tc)
{
   apx_fileMap_t fileMap;
   apx_file2_t *file1;
   apx_file2_t *file2;
   rmf_fileInfo_t info;
   adt_list_elem_t *iter;
   apx_file2_t *pFile;

   rmf_fileInfo_create(&info, "file1.out", 10000, 10, RMF_FILE_TYPE_FIXED);
   file1 = apx_file2_newLocal(&info, NULL);
   apx_fileMap_create(&fileMap);
   apx_fileMap_insertFile(&fileMap, file1);
   iter = adt_list_iter_first(&fileMap.fileList);
   pFile = (apx_file2_t*) iter->pItem;
   CuAssertPtrEquals(tc, iter->pItem, file1 );
   CuAssertUIntEquals(tc, 10000, pFile->fileInfo.address);
   iter = adt_list_iter_next(iter);
   CuAssertPtrEquals(tc, 0, iter);

   rmf_fileInfo_create(&info, "file2.out", 2000, 50, RMF_FILE_TYPE_FIXED);
   file2 = apx_file2_newLocal(&info, NULL);
   apx_fileMap_insertFile(&fileMap, file2);
   iter = adt_list_iter_first(&fileMap.fileList);
   CuAssertPtrNotNull(tc, iter);
   CuAssertPtrEquals(tc, iter->pItem, file2 );
   pFile = (apx_file2_t*) iter->pItem;
   CuAssertUIntEquals(tc, 2000, pFile->fileInfo.address);
   iter = adt_list_iter_next(iter);
   CuAssertPtrNotNull(tc, iter);
   CuAssertPtrEquals(tc, iter->pItem, file1 );
   iter = adt_list_iter_next(iter);
   CuAssertPtrEquals(tc, 0, iter);
   apx_fileMap_destroy(&fileMap);
}
