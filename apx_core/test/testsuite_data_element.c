//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/data_element.h"
#include "dtl_type.h"


#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_apx_dataElement_derive_proper_init_value_uint8(CuTest* tc);
static void test_apx_dataElement_derive_proper_init_value_array_of_records(CuTest* tc);
static void test_apx_dataElement_to_string_uint8_without_limits(CuTest* tc);
static void test_apx_dataElement_to_string_uint8_with_limits(CuTest* tc);
static void test_apx_dataElement_to_string_uint8_array_without_limits(CuTest* tc);
static void test_apx_dataElement_to_string_uint8_array_with_limits(CuTest* tc);
static void test_apx_dataElement_to_string_dynamic_uint8_array_with_limits(CuTest* tc);




//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_dataElement(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_dataElement_derive_proper_init_value_uint8);
   SUITE_ADD_TEST(suite, test_apx_dataElement_derive_proper_init_value_array_of_records);
   SUITE_ADD_TEST(suite, test_apx_dataElement_to_string_uint8_without_limits);
   SUITE_ADD_TEST(suite, test_apx_dataElement_to_string_uint8_with_limits);
   SUITE_ADD_TEST(suite, test_apx_dataElement_to_string_uint8_array_without_limits);
   SUITE_ADD_TEST(suite, test_apx_dataElement_to_string_uint8_array_with_limits);
   SUITE_ADD_TEST(suite, test_apx_dataElement_to_string_dynamic_uint8_array_with_limits);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_dataElement_derive_proper_init_value_uint8(CuTest* tc)
{
/*
Signature: C
*/
   apx_dataElement_t* data_element = apx_dataElement_new(APX_TYPE_CODE_UINT8);
   CuAssertPtrNotNull(tc, data_element);
   dtl_sv_t* parsed_init_value = dtl_sv_make_u32(7u);
   dtl_dv_t* proper_init_value = NULL;
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_dataElement_derive_proper_init_value(data_element, (dtl_dv_t*)parsed_init_value, &proper_init_value));
   CuAssertPtrNotNull(tc, proper_init_value);
   dtl_dec_ref(parsed_init_value);
   dtl_dec_ref(proper_init_value);
   apx_dataElement_delete(data_element);
}

static void test_apx_dataElement_derive_proper_init_value_array_of_records(CuTest* tc)
{
   /*
   APX:
   {\"Id\"S\"Value\"C}[2]:={ {0xFFFF,0}, {0xFFFF,0} }
   */
   apx_dataElement_t* data_element = apx_dataElement_new(APX_TYPE_CODE_RECORD);
   CuAssertPtrNotNull(tc, data_element);
   apx_dataElement_t* child_element = apx_dataElement_new(APX_TYPE_CODE_UINT16);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_dataElement_set_name_cstr(child_element, "Id"));
   apx_dataElement_append_child(data_element, child_element);
   child_element = apx_dataElement_new(APX_TYPE_CODE_UINT8);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_dataElement_set_name_cstr(child_element, "Value"));
   apx_dataElement_append_child(data_element, child_element);
   apx_dataElement_set_array_length(data_element, 2u);

   dtl_av_t* parsed_init_value;
   dtl_av_t* child_init_value;
   dtl_dv_t* proper_init_value = NULL;

   parsed_init_value = dtl_av_new();
   child_init_value = dtl_av_new();
   dtl_av_push(child_init_value, (dtl_dv_t*)dtl_sv_make_u32(0xFFFF), false);
   dtl_av_push(child_init_value, (dtl_dv_t*)dtl_sv_make_u32(0u), false);
   dtl_av_push(parsed_init_value, (dtl_dv_t*)child_init_value, false);
   child_init_value = dtl_av_new();
   dtl_av_push(child_init_value, (dtl_dv_t*)dtl_sv_make_u32(0xFFFF), false);
   dtl_av_push(child_init_value, (dtl_dv_t*)dtl_sv_make_u32(0u), false);
   dtl_av_push(parsed_init_value, (dtl_dv_t*)child_init_value, false);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_dataElement_derive_proper_init_value(data_element, (dtl_dv_t*) parsed_init_value, &proper_init_value));
   dtl_dec_ref(parsed_init_value);
   dtl_dec_ref(proper_init_value);
   apx_dataElement_delete(data_element);
}

static void test_apx_dataElement_to_string_uint8_without_limits(CuTest* tc)
{
   apx_dataElement_t* data_element = apx_dataElement_new(APX_TYPE_CODE_UINT8);
   CuAssertPtrNotNull(tc, data_element);
   adt_str_t* str = apx_dataElement_to_string(data_element, false);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "C", adt_str_cstr(str));
   adt_str_delete(str);
   apx_dataElement_delete(data_element);
}

static void test_apx_dataElement_to_string_uint8_with_limits(CuTest* tc)
{
   apx_dataElement_t* data_element = apx_dataElement_new(APX_TYPE_CODE_UINT8);
   CuAssertPtrNotNull(tc, data_element);
   apx_dataElement_set_limits_uint32(data_element, 0u, 7u);
   adt_str_t* str = apx_dataElement_to_string(data_element, false);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "C(0,7)", adt_str_cstr(str));
   adt_str_delete(str);
   apx_dataElement_delete(data_element);
}

static void test_apx_dataElement_to_string_uint8_array_without_limits(CuTest* tc)
{
   apx_dataElement_t* data_element = apx_dataElement_new(APX_TYPE_CODE_UINT8);
   CuAssertPtrNotNull(tc, data_element);
   apx_dataElement_set_array_length(data_element, 10u);
   adt_str_t* str = apx_dataElement_to_string(data_element, false);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "C[10]", adt_str_cstr(str));
   adt_str_delete(str);
   apx_dataElement_delete(data_element);
}

static void test_apx_dataElement_to_string_uint8_array_with_limits(CuTest* tc)
{
   apx_dataElement_t* data_element = apx_dataElement_new(APX_TYPE_CODE_UINT8);
   CuAssertPtrNotNull(tc, data_element);
   apx_dataElement_set_array_length(data_element, 10u);
   apx_dataElement_set_limits_uint32(data_element, 0u, 7u);
   adt_str_t* str = apx_dataElement_to_string(data_element, false);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "C(0,7)[10]", adt_str_cstr(str));
   adt_str_delete(str);
   apx_dataElement_delete(data_element);
}

static void test_apx_dataElement_to_string_dynamic_uint8_array_with_limits(CuTest* tc)
{
   apx_dataElement_t* data_element = apx_dataElement_new(APX_TYPE_CODE_UINT8);
   CuAssertPtrNotNull(tc, data_element);
   apx_dataElement_set_array_length(data_element, 10u);
   apx_dataElement_set_dynamic_array(data_element);
   apx_dataElement_set_limits_uint32(data_element, 0u, 7u);
   adt_str_t* str = apx_dataElement_to_string(data_element, false);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "C(0,7)[10*]", adt_str_cstr(str));
   adt_str_delete(str);
   apx_dataElement_delete(data_element);
}