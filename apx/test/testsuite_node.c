//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/node.h"
#include "apx/error.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_create_empty_node(CuTest* tc);
static void test_append_data_type(CuTest* tc);
static void test_append_provide_port(CuTest* tc);
static void test_append_require_port(CuTest* tc);
static void test_same_require_and_provide_port(CuTest* tc);
static void test_get_type_by_id(CuTest* tc);
static void test_get_require_port_by_id(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_node(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_empty_node);
   SUITE_ADD_TEST(suite, test_append_data_type);
   SUITE_ADD_TEST(suite, test_append_provide_port);
   SUITE_ADD_TEST(suite, test_append_require_port);
   SUITE_ADD_TEST(suite, test_same_require_and_provide_port);
   SUITE_ADD_TEST(suite, test_get_type_by_id);
   SUITE_ADD_TEST(suite, test_get_require_port_by_id);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_create_empty_node(CuTest* tc)
{
   apx_node_t node;
   apx_node_create(&node, "TestNode");
   CuAssertStrEquals(tc, "TestNode", apx_node_get_name(&node));
   CuAssertIntEquals(tc, 0, apx_node_num_provide_ports(&node));
   CuAssertIntEquals(tc, 0, apx_node_num_require_ports(&node));
   CuAssertIntEquals(tc, 0, apx_node_num_data_types(&node));
   apx_node_destroy(&node);
}

static void test_append_data_type(CuTest* tc)
{
   apx_node_t node;
   apx_node_create(&node, "TestNode");
   apx_dataType_t *data_type = apx_dataType_new("VehicleSpeed_T", 2u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_data_type(&node, data_type));
   CuAssertIntEquals(tc, 1, apx_node_num_data_types(&node));
   CuAssertUIntEquals(tc, 0u, apx_dataType_get_id(data_type));
   data_type = apx_dataType_new("EngineSpeed_T", 3u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_data_type(&node, data_type));
   CuAssertIntEquals(tc, 2, apx_node_num_data_types(&node));
   CuAssertUIntEquals(tc, 1u, apx_dataType_get_id(data_type));
   data_type = apx_dataType_new("VehicleSpeed_T", 2u);
   CuAssertIntEquals(tc, APX_TYPE_ALREADY_EXIST_ERROR, apx_node_append_data_type(&node, data_type));
   apx_dataType_delete(data_type);
   apx_node_destroy(&node);
}

static void test_append_provide_port(CuTest* tc)
{
   apx_node_t node;
   apx_node_create(&node, "TestNode");
   apx_port_t* port = apx_port_new(APX_PROVIDE_PORT, "VehicleSpeed", 2u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_port(&node, port));
   CuAssertIntEquals(tc, 1, apx_node_num_provide_ports(&node));
   CuAssertUIntEquals(tc, 0u, apx_port_get_id(port));
   port = apx_port_new(APX_PROVIDE_PORT, "EngineSpeed", 3u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_port(&node, port));
   CuAssertIntEquals(tc, 2, apx_node_num_provide_ports(&node));
   CuAssertUIntEquals(tc, 1u, apx_port_get_id(port));
   apx_node_destroy(&node);
}

static void test_append_require_port(CuTest* tc)
{
   apx_node_t node;
   apx_node_create(&node, "TestNode");
   apx_port_t* port = apx_port_new(APX_REQUIRE_PORT, "VehicleSpeed", 2u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_port(&node, port));
   CuAssertIntEquals(tc, 1, apx_node_num_require_ports(&node));
   CuAssertUIntEquals(tc, 0u, apx_port_get_id(port));
   port = apx_port_new(APX_REQUIRE_PORT, "EngineSpeed", 3u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_port(&node, port));
   CuAssertIntEquals(tc, 2, apx_node_num_require_ports(&node));
   CuAssertUIntEquals(tc, 1u, apx_port_get_id(port));
   apx_node_destroy(&node);
}

static void test_same_require_and_provide_port(CuTest* tc)
{
   apx_node_t node;
   apx_node_create(&node, "TestNode");
   apx_port_t* port = apx_port_new(APX_REQUIRE_PORT, "VehicleSpeed", 2u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_port(&node, port));
   port = apx_port_new(APX_PROVIDE_PORT, "VehicleSpeed", 3u);
   CuAssertIntEquals(tc, APX_PORT_ALREADY_EXIST_ERROR, apx_node_append_port(&node, port));
   apx_port_delete(port);
   apx_node_destroy(&node);
}

static void test_get_type_by_id(CuTest* tc)
{
   apx_node_t node;
   apx_node_create(&node, "TestNode");
   apx_dataType_t* data_type = apx_dataType_new("VehicleSpeed_T", 2u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_data_type(&node, data_type));
   CuAssertIntEquals(tc, 1, apx_node_num_data_types(&node));
   CuAssertUIntEquals(tc, 0u, apx_dataType_get_id(data_type));
   data_type = apx_dataType_new("EngineSpeed_T", 3u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_data_type(&node, data_type));
   CuAssertIntEquals(tc, 2, apx_node_num_data_types(&node));
   CuAssertUIntEquals(tc, 1u, apx_dataType_get_id(data_type));
   data_type = apx_node_get_data_type(&node, 0u);
   CuAssertPtrNotNull(tc, data_type);
   CuAssertStrEquals(tc, "VehicleSpeed_T", apx_dataType_get_name(data_type));
   apx_node_destroy(&node);
}

static void test_get_require_port_by_id(CuTest* tc)
{
   apx_node_t node;
   apx_node_create(&node, "TestNode");
   apx_port_t* port = apx_port_new(APX_REQUIRE_PORT, "VehicleSpeed", 2u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_port(&node, port));
   CuAssertIntEquals(tc, 1, apx_node_num_require_ports(&node));
   CuAssertUIntEquals(tc, 0u, apx_port_get_id(port));
   port = apx_port_new(APX_REQUIRE_PORT, "EngineSpeed", 3u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_append_port(&node, port));
   CuAssertIntEquals(tc, 2, apx_node_num_require_ports(&node));
   CuAssertUIntEquals(tc, 1u, apx_port_get_id(port));
   port = apx_node_get_require_port(&node, 0u);
   CuAssertPtrNotNull(tc, port);
   CuAssertStrEquals(tc, "VehicleSpeed", apx_port_get_name(port));
   apx_node_destroy(&node);
}
