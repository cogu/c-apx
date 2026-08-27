//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"
#include "apx/signature_parser.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_signatureParser_uint8(CuTest* tc);
static void test_signatureParser_uint8_with_limits(CuTest* tc);
static void test_signatureParser_uint8_array(CuTest* tc);
static void test_signatureParser_uint8_array_with_limits(CuTest* tc);
static void test_signatureParser_uint16(CuTest* tc);
static void test_signatureParser_uint32(CuTest* tc);
static void test_signatureParser_uint64(CuTest* tc);
static void test_signatureParser_int8(CuTest* tc);
static void test_signatureParser_int8_with_limits(CuTest* tc);
static void test_signatureParser_int8_array(CuTest* tc);
static void test_signatureParser_int8_array_with_limits(CuTest* tc);
static void test_signatureParser_int16(CuTest* tc);
static void test_signatureParser_int32(CuTest* tc);
static void test_signatureParser_int64(CuTest* tc);
static void test_signatureParser_byte_array(CuTest* tc);
static void test_signatureParser_dynamic_byte_array(CuTest* tc);
static void test_signatureParser_char_array(CuTest* tc);
static void test_signatureParser_dynamic_char_array(CuTest* tc);
static void test_signatureParser_char8_array(CuTest* tc);
static void test_signatureParser_dynamic_char8_array(CuTest* tc);
static void test_signatureParser_record_U8_U8_U8(CuTest* tc);
static void test_signatureParser_record_U8U8_U8U8(CuTest* tc);
static void test_signatureParser_record_dynstring_U16(CuTest* tc);
static void test_signatureParser_parse_error_in_record(CuTest* tc);
static void test_signatureParser_type_reference_by_id(CuTest* tc);
static void test_signatureParser_type_array_of_reference_by_id(CuTest* tc);
static void test_signatureParser_type_reference_by_name(CuTest* tc);
static void test_signatureParser_type_reference_by_id_inside_record(CuTest* tc);
static void test_signatureParser_type_reference_by_name_inside_record(CuTest* tc);
static void test_signatureParser_type_dynamic_array_of_record(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_signatureParser(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_signatureParser_uint8);
   SUITE_ADD_TEST(suite, test_signatureParser_uint8_with_limits);
   SUITE_ADD_TEST(suite, test_signatureParser_uint8_array);
   SUITE_ADD_TEST(suite, test_signatureParser_uint8_array_with_limits);
   SUITE_ADD_TEST(suite, test_signatureParser_uint16);
   SUITE_ADD_TEST(suite, test_signatureParser_uint32);
   SUITE_ADD_TEST(suite, test_signatureParser_uint64);
   SUITE_ADD_TEST(suite, test_signatureParser_int8);
   SUITE_ADD_TEST(suite, test_signatureParser_int8_with_limits);
   SUITE_ADD_TEST(suite, test_signatureParser_int8_array);
   SUITE_ADD_TEST(suite, test_signatureParser_int8_array_with_limits);
   SUITE_ADD_TEST(suite, test_signatureParser_int16);
   SUITE_ADD_TEST(suite, test_signatureParser_int32);
   SUITE_ADD_TEST(suite, test_signatureParser_int64);
   SUITE_ADD_TEST(suite, test_signatureParser_byte_array);
   SUITE_ADD_TEST(suite, test_signatureParser_dynamic_byte_array);
   SUITE_ADD_TEST(suite, test_signatureParser_char_array);
   SUITE_ADD_TEST(suite, test_signatureParser_dynamic_char_array);
   SUITE_ADD_TEST(suite, test_signatureParser_char8_array);
   SUITE_ADD_TEST(suite, test_signatureParser_dynamic_char8_array);
   SUITE_ADD_TEST(suite, test_signatureParser_record_U8_U8_U8);
   SUITE_ADD_TEST(suite, test_signatureParser_record_U8U8_U8U8);
   SUITE_ADD_TEST(suite, test_signatureParser_record_dynstring_U16);
   SUITE_ADD_TEST(suite, test_signatureParser_parse_error_in_record);
   SUITE_ADD_TEST(suite, test_signatureParser_type_reference_by_id);
   SUITE_ADD_TEST(suite, test_signatureParser_type_array_of_reference_by_id);
   SUITE_ADD_TEST(suite, test_signatureParser_type_reference_by_name);
   SUITE_ADD_TEST(suite, test_signatureParser_type_reference_by_id_inside_record);
   SUITE_ADD_TEST(suite, test_signatureParser_type_reference_by_name_inside_record);
   SUITE_ADD_TEST(suite, test_signatureParser_type_dynamic_array_of_record);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_signatureParser_uint8(CuTest* tc)
{
   const char* signature = "C";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_uint8_with_limits(CuTest* tc)
{
   const char* signature = "C(0,3)";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   uint32_t lower_limit, upper_limit;
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertTrue(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 3u, upper_limit);
   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_uint8_array(CuTest* tc)
{
   const char* signature = "C[4]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 4u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_uint8_array_with_limits(CuTest* tc)
{
   const char* signature = "C(0,3)[4]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   uint32_t lower_limit, upper_limit;
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_uint32(data_element, &lower_limit, &upper_limit));
   CuAssertUIntEquals(tc, 0u, lower_limit);
   CuAssertUIntEquals(tc, 3u, upper_limit);
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 4u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_uint16(CuTest* tc)
{
   const char* signature = "S";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT16, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_uint32(CuTest* tc)
{
   const char* signature = "L";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT32, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_uint64(CuTest* tc)
{
   const char* signature = "Q";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT64, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_int8(CuTest* tc)
{
   const char* signature = "c";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_INT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_int8_with_limits(CuTest* tc)
{
   const char* signature = "c(-10,10)";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   int32_t lower_limit, upper_limit;
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_INT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertTrue(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_int32(data_element, &lower_limit, &upper_limit));
   CuAssertIntEquals(tc, -10, lower_limit);
   CuAssertIntEquals(tc, 10, upper_limit);
   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_int8_array(CuTest* tc)
{
   const char* signature = "c[6]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_INT8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 6u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_int8_array_with_limits(CuTest* tc)
{
   const char* signature = "c(-1,10)[6]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   int32_t lower_limit, upper_limit;
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_INT8, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_get_limits_int32(data_element, &lower_limit, &upper_limit));
   CuAssertIntEquals(tc, -1, lower_limit);
   CuAssertIntEquals(tc, 10, upper_limit);
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 6u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_int16(CuTest* tc)
{
   const char* signature = "s";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_INT16, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_int32(CuTest* tc)
{
   const char* signature = "l";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_INT32, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_int64(CuTest* tc)
{
   const char* signature = "q";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_INT64, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_byte_array(CuTest* tc)
{
   const char* signature = "B[8]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_BYTE, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 8u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_dynamic_byte_array(CuTest* tc)
{
   const char* signature = "B[8*]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_BYTE, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertTrue(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 8u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_char_array(CuTest* tc)
{
   const char* signature = "a[32]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 32u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_dynamic_char_array(CuTest* tc)
{
   const char* signature = "a[32*]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertTrue(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 32u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_char8_array(CuTest* tc)
{
   const char* signature = "A[32]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertFalse(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 32u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_dynamic_char8_array(CuTest* tc)
{
   const char* signature = "A[32*]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR8, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertTrue(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertUIntEquals(tc, 32u, apx_dataElement_get_array_length(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_record_U8_U8_U8(CuTest* tc)
{
   const char* signature = "{\"ID\"C(0,127)\"Stat\"C(0,3)\"Type\"C(0,7)}";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertUIntEquals(tc, 3, apx_dataElement_get_num_child_elements(data_element));
   apx_dataElement_t* child_element;
   child_element = apx_dataElement_get_child_at(data_element, 0);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_element));
   CuAssertTrue(tc, apx_dataElement_has_limits(child_element));
   child_element = apx_dataElement_get_child_at(data_element, 1);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_element));
   CuAssertTrue(tc, apx_dataElement_has_limits(child_element));
   child_element = apx_dataElement_get_child_at(data_element, 2);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(child_element));
   CuAssertTrue(tc, apx_dataElement_has_limits(child_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_record_U8U8_U8U8(CuTest* tc)
{
   const char* signature = "{\"Notification1\"{\"ID1\"C(0,127)\"Stat1\"C(0,3)}\"Notification2\"{\"ID2\"C(0,127)\"Stat2\"C(0,3)}}";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_has_limits(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(data_element));
   apx_dataElement_t* child_element;
   child_element = apx_dataElement_get_child_at(data_element, 0);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(child_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(child_element));
   CuAssertStrEquals(tc, "Notification1", apx_dataElement_get_name(child_element));
   apx_dataElement_t* grand_child;
   grand_child = apx_dataElement_get_child_at(child_element, 0);
   CuAssertPtrNotNull(tc, grand_child);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(grand_child));
   CuAssertStrEquals(tc, "ID1", apx_dataElement_get_name(grand_child));
   grand_child = apx_dataElement_get_child_at(child_element, 1);
   CuAssertPtrNotNull(tc, grand_child);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(grand_child));
   CuAssertStrEquals(tc, "Stat1", apx_dataElement_get_name(grand_child));
   child_element = apx_dataElement_get_child_at(data_element, 1);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(child_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(child_element));
   CuAssertStrEquals(tc, "Notification2", apx_dataElement_get_name(child_element));
   grand_child = apx_dataElement_get_child_at(child_element, 0);
   CuAssertPtrNotNull(tc, grand_child);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(grand_child));
   CuAssertStrEquals(tc, "ID2", apx_dataElement_get_name(grand_child));
   grand_child = apx_dataElement_get_child_at(child_element, 1);
   CuAssertPtrNotNull(tc, grand_child);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT8, apx_dataElement_get_type_code(grand_child));
   CuAssertStrEquals(tc, "Stat2", apx_dataElement_get_name(grand_child));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);

}

static void test_signatureParser_record_dynstring_U16(CuTest* tc)
{
   const char* signature = "{\"Name\"a[32*]\"ID\"S}";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertUIntEquals(tc, 2, apx_dataElement_get_num_child_elements(data_element));
   apx_dataElement_t* child_element;
   child_element = apx_dataElement_get_child_at(data_element, 0);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_CHAR, apx_dataElement_get_type_code(child_element));
   CuAssertStrEquals(tc, "Name", apx_dataElement_get_name(child_element));
   CuAssertTrue(tc, apx_dataElement_is_array(child_element));
   CuAssertTrue(tc, apx_dataElement_is_dynamic_array(child_element));
   child_element = apx_dataElement_get_child_at(data_element, 1);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT16, apx_dataElement_get_type_code(child_element));
   CuAssertStrEquals(tc, "ID", apx_dataElement_get_name(child_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);

}

static void test_signatureParser_parse_error_in_record(CuTest* tc)
{
   const char* signature = "{{\"ID\"C(0,127)\"Stat\"C(0,3)\"Type\"C(0,7)}";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, NULL, result);

   apx_signatureParser_destroy(&parser);
}

static void test_signatureParser_type_reference_by_id(CuTest* tc)
{
   const char* signature = "T[0]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_ID, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertUIntEquals(tc, 0u, apx_dataElement_get_type_ref_id(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_type_array_of_reference_by_id(CuTest* tc)
{
   const char* signature = "T[2][8]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_ID, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertIntEquals(tc, 8, apx_dataElement_get_array_length(data_element));
   CuAssertUIntEquals(tc, 2u, apx_dataElement_get_type_ref_id(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_type_reference_by_name(CuTest* tc)
{

   const char* signature = "T[\"TypeName\"]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_NAME, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertStrEquals(tc, "TypeName", apx_dataElement_get_type_ref_name(data_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);

}

static void test_signatureParser_type_reference_by_id_inside_record(CuTest* tc)
{
   const char* signature = "{\"First\"T[0]\"Second\"T[1]}";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertIntEquals(tc, 2, apx_dataElement_get_num_child_elements(data_element));
   apx_dataElement_t* child_element = apx_dataElement_get_child_at(data_element, 0);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_ID, apx_dataElement_get_type_code(child_element));
   CuAssertStrEquals(tc, "First", apx_dataElement_get_name(child_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_element));
   CuAssertUIntEquals(tc, 0u, apx_dataElement_get_type_ref_id(child_element));
   child_element = apx_dataElement_get_child_at(data_element, 1);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_ID, apx_dataElement_get_type_code(child_element));
   CuAssertStrEquals(tc, "Second", apx_dataElement_get_name(child_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_element));
   CuAssertUIntEquals(tc, 1u, apx_dataElement_get_type_ref_id(child_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_type_reference_by_name_inside_record(CuTest* tc)
{
   const char* signature = "{\"First\"T[\"Type1\"]\"Second\"T[\"Type2\"]}";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(data_element));
   CuAssertFalse(tc, apx_dataElement_is_array(data_element));
   CuAssertIntEquals(tc, 2, apx_dataElement_get_num_child_elements(data_element));
   apx_dataElement_t* child_element = apx_dataElement_get_child_at(data_element, 0);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_NAME, apx_dataElement_get_type_code(child_element));
   CuAssertStrEquals(tc, "First", apx_dataElement_get_name(child_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_element));
   CuAssertStrEquals(tc, "Type1", apx_dataElement_get_type_ref_name(child_element));
   child_element = apx_dataElement_get_child_at(data_element, 1);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_REF_NAME, apx_dataElement_get_type_code(child_element));
   CuAssertStrEquals(tc, "Second", apx_dataElement_get_name(child_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_element));
   CuAssertStrEquals(tc, "Type2", apx_dataElement_get_type_ref_name(child_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}

static void test_signatureParser_type_dynamic_array_of_record(CuTest* tc)
{
   const char* signature = "{\"Id\"S\"Status\"b}[50*]";
   const uint8_t* begin = (const uint8_t*)signature;
   const uint8_t* end = begin + strlen(signature);
   apx_signatureParser_t parser;
   apx_signatureParser_create(&parser);
   const uint8_t* result = apx_signatureParser_parse_data_signature(&parser, begin, end);
   CuAssertConstPtrEquals(tc, end, result);
   apx_dataElement_t* data_element = apx_signatureParser_take_data_element(&parser);
   CuAssertPtrNotNull(tc, data_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_RECORD, apx_dataElement_get_type_code(data_element));
   CuAssertTrue(tc, apx_dataElement_is_array(data_element));
   CuAssertTrue(tc, apx_dataElement_is_dynamic_array(data_element));
   CuAssertIntEquals(tc, 50, apx_dataElement_get_array_length(data_element));
   CuAssertIntEquals(tc, 2, apx_dataElement_get_num_child_elements(data_element));
   apx_dataElement_t* child_element = apx_dataElement_get_child_at(data_element, 0);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_UINT16, apx_dataElement_get_type_code(child_element));
   CuAssertStrEquals(tc, "Id", apx_dataElement_get_name(child_element));
   CuAssertFalse(tc, apx_dataElement_is_array(child_element));
   child_element = apx_dataElement_get_child_at(data_element, 1);
   CuAssertPtrNotNull(tc, child_element);
   CuAssertUIntEquals(tc, APX_TYPE_CODE_BOOL, apx_dataElement_get_type_code(child_element));
   CuAssertStrEquals(tc, "Status", apx_dataElement_get_name(child_element));

   apx_signatureParser_destroy(&parser);
   apx_dataElement_delete(data_element);
}
