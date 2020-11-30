//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include "CuTest.h"
#include "apx/data_type.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_datatype_create(CuTest *tc);
static void test_apx_datatype_createWithErrors(CuTest *tc);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_datatype(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_datatype_create);
   SUITE_ADD_TEST(suite, test_apx_datatype_createWithErrors);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_datatype_create(CuTest *tc)
{
   apx_error_t err;
   apx_datatype_t *datatype = apx_datatype_new("MyType", "C(0,3)", NULL, 4, &err);
   CuAssertPtrNotNull(tc, datatype);
   CuAssertIntEquals(tc, 4, apx_datatype_getLineNumber(datatype));
   apx_datatype_delete(datatype);

   datatype = apx_datatype_new("MyType", "T[2]", NULL, 4, &err);
   CuAssertPtrNotNull(tc, datatype);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_ID, datatype->dataSignature->dataElement->baseType);
   CuAssertIntEquals(tc, 2, datatype->dataSignature->dataElement->typeRef.id);
   apx_datatype_delete(datatype);

}

static void test_apx_datatype_createWithErrors(CuTest *tc)
{
   apx_error_t err;
   apx_datatype_t *datatype = apx_datatype_new("MyType", "Q", NULL, 1, &err);
   CuAssertPtrEquals(tc, NULL, datatype);
   CuAssertIntEquals(tc, APX_ELEMENT_TYPE_ERROR, err);

   datatype = apx_datatype_new("MyType", "T[2", NULL, 1, &err);
   CuAssertPtrEquals(tc, NULL, datatype);
   CuAssertIntEquals(tc, APX_UNMATCHED_BRACKET_ERROR, err);

}


