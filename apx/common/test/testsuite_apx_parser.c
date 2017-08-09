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
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#define APX_TEST_DATA_PATH "..\\..\\..\\apx\\common\\test\\data\\"
#else 
#define APX_TEST_DATA_PATH  "../../../apx/common/test/data/"
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_parser_file(CuTest* tc);
static void test_apx_parser_fileWithErrorErrors(CuTest* tc);
static void test_apx_parser_fileWithInitValues(CuTest* tc);

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

   SUITE_ADD_TEST(suite, test_apx_parser_file);
   SUITE_ADD_TEST(suite, test_apx_parser_fileWithErrorErrors);
   SUITE_ADD_TEST(suite, test_apx_parser_fileWithInitValues);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_parser_file(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   apx_parser_create(&parser);
   node=apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test5.apx");
   CuAssertPtrNotNull(tc,node);
   apx_parser_destroy(&parser);
}

static void test_apx_parser_fileWithErrorErrors(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   apx_parser_create(&parser);
   node=apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test6.apx");
   CuAssertPtrNotNull(tc,node);
   apx_parser_destroy(&parser);
}

static void test_apx_parser_fileWithInitValues(CuTest* tc)
{
   apx_parser_t parser;
   apx_node_t *node;
   int32_t numRequirePorts=0;
   dtl_sv_t *sv;
   apx_portAttributes_t *attr;
   apx_port_t *port = 0;
   apx_parser_create(&parser);
   node=apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test7.apx");
   CuAssertPtrNotNull(tc,node);
   numRequirePorts = apx_node_getNumRequirePorts(node);
   CuAssertIntEquals(tc, 3, numRequirePorts);
   port = apx_node_getRequirePort(node, 0);
   attr = port->portAttributes;
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(attr->initValue));
   sv = (dtl_sv_t*) attr->initValue;
   CuAssertUIntEquals(tc, 65535, dtl_sv_get_u32(sv));

   port = apx_node_getRequirePort(node, 1);
   attr = port->portAttributes;
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(attr->initValue));
   sv = (dtl_sv_t*) attr->initValue;
   CuAssertUIntEquals(tc, 7, dtl_sv_get_u32(sv));

   port = apx_node_getRequirePort(node, 2);
   attr = port->portAttributes;
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(attr->initValue));
   sv = (dtl_sv_t*) attr->initValue;
   CuAssertUIntEquals(tc, 15, dtl_sv_get_u32(sv));


   apx_parser_destroy(&parser);
}
