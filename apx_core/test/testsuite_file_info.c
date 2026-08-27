//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/file_info.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_create_local_file(CuTest* tc);
static void test_name_ends_with(CuTest* tc);
static void test_base_name(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_file_info(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_local_file);
   SUITE_ADD_TEST(suite, test_name_ends_with);
   SUITE_ADD_TEST(suite, test_base_name);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_local_file(CuTest* tc)
{
   rmf_fileInfo_t* info = rmf_fileInfo_make_fixed("TestNode.apx", 40, RMF_INVALID_ADDRESS);
   CuAssertPtrNotNull(tc, info);
   CuAssertStrEquals(tc, "TestNode.apx", rmf_fileInfo_name(info));
   CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, rmf_fileInfo_rmf_file_type(info));
   CuAssertUIntEquals(tc, 40u, rmf_fileInfo_size(info));
   CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, rmf_fileInfo_address(info));
   CuAssertUIntEquals(tc, RMF_DIGEST_TYPE_NONE, rmf_fileInfo_digest_type(info));
   rmf_fileInfo_delete(info);
}

static void test_name_ends_with(CuTest* tc)
{
   rmf_fileInfo_t* info1 = rmf_fileInfo_make_fixed("TestNode.apx", 40, RMF_INVALID_ADDRESS);
   CuAssertTrue(tc, rmf_fileInfo_name_ends_with(info1, ".apx"));
   CuAssertFalse(tc, rmf_fileInfo_name_ends_with(info1, ".out"));
   CuAssertFalse(tc, rmf_fileInfo_name_ends_with(info1, ".in"));
   rmf_fileInfo_delete(info1);

   rmf_fileInfo_t* info2 = rmf_fileInfo_make_fixed("TestNode.out", 1, RMF_INVALID_ADDRESS);
   CuAssertFalse(tc, rmf_fileInfo_name_ends_with(info2, ".apx"));
   CuAssertTrue(tc, rmf_fileInfo_name_ends_with(info2, ".out"));
   CuAssertFalse(tc, rmf_fileInfo_name_ends_with(info2, ".in"));
   rmf_fileInfo_delete(info2);

   rmf_fileInfo_t* info3 = rmf_fileInfo_make_fixed("TestNode.in", 1, RMF_INVALID_ADDRESS);
   CuAssertFalse(tc, rmf_fileInfo_name_ends_with(info3, ".apx"));
   CuAssertFalse(tc, rmf_fileInfo_name_ends_with(info3, ".out"));
   CuAssertTrue(tc, rmf_fileInfo_name_ends_with(info3, ".in"));
   rmf_fileInfo_delete(info3);
}


static void test_base_name(CuTest* tc)
{
   rmf_fileInfo_t* info1 = rmf_fileInfo_make_fixed("TestNode.apx", 40, RMF_INVALID_ADDRESS);
   rmf_fileInfo_t* info2 = rmf_fileInfo_make_fixed("TestNode.out", 1, RMF_INVALID_ADDRESS);
   rmf_fileInfo_t* info3 = rmf_fileInfo_make_fixed("TestNode.in", 1, RMF_INVALID_ADDRESS);
   char* base_name1 = rmf_fileInfo_base_name(info1);
   char* base_name2 = rmf_fileInfo_base_name(info2);
   char* base_name3 = rmf_fileInfo_base_name(info3);
   CuAssertStrEquals(tc, "TestNode", base_name1);
   CuAssertStrEquals(tc, "TestNode", base_name2);
   CuAssertStrEquals(tc, "TestNode", base_name3);
   free(base_name1);
   free(base_name2);
   free(base_name3);
   rmf_fileInfo_delete(info1);
   rmf_fileInfo_delete(info2);
   rmf_fileInfo_delete(info3);
}
