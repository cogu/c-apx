//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_file2.h"
#include "apx_fileManagerLocal.h"
#include "ApxNode_TestNode1.h"
#include "adt_bytearray.h"
#include "rmf.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerLocal_create(CuTest* tc);
static void test_apx_fileManagerLocal_attachFile(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_fileManagerLocal(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_fileManagerLocal_create);
   SUITE_ADD_TEST(suite, test_apx_fileManagerLocal_attachFile);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerLocal_create(CuTest* tc)
{
   apx_fileManagerShared_t shared;
   apx_fileManagerLocal_t local;
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_create(&shared));
   apx_fileManagerLocal_create(&local, &shared);
   CuAssertIntEquals(tc, 0, apx_fileManagerLocal_getNumFiles(&local));
   apx_fileManagerLocal_destroy(&local);
   apx_fileManagerShared_destroy(&shared);
}

static void test_apx_fileManagerLocal_attachFile(CuTest* tc)
{
   apx_nodeData_t *nodeData;
   apx_fileManagerShared_t shared;
   apx_fileManagerLocal_t local;
   apx_file2_t *definitionFile;
   apx_file2_t *outDataFile;
   ApxNode_Init_TestNode1();
   nodeData = ApxNode_GetNodeData_TestNode1();
   apx_fileManagerShared_create(&shared);
   apx_fileManagerLocal_create(&local, &shared);
   CuAssertIntEquals(tc, 0, apx_fileManagerLocal_getNumFiles(&local));
   definitionFile = apx_nodeData_newLocalDefinitionFile(nodeData);
   outDataFile = apx_nodeData_newLocalOutPortDataFile(nodeData);
   CuAssertPtrNotNull(tc, definitionFile);
   CuAssertPtrNotNull(tc, outDataFile);
   CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, definitionFile->fileInfo.address);
   CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, outDataFile->fileInfo.address);
   apx_fileManagerLocal_attachFile(&local, definitionFile, NULL);
   apx_fileManagerLocal_attachFile(&local, outDataFile, NULL);
   CuAssertIntEquals(tc, 2, apx_fileManagerLocal_getNumFiles(&local));
   CuAssertUIntEquals(tc, 0x4000000, definitionFile->fileInfo.address);
   CuAssertUIntEquals(tc, 0, outDataFile->fileInfo.address);
   apx_fileManagerLocal_destroy(&local);
   apx_fileManagerShared_destroy(&shared);
}
