//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/serializer.h"
#include "apx/vm_defs.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_set_write_buffer(CuTest* tc);
static void test_set_scalar_value(CuTest* tc);
static void test_set_array_value(CuTest* tc);
static void test_set_hash_value(CuTest* tc);
static void test_pack_uint8_from_int32_value(CuTest* tc);
static void test_pack_uint8_from_uint32_value(CuTest* tc);
static void test_pack_uint8_from_value_out_of_range_returns_range_error(CuTest* tc);
static void test_pack_uint8_from_negative_value_returns_conversion_error(CuTest* tc);
static void test_pack_uint8_array(CuTest* tc);
static void test_pack_char_string(CuTest* tc);
static void test_pack_shorter_char_string(CuTest* tc);
static void test_pack_too_large_char_string(CuTest* tc);
static void test_pack_char8_string(CuTest* tc);
static void test_pack_record_uint8(CuTest* tc);
static void test_pack_record_uint8_with_range_check(CuTest* tc);
static void test_pack_uint16_from_int32_value(CuTest* tc);
static void test_pack_uint16_from_uint32_value(CuTest* tc);
static void test_pack_uint16_array(CuTest* tc);
static void test_pack_uint16_array_with_range_check(CuTest* tc);
static void test_pack_uint32_value(CuTest* tc);
static void test_pack_uint32_array(CuTest* tc);
static void test_pack_uint32_array_with_range_check(CuTest* tc);
static void test_range_check_uint32_value(CuTest* tc);
static void test_range_check_uint32_array_with_mixed_value_types(CuTest* tc);
static void test_range_check_uint32_array_with_out_of_range_value(CuTest* tc);
static void test_range_check_uint32_array_with_negative_value(CuTest* tc);
static void test_range_check_int32_value(CuTest* tc);
static void test_range_check_int32_array_with_mixed_value_types(CuTest* tc);
static void test_range_check_int32_array_with_out_of_range_value(CuTest* tc);
static void test_range_check_uint64_value(CuTest* tc);
static void test_range_check_int64_value(CuTest* tc);
static void test_pack_int8_from_int32_value(CuTest* tc);
static void test_pack_int8_from_value_out_of_range_returns_range_error(CuTest* tc);
static void test_pack_int8_array(CuTest* tc);
static void test_pack_int16_from_int32_value(CuTest* tc);
static void test_pack_int16_from_value_out_of_range_returns_range_error(CuTest* tc);
static void test_pack_int16_array(CuTest* tc);
static void test_pack_int32_value(CuTest* tc);
static void test_pack_int32_from_value_out_of_range_returns_conversion_error(CuTest* tc);
static void test_pack_int32_array(CuTest* tc);
static void test_pack_int64_value(CuTest* tc);
static void test_pack_int64_from_value_out_of_range_returns_conversion_error(CuTest* tc);
static void test_pack_int64_array(CuTest* tc);
static void test_pack_uint64_value(CuTest* tc);
static void test_pack_uint64_from_negative_value_returns_conversion_error(CuTest* tc);
static void test_pack_uint64_array(CuTest* tc);
static void test_pack_bool_from_bool_value(CuTest* tc);
static void test_pack_bool_from_uint32_value(CuTest* tc);
static void test_pack_single_byte(CuTest* tc);
static void test_pack_byte_array(CuTest* tc);
static void test_pack_zero_length_byte_array_is_treated_as_length_one(CuTest* tc);
static void test_pack_byte_array_with_inconsistent_length_returns_length_error(CuTest* tc);
static void test_pack_dynamic_uint8_array_with_uint8_length(CuTest* tc);
static void test_pack_dynamic_uint16_array_with_uint8_length(CuTest* tc);
static void test_pack_dynamic_uint32_array_with_uint8_length(CuTest* tc);
static void test_pack_dynamic_int8_array_with_uint8_length(CuTest* tc);
static void test_pack_dynamic_int16_array_with_uint8_length(CuTest* tc);
static void test_pack_dynamic_int32_array_with_uint8_length(CuTest* tc);
static void test_pack_dynamic_bool_array_with_uint8_length(CuTest* tc);
static void test_pack_dynamic_byte_array_with_uint8_length(CuTest* tc);
static void test_pack_dynamic_byte_array_with_uint16_length(CuTest* tc);
static void test_pack_dynamic_string_with_uint8_length(CuTest* tc);
static void test_pack_string_in_record(CuTest* tc);
static void test_pack_dynamic_string_in_record(CuTest* tc);
static void test_pack_dynamic_uint16_array_in_record(CuTest* tc);
static void test_pack_uint8_queued_element(CuTest* tc);
static void test_pack_uint8_multiple_queued_elements(CuTest* tc);
static void test_pack_record_inside_record__uint8_uint16__uint16_uint32(CuTest* tc);
static void test_pack_array_of_record_uint16_uint8(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_vm_serializer(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_set_write_buffer);
   SUITE_ADD_TEST(suite, test_set_scalar_value);
   SUITE_ADD_TEST(suite, test_set_array_value);
   SUITE_ADD_TEST(suite, test_set_hash_value);
   SUITE_ADD_TEST(suite, test_pack_uint8_from_int32_value);
   SUITE_ADD_TEST(suite, test_pack_uint8_from_uint32_value);
   SUITE_ADD_TEST(suite, test_pack_uint8_from_value_out_of_range_returns_range_error);
   SUITE_ADD_TEST(suite, test_pack_uint8_from_negative_value_returns_conversion_error);
   SUITE_ADD_TEST(suite, test_pack_uint8_array);
   SUITE_ADD_TEST(suite, test_pack_char_string);
   SUITE_ADD_TEST(suite, test_pack_shorter_char_string);
   SUITE_ADD_TEST(suite, test_pack_too_large_char_string);
   SUITE_ADD_TEST(suite, test_pack_char8_string);
   SUITE_ADD_TEST(suite, test_pack_record_uint8);
   SUITE_ADD_TEST(suite, test_pack_record_uint8_with_range_check);
   SUITE_ADD_TEST(suite, test_pack_uint16_from_int32_value);
   SUITE_ADD_TEST(suite, test_pack_uint16_from_uint32_value);
   SUITE_ADD_TEST(suite, test_pack_uint16_array);
   SUITE_ADD_TEST(suite, test_pack_uint16_array_with_range_check);
   SUITE_ADD_TEST(suite, test_pack_uint32_value);
   SUITE_ADD_TEST(suite, test_pack_uint32_array);
   SUITE_ADD_TEST(suite, test_pack_uint32_array_with_range_check);
   SUITE_ADD_TEST(suite, test_range_check_uint32_value);
   SUITE_ADD_TEST(suite, test_range_check_uint32_array_with_mixed_value_types);
   SUITE_ADD_TEST(suite, test_range_check_uint32_array_with_out_of_range_value);
   SUITE_ADD_TEST(suite, test_range_check_uint32_array_with_negative_value);
   SUITE_ADD_TEST(suite, test_range_check_int32_value);
   SUITE_ADD_TEST(suite, test_range_check_int32_array_with_mixed_value_types);
   SUITE_ADD_TEST(suite, test_range_check_int32_array_with_out_of_range_value);
   SUITE_ADD_TEST(suite, test_range_check_uint64_value);
   SUITE_ADD_TEST(suite, test_range_check_int64_value);
   SUITE_ADD_TEST(suite, test_pack_int8_from_int32_value);
   SUITE_ADD_TEST(suite, test_pack_int8_from_value_out_of_range_returns_range_error);
   SUITE_ADD_TEST(suite, test_pack_int8_array);
   SUITE_ADD_TEST(suite, test_pack_int16_from_int32_value);
   SUITE_ADD_TEST(suite, test_pack_int16_from_value_out_of_range_returns_range_error);
   SUITE_ADD_TEST(suite, test_pack_int16_array);
   SUITE_ADD_TEST(suite, test_pack_int32_value);
   SUITE_ADD_TEST(suite, test_pack_int32_from_value_out_of_range_returns_conversion_error);
   SUITE_ADD_TEST(suite, test_pack_int32_array);
   SUITE_ADD_TEST(suite, test_pack_int64_value);
   SUITE_ADD_TEST(suite, test_pack_int64_from_value_out_of_range_returns_conversion_error);
   SUITE_ADD_TEST(suite, test_pack_int64_array);
   SUITE_ADD_TEST(suite, test_pack_uint64_value);
   SUITE_ADD_TEST(suite, test_pack_uint64_from_negative_value_returns_conversion_error);
   SUITE_ADD_TEST(suite, test_pack_uint64_array);
   SUITE_ADD_TEST(suite, test_pack_bool_from_bool_value);
   SUITE_ADD_TEST(suite, test_pack_bool_from_uint32_value);
   SUITE_ADD_TEST(suite, test_pack_single_byte);
   SUITE_ADD_TEST(suite, test_pack_byte_array);
   SUITE_ADD_TEST(suite, test_pack_zero_length_byte_array_is_treated_as_length_one);
   SUITE_ADD_TEST(suite, test_pack_byte_array_with_inconsistent_length_returns_length_error);
   SUITE_ADD_TEST(suite, test_pack_dynamic_uint8_array_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_uint16_array_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_uint32_array_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_int8_array_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_int16_array_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_int32_array_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_bool_array_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_byte_array_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_byte_array_with_uint16_length);
   SUITE_ADD_TEST(suite, test_pack_dynamic_string_with_uint8_length);
   SUITE_ADD_TEST(suite, test_pack_string_in_record);
   SUITE_ADD_TEST(suite, test_pack_dynamic_string_in_record);
   SUITE_ADD_TEST(suite, test_pack_dynamic_uint16_array_in_record);
   SUITE_ADD_TEST(suite, test_pack_uint8_queued_element);
   SUITE_ADD_TEST(suite, test_pack_uint8_multiple_queued_elements);
   SUITE_ADD_TEST(suite, test_pack_record_inside_record__uint8_uint16__uint16_uint32);
   SUITE_ADD_TEST(suite, test_pack_array_of_record_uint16_uint8);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_set_write_buffer(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[10];
   apx_vm_serializer_create(&sr);
   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_vm_serializer_set_write_buffer(&sr, NULL, sizeof(buf)));
   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], 0u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   apx_vm_serializer_destroy(&sr);
}

static void test_set_scalar_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_sv_t* sv = dtl_sv_make_i32(0);
   apx_vm_serializer_create(&sr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, DTL_DV_SCALAR, sr.state->value_type);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_set_array_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, sr.state->value_type);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_set_hash_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_hv_t* hv = dtl_hv_new();
   apx_vm_serializer_create(&sr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_hv(&sr, hv));
   CuAssertIntEquals(tc, DTL_DV_HASH, sr.state->value_type);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(hv);
}

static void test_pack_uint8_from_int32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(7);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE, (unsigned int) apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 7u, buf[0]);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint8_from_uint32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(255);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE, (unsigned int) apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 255u, buf[0]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);

}

static void test_pack_uint8_from_value_out_of_range_returns_range_error(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(256);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0u, (unsigned int)apx_vm_serializer_bytes_written(&sr));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint8_from_negative_value_returns_conversion_error(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(-1);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_CONVERSION_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0u, (unsigned int)apx_vm_serializer_bytes_written(&sr));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint8_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE*4];
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(1), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(2), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(3), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(4), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE * array_length, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 1u, buf[0]);
   CuAssertUIntEquals(tc, 2u, buf[1]);
   CuAssertUIntEquals(tc, 3u, buf[2]);
   CuAssertUIntEquals(tc, 4u, buf[3]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_char_string(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE*4];
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_cstr("Test");
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_char(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE*4, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 'T', buf[0]);
   CuAssertUIntEquals(tc, 'e', buf[1]);
   CuAssertUIntEquals(tc, 's', buf[2]);
   CuAssertUIntEquals(tc, 't', buf[3]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_shorter_char_string(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE * 6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
   uint32_t const array_length = 6u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_cstr("Tst");
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_char(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE * 6, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 'T', buf[0]);
   CuAssertUIntEquals(tc, 's', buf[1]);
   CuAssertUIntEquals(tc, 't', buf[2]);
   CuAssertUIntEquals(tc, '\0', buf[3]);
   CuAssertUIntEquals(tc, '\0', buf[4]);
   CuAssertUIntEquals(tc, '\0', buf[5]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_too_large_char_string(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE * 4];
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_cstr("Short");
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vm_serializer_pack_char(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0, (unsigned int)apx_vm_serializer_bytes_written(&sr));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_char8_string(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE * 4];
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_cstr("Test");
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_char8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE * 4, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 'T', buf[0]);
   CuAssertUIntEquals(tc, 'e', buf[1]);
   CuAssertUIntEquals(tc, 's', buf[2]);
   CuAssertUIntEquals(tc, 't', buf[3]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_record_uint8(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE * 3];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   memset(buf, 0xFF, sizeof(buf));
   dtl_hv_t* hv = dtl_hv_new();
   apx_vm_serializer_create(&sr);

   dtl_hv_set_cstr(hv, "Red", (dtl_dv_t*)dtl_sv_make_u32(0x02), false);
   dtl_hv_set_cstr(hv, "Green", (dtl_dv_t*)dtl_sv_make_u32(0x12), false);
   dtl_hv_set_cstr(hv, "Blue", (dtl_dv_t*)dtl_sv_make_u32(0xaa), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_hv(&sr, hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Red", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Green", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Blue", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE * 3, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x02, buf[0]);
   CuAssertUIntEquals(tc, 0x12u, buf[1]);
   CuAssertUIntEquals(tc, 0xaau, buf[2]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(hv);
}

static void test_pack_record_uint8_with_range_check(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE * 3];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   memset(buf, 0xFF, sizeof(buf));
   dtl_hv_t* hv = dtl_hv_new();
   apx_vm_serializer_create(&sr);

   dtl_hv_set_cstr(hv, "First", (dtl_dv_t*)dtl_sv_make_u32(0x7), false);
   dtl_hv_set_cstr(hv, "Second", (dtl_dv_t*)dtl_sv_make_u32(0x3), false);
   dtl_hv_set_cstr(hv, "Third", (dtl_dv_t*)dtl_sv_make_u32(0x3), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_hv(&sr, hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 7u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Second", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 3u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Third", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 3u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE * 3, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x7, buf[0]);
   CuAssertUIntEquals(tc, 0x3, buf[1]);
   CuAssertUIntEquals(tc, 0x3, buf[2]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(hv);
}

static void test_pack_uint16_from_int32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT16_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(0x1234);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x34u, buf[0]);
   CuAssertUIntEquals(tc, 0x12u, buf[1]);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint16_from_uint32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT16_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_u32(0xffff);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int) sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0xffu, buf[0]);
   CuAssertUIntEquals(tc, 0xffu, buf[1]);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint16_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT16_SIZE * 5];
   uint32_t const array_length = 5u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(0x11a), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(0x21b), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(0x31c), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(0x41d), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(0x51e), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x1au, buf[0]);
   CuAssertUIntEquals(tc, 0x01u, buf[1]);
   CuAssertUIntEquals(tc, 0x1bu, buf[2]);
   CuAssertUIntEquals(tc, 0x02u, buf[3]);
   CuAssertUIntEquals(tc, 0x1cu, buf[4]);
   CuAssertUIntEquals(tc, 0x03u, buf[5]);
   CuAssertUIntEquals(tc, 0x1du, buf[6]);
   CuAssertUIntEquals(tc, 0x04u, buf[7]);
   CuAssertUIntEquals(tc, 0x1eu, buf[8]);
   CuAssertUIntEquals(tc, 0x05u, buf[9]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_uint16_array_with_range_check(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT16_SIZE * 2];
   uint32_t const array_length = 2u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(999), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(999), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0, 999));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0xe7u, buf[0]);
   CuAssertUIntEquals(tc, 0x03u, buf[1]);
   CuAssertUIntEquals(tc, 0xe7u, buf[2]);
   CuAssertUIntEquals(tc, 0x03u, buf[3]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_uint32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT32_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_u32(0x12345678);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint32(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x78u, buf[0]);
   CuAssertUIntEquals(tc, 0x56u, buf[1]);
   CuAssertUIntEquals(tc, 0x34u, buf[2]);
   CuAssertUIntEquals(tc, 0x12u, buf[3]);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint32_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT32_SIZE * 2];
   uint32_t const array_length = 2u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(100000), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(200000), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint32(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0xa0u, buf[0]);
   CuAssertUIntEquals(tc, 0x86u, buf[1]);
   CuAssertUIntEquals(tc, 0x01u, buf[2]);
   CuAssertUIntEquals(tc, 0x00u, buf[3]);
   CuAssertUIntEquals(tc, 0x40u, buf[4]);
   CuAssertUIntEquals(tc, 0x0du, buf[5]);
   CuAssertUIntEquals(tc, 0x03u, buf[6]);
   CuAssertUIntEquals(tc, 0x00u, buf[7]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_uint32_array_with_range_check(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT32_SIZE * 2];
   uint32_t const array_length = 2u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(99999), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(99999), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0, 99999));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint32(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x9fu, buf[0]);
   CuAssertUIntEquals(tc, 0x86u, buf[1]);
   CuAssertUIntEquals(tc, 0x01u, buf[2]);
   CuAssertUIntEquals(tc, 0x00u, buf[3]);
   CuAssertUIntEquals(tc, 0x9fu, buf[4]);
   CuAssertUIntEquals(tc, 0x86u, buf[5]);
   CuAssertUIntEquals(tc, 0x01u, buf[6]);
   CuAssertUIntEquals(tc, 0x00u, buf[7]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_range_check_uint32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_sv_t* sv = dtl_sv_make_u32(0u);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 7u));
   dtl_sv_set_u32(sv, 7u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 7u));
   dtl_sv_set_u32(sv, 8u);
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 7u));
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_range_check_uint32_array_with_mixed_value_types(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(7), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(3), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 7u));
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_range_check_uint32_array_with_out_of_range_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(8), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(7), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 7u));
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_range_check_uint32_array_with_negative_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(7), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-1), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_VALUE_CONVERSION_ERROR, apx_vm_serializer_check_value_range_uint32(&sr, 0u, 7u));
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_range_check_int32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_sv_t* sv = dtl_sv_make_i32(11);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_check_value_range_int32(&sr, -10, 10));
   dtl_sv_set_i32(sv, 10);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_int32(&sr, -10, 10));
   dtl_sv_set_i32(sv, 0);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_int32(&sr, -10, 10));
   dtl_sv_set_i32(sv, -10);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_int32(&sr, -10, 10));
   dtl_sv_set_i32(sv, -11);
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_check_value_range_int32(&sr, -10, 10));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_range_check_int32_array_with_mixed_value_types(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-10), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(9), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(3), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_int32(&sr, -10, 10));
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_range_check_int32_array_with_out_of_range_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-10), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(9), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(11), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_check_value_range_int32(&sr, -10, 10));
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_range_check_uint64_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_sv_t* sv = dtl_sv_make_u64(250000001ULL);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_check_value_range_uint64(&sr, 0u, 250000000ull));
   dtl_sv_set_u64(sv, 250000000ULL);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint64(&sr, 0u, 250000000ull));
   dtl_sv_set_u64(sv, 0);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_uint64(&sr, 0u, 250000000ull));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_range_check_int64_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   dtl_sv_t* sv = dtl_sv_make_i64(250000001ULL);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_check_value_range_int64(&sr, -250000000, 250000000ll));
   dtl_sv_set_i64(sv, 250000000ll);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_int64(&sr, -250000000LL, 250000000LL));
   dtl_sv_set_i64(sv, 0);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_int64(&sr, -250000000LL, 250000000LL));
   dtl_sv_set_i64(sv, -250000000ll);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_check_value_range_int64(&sr, -250000000LL, 250000000LL));
   dtl_sv_set_i64(sv, -250000001LL);
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_check_value_range_int64(&sr, -250000000LL, 250000000LL));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int8_from_int32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(-128);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x80, buf[0]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int8_from_value_out_of_range_returns_range_error(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(128);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_pack_int8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0u, (unsigned int)apx_vm_serializer_bytes_written(&sr));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int8_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE * 4];
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-128), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(127), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int8(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x80u, buf[0]);
   CuAssertUIntEquals(tc, 0xffu, buf[1]);
   CuAssertUIntEquals(tc, 0x00u, buf[2]);
   CuAssertUIntEquals(tc, 0x7fu, buf[3]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_int16_from_int32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT16_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(-32768);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int16(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x00u, buf[0]);
   CuAssertUIntEquals(tc, 0x80u, buf[1]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int16_from_value_out_of_range_returns_range_error(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT16_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(32768);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_serializer_pack_int16(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0u, (unsigned int)apx_vm_serializer_bytes_written(&sr));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int16_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT16_SIZE * 4];
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-32768), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(32767), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int16(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x00u, buf[0]);
   CuAssertUIntEquals(tc, 0x80u, buf[1]);
   CuAssertUIntEquals(tc, 0xffu, buf[2]);
   CuAssertUIntEquals(tc, 0xffu, buf[3]);
   CuAssertUIntEquals(tc, 0x00u, buf[4]);
   CuAssertUIntEquals(tc, 0x00u, buf[5]);
   CuAssertUIntEquals(tc, 0xffu, buf[6]);
   CuAssertUIntEquals(tc, 0x7fu, buf[7]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_int32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT32_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(INT32_MIN);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int32(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x00u, buf[0]);
   CuAssertUIntEquals(tc, 0x00u, buf[1]);
   CuAssertUIntEquals(tc, 0x00u, buf[2]);
   CuAssertUIntEquals(tc, 0x80u, buf[3]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int32_from_value_out_of_range_returns_conversion_error(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT32_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_u32(0x80000000UL);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_CONVERSION_ERROR, apx_vm_serializer_pack_int32(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0u, (unsigned int)apx_vm_serializer_bytes_written(&sr));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int32_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT32_SIZE * 4];
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(INT32_MIN), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(INT32_MAX), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int32(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x00u, buf[0]);
   CuAssertUIntEquals(tc, 0x00u, buf[1]);
   CuAssertUIntEquals(tc, 0x00u, buf[2]);
   CuAssertUIntEquals(tc, 0x80u, buf[3]);
   CuAssertUIntEquals(tc, 0xffu, buf[4]);
   CuAssertUIntEquals(tc, 0xffu, buf[5]);
   CuAssertUIntEquals(tc, 0xffu, buf[6]);
   CuAssertUIntEquals(tc, 0xffu, buf[7]);
   CuAssertUIntEquals(tc, 0x00u, buf[8]);
   CuAssertUIntEquals(tc, 0x00u, buf[9]);
   CuAssertUIntEquals(tc, 0x00u, buf[10]);
   CuAssertUIntEquals(tc, 0x00u, buf[11]);
   CuAssertUIntEquals(tc, 0xffu, buf[12]);
   CuAssertUIntEquals(tc, 0xffu, buf[13]);
   CuAssertUIntEquals(tc, 0xffu, buf[14]);
   CuAssertUIntEquals(tc, 0x7fu, buf[15]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_int64_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT64_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i64(INT64_MIN);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int64(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x00u, buf[0]);
   CuAssertUIntEquals(tc, 0x00u, buf[1]);
   CuAssertUIntEquals(tc, 0x00u, buf[2]);
   CuAssertUIntEquals(tc, 0x00u, buf[3]);
   CuAssertUIntEquals(tc, 0x00u, buf[4]);
   CuAssertUIntEquals(tc, 0x00u, buf[5]);
   CuAssertUIntEquals(tc, 0x00u, buf[6]);
   CuAssertUIntEquals(tc, 0x80u, buf[7]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int64_from_value_out_of_range_returns_conversion_error(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT32_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_u64(0x8000000000000000ULL);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_CONVERSION_ERROR, apx_vm_serializer_pack_int64(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0u, (unsigned int)apx_vm_serializer_bytes_written(&sr));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_int64_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT64_SIZE * 4];
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i64(INT64_MIN), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i64(-1), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i64(0), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i64(INT64_MAX), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int64(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x00u, buf[0]);
   CuAssertUIntEquals(tc, 0x00u, buf[1]);
   CuAssertUIntEquals(tc, 0x00u, buf[2]);
   CuAssertUIntEquals(tc, 0x00u, buf[3]);
   CuAssertUIntEquals(tc, 0x00u, buf[4]);
   CuAssertUIntEquals(tc, 0x00u, buf[5]);
   CuAssertUIntEquals(tc, 0x00u, buf[6]);
   CuAssertUIntEquals(tc, 0x80u, buf[7]);
   CuAssertUIntEquals(tc, 0xffu, buf[8]);
   CuAssertUIntEquals(tc, 0xffu, buf[9]);
   CuAssertUIntEquals(tc, 0xffu, buf[10]);
   CuAssertUIntEquals(tc, 0xffu, buf[11]);
   CuAssertUIntEquals(tc, 0xffu, buf[12]);
   CuAssertUIntEquals(tc, 0xffu, buf[13]);
   CuAssertUIntEquals(tc, 0xffu, buf[14]);
   CuAssertUIntEquals(tc, 0xffu, buf[15]);
   CuAssertUIntEquals(tc, 0x00u, buf[16]);
   CuAssertUIntEquals(tc, 0x00u, buf[17]);
   CuAssertUIntEquals(tc, 0x00u, buf[18]);
   CuAssertUIntEquals(tc, 0x00u, buf[19]);
   CuAssertUIntEquals(tc, 0x00u, buf[20]);
   CuAssertUIntEquals(tc, 0x00u, buf[21]);
   CuAssertUIntEquals(tc, 0x00u, buf[22]);
   CuAssertUIntEquals(tc, 0x00u, buf[23]);
   CuAssertUIntEquals(tc, 0xffu, buf[24]);
   CuAssertUIntEquals(tc, 0xffu, buf[25]);
   CuAssertUIntEquals(tc, 0xffu, buf[26]);
   CuAssertUIntEquals(tc, 0xffu, buf[27]);
   CuAssertUIntEquals(tc, 0xffu, buf[28]);
   CuAssertUIntEquals(tc, 0xffu, buf[29]);
   CuAssertUIntEquals(tc, 0xffu, buf[30]);
   CuAssertUIntEquals(tc, 0x7fu, buf[31]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_uint64_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT64_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_u64(UINT64_MAX);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint64(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0xffu, buf[0]);
   CuAssertUIntEquals(tc, 0xffu, buf[1]);
   CuAssertUIntEquals(tc, 0xffu, buf[2]);
   CuAssertUIntEquals(tc, 0xffu, buf[3]);
   CuAssertUIntEquals(tc, 0xffu, buf[4]);
   CuAssertUIntEquals(tc, 0xffu, buf[5]);
   CuAssertUIntEquals(tc, 0xffu, buf[6]);
   CuAssertUIntEquals(tc, 0xffu, buf[7]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint64_from_negative_value_returns_conversion_error(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT32_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i64(-1);
   apx_vm_serializer_create(&sr);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_CONVERSION_ERROR, apx_vm_serializer_pack_uint64(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0u, (unsigned int)apx_vm_serializer_bytes_written(&sr));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint64_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT64_SIZE * 3];
   uint32_t const array_length = 3u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u64(0), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u64(0x0123456789ABCDEFULL), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u64(UINT64_MAX), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint64(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x00u, buf[0]);
   CuAssertUIntEquals(tc, 0x00u, buf[1]);
   CuAssertUIntEquals(tc, 0x00u, buf[2]);
   CuAssertUIntEquals(tc, 0x00u, buf[3]);
   CuAssertUIntEquals(tc, 0x00u, buf[4]);
   CuAssertUIntEquals(tc, 0x00u, buf[5]);
   CuAssertUIntEquals(tc, 0x00u, buf[6]);
   CuAssertUIntEquals(tc, 0x00u, buf[7]);
   CuAssertUIntEquals(tc, 0xEFu, buf[8]);
   CuAssertUIntEquals(tc, 0xCDu, buf[9]);
   CuAssertUIntEquals(tc, 0xABu, buf[10]);
   CuAssertUIntEquals(tc, 0x89u, buf[11]);
   CuAssertUIntEquals(tc, 0x67u, buf[12]);
   CuAssertUIntEquals(tc, 0x45u, buf[13]);
   CuAssertUIntEquals(tc, 0x23u, buf[14]);
   CuAssertUIntEquals(tc, 0x01u, buf[15]);
   CuAssertUIntEquals(tc, 0xffu, buf[16]);
   CuAssertUIntEquals(tc, 0xffu, buf[17]);
   CuAssertUIntEquals(tc, 0xffu, buf[18]);
   CuAssertUIntEquals(tc, 0xffu, buf[19]);
   CuAssertUIntEquals(tc, 0xffu, buf[20]);
   CuAssertUIntEquals(tc, 0xffu, buf[21]);
   CuAssertUIntEquals(tc, 0xffu, buf[22]);
   CuAssertUIntEquals(tc, 0xffu, buf[23]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_bool_from_bool_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[BOOL_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_bool(false);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_bool(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x0u, buf[0]);
   dtl_sv_set_bool(sv, true);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_bool(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x1u, buf[0]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_bool_from_uint32_value(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[BOOL_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_u32(0u);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_bool(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x0u, buf[0]);
   dtl_sv_set_u32(sv, 1u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_bool(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x1u, buf[0]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_single_byte(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[BYTE_SIZE];
   uint8_t data[] = { 0xaau };
   uint32_t const array_length = (uint32_t)sizeof(data);
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_bytearray_raw(&data[0], (uint32_t)sizeof(data));
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_byte(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0xaau, buf[0]);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_byte_array(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[BYTE_SIZE * 2u];
   uint8_t data[] = { 0x12, 0x34 };
   uint32_t const array_length = (uint32_t)sizeof(data);
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_bytearray_raw(&data[0], (uint32_t)sizeof(data));
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_byte(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int) sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x12u, buf[0]);
   CuAssertUIntEquals(tc, 0x34u, buf[1]);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_zero_length_byte_array_is_treated_as_length_one(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[BYTE_SIZE];
   uint8_t data[] = { 0xaau };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_bytearray_raw(&data[0], (uint32_t)sizeof(data));
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_byte(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, data[0], buf[0]);
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_byte_array_with_inconsistent_length_returns_length_error(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[BYTE_SIZE * 2];
   uint8_t data[] = { 0x11u };
   uint32_t const array_length = 2u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_bytearray_raw(&data[0], (uint32_t)sizeof(data));
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_VALUE_LENGTH_ERROR, apx_vm_serializer_pack_byte(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 0, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_dynamic_uint8_array_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + UINT8_SIZE * 4];
   uint32_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(10), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT8_SIZE, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 1u, buf[0]);
   CuAssertUIntEquals(tc, 10u, buf[1]);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(11), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT8_SIZE * 2, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 2u, buf[0]);
   CuAssertUIntEquals(tc, 10u, buf[1]);
   CuAssertUIntEquals(tc, 11u, buf[2]);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(12), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT8_SIZE * 3, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 3u, buf[0]);
   CuAssertUIntEquals(tc, 10u, buf[1]);
   CuAssertUIntEquals(tc, 11u, buf[2]);
   CuAssertUIntEquals(tc, 12u, buf[3]);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(13), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT8_SIZE * 4, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 4u,  buf[0]);
   CuAssertUIntEquals(tc, 10u, buf[1]);
   CuAssertUIntEquals(tc, 11u, buf[2]);
   CuAssertUIntEquals(tc, 12u, buf[3]);
   CuAssertUIntEquals(tc, 13u, buf[4]);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(14), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_VALUE_LENGTH_ERROR, apx_vm_serializer_pack_uint8(&sr, max_array_length, dynamic_size_type));

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_dynamic_uint16_array_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + UINT16_SIZE * 4];
   uint32_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(200), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(400), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT16_SIZE * 2, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 2u, buf[0]);
   CuAssertUIntEquals(tc, 0xC8u, buf[1]);
   CuAssertUIntEquals(tc, 0x00u, buf[2]);
   CuAssertUIntEquals(tc, 0x90u, buf[3]);
   CuAssertUIntEquals(tc, 0x01u, buf[4]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);

}

static void test_pack_dynamic_uint32_array_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + UINT32_SIZE * 4];
   uint32_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(200000), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_u32(400000), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint32(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT32_SIZE * 2, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 2u,   buf[0]);
   CuAssertUIntEquals(tc, 0x40u, buf[1]);
   CuAssertUIntEquals(tc, 0x0Du, buf[2]);
   CuAssertUIntEquals(tc, 0x03u, buf[3]);
   CuAssertUIntEquals(tc, 0x00u, buf[4]);
   CuAssertUIntEquals(tc, 0x80u, buf[5]);
   CuAssertUIntEquals(tc, 0x1Au, buf[6]);
   CuAssertUIntEquals(tc, 0x06u, buf[7]);
   CuAssertUIntEquals(tc, 0x000u, buf[8]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);

}

static void test_pack_dynamic_int8_array_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + INT8_SIZE * 4];
   uint32_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-100), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(100), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int8(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + INT8_SIZE * 2, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 2u, buf[0]);
   CuAssertUIntEquals(tc, 0x9Cu, buf[1]);
   CuAssertUIntEquals(tc, 0x64u, buf[2]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);

}

static void test_pack_dynamic_int16_array_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + INT16_SIZE * 4];
   uint32_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-1000), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(1000), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int16(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + INT16_SIZE * 2, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 2u, buf[0]);
   CuAssertUIntEquals(tc, 0x18u, buf[1]);
   CuAssertUIntEquals(tc, 0xFCu, buf[2]);
   CuAssertUIntEquals(tc, 0xE8u, buf[3]);
   CuAssertUIntEquals(tc, 0x03u, buf[4]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);

}

static void test_pack_dynamic_int32_array_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + INT32_SIZE * 4];
   uint32_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(-200000), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_i32(200000), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_int32(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + INT32_SIZE * 2, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 2u, buf[0]);
   CuAssertUIntEquals(tc, 0xC0u, buf[1]);
   CuAssertUIntEquals(tc, 0xF2u, buf[2]);
   CuAssertUIntEquals(tc, 0xFCu, buf[3]);
   CuAssertUIntEquals(tc, 0xFFu, buf[4]);
   CuAssertUIntEquals(tc, 0x40u, buf[5]);
   CuAssertUIntEquals(tc, 0x0Du, buf[6]);
   CuAssertUIntEquals(tc, 0x03u, buf[7]);
   CuAssertUIntEquals(tc, 0x00u, buf[8]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_dynamic_bool_array_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + BOOL_SIZE * 4];
   uint32_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_av_t* av = dtl_av_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_bool(false), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_bool(true), false);
   dtl_av_push(av, (dtl_dv_t*)dtl_sv_make_bool(true), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_bool(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + BOOL_SIZE * 3, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 3u, buf[0]);
   CuAssertUIntEquals(tc, 0x0u, buf[1]);
   CuAssertUIntEquals(tc, 0x1u, buf[2]);
   CuAssertUIntEquals(tc, 0x1u, buf[3]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}

static void test_pack_dynamic_byte_array_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + BYTE_SIZE * 4];
   uint8_t const data[] = { 1u, 2u, 3u, 4, };
   uint32_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_sv_t* sv = dtl_sv_make_bytearray_raw(&data[0], (uint32_t)sizeof(data));
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_byte(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + BYTE_SIZE * 4, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 4u, buf[0]);
   CuAssertUIntEquals(tc, 0x1u, buf[1]);
   CuAssertUIntEquals(tc, 0x2u, buf[2]);
   CuAssertUIntEquals(tc, 0x3u, buf[3]);
   CuAssertUIntEquals(tc, 0x4u, buf[4]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);

}

static void test_pack_dynamic_byte_array_with_uint16_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT16_SIZE + BYTE_SIZE * 1023];
   uint8_t data[1023];
   uint32_t const max_array_length = 1023u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT16;
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));
   uint32_t i;
   uint8_t value = 0;
   for (i = 0; i < max_array_length; i++)
   {
      data[i] = value++;
   }
   dtl_sv_t* sv = dtl_sv_make_bytearray_raw(&data[0], (uint32_t)sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_byte(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT16_SIZE + BYTE_SIZE * 1023, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0xffu, buf[0]);
   CuAssertUIntEquals(tc, 0x03u, buf[1]);
   value = 0;
   for (i = 0; i < max_array_length; i++)
   {
      CuAssertUIntEquals(tc, value, buf[i+2]);
      value++; //This will wraparound after 255.
   }

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_dynamic_string_with_uint8_length(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + CHAR_SIZE * 32];
   char const *data = "Name";
   uint32_t const max_array_length = 32u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_sv_t* sv = dtl_sv_make_cstr(data);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_char(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + CHAR_SIZE * 4, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 4u, buf[0]);
   CuAssertUIntEquals(tc, 'N', buf[1]);
   CuAssertUIntEquals(tc, 'a', buf[2]);
   CuAssertUIntEquals(tc, 'm', buf[3]);
   CuAssertUIntEquals(tc, 'e', buf[4]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_string_in_record(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[CHAR_SIZE * 20];
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_hv_t* hv = dtl_hv_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));
   dtl_hv_set_cstr(hv, "First", (dtl_dv_t*) dtl_sv_make_cstr("Hello"), false);
   dtl_hv_set_cstr(hv, "Second", (dtl_dv_t*) dtl_sv_make_cstr("APX"), false);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_hv(&sr, hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_char(&sr, 10u, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Second", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_char(&sr, 10u, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int) sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 'H',  buf[0]);
   CuAssertUIntEquals(tc, 'e',  buf[1]);
   CuAssertUIntEquals(tc, 'l',  buf[2]);
   CuAssertUIntEquals(tc, 'l',  buf[3]);
   CuAssertUIntEquals(tc, 'o',  buf[4]);
   CuAssertUIntEquals(tc, '\0', buf[5]);
   CuAssertUIntEquals(tc, '\0', buf[6]);
   CuAssertUIntEquals(tc, '\0', buf[7]);
   CuAssertUIntEquals(tc, '\0', buf[8]);
   CuAssertUIntEquals(tc, '\0', buf[9]);
   CuAssertUIntEquals(tc, 'A', buf[10]);
   CuAssertUIntEquals(tc, 'P', buf[11]);
   CuAssertUIntEquals(tc, 'X', buf[12]);
   CuAssertUIntEquals(tc, '\0', buf[13]);
   CuAssertUIntEquals(tc, '\0', buf[14]);
   CuAssertUIntEquals(tc, '\0', buf[15]);
   CuAssertUIntEquals(tc, '\0', buf[16]);
   CuAssertUIntEquals(tc, '\0', buf[17]);
   CuAssertUIntEquals(tc, '\0', buf[18]);
   CuAssertUIntEquals(tc, '\0', buf[19]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(hv);
}

static void test_pack_dynamic_string_in_record(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + CHAR_SIZE * 10 + UINT8_SIZE + CHAR_SIZE * 10];
   uint32_t const max_array_length = 10u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_hv_t* hv = dtl_hv_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));
   dtl_hv_set_cstr(hv, "First", (dtl_dv_t*)dtl_sv_make_cstr("Hello"), false);
   dtl_hv_set_cstr(hv, "Second", (dtl_dv_t*)dtl_sv_make_cstr("APX"), false);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_hv(&sr, hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_char(&sr, max_array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Second", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_char(&sr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, 1+10+1+3, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 5u,  buf[0]);
   CuAssertUIntEquals(tc, 'H', buf[1]);
   CuAssertUIntEquals(tc, 'e', buf[2]);
   CuAssertUIntEquals(tc, 'l', buf[3]);
   CuAssertUIntEquals(tc, 'l', buf[4]);
   CuAssertUIntEquals(tc, 'o', buf[5]);
   CuAssertUIntEquals(tc, '\0', buf[6]);
   CuAssertUIntEquals(tc, '\0', buf[7]);
   CuAssertUIntEquals(tc, '\0', buf[8]);
   CuAssertUIntEquals(tc, '\0', buf[9]);
   CuAssertUIntEquals(tc, '\0', buf[10]);
   CuAssertUIntEquals(tc, 3u,  buf[11]);
   CuAssertUIntEquals(tc, 'A', buf[12]);
   CuAssertUIntEquals(tc, 'P', buf[13]);
   CuAssertUIntEquals(tc, 'X', buf[14]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(hv);
}

static void test_pack_dynamic_uint16_array_in_record(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + UINT16_SIZE * 5 + UINT8_SIZE];
   uint32_t const max_array_length = 5u;
   apx_sizeType_t const dynamic_size_type1 = APX_SIZE_TYPE_UINT8;
   apx_sizeType_t const dynamic_size_type2 = APX_SIZE_TYPE_NONE;
   dtl_hv_t* hv = dtl_hv_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));
   dtl_av_t* av = dtl_av_new();
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(1000), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(2000), false);
   dtl_hv_set_cstr(hv, "First", (dtl_dv_t*)av, false);
   dtl_hv_set_cstr(hv, "Second", (dtl_dv_t*) dtl_sv_make_u32(255) , false);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_hv(&sr, hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, max_array_length, dynamic_size_type1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Second", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, 0u, dynamic_size_type2));
   CuAssertUIntEquals(tc, (unsigned int) sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 2u, buf[0]);
   CuAssertUIntEquals(tc, 0xE8u, buf[1]);
   CuAssertUIntEquals(tc, 0x03u, buf[2]);
   CuAssertUIntEquals(tc, 0xD0u, buf[3]);
   CuAssertUIntEquals(tc, 0x07u, buf[4]);
   CuAssertUIntEquals(tc, 0x00u, buf[5]);
   CuAssertUIntEquals(tc, 0x00u, buf[6]);
   CuAssertUIntEquals(tc, 0x00u, buf[7]);
   CuAssertUIntEquals(tc, 0x00u, buf[8]);
   CuAssertUIntEquals(tc, 0x00u, buf[9]);
   CuAssertUIntEquals(tc, 0x00u, buf[10]);
   CuAssertUIntEquals(tc, 255u, buf[11]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(hv);

}

static void test_pack_uint8_queued_element(CuTest* tc)
{
   //DATA SIGNATURE: C:Q[5]
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + UINT8_SIZE * 5];
   uint32_t const queue_length = 5u;
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(7);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_queued_write_begin(&sr, UINT8_SIZE, queue_length, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_queued_write_end(&sr));
   CuAssertUIntEquals(tc, UINT8_SIZE * 2, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 1u, buf[0]);
   CuAssertUIntEquals(tc, 7u, buf[1]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_uint8_multiple_queued_elements(CuTest* tc)
{
   //DATA SIGNATURE: C:Q[5]
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + UINT8_SIZE * 5];
   uint32_t const queue_length = 5u;
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = dtl_sv_make_i32(7);
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_queued_write_begin(&sr, UINT8_SIZE, queue_length, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   dtl_sv_set_i32(sv, 8);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   dtl_sv_set_i32(sv, 9);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_sv(&sr, sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_queued_write_end(&sr));
   CuAssertUIntEquals(tc, UINT8_SIZE * 4, (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 3u, buf[0]);
   CuAssertUIntEquals(tc, 7u, buf[1]);
   CuAssertUIntEquals(tc, 8u, buf[2]);
   CuAssertUIntEquals(tc, 9u, buf[3]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(sv);
}

static void test_pack_record_inside_record__uint8_uint16__uint16_uint32(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[UINT8_SIZE + UINT16_SIZE * 2 + UINT32_SIZE];
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_hv_t* hv = dtl_hv_new();
   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));
   dtl_hv_t* inner_hv1 = dtl_hv_new();
   dtl_hv_set_cstr(inner_hv1, "Inner1", (dtl_dv_t*)dtl_sv_make_u32(0x12u), false);
   dtl_hv_set_cstr(inner_hv1, "Inner2", (dtl_dv_t*)dtl_sv_make_u32(0x1234u), false);
   dtl_hv_t* inner_hv2 = dtl_hv_new();
   dtl_hv_set_cstr(inner_hv2, "Inner3", (dtl_dv_t*)dtl_sv_make_u32(0x1234u), false);
   dtl_hv_set_cstr(inner_hv2, "Inner4", (dtl_dv_t*)dtl_sv_make_u32(0x12345678u), false);

   dtl_hv_set_cstr(hv, "First", (dtl_dv_t*)inner_hv1, false);
   dtl_hv_set_cstr(hv, "Second", (dtl_dv_t*)inner_hv2, false);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_hv(&sr, hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_record(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_record(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Inner1", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Inner2", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Second", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_record(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Inner3", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Inner4", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint32(&sr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0x12u, buf[0]);
   CuAssertUIntEquals(tc, 0x34u, buf[1]);
   CuAssertUIntEquals(tc, 0x12u, buf[2]);
   CuAssertUIntEquals(tc, 0x34u, buf[3]);
   CuAssertUIntEquals(tc, 0x12u, buf[4]);
   CuAssertUIntEquals(tc, 0x78u, buf[5]);
   CuAssertUIntEquals(tc, 0x56u, buf[6]);
   CuAssertUIntEquals(tc, 0x34u, buf[7]);
   CuAssertUIntEquals(tc, 0x12u, buf[8]);

   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(hv);
}

static void test_pack_array_of_record_uint16_uint8(CuTest* tc)
{
   apx_vm_serializer_t sr;
   uint8_t buf[(UINT16_SIZE + UINT8_SIZE) * 3];
   uint32_t const array_length = 3u;
   uint32_t const child_array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   bool is_last = false;

   apx_vm_serializer_create(&sr);
   memset(buf, 0, sizeof(buf));
   dtl_hv_t* child_hv1 = dtl_hv_new();
   dtl_hv_set_cstr(child_hv1, "Id", (dtl_dv_t*)dtl_sv_make_u32(1000u), false);
   dtl_hv_set_cstr(child_hv1, "Value", (dtl_dv_t*)dtl_sv_make_u32(1u), false);
   dtl_hv_t* child_hv2 = dtl_hv_new();
   dtl_hv_set_cstr(child_hv2, "Id", (dtl_dv_t*)dtl_sv_make_u32(2000u), false);
   dtl_hv_set_cstr(child_hv2, "Value", (dtl_dv_t*)dtl_sv_make_u32(0u), false);
   dtl_hv_t* child_hv3 = dtl_hv_new();
   dtl_hv_set_cstr(child_hv3, "Id", (dtl_dv_t*)dtl_sv_make_u32(4000u), false);
   dtl_hv_set_cstr(child_hv3, "Value", (dtl_dv_t*)dtl_sv_make_u32(1u), false);
   dtl_av_t* av = dtl_av_new();

   dtl_av_push(av, (dtl_dv_t*)child_hv1, false);
   dtl_av_push(av, (dtl_dv_t*)child_hv2, false);
   dtl_av_push(av, (dtl_dv_t*)child_hv3, false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_write_buffer(&sr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_set_value_av(&sr, av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_record(&sr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Id", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, child_array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Value", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, child_array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_array_next(&sr, &is_last));
   CuAssertFalse(tc, is_last);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Id", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, child_array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Value", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, child_array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_array_next(&sr, &is_last));
   CuAssertFalse(tc, is_last);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Id", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint16(&sr, child_array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_record_select(&sr, "Value", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_pack_uint8(&sr, child_array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_serializer_array_next(&sr, &is_last));
   CuAssertTrue(tc, is_last);
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_serializer_bytes_written(&sr));
   CuAssertUIntEquals(tc, 0xE8u, buf[0]);
   CuAssertUIntEquals(tc, 0x03u, buf[1]);
   CuAssertUIntEquals(tc, 0x01u, buf[2]);
   CuAssertUIntEquals(tc, 0xD0u, buf[3]);
   CuAssertUIntEquals(tc, 0x07u, buf[4]);
   CuAssertUIntEquals(tc, 0x00u, buf[5]);
   CuAssertUIntEquals(tc, 0xA0u, buf[6]);
   CuAssertUIntEquals(tc, 0x0Fu, buf[7]);
   CuAssertUIntEquals(tc, 0x01u, buf[8]);


   apx_vm_serializer_destroy(&sr);
   dtl_dec_ref(av);
}
