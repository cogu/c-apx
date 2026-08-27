//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/file_manager_shared.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_auto_assign_local_address(CuTest* tc);
static void test_create_port_file_after_definition_file(CuTest* tc);
static void test_add_too_many_files_of_same_type(CuTest* tc);
static void test_find_file_by_address(CuTest* tc);
static void test_find_file_by_name(CuTest* tc);
static void test_find_remote_file_by_local_address(CuTest* tc);

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

   SUITE_ADD_TEST(suite, test_auto_assign_local_address);
   SUITE_ADD_TEST(suite, test_create_port_file_after_definition_file);
   SUITE_ADD_TEST(suite, test_add_too_many_files_of_same_type);
   SUITE_ADD_TEST(suite, test_find_file_by_address);
   SUITE_ADD_TEST(suite, test_find_file_by_name);
   SUITE_ADD_TEST(suite, test_find_remote_file_by_local_address);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_auto_assign_local_address(CuTest* tc)
{
   apx_fileMap_t map;
   apx_fileMap_create(&map, false);
   rmf_fileInfo_t* info = rmf_fileInfo_make_fixed("TestNode1.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, 0u, apx_file_get_address(file));
   info = rmf_fileInfo_make_fixed("TestNode2.out", 10u, RMF_INVALID_ADDRESS);
   file = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_PORT_DATA_ADDRESS_ALIGNMENT, apx_file_get_address(file));
   info = rmf_fileInfo_make_fixed("TestNode2.out", 10u, RMF_INVALID_ADDRESS);
   file = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_PORT_DATA_ADDRESS_ALIGNMENT*2, apx_file_get_address(file));
   info = rmf_fileInfo_make_fixed("TestNode1.apx", 100u, RMF_INVALID_ADDRESS);
   file = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_DEFINITION_ADDRESS_START, apx_file_get_address(file));
   info = rmf_fileInfo_make_fixed("TestNode2.apx", 100u, RMF_INVALID_ADDRESS);
   file = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_DEFINITION_ADDRESS_START + APX_DEFINITION_ADDRESS_ALIGNMENT, apx_file_get_address(file));
   info = rmf_fileInfo_make_fixed("TestNode4.out", 10u, RMF_INVALID_ADDRESS);
   file = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_PORT_DATA_ADDRESS_ALIGNMENT * 3, apx_file_get_address(file));
   apx_fileMap_destroy(&map);
}

static void test_create_port_file_after_definition_file(CuTest* tc)
{
   apx_fileMap_t map;
   apx_fileMap_create(&map, false);
   rmf_fileInfo_t* info = rmf_fileInfo_make_fixed("TestNode1.apx", 100u, RMF_INVALID_ADDRESS);
   apx_file_t const* file1 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file1);
   info = rmf_fileInfo_make_fixed("TestNode1.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file2 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file2);
   adt_list_elem_t* next = adt_list_iter_first(apx_fileMap_get_list(&map));
   CuAssertConstPtrEquals(tc, file2, next->pItem);
   next = adt_list_iter_next(next);
   CuAssertConstPtrEquals(tc, file1, next->pItem);
   apx_fileMap_destroy(&map);
}

static void test_add_too_many_files_of_same_type(CuTest* tc)
{
   apx_fileMap_t map;
   apx_fileMap_create(&map, false);

   rmf_fileInfo_t* info = rmf_fileInfo_make_fixed("TestNode1.apx", 10000u, RMF_INVALID_ADDRESS);
   apx_file_t const* file = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file);
   CuAssertUIntEquals(tc, APX_DEFINITION_ADDRESS_START, apx_file_get_address(file));
   uint32_t const file_size = 1024 * 1024;
   uint32_t expected_address = 0u;
   unsigned int i;
   for (i = 0; i < 64; i++)
   {
      char name[64];
      sprintf(name, "TestNode%u.out", i);
      info = rmf_fileInfo_make_fixed(name, file_size, RMF_INVALID_ADDRESS);
      file = apx_fileMap_create_file(&map, info);
      CuAssertPtrNotNull(tc, file);
      rmf_fileInfo_delete(info);
      CuAssertUIntEquals(tc, expected_address, apx_file_get_address(file));
      expected_address += file_size;
   }
   info = rmf_fileInfo_make_fixed("TestNode64.out", 1u, RMF_INVALID_ADDRESS);
   apx_file_t const* file2 = apx_fileMap_create_file(&map, info);
   CuAssertConstPtrEquals(tc, NULL, file2);
   rmf_fileInfo_delete(info);
   apx_fileMap_destroy(&map);
}

static void test_find_file_by_address(CuTest* tc)
{
   apx_fileMap_t map;
   apx_fileMap_create(&map, false);

   rmf_fileInfo_t* info = rmf_fileInfo_make_fixed("TestNode1.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file1 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file1);
   CuAssertUIntEquals(tc, 0u, apx_file_get_address(file1));
   info = rmf_fileInfo_make_fixed("TestNode2.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file2 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file2);
   CuAssertUIntEquals(tc, 1024u, apx_file_get_address(file2));
   info = rmf_fileInfo_make_fixed("TestNode3.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file3 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file3);
   CuAssertUIntEquals(tc, 2048u, apx_file_get_address(file3));
   info = rmf_fileInfo_make_fixed("TestNode1.apx", 100u, RMF_INVALID_ADDRESS);
   apx_file_t const* file4 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file4);
   info = rmf_fileInfo_make_fixed("TestNode2.apx", 100u, RMF_INVALID_ADDRESS);
   apx_file_t const* file5 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file5);
   apx_file_t const* file = apx_fileMap_find_by_address(&map, 2048);
   CuAssertConstPtrEquals(tc, file3, file);
   file = apx_fileMap_find_by_address(&map, 2048); //This should used cached value
   CuAssertConstPtrEquals(tc, file3, file);
   file = apx_fileMap_find_by_address(&map, 1024);
   CuAssertConstPtrEquals(tc, file2, file);
   apx_fileMap_destroy(&map);
}

static void test_find_file_by_name(CuTest* tc)
{
   apx_fileMap_t map;
   apx_fileMap_create(&map, false);

   rmf_fileInfo_t* info = rmf_fileInfo_make_fixed("TestNode1.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file1 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file1);
   CuAssertUIntEquals(tc, 0u, apx_file_get_address(file1));
   info = rmf_fileInfo_make_fixed("TestNode2.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file2 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file2);
   CuAssertUIntEquals(tc, 1024u, apx_file_get_address(file2));
   info = rmf_fileInfo_make_fixed("TestNode3.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file3 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file3);
   CuAssertUIntEquals(tc, 2048u, apx_file_get_address(file3));
   info = rmf_fileInfo_make_fixed("TestNode1.apx", 100u, RMF_INVALID_ADDRESS);
   apx_file_t const* file4 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file4);
   info = rmf_fileInfo_make_fixed("TestNode2.apx", 100u, RMF_INVALID_ADDRESS);
   apx_file_t const* file5 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file5);
   apx_file_t const* file = apx_fileMap_find_by_name(&map, "TestNode1.apx");
   CuAssertConstPtrEquals(tc, file4, file);
   file = apx_fileMap_find_by_name(&map, "TestNode1.out");
   CuAssertConstPtrEquals(tc, file1, file);
   apx_fileMap_destroy(&map);
}

static void test_find_remote_file_by_local_address(CuTest* tc)
{
   apx_fileMap_t map;
   apx_fileMap_create(&map, true);

   rmf_fileInfo_t* info = rmf_fileInfo_make_fixed("TestNode1.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file1 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file1);
   CuAssertUIntEquals(tc, RMF_REMOTE_ADDRESS_BIT, apx_file_get_address(file1));
   info = rmf_fileInfo_make_fixed("TestNode2.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file2 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file2);
   CuAssertUIntEquals(tc, RMF_REMOTE_ADDRESS_BIT | 1024u, apx_file_get_address(file2));
   info = rmf_fileInfo_make_fixed("TestNode3.out", 10u, RMF_INVALID_ADDRESS);
   apx_file_t const* file3 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file3);
   CuAssertUIntEquals(tc, RMF_REMOTE_ADDRESS_BIT | 2048u, apx_file_get_address(file3));
   info = rmf_fileInfo_make_fixed("TestNode1.apx", 100u, RMF_INVALID_ADDRESS);
   apx_file_t const* file4 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file4);
   info = rmf_fileInfo_make_fixed("TestNode2.apx", 100u, RMF_INVALID_ADDRESS);
   apx_file_t const* file5 = apx_fileMap_create_file(&map, info);
   rmf_fileInfo_delete(info);
   CuAssertPtrNotNull(tc, file5);
   apx_file_t const* file = apx_fileMap_find_by_address(&map, 2048);
   CuAssertConstPtrEquals(tc, file3, file);
   file = apx_fileMap_find_by_address(&map, 1024);
   CuAssertConstPtrEquals(tc, file2, file);
   file = apx_fileMap_find_by_address(&map, 9u);
   CuAssertConstPtrEquals(tc, file1, file);
   apx_fileMap_destroy(&map);
}
