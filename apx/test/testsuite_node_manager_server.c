//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "apx/node_manager.h"
#include "apx/node_data.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_empty_definition_data_is_initialized_on_new_instance(CuTest *tc);
static void test_port_data_created_after_definition_data_is_written(CuTest* tc);
static void test_checksum_data_is_copied_from_file_info(CuTest* tc);
static void test_port_signature_uint8(CuTest* tc);
static void test_port_signature_uint8_with_limits(CuTest* tc);
static void test_port_signature_uint8_array(CuTest* tc);
static void test_port_signature_dynamic_uint8_array(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeManager_server_mode(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_empty_definition_data_is_initialized_on_new_instance);
   SUITE_ADD_TEST(suite, test_port_data_created_after_definition_data_is_written);
   SUITE_ADD_TEST(suite, test_checksum_data_is_copied_from_file_info);
   SUITE_ADD_TEST(suite, test_port_signature_uint8);
   SUITE_ADD_TEST(suite, test_port_signature_uint8_with_limits);
   SUITE_ADD_TEST(suite, test_port_signature_uint8_array);
   SUITE_ADD_TEST(suite, test_port_signature_dynamic_uint8_array);

   return suite;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_empty_definition_data_is_initialized_on_new_instance(CuTest *tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"U16Signal\"S:=65535\n"
      "P\"U8Signal1\"C:=7\n"
      "P\"U8Signal2\"C:=15\n"
      "R\"U8Signal3\"C:=7\n"
      "R\"U32Signal\"L:=0\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   apx_nodeManager_t* manager = apx_nodeManager_new(APX_SERVER_MODE);
   rmf_fileInfo_t* file_info = rmf_fileInfo_make_fixed("TestNode.apx", (uint32_t)definition_size, APX_DEFINITION_ADDRESS_START);
   CuAssertPtrNotNull(tc, file_info);
   bool file_open_request = false;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_init_node_from_file_info(manager, file_info, &file_open_request));
   CuAssertTrue(tc, file_open_request);
   rmf_fileInfo_delete(file_info);
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);

   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, definition_size, apx_nodeData_definition_data_size(node_data));
   uint8_t* snapshot = apx_nodeData_take_definition_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, snapshot);
   apx_size_t i;
   for (i = 0; i < definition_size; i++)
   {
      CuAssertUIntEquals(tc, 0u, snapshot[i]);
   }
   free(snapshot);
   apx_nodeManager_delete(manager);
}

static void test_port_data_created_after_definition_data_is_written(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"U16Signal\"S:=65535\n"
      "P\"U8Signal1\"C:=7\n"
      "P\"U8Signal2\"C:=15\n"
      "R\"U8Signal3\"C:=7\n"
      "R\"U32Signal\"L:=0\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   apx_nodeManager_t* manager = apx_nodeManager_new(APX_SERVER_MODE);
   rmf_fileInfo_t* file_info = rmf_fileInfo_make_fixed("TestNode.apx", (uint32_t)definition_size, APX_DEFINITION_ADDRESS_START);
   CuAssertPtrNotNull(tc, file_info);
   bool file_open_request = false;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_init_node_from_file_info(manager, file_info, &file_open_request));
   CuAssertTrue(tc, file_open_request);
   rmf_fileInfo_delete(file_info);
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);

   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, (unsigned int)definition_size, apx_nodeData_definition_data_size(node_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_write_definition_data(node_data, 0u, (uint8_t const*)apx_text, definition_size));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node_from_data(manager, node_instance));
   CuAssertUIntEquals(tc, 3u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 2u, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, UINT16_SIZE + UINT8_SIZE + UINT8_SIZE, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT32_SIZE, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   uint8_t* require_port_data = apx_nodeData_take_require_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertPtrNotNull(tc, require_port_data);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[0]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[1]);
   CuAssertUIntEquals(tc, 7u, provide_port_data[2]);
   CuAssertUIntEquals(tc, 15u, provide_port_data[3]);
   CuAssertUIntEquals(tc, 7u, require_port_data[0]);
   CuAssertUIntEquals(tc, 0u, require_port_data[1]);
   CuAssertUIntEquals(tc, 0u, require_port_data[2]);
   CuAssertUIntEquals(tc, 0u, require_port_data[3]);
   CuAssertUIntEquals(tc, 0u, require_port_data[4]);
   free(provide_port_data);
   free(require_port_data);
   apx_nodeManager_delete(manager);
}

static void test_checksum_data_is_copied_from_file_info(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"U16Signal\"S:=65535\n";

   apx_size_t definition_size = (apx_size_t)strlen(apx_text);
   apx_nodeManager_t* manager = apx_nodeManager_new(APX_SERVER_MODE);
   uint8_t const checksum_data[RMF_SHA256_SIZE] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
      17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32 };
   rmf_digestType_t const checksum_type = RMF_DIGEST_TYPE_SHA256;
   rmf_fileInfo_t* file_info = rmf_fileInfo_make_fixed_with_digest("TestNode.apx", (uint32_t)definition_size, APX_DEFINITION_ADDRESS_START,
      checksum_type, checksum_data);
   CuAssertPtrNotNull(tc, file_info);
   bool file_open_request = false;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_init_node_from_file_info(manager, file_info, &file_open_request));
   rmf_fileInfo_delete(file_info);
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);

   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, (unsigned int)definition_size, apx_nodeData_definition_data_size(node_data));
   CuAssertUIntEquals(tc, checksum_type, apx_nodeData_get_checksum_type(node_data));
   uint8_t const* stored_checksum_data = apx_nodeData_get_checksum_data(node_data);
   CuAssertIntEquals(tc, 0, memcmp(checksum_data, stored_checksum_data, RMF_SHA256_SIZE));
   apx_nodeManager_delete(manager);
}

static void test_port_signature_uint8(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"TestPort\"C:=255\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   apx_portInstance_t* port = apx_nodeInstance_get_provide_port(node_instance, 0);
   CuAssertPtrNotNull(tc, port);
   bool has_dynamic_data = false;
   CuAssertStrEquals(tc, "\"TestPort\"C", apx_portInstance_get_port_signature(port, &has_dynamic_data));
   apx_nodeManager_delete(manager);
}

static void test_port_signature_uint8_with_limits(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"TestPort\"C(0,7):=7\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   apx_portInstance_t* port = apx_nodeInstance_get_provide_port(node_instance, 0);
   CuAssertPtrNotNull(tc, port);
   bool has_dynamic_data = false;
   CuAssertStrEquals(tc, "\"TestPort\"C(0,7)", apx_portInstance_get_port_signature(port, &has_dynamic_data));
   apx_nodeManager_delete(manager);
}

static void test_port_signature_uint8_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"TestPort\"C[10]\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   apx_portInstance_t* port = apx_nodeInstance_get_provide_port(node_instance, 0);
   CuAssertPtrNotNull(tc, port);
   bool has_dynamic_data = false;
   CuAssertStrEquals(tc, "\"TestPort\"C[10]", apx_portInstance_get_port_signature(port, &has_dynamic_data));
   apx_nodeManager_delete(manager);
}

static void test_port_signature_dynamic_uint8_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"TestPort\"C[100*]:={}\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, manager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   apx_portInstance_t* port = apx_nodeInstance_get_provide_port(node_instance, 0);
   CuAssertPtrNotNull(tc, port);
   bool has_dynamic_data = false;
   CuAssertStrEquals(tc, "\"TestPort\"C[*]", apx_portInstance_get_port_signature(port, &has_dynamic_data));
   apx_nodeManager_delete(manager);
}
