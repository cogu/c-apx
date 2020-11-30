//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/port.h"
#include "adt_ary.h"
#include "apx/parser.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_port_create_typeRef(CuTest* tc);
static void test_apx_port_createDerivedDataSignature(CuTest* tc);
static void test_apx_port_create_invalidRecordDsg(CuTest* tc);
static void test_apx_port_create_U8DynamicArray(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_port(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_port_create_typeRef);
   SUITE_ADD_TEST(suite, test_apx_port_createDerivedDataSignature);
   SUITE_ADD_TEST(suite, test_apx_port_create_invalidRecordDsg);
   SUITE_ADD_TEST(suite, test_apx_port_create_U8DynamicArray);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_port_create_typeRef(CuTest* tc)
{
   apx_port_t port;

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_create(&port, APX_REQUIRE_PORT, "SootLevel","T[0]", NULL, 0));
   CuAssertPtrEquals(tc, NULL, port.portAttributes);
   CuAssertStrEquals(tc, "SootLevel" ,port.name);
   CuAssertPtrEquals(tc, NULL, port.derivedPortSignature);

   apx_port_destroy(&port);

}

static void test_apx_port_createDerivedDataSignature(CuTest* tc)
{
   apx_error_t err = APX_NO_ERROR;
   apx_port_t port;
   adt_ary_t typeList;
   adt_ary_create(&typeList, apx_datatype_vdelete);

   adt_ary_push(&typeList, apx_datatype_new("SootLevel_T", "C", NULL, 1, &err));
   CuAssertIntEquals(tc, APX_NO_ERROR, err);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_create(&port, APX_REQUIRE_PORT, "SootLevel","T[0]", NULL, 0));
   CuAssertIntEquals(tc, APX_NO_ERROR, err);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_resolveTypes(&port, &typeList, NULL));
   CuAssertPtrEquals(tc, NULL, port.derivedPortSignature);
   CuAssertStrEquals(tc, "\"SootLevel\"C", apx_port_getDerivedPortSignature(&port));
   CuAssertPtrNotNull(tc, port.derivedPortSignature);
   CuAssertIntEquals(tc, APX_NO_ERROR, err);

   apx_port_destroy(&port);
   adt_ary_destroy(&typeList);
}

static void test_apx_port_create_invalidRecordDsg(CuTest* tc)
{
   apx_port_t port;
   CuAssertIntEquals(tc, APX_UNMATCHED_BRACE_ERROR, apx_port_create(&port, APX_REQUIRE_PORT, "Hello", "{\"UserId\"S", "=0", 0));
}

static void test_apx_port_create_U8DynamicArray(CuTest* tc)
{
   apx_port_t port;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_port_create(&port, APX_REQUIRE_PORT, "Events","C[*]", "D[10]", 0));
   apx_port_destroy(&port);
}
