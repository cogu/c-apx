//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "apx/parser.h"
#include "apx/node_instance.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_create_empty_node_data(CuTest *tc);
static void test_write_provide_port_data_uint8(CuTest* tc);
static void test_write_provide_port_data_uint16(CuTest* tc);
static void test_write_require_port_data_uint8(CuTest* tc);
static void test_write_require_port_data_uint16(CuTest* tc);
static void test_take_provide_port_data_snapshot(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeData(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_empty_node_data);
   SUITE_ADD_TEST(suite, test_write_provide_port_data_uint8);
   SUITE_ADD_TEST(suite, test_write_provide_port_data_uint16);
   SUITE_ADD_TEST(suite, test_write_require_port_data_uint8);
   SUITE_ADD_TEST(suite, test_write_require_port_data_uint16);
   SUITE_ADD_TEST(suite, test_take_provide_port_data_snapshot);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_empty_node_data(CuTest *tc)
{
   apx_nodeData_t * node_data;
   node_data =  apx_nodeData_new();
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 0u, apx_nodeData_definition_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_nodeData_require_port_data_size(node_data));
   apx_nodeData_delete(node_data);
}

static void test_write_provide_port_data_uint8(CuTest* tc)
{
   apx_nodeData_t* node_data;
   uint8_t const init_data[UINT8_SIZE] = { 0x07u };
   uint8_t buf[sizeof(init_data)];
   uint8_t new_value[sizeof(init_data)] = { 0x03 };
   node_data = apx_nodeData_new();
   memset(buf, 0, sizeof(buf));
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_create_provide_port_data(node_data, 1u, init_data, sizeof(init_data)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x07, buf[0]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_write_provide_port_data(node_data, 0u, new_value, sizeof(new_value)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x03, buf[0]);
   apx_nodeData_delete(node_data);
}

static void test_write_provide_port_data_uint16(CuTest* tc)
{
   apx_nodeData_t* node_data;
   uint8_t const init_data[UINT16_SIZE] = { 0xffu, 0xffu };
   uint8_t buf[sizeof(init_data)];
   uint8_t new_value[sizeof(init_data)] = { 0x34u, 0x12u };
   node_data = apx_nodeData_new();
   memset(buf, 0, sizeof(buf));
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_create_provide_port_data(node_data, 1u, init_data, sizeof(init_data)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0xff, buf[0]);
   CuAssertUIntEquals(tc, 0xff, buf[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_write_provide_port_data(node_data, 0u, new_value, sizeof(new_value)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x34, buf[0]);
   CuAssertUIntEquals(tc, 0x12, buf[1]);
   apx_nodeData_delete(node_data);
}

static void test_write_require_port_data_uint8(CuTest* tc)
{
   apx_nodeData_t* node_data;
   uint8_t const init_data[UINT8_SIZE] = { 0x07u };
   uint8_t buf[sizeof(init_data)];
   uint8_t new_value[sizeof(init_data)] = { 0x03 };
   node_data = apx_nodeData_new();
   memset(buf, 0, sizeof(buf));
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_create_require_port_data(node_data, 1u, init_data, sizeof(init_data)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_require_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x07, buf[0]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_write_require_port_data(node_data, 0u, new_value, sizeof(new_value)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_require_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x03, buf[0]);
   apx_nodeData_delete(node_data);
}

static void test_write_require_port_data_uint16(CuTest* tc)
{
   apx_nodeData_t* node_data;
   uint8_t const init_data[UINT16_SIZE] = { 0xffu, 0xffu };
   uint8_t buf[sizeof(init_data)];
   uint8_t new_value[sizeof(init_data)] = { 0x34u, 0x12u };
   node_data = apx_nodeData_new();
   memset(buf, 0, sizeof(buf));
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_create_require_port_data(node_data, 1u, init_data, sizeof(init_data)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_require_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0xff, buf[0]);
   CuAssertUIntEquals(tc, 0xff, buf[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_write_require_port_data(node_data, 0u, new_value, sizeof(new_value)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_require_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x34, buf[0]);
   CuAssertUIntEquals(tc, 0x12, buf[1]);
   apx_nodeData_delete(node_data);
}

static void test_take_provide_port_data_snapshot(CuTest* tc)
{
   apx_nodeData_t* node_data;
   uint8_t const init_data[UINT32_SIZE] = { 0x78, 0x56, 0x34, 0x12 };
   uint8_t* snapshot;
   node_data = apx_nodeData_new();
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_create_provide_port_data(node_data, 1u, init_data, sizeof(init_data)));
   snapshot = apx_nodeData_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, snapshot);
   CuAssertUIntEquals(tc, 0x78, snapshot[0]);
   CuAssertUIntEquals(tc, 0x56, snapshot[1]);
   CuAssertUIntEquals(tc, 0x34, snapshot[2]);
   CuAssertUIntEquals(tc, 0x12, snapshot[3]);
   free(snapshot);
   apx_nodeData_delete(node_data);
}

