//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
#include "CuTest.h"
#include "apx/attribute_parser.h"
#include "apx/computation.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_parse_init_value_zero(CuTest* tc);
static void test_parse_init_value_minus_one(CuTest* tc);
static void test_parse_init_value_uint32_max(CuTest* tc);
static void test_parse_init_value_int32_min(CuTest* tc);
static void test_parse_init_value_int32_max(CuTest* tc);
static void test_parse_init_value_int32_list(CuTest* tc);
static void test_parse_init_value_empty_list(CuTest* tc);
static void test_parse_init_value_int32_lists_in_list(CuTest* tc);
static void test_parse_init_value_empty_string(CuTest* tc);
static void test_parse_init_value_string_literal(CuTest* tc);
static void test_parse_init_value_with_stray_characters(CuTest* tc);
static void test_parse_type_attribute_default_value_table(CuTest* tc);
static void test_parse_type_attribute_value_table_with_offset(CuTest* tc);
static void test_parse_type_attribute_value_table_with_negative_range(CuTest* tc);
static void test_parse_type_attribute_value_table_ranges(CuTest* tc);
static void test_parse_type_attribute_rational_scaling_vehicle_speed(CuTest* tc);
static void test_parse_combined_type_attributes(CuTest* tc);
static void test_parse_port_attribute_queue_length(CuTest* tc);
static void test_parse_port_attribute_parameter(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_apx_attributesParser(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_parse_init_value_zero);
   SUITE_ADD_TEST(suite, test_parse_init_value_minus_one);
   SUITE_ADD_TEST(suite, test_parse_init_value_uint32_max);
   SUITE_ADD_TEST(suite, test_parse_init_value_int32_min);
   SUITE_ADD_TEST(suite, test_parse_init_value_int32_max);
   SUITE_ADD_TEST(suite, test_parse_init_value_int32_list);
   SUITE_ADD_TEST(suite, test_parse_init_value_empty_list);
   SUITE_ADD_TEST(suite, test_parse_init_value_int32_lists_in_list);
   SUITE_ADD_TEST(suite, test_parse_init_value_empty_string);
   SUITE_ADD_TEST(suite, test_parse_init_value_string_literal);
   SUITE_ADD_TEST(suite, test_parse_init_value_with_stray_characters);
   SUITE_ADD_TEST(suite, test_parse_type_attribute_default_value_table);
   SUITE_ADD_TEST(suite, test_parse_type_attribute_value_table_with_offset);
   SUITE_ADD_TEST(suite, test_parse_type_attribute_value_table_with_negative_range);
   SUITE_ADD_TEST(suite, test_parse_type_attribute_value_table_ranges);
   SUITE_ADD_TEST(suite, test_parse_type_attribute_rational_scaling_vehicle_speed);
   SUITE_ADD_TEST(suite, test_parse_combined_type_attributes);
   SUITE_ADD_TEST(suite, test_parse_port_attribute_queue_length);
   SUITE_ADD_TEST(suite, test_parse_port_attribute_parameter);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_parse_init_value_zero(CuTest* tc)
{
   const char* init_value = "0";
   const uint8_t* begin = (const uint8_t*) init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_minus_one(CuTest* tc)
{
   const char* init_value1 = "-";
   const char* init_value2 = "-1";
   const uint8_t* begin = (const uint8_t*)init_value1;
   const uint8_t* end = begin + strlen(init_value1);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;
   uint8_t const* result;

   apx_attributeParser_create(&parser);
   result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, NULL, result);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, apx_attributeParser_get_last_error(&parser, NULL));
   begin = (const uint8_t*)init_value2;
   end = begin + strlen(init_value2);
   result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_uint32_max(CuTest* tc)
{
   const char* init_value = "0xFFFFFFFF";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(sv));
   CuAssertUIntEquals(tc, 0xFFFFFFFF, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_int32_min(CuTest* tc)
{
   const char* init_value = "-2147483648";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, INT32_MIN, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_int32_max(CuTest* tc)
{
   const char* init_value = "2147483647";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, INT32_MAX, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_int32_list(CuTest* tc)
{
   const char* init_value = "{3, 4}";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   dtl_av_t* av = (dtl_av_t*)dv;
   CuAssertIntEquals(tc, 2, dtl_av_length(av));
   dtl_sv_t* sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 3, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 4, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_empty_list(CuTest* tc)
{
   const char* init_value = "{}";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   dtl_av_t* av = (dtl_av_t*)dv;
   CuAssertTrue(tc, dtl_av_is_empty(av));

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);

}

static void test_parse_init_value_int32_lists_in_list(CuTest* tc)
{
   const char* init_value = "{{1, 2}, {3, 4}, {5}}";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   dtl_av_t* av = (dtl_av_t*)dv;
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   dtl_dv_t* child_dv = dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(child_dv));
   dtl_av_t* child_av = (dtl_av_t*)child_dv;
   CuAssertIntEquals(tc, 2, dtl_av_length(child_av));
   dtl_sv_t* sv = (dtl_sv_t*) dtl_av_value(child_av, 0);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(child_av, 1);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 2, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   child_dv = dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(child_dv));
   child_av = (dtl_av_t*)child_dv;
   CuAssertIntEquals(tc, 2, dtl_av_length(child_av));
   sv = (dtl_sv_t*)dtl_av_value(child_av, 0);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 3, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(child_av, 1);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 4, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   child_dv = dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(child_dv));
   child_av = (dtl_av_t*)child_dv;
   CuAssertIntEquals(tc, 1, dtl_av_length(child_av));
   sv = (dtl_sv_t*)dtl_av_value(child_av, 0);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 5, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_empty_string(CuTest* tc)
{
   const char* init_value = "\"\"";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   CuAssertIntEquals(tc, DTL_SV_STR, dtl_sv_type(sv));
   adt_str_t* tmp = dtl_sv_to_str(sv, &ok);
   CuAssertTrue(tc, ok);
   CuAssertStrEquals(tc, "", adt_str_cstr(tmp));

   adt_str_delete(tmp);
   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_string_literal(CuTest* tc)
{
   const char* init_value = "\"InitText\"";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;
   bool ok = false;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   dtl_sv_t* sv = (dtl_sv_t*)dv;
   CuAssertIntEquals(tc, DTL_SV_STR, dtl_sv_type(sv));
   adt_str_t* tmp = dtl_sv_to_str(sv, &ok);
   CuAssertTrue(tc, ok);
   CuAssertStrEquals(tc, "InitText", adt_str_cstr(tmp));

   adt_str_delete(tmp);
   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_init_value_with_stray_characters(CuTest* tc)
{
   const char* init_value = "255, ";
   const uint8_t* begin = (const uint8_t*)init_value;
   const uint8_t* end = begin + strlen(init_value);
   apx_attributeParser_t parser;
   dtl_dv_t* dv = NULL;

   apx_attributeParser_create(&parser);
   uint8_t const* result = apx_attributeParser_parse_initializer(&parser, begin, end, &dv);
   CuAssertConstPtrEquals(tc, end-2, result);
   CuAssertPtrNotNull(tc, dv);

   dtl_dec_ref(dv);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_type_attribute_default_value_table(CuTest* tc)
{
   const char* attribute_string = "VT(\"OnOff_Off\", \"OnOff_On\", \"OnOff_Error\", \"OnOff_NotAvailable\")";
   const uint8_t* begin = (const uint8_t*)attribute_string;
   const uint8_t* end = begin + strlen(attribute_string);
   apx_attributeParser_t parser;
   uint8_t const* result = NULL;
   apx_typeAttributes_t attr;
   apx_valueTable_t* vt = NULL;
   adt_str_t* str = NULL;

   apx_attributeParser_create(&parser);
   apx_typeAttributes_create(&attr);
   result = apx_attributeParser_parse_type_attributes(&parser, begin, end, &attr);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertIntEquals(tc, 1, apx_typeAttributes_num_computations(&attr));
   vt = (apx_valueTable_t*)apx_typeAttributes_get_computation(&attr, 0);
   CuAssertPtrNotNull(tc, vt);
   CuAssertIntEquals(tc, 4, apx_valueTable_length(vt));
   CuAssertFalse(tc, vt->base.is_signed_range);
   CuAssertIntEquals(tc, 0, vt->base.lower_limit.u32);
   CuAssertIntEquals(tc, 3, vt->base.upper_limit.u32);
   str = apx_valueTable_get_value(vt, 0);
   CuAssertStrEquals(tc, "OnOff_Off", adt_str_cstr(str));
   str = apx_valueTable_get_value(vt, 1);
   CuAssertStrEquals(tc, "OnOff_On", adt_str_cstr(str));
   str = apx_valueTable_get_value(vt, 2);
   CuAssertStrEquals(tc, "OnOff_Error", adt_str_cstr(str));
   str = apx_valueTable_get_value(vt, 3);
   CuAssertStrEquals(tc, "OnOff_NotAvailable", adt_str_cstr(str));

   apx_typeAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_type_attribute_value_table_with_offset(CuTest* tc)
{
   const char* attribute_string = "VT(4, \"OnOff_Off\", \"OnOff_On\", \"OnOff_Error\", \"OnOff_NotAvailable\")";
   const uint8_t* begin = (const uint8_t*)attribute_string;
   const uint8_t* end = begin + strlen(attribute_string);
   apx_attributeParser_t parser;
   uint8_t const* result = NULL;
   apx_typeAttributes_t attr;
   apx_valueTable_t* vt = NULL;

   apx_attributeParser_create(&parser);
   apx_typeAttributes_create(&attr);
   result = apx_attributeParser_parse_type_attributes(&parser, begin, end, &attr);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertIntEquals(tc, 1, apx_typeAttributes_num_computations(&attr));
   vt = (apx_valueTable_t*)apx_typeAttributes_get_computation(&attr, 0);
   CuAssertPtrNotNull(tc, vt);
   CuAssertIntEquals(tc, 4, apx_valueTable_length(vt));
   CuAssertFalse(tc, vt->base.is_signed_range);
   CuAssertIntEquals(tc, 4, vt->base.lower_limit.u32);
   CuAssertIntEquals(tc, 7, vt->base.upper_limit.u32);

   apx_typeAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_type_attribute_value_table_with_negative_range(CuTest* tc)
{
   const char* attribute_string = "VT(-3, 0, \"ErrorCode3\", \"ErrorCode2\", \"ErrorCode1\", \"NoError\")";
   const uint8_t* begin = (const uint8_t*)attribute_string;
   const uint8_t* end = begin + strlen(attribute_string);
   apx_attributeParser_t parser;
   uint8_t const* result = NULL;
   apx_typeAttributes_t attr;
   apx_valueTable_t* vt = NULL;
   adt_str_t* str = NULL;

   apx_attributeParser_create(&parser);
   apx_typeAttributes_create(&attr);
   result = apx_attributeParser_parse_type_attributes(&parser, begin, end, &attr);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertIntEquals(tc, 1, apx_typeAttributes_num_computations(&attr));
   vt = (apx_valueTable_t*)apx_typeAttributes_get_computation(&attr, 0);
   CuAssertPtrNotNull(tc, vt);
   CuAssertIntEquals(tc, 4, apx_valueTable_length(vt));
   CuAssertTrue(tc, vt->base.is_signed_range);
   CuAssertIntEquals(tc, -3, vt->base.lower_limit.i32);
   CuAssertIntEquals(tc, 0, vt->base.upper_limit.i32);

   str = apx_valueTable_get_value(vt, 0);
   CuAssertStrEquals(tc, "ErrorCode3", adt_str_cstr(str));
   str = apx_valueTable_get_value(vt, 1);
   CuAssertStrEquals(tc, "ErrorCode2", adt_str_cstr(str));
   str = apx_valueTable_get_value(vt, 2);
   CuAssertStrEquals(tc, "ErrorCode1", adt_str_cstr(str));
   str = apx_valueTable_get_value(vt, 3);
   CuAssertStrEquals(tc, "NoError", adt_str_cstr(str));

   apx_typeAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_type_attribute_value_table_ranges(CuTest* tc)
{
   const char* attribute_string = "VT(251, 254, \"Error\"), VT(255, \"NotAvailable\")";
   const uint8_t* begin = (const uint8_t*)attribute_string;
   const uint8_t* end = begin + strlen(attribute_string);
   apx_attributeParser_t parser;
   uint8_t const* result = NULL;
   apx_typeAttributes_t attr;
   apx_valueTable_t* vt = NULL;
   adt_str_t* str = NULL;

   apx_attributeParser_create(&parser);
   apx_typeAttributes_create(&attr);
   result = apx_attributeParser_parse_type_attributes(&parser, begin, end, &attr);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertIntEquals(tc, 2, apx_typeAttributes_num_computations(&attr));
   vt = (apx_valueTable_t*)apx_typeAttributes_get_computation(&attr, 0);
   CuAssertPtrNotNull(tc, vt);
   CuAssertIntEquals(tc, 1, apx_valueTable_length(vt));
   CuAssertFalse(tc, vt->base.is_signed_range);
   CuAssertIntEquals(tc, 251, vt->base.lower_limit.u32);
   CuAssertIntEquals(tc, 254, vt->base.upper_limit.u32);
   str = apx_valueTable_get_value(vt, 0);
   CuAssertStrEquals(tc, "Error", adt_str_cstr(str));

   vt = (apx_valueTable_t*)apx_typeAttributes_get_computation(&attr, 1);
   CuAssertPtrNotNull(tc, vt);
   CuAssertIntEquals(tc, 1, apx_valueTable_length(vt));
   CuAssertFalse(tc, vt->base.is_signed_range);
   CuAssertIntEquals(tc, 255, vt->base.lower_limit.u32);
   CuAssertIntEquals(tc, 255, vt->base.upper_limit.u32);
   str = apx_valueTable_get_value(vt, 0);
   CuAssertStrEquals(tc, "NotAvailable", adt_str_cstr(str));

   apx_typeAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);

}

static void test_parse_type_attribute_rational_scaling_vehicle_speed(CuTest* tc)
{
   const char* attribute_string = "RS(0, 65280, 0, 1, 64, \"km/h\")";
   const uint8_t* begin = (const uint8_t*)attribute_string;
   const uint8_t* end = begin + strlen(attribute_string);
   apx_attributeParser_t parser;
   uint8_t const* result = NULL;
   apx_typeAttributes_t attr;
   apx_rationalScaling_t* rs = NULL;

   apx_attributeParser_create(&parser);
   apx_typeAttributes_create(&attr);
   result = apx_attributeParser_parse_type_attributes(&parser, begin, end, &attr);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertIntEquals(tc, 1, apx_typeAttributes_num_computations(&attr));

   rs = (apx_rationalScaling_t*)apx_typeAttributes_get_computation(&attr, 0);
   CuAssertPtrNotNull(tc, rs);
   CuAssertFalse(tc, rs->base.is_signed_range);
   CuAssertIntEquals(tc, 0u, rs->base.lower_limit.u32);
   CuAssertIntEquals(tc, 65280u, rs->base.upper_limit.u32);
   CuAssertDblEquals(tc, 0.0, rs->offset, 0.0001);
   CuAssertIntEquals(tc, 1, rs->numerator);
   CuAssertIntEquals(tc, 64, rs->denominator);
   CuAssertStrEquals(tc, "km/h", rs->unit);

   apx_typeAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_combined_type_attributes(CuTest* tc)
{
   const char* attribute_string = "RS(0, 0xFDFF, 0, 1, 64, \"km/h\"),VT(0xFE00, 0xFEFF, \"Error\"),VT(0xFF00, 0xFFFF, \"NotAvailable\")";
   const uint8_t* begin = (const uint8_t*)attribute_string;
   const uint8_t* end = begin + strlen(attribute_string);
   apx_attributeParser_t parser;
   uint8_t const* result = NULL;
   apx_typeAttributes_t attr;
   apx_rationalScaling_t* rs = NULL;
   apx_valueTable_t* vt = NULL;
   adt_str_t* str = NULL;

   apx_attributeParser_create(&parser);
   apx_typeAttributes_create(&attr);
   result = apx_attributeParser_parse_type_attributes(&parser, begin, end, &attr);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertIntEquals(tc, 3, apx_typeAttributes_num_computations(&attr));

   rs = (apx_rationalScaling_t*)apx_typeAttributes_get_computation(&attr, 0);
   CuAssertPtrNotNull(tc, rs);
   CuAssertFalse(tc, rs->base.is_signed_range);
   CuAssertIntEquals(tc, 0u, rs->base.lower_limit.u32);
   CuAssertIntEquals(tc, 0xFDFFu, rs->base.upper_limit.u32);
   CuAssertDblEquals(tc, 0.0, rs->offset, 0.0001);
   CuAssertIntEquals(tc, 1, rs->numerator);
   CuAssertIntEquals(tc, 64, rs->denominator);
   CuAssertStrEquals(tc, "km/h", rs->unit);

   vt = (apx_valueTable_t*)apx_typeAttributes_get_computation(&attr, 1);
   CuAssertPtrNotNull(tc, vt);
   CuAssertIntEquals(tc, 1, apx_valueTable_length(vt));
   CuAssertFalse(tc, vt->base.is_signed_range);
   CuAssertIntEquals(tc, 0xFE00u, vt->base.lower_limit.u32);
   CuAssertIntEquals(tc, 0xFEFFu, vt->base.upper_limit.u32);
   str = apx_valueTable_get_value(vt, 0);
   CuAssertStrEquals(tc, "Error", adt_str_cstr(str));

   vt = (apx_valueTable_t*)apx_typeAttributes_get_computation(&attr, 2);
   CuAssertPtrNotNull(tc, vt);
   CuAssertIntEquals(tc, 1, apx_valueTable_length(vt));
   CuAssertFalse(tc, vt->base.is_signed_range);
   CuAssertIntEquals(tc, 0xFF00u, vt->base.lower_limit.u32);
   CuAssertIntEquals(tc, 0xFFFFu, vt->base.upper_limit.u32);
   str = apx_valueTable_get_value(vt, 0);
   CuAssertStrEquals(tc, "NotAvailable", adt_str_cstr(str));

   apx_typeAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);

}

static void test_parse_port_attribute_queue_length(CuTest* tc)
{

   const char* attribute_string = "Q[10]";
   const uint8_t* begin = (const uint8_t*)attribute_string;
   const uint8_t* end = begin + strlen(attribute_string);
   apx_attributeParser_t parser;
   apx_portAttributes_t attr;
   uint8_t const* result = NULL;

   apx_attributeParser_create(&parser);
   apx_portAttributes_create(&attr);

   result = apx_attributeParser_parse_port_attributes(&parser, begin, end, &attr);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertFalse(tc, attr.is_parameter);
   CuAssertUIntEquals(tc, 10u, attr.queue_length);

   apx_portAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);
}

static void test_parse_port_attribute_parameter(CuTest* tc)
{

   const char* attribute_string = "P";
   const uint8_t* begin = (const uint8_t*)attribute_string;
   const uint8_t* end = begin + strlen(attribute_string);
   apx_attributeParser_t parser;
   apx_portAttributes_t attr;
   uint8_t const* result = NULL;

   apx_attributeParser_create(&parser);
   apx_portAttributes_create(&attr);

   result = apx_attributeParser_parse_port_attributes(&parser, begin, end, &attr);
   CuAssertConstPtrEquals(tc, end, result);
   CuAssertTrue(tc, attr.is_parameter);

   apx_portAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);

}