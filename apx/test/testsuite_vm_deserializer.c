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
#include "apx/deserializer.h"
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
static void test_set_read_buffer(CuTest* tc);
static void test_unpack_uint8(CuTest* tc);
static void test_unpack_uint8_array(CuTest* tc);
static void test_unpack_uint8_array_with_too_small_buffer(CuTest* tc);
static void test_unpack_uint16(CuTest* tc);
static void test_unpack_uint16_array(CuTest* tc);
static void test_unpack_uint32(CuTest* tc);
static void test_unpack_uint32_array(CuTest* tc);
static void test_unpack_uint64(CuTest* tc);
static void test_unpack_uint64_array(CuTest* tc);
static void test_unpack_int8(CuTest* tc);
static void test_unpack_int8_array(CuTest* tc);
static void test_unpack_int16(CuTest* tc);
static void test_unpack_int16_array(CuTest* tc);
static void test_unpack_int32(CuTest* tc);
static void test_unpack_int32_array(CuTest* tc);
static void test_unpack_int64(CuTest* tc);
static void test_unpack_int64_array(CuTest* tc);
static void test_unpack_char(CuTest* tc);
static void test_unpack_char_array_with_trailing_null(CuTest* tc);
static void test_unpack_filled_char_array(CuTest* tc);
static void test_unpack_dynamic_uint8_array_with_uint8_size(CuTest* tc);
static void test_unpack_dynamic_char_array_with_uint8_size(CuTest* tc);
static void test_unpack_dynamic_char_array_with_embedded_null(CuTest* tc);
static void test_unpack_boolean(CuTest* tc);
static void test_unpack_boolean_array(CuTest* tc);
static void test_unpack_byte(CuTest* tc);
static void test_unpack_byte_array(CuTest* tc);
static void test_unpack_dynamic_byte_array_with_uint8_size(CuTest* tc);
static void test_unpack_record_u8_u8(CuTest* tc);
static void test_unpack_record_u8array_u16(CuTest* tc);
static void test_unpack_record_string_string_bool(CuTest* tc);
static void test_unpack_record_dynstring_u32array(CuTest* tc);
static void test_unpack_record_bool_dynstring(CuTest* tc);
static void test_unpack_record_inside_record_u8_u16__u16_u32(CuTest* tc);
static void test_unpack_array_of_record_u16_u8(CuTest* tc);
static void test_unpack_uint8_queued_element(CuTest* tc);
static void test_unpack_uint8_multiple_queued_elements(CuTest* tc);
static void test_unpack_uint8_with_range_check(CuTest* tc);
static void test_unpack_uint8_array_with_range_check(CuTest* tc);
static void test_range_check_uint8_scalar(CuTest* tc);
static void test_range_check_uint8_with_out_of_range_value(CuTest* tc);
static void test_range_check_uint8_array(CuTest* tc);
static void test_range_check_uint8_array_with_out_of_range_value(CuTest* tc);
static void test_range_check_int8_scalar(CuTest* tc);
static void test_range_check_int8_with_out_of_range_value(CuTest* tc);
static void test_range_check_int8_array(CuTest* tc);
static void test_range_check_int8_array_with_out_of_range_value(CuTest* tc);
static void test_range_check_uint64_scalar(CuTest* tc);
static void test_range_check_int64_scalar(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_vm_deserializer(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_set_read_buffer);
   SUITE_ADD_TEST(suite, test_unpack_uint8);
   SUITE_ADD_TEST(suite, test_unpack_uint8_array);
   SUITE_ADD_TEST(suite, test_unpack_uint8_array_with_too_small_buffer);
   SUITE_ADD_TEST(suite, test_unpack_uint16);
   SUITE_ADD_TEST(suite, test_unpack_uint16_array);
   SUITE_ADD_TEST(suite, test_unpack_uint32);
   SUITE_ADD_TEST(suite, test_unpack_uint32_array);
   SUITE_ADD_TEST(suite, test_unpack_uint64);
   SUITE_ADD_TEST(suite, test_unpack_uint64_array);
   SUITE_ADD_TEST(suite, test_unpack_int8);
   SUITE_ADD_TEST(suite, test_unpack_int8_array);
   SUITE_ADD_TEST(suite, test_unpack_int16);
   SUITE_ADD_TEST(suite, test_unpack_int16_array);
   SUITE_ADD_TEST(suite, test_unpack_int32);
   SUITE_ADD_TEST(suite, test_unpack_int32_array);
   SUITE_ADD_TEST(suite, test_unpack_int64);
   SUITE_ADD_TEST(suite, test_unpack_int64_array);
   SUITE_ADD_TEST(suite, test_unpack_char);
   SUITE_ADD_TEST(suite, test_unpack_dynamic_uint8_array_with_uint8_size);
   SUITE_ADD_TEST(suite, test_unpack_dynamic_char_array_with_uint8_size);
   SUITE_ADD_TEST(suite, test_unpack_dynamic_char_array_with_embedded_null);
   SUITE_ADD_TEST(suite, test_unpack_boolean);
   SUITE_ADD_TEST(suite, test_unpack_boolean_array);
   SUITE_ADD_TEST(suite, test_unpack_byte);
   SUITE_ADD_TEST(suite, test_unpack_byte_array);
   SUITE_ADD_TEST(suite, test_unpack_dynamic_byte_array_with_uint8_size);
   SUITE_ADD_TEST(suite, test_unpack_record_u8_u8);
   SUITE_ADD_TEST(suite, test_unpack_record_u8array_u16);
   SUITE_ADD_TEST(suite, test_unpack_record_string_string_bool);
   SUITE_ADD_TEST(suite, test_unpack_record_dynstring_u32array);
   SUITE_ADD_TEST(suite, test_unpack_record_bool_dynstring);
   SUITE_ADD_TEST(suite, test_unpack_record_inside_record_u8_u16__u16_u32);
   SUITE_ADD_TEST(suite, test_unpack_array_of_record_u16_u8);
   SUITE_ADD_TEST(suite, test_unpack_uint8_queued_element);
   SUITE_ADD_TEST(suite, test_unpack_uint8_multiple_queued_elements);   
   SUITE_ADD_TEST(suite, test_range_check_uint8_scalar);
   SUITE_ADD_TEST(suite, test_range_check_uint8_with_out_of_range_value);
   SUITE_ADD_TEST(suite, test_range_check_uint8_array);
   SUITE_ADD_TEST(suite, test_range_check_uint8_array_with_out_of_range_value);
   SUITE_ADD_TEST(suite, test_range_check_int8_scalar);
   SUITE_ADD_TEST(suite, test_range_check_int8_with_out_of_range_value);
   SUITE_ADD_TEST(suite, test_range_check_int8_array);
   SUITE_ADD_TEST(suite, test_range_check_int8_array_with_out_of_range_value);
   SUITE_ADD_TEST(suite, test_range_check_uint64_scalar);
   SUITE_ADD_TEST(suite, test_range_check_int64_scalar);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_set_read_buffer(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[10];
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, NULL, sizeof(buf)));
   CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], 0u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint8(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE] = { 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[0] = 255u;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 255u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint8_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE*2] = { 0x03u, 0x07u };
   uint32_t const array_length = 2u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int) sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, 2u, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 3u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 7u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint8_array_with_too_small_buffer(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE * 2] = { 0x03u, 0x07u };
   uint32_t const array_length = 3;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));

   apx_vm_deserializer_destroy(&dsr);
}


static void test_unpack_uint16(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT16_SIZE] = { 0u, 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   memset(buf, 255, sizeof(buf));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 65535, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint16_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT16_SIZE * 3] = { 0x00u, 0x00u, 0x34, 0x12, 0xff, 0xff };
   uint32_t const array_length = 3u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, (int) array_length, dtl_av_length(av));
   sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x0000u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xffff, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint32(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT32_SIZE] = { 0u, 0u, 0u, 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint32(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   memset(buf, 255, sizeof(buf));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint32(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint32_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT32_SIZE * 3] = { 0x00u, 0x00u, 0x00u, 0x00u, 0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff };
   uint32_t const array_length = 3u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint32(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, (int)array_length, dtl_av_length(av));
   sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x00000000u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x12345678, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint64(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT64_SIZE] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertULIntEquals(tc, 0u, dtl_sv_to_u64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   memset(buf, 255, sizeof(buf));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertULIntEquals(tc, 0xffffffffffffffffull, dtl_sv_to_u64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint64_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT64_SIZE * 3] = { 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0x01,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
   uint32_t const array_length = 3u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, (int)array_length, dtl_av_length(av));
   sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertULIntEquals(tc, 0x0000000000000000ull, dtl_sv_to_u64(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertULIntEquals(tc, 0x0123456789abcdefull, dtl_sv_to_u64(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertULIntEquals(tc, 0xffffffffffffffffull, dtl_sv_to_u64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_int8(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[INT8_SIZE] = { 0u, };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0u, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[0] = 0xff;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[0] = 0x7f;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 127, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_int8_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE * 4] = { 0x80u, 0xff, 0x00, 0x7f };
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, (int)array_length, dtl_av_length(av));
   sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -128, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 127, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_int16(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[INT16_SIZE] = { 0u, 0u};
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int16(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0u, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[0] = buf[1] = 0xff;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int16(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[1] = 0x7f;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int16(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 32767, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_int16_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT16_SIZE * 4] = { 0x00u, 0x80u, 0xff, 0xff, 0x00, 0x00, 0xff, 0x7f };
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int16(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, (int)array_length, dtl_av_length(av));
   sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -32768, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 32767, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_int32(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[INT32_SIZE] = { 0u, 0u, 0u, 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int32(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0u, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   memset(buf, 0xff, sizeof(buf));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int32(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[3] = 0x7f;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int32(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, INT32_MAX, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_int32_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT32_SIZE * 4] = { 0x00u, 0x00u, 0x00u, 0x80u, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x7f };
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int32(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, (int)array_length, dtl_av_length(av));
   sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, INT32_MIN, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, INT32_MAX, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_int64(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[INT64_SIZE] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertLIntEquals(tc, 0u, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   memset(buf, 0xff, sizeof(buf));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertLIntEquals(tc, -1, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[7] = 0x7f;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertLIntEquals(tc, INT64_MAX, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_int64_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT64_SIZE * 4] = { 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f };
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, (int)array_length, dtl_av_length(av));
   sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertLIntEquals(tc, INT64_MIN, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertLIntEquals(tc, -1, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertLIntEquals(tc, 0, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertLIntEquals(tc, INT64_MAX, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_char(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[CHAR_SIZE] = { 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0u, dtl_sv_to_char(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   memset(buf, 127, sizeof(buf));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 127, dtl_sv_to_char(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[0] = 0x80;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -128, dtl_sv_to_char(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_char_array_with_trailing_null(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[CHAR_SIZE * 8] = { 'a', 'b', 'c', 'd', '\0', '\0', '\0', '\0' };
   uint32_t const array_length = 8u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertStrEquals(tc, "abcd", dtl_sv_to_cstr(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_filled_char_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[CHAR_SIZE * 8] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };
   uint32_t const array_length = 8u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertStrEquals(tc, "abcdefgh", dtl_sv_to_cstr(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_dynamic_uint8_array_with_uint8_size(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t const current_array_length = 2u;
   uint8_t buf[UINT8_SIZE + UINT8_SIZE * 4] = { current_array_length, 0x03, 0x7, 0, 0 };
   uint8_t const max_array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE + UINT8_SIZE*current_array_length, (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, current_array_length, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x03u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x07u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_dynamic_char_array_with_uint8_size(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t const current_array_length = 3u;
   uint8_t buf[UINT8_SIZE + CHAR_SIZE * 3] = { current_array_length, 'a', 'b', 'c' };
   uint8_t const max_array_length = 10u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertStrEquals(tc, "abc", dtl_sv_to_cstr(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_dynamic_char_array_with_embedded_null(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t const current_array_length = 5u;
   uint8_t buf[UINT8_SIZE + CHAR_SIZE * 5] = { current_array_length, 'a', 'b', '\0', 'c', 'd' };
   uint8_t const max_array_length = 10u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_sv_t* sv = NULL;
   adt_str_t* str = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   str = dtl_sv_to_str(sv, &ok);
   CuAssertTrue(tc, ok);
   CuAssertIntEquals(tc, 5, adt_str_size(str));
   adt_str_delete(str);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_boolean(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[BOOL_SIZE] = { 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_bool(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertFalse(tc, dtl_sv_to_bool(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   buf[0] = 1u;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_bool(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertTrue(tc, dtl_sv_to_bool(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_boolean_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[BOOL_SIZE * 3] = { 0, 1, 0 };
   uint32_t const array_length = 3u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_av_t* av = NULL;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_bool(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, (int)array_length, dtl_av_length(av));
   sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertFalse(tc, dtl_sv_to_bool(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertTrue(tc, dtl_sv_to_bool(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertFalse(tc, dtl_sv_to_bool(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_byte(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[BYTE_SIZE] = { 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   adt_bytearray_t const* array = NULL;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_byte(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_BYTEARRAY, dtl_sv_type(sv));
   array = dtl_sv_get_bytearray(sv);
   CuAssertPtrNotNull(tc, array);
   CuAssertTrue(tc, adt_bytearray_data_equals(array, &buf[0], sizeof(buf)));
   dtl_dec_ref(sv);
   buf[0] = 255u;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_byte(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_BYTEARRAY, dtl_sv_type(sv));
   array = dtl_sv_get_bytearray(sv);
   CuAssertPtrNotNull(tc, array);
   CuAssertTrue(tc, adt_bytearray_data_equals(array, &buf[0], sizeof(buf)));
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_byte_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[BYTE_SIZE * 4] = { 0u, 1u, 2u, 3u };
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   adt_bytearray_t const* array = NULL;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_byte(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_BYTEARRAY, dtl_sv_type(sv));
   array = dtl_sv_get_bytearray(sv);
   CuAssertPtrNotNull(tc, array);
   CuAssertTrue(tc, adt_bytearray_data_equals(array, &buf[0], sizeof(buf)));
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_dynamic_byte_array_with_uint8_size(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t expected[3] = { 0xb, 0xc, 0xd };
   uint8_t buf[BYTE_SIZE * 10] = { 3u, 0xb, 0xc, 0xd, 0, 0, 0, 0, 0, 0 };
   uint32_t const max_array_length = 10u;
   uint32_t const current_array_length = 3u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_UINT8;
   dtl_sv_t* sv = NULL;
   adt_bytearray_t const* array = NULL;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_byte(&dsr, max_array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, UINT8_SIZE * (1 + current_array_length), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_BYTEARRAY, dtl_sv_type(sv));
   array = dtl_sv_get_bytearray(sv);
   CuAssertPtrNotNull(tc, array);
   CuAssertTrue(tc, adt_bytearray_data_equals(array, &expected[0], sizeof(expected)));
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_record_u8_u8(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE + UINT8_SIZE] = { 3u, 7u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_hv_t* hv = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Second", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_HASH, apx_vm_deserializer_value_type(&dsr));
   hv = apx_vm_deserializer_take_hv(&dsr);
   CuAssertPtrNotNull(tc, hv);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x03u, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x07u, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(hv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_record_u8array_u16(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE*3 + UINT16_SIZE] = { 0x01u, 0x02u, 0x03u,  0x34u, 0x12u };
   uint32_t const outer_array_length = 0u;
   uint32_t const inner_array_length1 = 3u;
   uint32_t const inner_array_length2 = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_hv_t* hv = NULL;
   dtl_av_t* child_av = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, outer_array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, inner_array_length1, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Second", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, inner_array_length2, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_HASH, apx_vm_deserializer_value_type(&dsr));
   hv = apx_vm_deserializer_take_hv(&dsr);
   CuAssertPtrNotNull(tc, hv);
   child_av = (dtl_av_t*)dtl_hv_get_cstr(hv, "First");
   CuAssertPtrNotNull(tc, child_av);
   CuAssertIntEquals(tc, 3, dtl_av_length(child_av));
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 0);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x01, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 1);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x02, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 2);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x03, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x1234u, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(hv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_record_string_string_bool(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[5 + 5 + BOOL_SIZE] = { 'H', 'e', 'l', 'l', 'o',
      'W', 'o', 'r', 'l', 'd',
      0x01 };
   uint32_t const array_length = 0u;
   uint32_t const str_max_length1 = 5u;
   uint32_t const str_max_length2 = 5u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_hv_t* hv = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, str_max_length1, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Second", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, str_max_length2, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Third", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_bool(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_HASH, apx_vm_deserializer_value_type(&dsr));
   hv = apx_vm_deserializer_take_hv(&dsr);
   CuAssertPtrNotNull(tc, hv);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertStrEquals(tc, "Hello", dtl_sv_to_cstr(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertStrEquals(tc, "World", dtl_sv_to_cstr(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "Third");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_SV_BOOL, dtl_sv_type(child_sv));
   CuAssertTrue(tc, dtl_sv_to_bool(child_sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(hv);
   apx_vm_deserializer_destroy(&dsr);

}

static void test_unpack_record_dynstring_u32array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint32_t const current_string_length = 5u;
   uint32_t const max_string_length = 10u;
   uint32_t const u32_array_size = 2u;
   uint8_t buf[UINT8_SIZE + CHAR_SIZE * 10 + UINT32_SIZE * 2] = { (uint8_t)current_string_length,
      'D', 'a', 't', 'a', '1', 0xee, 0xee, 0xee, 0xee, 0xee,
      0x0, 0x0, 0x0, 0x0, 0x78, 0x56, 0x34, 0x12 };
   apx_sizeType_t const record_dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_sizeType_t const str_dynamic_size_type = APX_SIZE_TYPE_UINT8;
   apx_sizeType_t const u32_array_size_type = APX_SIZE_TYPE_NONE;
   dtl_hv_t* hv = NULL;
   dtl_av_t* child_av = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, 0u, record_dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, max_string_length, str_dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Second", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint32(&dsr, u32_array_size, u32_array_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_HASH, apx_vm_deserializer_value_type(&dsr));
   hv = apx_vm_deserializer_take_hv(&dsr);
   CuAssertPtrNotNull(tc, hv);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertStrEquals(tc, "Data1", dtl_sv_to_cstr(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_av = (dtl_av_t*)dtl_hv_get_cstr(hv, "Second");
   CuAssertPtrNotNull(tc, child_av);
   CuAssertIntEquals(tc, 2, dtl_av_length(child_av));
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 0);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x0, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 1);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x12345678, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(hv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_record_bool_dynstring(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint32_t const current_string_length = 4u;
   uint32_t const max_string_length = 10u;
   uint8_t buf[BOOL_SIZE + UINT8_SIZE + CHAR_SIZE * 4] = { (uint8_t)current_string_length,
      (uint8_t)current_string_length, 'D', 'a', 't', 'a'};
   apx_sizeType_t const record_size_type = APX_SIZE_TYPE_NONE;
   apx_sizeType_t const first_field_size_type = APX_SIZE_TYPE_NONE;
   apx_sizeType_t const second_field_size_type = APX_SIZE_TYPE_UINT8;
   dtl_hv_t* hv = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, 0u, record_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_bool(&dsr, 0u, first_field_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Second", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_char(&dsr, max_string_length, second_field_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_HASH, apx_vm_deserializer_value_type(&dsr));
   hv = apx_vm_deserializer_take_hv(&dsr);
   CuAssertPtrNotNull(tc, hv);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_SV_BOOL, dtl_sv_type(child_sv));
   CuAssertTrue(tc, dtl_sv_to_bool(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertStrEquals(tc, "Data", dtl_sv_to_cstr(child_sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(hv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_record_inside_record_u8_u16__u16_u32(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE + UINT16_SIZE + UINT16_SIZE + UINT32_SIZE] =
   { 0x12, 0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12 };
   dtl_hv_t* hv = NULL;
   dtl_hv_t* child_hv = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "First", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Inner1", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Inner2", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Second", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Inner3", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Inner4", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint32(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_HASH, apx_vm_deserializer_value_type(&dsr));
   hv = apx_vm_deserializer_take_hv(&dsr);
   CuAssertPtrNotNull(tc, hv);
   child_hv = (dtl_hv_t*)dtl_hv_get_cstr(hv, "First");
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, 2u, dtl_hv_length(child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Inner1");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Inner2");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_hv = (dtl_hv_t*)dtl_hv_get_cstr(hv, "Second");
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, 2u, dtl_hv_length(child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Inner3");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Inner4");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x12345678, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dec_ref(hv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_array_of_record_u16_u8(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint32_t array_length = 3u;
   uint8_t buf[(UINT16_SIZE + UINT8_SIZE) * 3] = {
      0xE8, 0x03, 0x01,
      0xd0, 0x07, 0x00,
      0xA0, 0x0F, 0x01,
   };
   dtl_av_t* av = NULL;
   dtl_hv_t* child_hv = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   bool is_last = false;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_record(&dsr, array_length, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Id", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Value", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_array_next(&dsr, &is_last));
   CuAssertFalse(tc, is_last);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Id", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Value", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_array_next(&dsr, &is_last));
   CuAssertFalse(tc, is_last);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Id", false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint16(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_record_select(&dsr, "Value", true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, 0u, APX_SIZE_TYPE_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_array_next(&dsr, &is_last));
   CuAssertTrue(tc, is_last);
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertUIntEquals(tc, DTL_DV_ARRAY, apx_vm_deserializer_value_type(&dsr));
   av = apx_vm_deserializer_take_av(&dsr);
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, 3u, dtl_av_length(av));
   child_hv = (dtl_hv_t*)dtl_av_value(av, 0u);
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, 2u, dtl_hv_length(child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Id");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x3e8, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Value");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x1, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_hv = (dtl_hv_t*)dtl_av_value(av, 1u);
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, 2u, dtl_hv_length(child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Id");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x7d0, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Value");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x0, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_hv = (dtl_hv_t*)dtl_av_value(av, 2u);
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, 2u, dtl_hv_length(child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Id");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0xfa0, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Value");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x1, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint8_queued_element(CuTest* tc)
{
   //DATA SIGNATURE: C:Q[5]
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE + UINT8_SIZE * 5] = { 1, 0x12, 0xee, 0xee, 0xee, 0xee };
   uint32_t const queue_length = 5u;
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   bool is_last_item = false;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_queued_read_begin(&dsr, UINT8_SIZE, queue_length));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_queued_read_next(&dsr, &is_last_item));
   dtl_dec_ref(sv);
   CuAssertTrue(tc, is_last_item);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_unpack_uint8_multiple_queued_elements(CuTest* tc)
{
   //DATA SIGNATURE: C:Q[5]
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE + UINT8_SIZE * 5] = { 3, 7u, 8u, 9u, 0xee, 0xee };
   uint32_t const queue_length = 5u;
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   dtl_sv_t* sv = NULL;
   bool ok = false;
   apx_vm_deserializer_create(&dsr);
   bool is_last_item = false;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_queued_read_begin(&dsr, UINT8_SIZE, queue_length));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 7u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_queued_read_next(&dsr, &is_last_item));
   CuAssertFalse(tc, is_last_item);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   dtl_dec_ref(sv);
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 8u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_queued_read_next(&dsr, &is_last_item));
   CuAssertFalse(tc, is_last_item);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, DTL_DV_SCALAR, apx_vm_deserializer_value_type(&dsr));
   dtl_dec_ref(sv);
   sv = apx_vm_deserializer_take_sv(&dsr);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 9u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_queued_read_next(&dsr, &is_last_item));
   CuAssertTrue(tc, is_last_item);
   dtl_dec_ref(sv);
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_uint8_scalar(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE] = { 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_check_value_range_uint32(&dsr, 0u, 7u));
   buf[0] = 7;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_check_value_range_uint32(&dsr, 0u, 7u));
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_uint8_with_out_of_range_value(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE] = { 8u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_deserializer_check_value_range_uint32(&dsr, 0u, 7u));
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_uint8_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE * 2] = { 0x03u, 0x07u };
   uint32_t const array_length = 2u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_check_value_range_uint32(&dsr, 0u, 7u));
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_uint8_array_with_out_of_range_value(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE * 2] = { 0x03u, 0x08u };
   uint32_t const array_length = 2u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_deserializer_check_value_range_uint32(&dsr, 0u, 7u));
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_int8_scalar(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT8_SIZE] = { (uint8_t)-10 };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_check_value_range_int32(&dsr, -10, 10));
   buf[0] = 10;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_check_value_range_int32(&dsr, -10, 10));
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_int8_with_out_of_range_value(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[INT8_SIZE] = { (uint8_t)-11 };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_deserializer_check_value_range_int32(&dsr, -10, 10));
   buf[0] = 11;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_deserializer_check_value_range_int32(&dsr, -10, 10));
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_int8_array(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[INT8_SIZE * 4] = { (uint8_t)-10, (uint8_t)-1, 1, 10 };
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));   
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_check_value_range_int32(&dsr, -10, 10));   
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_int8_array_with_out_of_range_value(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[INT8_SIZE * 4] = { (uint8_t)-10, (uint8_t)-1, 1, 15 };
   uint32_t const array_length = 4u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int8(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_deserializer_check_value_range_int32(&dsr, -10, 10));
   apx_vm_deserializer_destroy(&dsr);

}

static void test_range_check_uint64_scalar(CuTest* tc)
{
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT64_SIZE] = { 0xFF, 0xFF, 0xFF, 0u, 0u, 0u, 0u, 0u };
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_check_value_range_uint64(&dsr, 0u, 16777215ull));
   buf[4] = 1;
   buf[0] = buf[1] = buf[2] = 0u;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_uint64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_deserializer_check_value_range_uint64(&dsr, 0u, 16777215ull));
   apx_vm_deserializer_destroy(&dsr);
}

static void test_range_check_int64_scalar(CuTest* tc)
{   
   apx_vm_deserializer_t dsr;
   uint8_t buf[UINT64_SIZE] = { 0x18, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; //-1000 encoded as little endian
   uint32_t const array_length = 0u;
   apx_sizeType_t const dynamic_size_type = APX_SIZE_TYPE_NONE;
   apx_vm_deserializer_create(&dsr);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_check_value_range_int64(&dsr, -1000, 1000));
   buf[0] = 0x17;  //Value is now -1001
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_set_read_buffer(&dsr, &buf[0], sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_deserializer_unpack_int64(&dsr, array_length, dynamic_size_type));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_deserializer_bytes_read(&dsr));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_deserializer_check_value_range_int64(&dsr, -1000, 1000));
   apx_vm_deserializer_destroy(&dsr);
}
