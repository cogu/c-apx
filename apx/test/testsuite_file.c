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
static void test_file_type_to_extension(CuTest* tc);
static void test_detect_file_type_from_name(CuTest* tc);
static void test_determine_local_or_remote_file(CuTest* tc);
static void test_digest_data_is_copied_between_files(CuTest* tc);
static void test_less_than_function(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_file(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_file_type_to_extension);
   SUITE_ADD_TEST(suite, test_detect_file_type_from_name);
   SUITE_ADD_TEST(suite, test_determine_local_or_remote_file);
   SUITE_ADD_TEST(suite, test_digest_data_is_copied_between_files);
   SUITE_ADD_TEST(suite, test_less_than_function);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_file_type_to_extension(CuTest* tc)
{
   CuAssertStrEquals(tc, ".apx", apx_file_type_to_extension(APX_DEFINITION_FILE_TYPE));
   CuAssertStrEquals(tc, ".out", apx_file_type_to_extension(APX_PROVIDE_PORT_DATA_FILE_TYPE));
   CuAssertStrEquals(tc, ".in", apx_file_type_to_extension(APX_REQUIRE_PORT_DATA_FILE_TYPE));
   CuAssertStrEquals(tc, ".cout", apx_file_type_to_extension(APX_PROVIDE_PORT_COUNT_FILE_TYPE));
   CuAssertStrEquals(tc, ".cin", apx_file_type_to_extension(APX_REQUIRE_PORT_COUNT_FILE_TYPE));
   CuAssertStrEquals(tc, "", apx_file_type_to_extension(APX_UNKNOWN_FILE_TYPE));
   CuAssertStrEquals(tc, "", apx_file_type_to_extension(APX_USER_DEFINED_FILE_TYPE_BEGIN));
}

static void test_detect_file_type_from_name(CuTest* tc)
{
   rmf_fileInfo_t* file_info = rmf_fileInfo_make_fixed("TestNode.apx", 40u, RMF_INVALID_ADDRESS);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_t file;
   apx_file_create(&file, file_info);
   CuAssertUIntEquals(tc, APX_DEFINITION_FILE_TYPE, apx_file_get_apx_file_type(&file));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);

   file_info = rmf_fileInfo_make_fixed("TestNode.out", 1u, RMF_INVALID_ADDRESS);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_create(&file, file_info);
   CuAssertUIntEquals(tc, APX_PROVIDE_PORT_DATA_FILE_TYPE, apx_file_get_apx_file_type(&file));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);

   file_info = rmf_fileInfo_make_fixed("TestNode.in", 1u, RMF_INVALID_ADDRESS);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_create(&file, file_info);
   CuAssertUIntEquals(tc, APX_REQUIRE_PORT_DATA_FILE_TYPE, apx_file_get_apx_file_type(&file));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);

   file_info = rmf_fileInfo_make_fixed("TestNode.cout", 1u, RMF_INVALID_ADDRESS);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_create(&file, file_info);
   CuAssertUIntEquals(tc, APX_PROVIDE_PORT_COUNT_FILE_TYPE, apx_file_get_apx_file_type(&file));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);

   file_info = rmf_fileInfo_make_fixed("TestNode.cin", 1u, RMF_INVALID_ADDRESS);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_create(&file, file_info);
   CuAssertUIntEquals(tc, APX_REQUIRE_PORT_COUNT_FILE_TYPE, apx_file_get_apx_file_type(&file));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);
}

static void test_determine_local_or_remote_file(CuTest* tc)
{
   rmf_fileInfo_t* file_info = rmf_fileInfo_make_fixed("TestNode.apx", 40u, RMF_INVALID_ADDRESS);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_t file;
   apx_file_create(&file, file_info);
   CuAssertFalse(tc, apx_file_has_valid_address(&file));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);

   file_info = rmf_fileInfo_make_fixed("TestNode.apx", 40u, 0x1000u);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_create(&file, file_info);
   CuAssertTrue(tc, apx_file_has_valid_address(&file));
   CuAssertTrue(tc, apx_file_is_local(&file));
   CuAssertFalse(tc, apx_file_is_remote(&file));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);

   file_info = rmf_fileInfo_make_fixed("TestNode.apx", 40u, 0x1000u | RMF_REMOTE_ADDRESS_BIT);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_create(&file, file_info);
   CuAssertTrue(tc, apx_file_has_valid_address(&file));
   CuAssertFalse(tc, apx_file_is_local(&file));
   CuAssertTrue(tc, apx_file_is_remote(&file));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);


}

static void test_digest_data_is_copied_between_files(CuTest* tc)
{
   uint8_t digest[RMF_SHA256_SIZE];
   unsigned int i;
   for (i = 0; i < RMF_SHA256_SIZE; i++)
   {
      digest[i] = (uint8_t)i;
   }
   rmf_fileInfo_t* file_info = rmf_fileInfo_make_fixed_with_digest("TestNode.apx", 40u, 0x1000u, RMF_DIGEST_TYPE_SHA256, digest);
   CuAssertPtrNotNull(tc, file_info);
   apx_file_t file;
   apx_file_create(&file, file_info);
   CuAssertIntEquals(tc, 0, memcmp(digest, apx_file_get_digest_data(&file), RMF_SHA256_SIZE));
   rmf_fileInfo_delete(file_info);
   apx_file_destroy(&file);

}

static void test_less_than_function(CuTest* tc)
{
   rmf_fileInfo_t* file_info1 = rmf_fileInfo_make_fixed("TestNode1.apx", 40u, 0x10000);
   rmf_fileInfo_t* file_info2 = rmf_fileInfo_make_fixed("TestNode2.apx", 40u, 0x20000);
   CuAssertPtrNotNull(tc, file_info1);
   CuAssertPtrNotNull(tc, file_info2);
   apx_file_t file1;
   apx_file_t file2;
   apx_file_create(&file1, file_info1);
   apx_file_create(&file2, file_info2);
   CuAssertFalse(tc, apx_file_less_than(&file2, &file1));
   CuAssertTrue(tc, apx_file_less_than(&file1, &file2));
   rmf_fileInfo_delete(file_info1);
   rmf_fileInfo_delete(file_info2);
   apx_file_destroy(&file1);
   apx_file_destroy(&file2);
}
