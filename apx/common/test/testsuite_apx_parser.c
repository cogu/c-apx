//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_parser.h"
#include "apx_error.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


const char *m_apx_node1 = "APX/1.2\n"
"N\"Node1\"\n"
"R\"WheelBasedVehicleSpeed\"S\n"
"R\"CabTiltLockWarning\"C(0,7)\n"
"P\"GearSelectionMode\"C(0,7)\n";

const char *m_apx_node2 = "APX/1.2\n"
"N\"Node2\"\n"
"R\"WheelBasedVehicleSpeed\"S:=65535\n"
"R\"CabTiltLockWarning\"C(0,7):=7\n"
"R\"VehicleMode\"C(0,15):=15\n";

const char *m_apx_node3 = "APX/1.3\n"
"N\"Node3\"\n"
"P\"DynArray\"C[*]:D[10]\n"; //D: Dynamic array length

const char *m_apx_node4 = "APX/1.3\n"
"N\"Node3\"\n"
"P\"DynArray\"C[1]:Q[1]\n"; //Q: Queue length


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_parser_nodeWithoutInitValues(CuTest* tc);
static void test_apx_parser_nodeWithInitValues(CuTest* tc);
static void test_apx_parser_providePortWithInvalidAttributeString(CuTest* tc);
static void test_apx_parser_requirePortWithInvalidAttributeString(CuTest* tc);
static void test_apx_parser_providePortWithInvalidDataSignature(CuTest* tc);
static void test_apx_parser_providePortWithDynamicArray(CuTest* tc);
static void test_apx_parser_providePortWithQueueLength(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

CuSuite* testSuite_apx_parser(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_parser_nodeWithoutInitValues);
   SUITE_ADD_TEST(suite, test_apx_parser_nodeWithInitValues);
   SUITE_ADD_TEST(suite, test_apx_parser_providePortWithInvalidAttributeString);
   SUITE_ADD_TEST(suite, test_apx_parser_requirePortWithInvalidAttributeString);
   SUITE_ADD_TEST(suite, test_apx_parser_providePortWithInvalidDataSignature);
   SUITE_ADD_TEST(suite, test_apx_parser_providePortWithDynamicArray);
   SUITE_ADD_TEST(suite, test_apx_parser_providePortWithQueueLength);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_parser_nodeWithoutInitValues(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   apx_parser_create(&parser);
   node=apx_parser_parseString(&parser, m_apx_node1);
   CuAssertPtrNotNull(tc,node);
   apx_parser_destroy(&parser);
}

static void test_apx_parser_nodeWithInitValues(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   int32_t numRequirePorts=0;
   dtl_sv_t *sv;
   apx_portAttributes_t *attr;
   apx_port_t *port = 0;
   apx_parser_create(&parser);
   node=apx_parser_parseString(&parser, m_apx_node2);
   CuAssertPtrNotNull(tc,node);
   numRequirePorts = apx_node_getNumRequirePorts(node);
   CuAssertIntEquals(tc, 3, numRequirePorts);
   port = apx_node_getRequirePort(node, 0);
   attr = port->portAttributes;
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(attr->initValue));
   sv = (dtl_sv_t*) attr->initValue;
   CuAssertUIntEquals(tc, 65535, dtl_sv_to_u32(sv, NULL));

   port = apx_node_getRequirePort(node, 1);
   attr = port->portAttributes;
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(attr->initValue));
   sv = (dtl_sv_t*) attr->initValue;
   CuAssertUIntEquals(tc, 7, dtl_sv_to_u32(sv, NULL));

   port = apx_node_getRequirePort(node, 2);
   attr = port->portAttributes;
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(attr->initValue));
   sv = (dtl_sv_t*) attr->initValue;
   CuAssertUIntEquals(tc, 15, dtl_sv_to_u32(sv, NULL));

   apx_parser_destroy(&parser);
}

static void test_apx_parser_providePortWithInvalidAttributeString(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   apx_parser_create(&parser);
   const char *apx_text = "APX/1.2\n"
"N\"test\"\n"
"P\"VehicleSpeed\"S:abcd\n";
   node=apx_parser_parseString(&parser, apx_text);
   CuAssertPtrEquals(tc, 0, node);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, apx_parser_getLastError(&parser));
   CuAssertIntEquals(tc, 3, apx_parser_getErrorLine(&parser));
   apx_parser_destroy(&parser);
}

static void test_apx_parser_requirePortWithInvalidAttributeString(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   apx_parser_create(&parser);
   const char *apx_text = "APX/1.2\n"
"N\"test\"\n"
"R\"VehicleSpeed\"S:abcd\n";
   node=apx_parser_parseString(&parser, apx_text);
   CuAssertPtrEquals(tc, 0, node);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, apx_parser_getLastError(&parser));
   CuAssertIntEquals(tc, 3, apx_parser_getErrorLine(&parser));
   apx_parser_destroy(&parser);
}

static void test_apx_parser_providePortWithInvalidDataSignature(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   apx_parser_create(&parser);
   const char *apx_text = "APX/1.2\n"
"N\"test\"\n"
"P\"Signal1\"{\"User\"L:=0\n" //missing terminating '}' character
"R\"Signal2\"S:=0";
   node=apx_parser_parseString(&parser, apx_text);
   CuAssertPtrEquals(tc, 0, node);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, apx_parser_getLastError(&parser));
   CuAssertIntEquals(tc, 3, apx_parser_getErrorLine(&parser));
   apx_parser_destroy(&parser);
}

static void test_apx_parser_providePortWithDynamicArray(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   apx_portAttributes_t *attr;
   apx_port_t *port = 0;
   int32_t numProvidePorts=0;
   apx_parser_create(&parser);
   node=apx_parser_parseString(&parser, m_apx_node3);
   CuAssertPtrNotNull(tc,node);

   numProvidePorts = apx_node_getNumProvidePorts(node);
   CuAssertIntEquals(tc, 1, numProvidePorts);
   port = apx_node_getProvidePort(node, 0);
   attr = port->portAttributes;
   CuAssertTrue(tc, attr->isDynamic);
   CuAssertTrue(tc, !attr->isQueued);
   apx_parser_destroy(&parser);
}

static void test_apx_parser_providePortWithQueueLength(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   apx_portAttributes_t *attr;
   apx_port_t *port = 0;
   int32_t numProvidePorts=0;
   apx_parser_create(&parser);
   node=apx_parser_parseString(&parser, m_apx_node4);
   CuAssertPtrNotNull(tc,node);

   numProvidePorts = apx_node_getNumProvidePorts(node);
   CuAssertIntEquals(tc, 1, numProvidePorts);
   port = apx_node_getProvidePort(node, 0);
   CuAssertPtrNotNull(tc, port->portAttributes);
   attr = port->portAttributes;
   CuAssertTrue(tc, !attr->isDynamic);
   CuAssertTrue(tc, attr->isQueued);
   apx_parser_destroy(&parser);

}
