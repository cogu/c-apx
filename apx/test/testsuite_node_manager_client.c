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
static void test_node_containing_unsigned_types_without_limits(CuTest *tc);
static void test_node_containing_unsigned_types_with_limits(CuTest* tc);
static void test_node_containing_out_of_range_range_init_value(CuTest* tc);
static void test_node_containing_signed_types_without_limits(CuTest* tc);
static void test_node_containing_signed_types_with_limits(CuTest* tc);
static void test_node_containing_signed_array_types(CuTest* tc);
static void test_node_containing_string_type(CuTest* tc);
static void test_node_containing_record_type(CuTest* tc);
static void test_provide_port_containing_type_reference(CuTest* tc);
static void test_provide_port_containing_array_of_records(CuTest* tc);
static void test_provide_port_containing_record_inside_record(CuTest* tc);
static void test_node_instance_by_name(CuTest* tc);
static void test_client_node_require_port_byte_map_creation(CuTest* tc);



//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeManager_client_mode(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_node_containing_unsigned_types_without_limits);
   SUITE_ADD_TEST(suite, test_node_containing_unsigned_types_with_limits);
   SUITE_ADD_TEST(suite, test_node_containing_out_of_range_range_init_value);
   SUITE_ADD_TEST(suite, test_node_containing_signed_types_without_limits);
   SUITE_ADD_TEST(suite, test_node_containing_signed_types_with_limits);
   SUITE_ADD_TEST(suite, test_node_containing_signed_array_types);
   SUITE_ADD_TEST(suite, test_node_containing_string_type);
   SUITE_ADD_TEST(suite, test_node_containing_record_type);
   SUITE_ADD_TEST(suite, test_provide_port_containing_type_reference);
   SUITE_ADD_TEST(suite, test_provide_port_containing_array_of_records);
   SUITE_ADD_TEST(suite, test_provide_port_containing_record_inside_record);
   SUITE_ADD_TEST(suite, test_node_instance_by_name);
   SUITE_ADD_TEST(suite, test_client_node_require_port_byte_map_creation);

   return suite;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_node_containing_unsigned_types_without_limits(CuTest *tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"U16Signal\"S:=65535\n"
      "P\"U8Signal1\"C:=7\n"
      "P\"U8Signal2\"C:=15\n"
      "R\"U8Signal3\"C:=7\n"
      "R\"U32Signal\"L:=0\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
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

static void test_node_containing_unsigned_types_with_limits(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"U16Signal\"S:=65535\n"
      "P\"U8Signal1\"C(0,7):=7\n"
      "P\"U8Signal2\"C(0,15):=15\n"
      "R\"U8Signal3\"C(0,7):=7\n"
      "R\"U32Signal\"L:=0\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
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

static void test_node_containing_out_of_range_range_init_value(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"CabTiltLockWarning\"C(0,7):=15\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_nodeManager_build_node(manager, apx_text));
   apx_nodeManager_delete(manager);
}

static void test_node_containing_signed_types_without_limits(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"S8Value\"c:=1\n"
      "P\"S16Value\"s:=2\n"
      "P\"S32Value\"l:=3\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 3u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, INT8_SIZE + INT16_SIZE + INT32_SIZE, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertUIntEquals(tc, 1u, provide_port_data[0]);
   CuAssertUIntEquals(tc, 2u, provide_port_data[1]);
   CuAssertUIntEquals(tc, 0u, provide_port_data[2]);
   CuAssertUIntEquals(tc, 3u, provide_port_data[3]);
   CuAssertUIntEquals(tc, 0u, provide_port_data[4]);
   CuAssertUIntEquals(tc, 0u, provide_port_data[5]);
   CuAssertUIntEquals(tc, 0u, provide_port_data[6]);
   free(provide_port_data);

   apx_nodeManager_delete(manager);
}

static void test_node_containing_signed_types_with_limits(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"S8Value\"c(-10,10):=-1\n"
      "P\"S16Value\"s(-1000, 1000):=0\n"
      "P\"S32Value\"l:=-1\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 3u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, INT8_SIZE + INT16_SIZE + INT32_SIZE, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[0]);
   CuAssertUIntEquals(tc, 0u, provide_port_data[1]);
   CuAssertUIntEquals(tc, 0u, provide_port_data[2]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[3]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[4]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[5]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[6]);
   free(provide_port_data);

   apx_nodeManager_delete(manager);
}

static void test_node_containing_signed_array_types(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"U8Array\"c(-10, 10)[4]:={-10, -10, -10, -10}\n"
      "P\"U16Array\"s[3]:={-1, -1, 0}\n"
      "P\"U32Array\"l[2]:={0, -1}\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 3u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, INT8_SIZE * 4 + INT16_SIZE * 3 + INT32_SIZE * 2, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertUIntEquals(tc, 0xf6u, provide_port_data[0]);
   CuAssertUIntEquals(tc, 0xf6u, provide_port_data[1]);
   CuAssertUIntEquals(tc, 0xf6u, provide_port_data[2]);
   CuAssertUIntEquals(tc, 0xf6u, provide_port_data[3]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[4]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[5]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[6]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[7]);
   CuAssertUIntEquals(tc, 0x00u, provide_port_data[8]);
   CuAssertUIntEquals(tc, 0x00u, provide_port_data[9]);
   CuAssertUIntEquals(tc, 0x00u, provide_port_data[10]);
   CuAssertUIntEquals(tc, 0x00u, provide_port_data[11]);
   CuAssertUIntEquals(tc, 0x00u, provide_port_data[12]);
   CuAssertUIntEquals(tc, 0x00u, provide_port_data[13]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[14]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[15]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[16]);
   CuAssertUIntEquals(tc, 0xffu, provide_port_data[17]);
   free(provide_port_data);
   apx_nodeManager_delete(manager);
}

static void test_node_containing_string_type(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"String\"a[12]:=\"Hello World!\"\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 1u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, CHAR_SIZE * 12, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertUIntEquals(tc, 'H', provide_port_data[0]);
   CuAssertUIntEquals(tc, 'e', provide_port_data[1]);
   CuAssertUIntEquals(tc, 'l', provide_port_data[2]);
   CuAssertUIntEquals(tc, 'l', provide_port_data[3]);
   CuAssertUIntEquals(tc, 'o', provide_port_data[4]);
   CuAssertUIntEquals(tc, ' ', provide_port_data[5]);
   CuAssertUIntEquals(tc, 'W', provide_port_data[6]);
   CuAssertUIntEquals(tc, 'o', provide_port_data[7]);
   CuAssertUIntEquals(tc, 'r', provide_port_data[8]);
   CuAssertUIntEquals(tc, 'l', provide_port_data[9]);
   CuAssertUIntEquals(tc, 'd', provide_port_data[10]);
   CuAssertUIntEquals(tc, '!', provide_port_data[11]);
   free(provide_port_data);
   apx_nodeManager_delete(manager);
}

static void test_node_containing_record_type(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"RecordPortOut\"{\"First\"C\"Second\"S}:={0x12, 0x1234}\n"
      "R\"RecordPortIn\"{\"First\"C\"Second\"S}:={0x12, 0x1234}\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 1u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 1u, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT16_SIZE, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT16_SIZE, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   uint8_t* require_port_data = apx_nodeData_take_require_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertPtrNotNull(tc, require_port_data);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[0]);
   CuAssertUIntEquals(tc, 0x34u, provide_port_data[1]);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[2]);
   CuAssertUIntEquals(tc, 0x12u, require_port_data[0]);
   CuAssertUIntEquals(tc, 0x34u, require_port_data[1]);
   CuAssertUIntEquals(tc, 0x12u, require_port_data[2]);
   free(provide_port_data);
   free(require_port_data);
   apx_nodeManager_delete(manager);
}

static void test_provide_port_containing_type_reference(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"VehicleSpeed_T\"S:RS(0, 0xFDFF, 0, 1, 64, \"km/h\"), VT(0xFE00, 0xFEFF, \"Error\"), VT(0xFF00, 0xFFFF, \"NotAvailable\")\n"
      "P\"VehicleSpeed\"T[0]\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 1u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, UINT16_SIZE, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertUIntEquals(tc, 0x00u, provide_port_data[0]);
   CuAssertUIntEquals(tc, 0x00u, provide_port_data[1]);
   apx_portInstance_t* port = apx_nodeInstance_get_provide_port(node_instance, 0u);
   CuAssertPtrNotNull(tc, port);
   CuAssertUIntEquals(tc, 3u, apx_portInstance_get_computation_list_length(port));
   CuAssertUIntEquals(tc, 0u, apx_portInstance_get_computation_list_id(port));
   apx_computation_t const* computation = apx_portInstance_get_computation(port, 0);
   CuAssertPtrNotNull(tc, computation);
   CuAssertFalse(tc, apx_computation_is_range_signed(computation));
   CuAssertUIntEquals(tc, 0u, apx_computation_get_lower_limit_unsigned(computation));
   CuAssertUIntEquals(tc, 0xFDFF, apx_computation_get_upper_limit_unsigned(computation));
   CuAssertUIntEquals(tc, APX_COMPUTATION_TYPE_RATIONAL_SCALING, apx_computation_type(computation));
   apx_rationalScaling_t const* scaling = (apx_rationalScaling_t const*)computation;
   CuAssertDblEquals(tc, 0.0, apx_rationalScaling_offset(scaling), 0.001);
   CuAssertIntEquals(tc, 1u, apx_rationalScaling_numerator(scaling));
   CuAssertIntEquals(tc, 64u, apx_rationalScaling_denominator(scaling));
   CuAssertStrEquals(tc, "km/h", apx_rationalScaling_unit(scaling));
   computation = apx_portInstance_get_computation(port, 1);
   CuAssertPtrNotNull(tc, computation);
   CuAssertFalse(tc, apx_computation_is_range_signed(computation));
   CuAssertUIntEquals(tc, 0xFE00, apx_computation_get_lower_limit_unsigned(computation));
   CuAssertUIntEquals(tc, 0xFEFF, apx_computation_get_upper_limit_unsigned(computation));
   CuAssertUIntEquals(tc, APX_COMPUTATION_TYPE_VALUE_TABLE, apx_computation_type(computation));
   apx_valueTable_t const* value_table = (apx_valueTable_t const*)computation;
   CuAssertStrEquals(tc, "Error", apx_valueTable_get_value_cstr(value_table, 0));
   computation = apx_portInstance_get_computation(port, 2);
   CuAssertPtrNotNull(tc, computation);
   CuAssertFalse(tc, apx_computation_is_range_signed(computation));
   CuAssertUIntEquals(tc, 0xFF00, apx_computation_get_lower_limit_unsigned(computation));
   CuAssertUIntEquals(tc, 0xFFFF, apx_computation_get_upper_limit_unsigned(computation));
   CuAssertUIntEquals(tc, APX_COMPUTATION_TYPE_VALUE_TABLE, apx_computation_type(computation));
   value_table = (apx_valueTable_t const*)computation;
   CuAssertStrEquals(tc, "NotAvailable", apx_valueTable_get_value_cstr(value_table, 0));

   free(provide_port_data);
   apx_nodeManager_delete(manager);
}

static void test_provide_port_containing_array_of_records(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"ArrayPortOut\"{\"First\"S\"Second\"C}[2]:={ {0x1234, 0x12}, {0x1234, 0x12} }\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 1u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 0, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, (UINT16_SIZE + UINT8_SIZE) * 2, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertUIntEquals(tc, 0x34u, provide_port_data[0]);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[1]);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[2]);
   CuAssertUIntEquals(tc, 0x34u, provide_port_data[3]);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[4]);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[5]);
   free(provide_port_data);
   apx_nodeManager_delete(manager);
}

static void test_provide_port_containing_record_inside_record(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"RecordPortOut\"{\"First\"{\"Inner1\"C\"Inner2\"S}\"Second\"{\"Inner3\"S\"Inner4\"L}}:={ {0x12, 0x1234}, {0x1234, 0x12345678} }\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 1u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 0, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, (UINT8_SIZE + UINT16_SIZE) + (UINT16_SIZE + UINT32_SIZE), apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_require_port_data_size(node_data));
   uint8_t* provide_port_data = apx_nodeData_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, provide_port_data);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[0]);
   CuAssertUIntEquals(tc, 0x34u, provide_port_data[1]);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[2]);
   CuAssertUIntEquals(tc, 0x34u, provide_port_data[3]);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[4]);
   CuAssertUIntEquals(tc, 0x78u, provide_port_data[5]);
   CuAssertUIntEquals(tc, 0x56u, provide_port_data[6]);
   CuAssertUIntEquals(tc, 0x34u, provide_port_data[7]);
   CuAssertUIntEquals(tc, 0x12u, provide_port_data[8]);
   free(provide_port_data);
   apx_nodeManager_delete(manager);
}

static void test_node_instance_by_name(CuTest* tc)
{
   const char* apx_text1 =
      "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"ProvdePortSignal1\"C(0,3):=3\n";
   const char* apx_text2 =
      "APX/1.2\n"
      "N\"TestNode2\"\n"
      "R\"RequirePortSignal1\"C(0,3):=3\n";
   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text2));
   CuAssertUIntEquals(tc, 2u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_find(manager, "TestNode1");
   CuAssertPtrNotNull(tc, node_instance);
   node_instance = apx_nodeManager_find(manager, "TestNode2");
   CuAssertPtrNotNull(tc, node_instance);
   node_instance = apx_nodeManager_find(manager, "TestNode3");
   CuAssertPtrEquals(tc, NULL, node_instance);

   apx_nodeManager_delete(manager);
}

static void test_client_node_require_port_byte_map_creation(CuTest* tc)
{
   const char* apx_text =
      "APX/1.2\n"
      "N\"TestNode\"\n"
      "R\"ComplexPort\"{\"Left\"L\"Right\"L}\n"
      "R\"U8Port\"C\n"
      "R\"U16Port\"S\n"
      "R\"NamePort\"A[21]\n";

   apx_nodeManager_t* manager = apx_nodeManager_new(APX_CLIENT_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_build_node(manager, apx_text));
   CuAssertUIntEquals(tc, 1u, apx_nodeManager_length(manager));
   apx_nodeInstance_t* node_instance = apx_nodeManager_get_last_attached(manager);
   CuAssertPtrNotNull(tc, node_instance);
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 0u, apx_nodeData_num_provide_ports(node_data));
   CuAssertUIntEquals(tc, 4, apx_nodeData_num_require_ports(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 32u, apx_nodeData_require_port_data_size(node_data));
   apx_bytePortMap_t const* byte_port_map = apx_nodeInstance_get_byte_port_map(node_instance);
   CuAssertPtrNotNull(tc, byte_port_map);
   apx_portId_t expected_map[32] = {
   0, 0, 0, 0, 0, 0, 0, 0,
   1,
   2, 2,
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
   };
   unsigned int i;
   for (i = 0; i < 32; i++)
   {
      CuAssertUIntEquals(tc, expected_map[i], apx_bytePortMap_lookup(byte_port_map, i));
   }

   apx_nodeManager_delete(manager);
}