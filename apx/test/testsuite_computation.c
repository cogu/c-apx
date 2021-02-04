//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <string.h>
#include "CuTest.h"
#include "apx/computation.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_value_table_to_string(CuTest* tc);
static void test_rational_scaling_to_string(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_computation(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_value_table_to_string);
   SUITE_ADD_TEST(suite, test_rational_scaling_to_string);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_value_table_to_string(CuTest* tc)
{
   adt_ary_t* values = adt_ary_new(adt_str_vdelete);
   apx_valueTable_t* vt = apx_valueTable_new();
   CuAssertPtrNotNull(tc, vt);
   adt_ary_push(values, adt_str_new_cstr("Off"));
   adt_ary_push(values, adt_str_new_cstr("On"));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_valueTable_move_values(vt, values));
   apx_valueTable_set_range_unsigned(vt, 0, 1);
   adt_str_t* str = apx_valueTable_to_string(vt);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "VT(0,1,\"Off\",\"On\")", adt_str_cstr(str));
   adt_str_delete(str);
   apx_valueTable_delete(vt);
   adt_ary_delete(values);
}

static void test_rational_scaling_to_string(CuTest* tc)
{
   apx_rationalScaling_t* rs = apx_rationalScaling_new(0.0, 4, 10, "Percent");
   CuAssertPtrNotNull(tc, rs);
   apx_rationalScaling_set_range_unsigned(rs, 0, 250);
   adt_str_t* str = apx_rationalScaling_to_string(rs);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "RS(0,250,0.00000000,4,10,\"Percent\")", adt_str_cstr(str));
   adt_str_delete(str);
   apx_rationalScaling_delete(rs);
}
