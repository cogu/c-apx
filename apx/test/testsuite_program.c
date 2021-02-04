//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/program.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_program_encode_pack_header_uint8_size(CuTest* tc);
static void test_apx_program_encode_pack_header_uint16_size(CuTest* tc);
static void test_apx_program_encode_pack_header_uint32_size(CuTest* tc);
static void test_apx_program_encode_pack_header_uint8_element_size_uint8_queue_size(CuTest* tc);
static void test_apx_program_encode_pack_header_uint8_element_size_uint16_queue_size(CuTest* tc);
static void test_apx_program_decode_pack_header_uint8_size(CuTest* tc);
static void test_apx_program_decode_pack_header_uint16_size(CuTest* tc);
static void test_apx_program_decode_pack_header_uint32_size(CuTest* tc);
static void test_apx_program_decode_pack_header_elem_size_2_queue_size_4(CuTest* tc);
static void test_apx_program_decode_pack_header_elem_size_1_queue_size_1000(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_program(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_program_encode_pack_header_uint8_size);
   SUITE_ADD_TEST(suite, test_apx_program_encode_pack_header_uint16_size);
   SUITE_ADD_TEST(suite, test_apx_program_encode_pack_header_uint32_size);
   SUITE_ADD_TEST(suite, test_apx_program_encode_pack_header_uint8_element_size_uint8_queue_size);
   SUITE_ADD_TEST(suite, test_apx_program_encode_pack_header_uint8_element_size_uint16_queue_size);
   SUITE_ADD_TEST(suite, test_apx_program_decode_pack_header_uint8_size);
   SUITE_ADD_TEST(suite, test_apx_program_decode_pack_header_uint16_size);
   SUITE_ADD_TEST(suite, test_apx_program_decode_pack_header_uint32_size);
   SUITE_ADD_TEST(suite, test_apx_program_decode_pack_header_elem_size_2_queue_size_4);
   SUITE_ADD_TEST(suite, test_apx_program_decode_pack_header_elem_size_1_queue_size_1000);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_program_encode_pack_header_uint8_size(CuTest* tc)
{
   uint8_t const expected_data1[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, 0u };
   uint8_t const expected_data2[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, 255u };
   apx_program_t expected;
   apx_program_t min_header;
   apx_program_t max_header;
   APX_PROGRAM_CREATE(&expected);
   APX_PROGRAM_CREATE(&min_header);
   adt_bytearray_append(&expected, expected_data1, (uint32_t)sizeof(expected_data1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(&min_header, APX_PACK_PROGRAM, 0u, 0u, false));
   CuAssertTrue(tc, adt_bytearray_equals(&expected, &min_header));
   APX_PROGRAM_DESTROY(&min_header);
   adt_bytearray_clear(&expected);
   adt_bytearray_append(&expected, expected_data2, (uint32_t)sizeof(expected_data2));
   APX_PROGRAM_CREATE(&max_header);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(&max_header, APX_PACK_PROGRAM, 255u, 0u, false));
   CuAssertTrue(tc, adt_bytearray_equals(&expected, &max_header));
   APX_PROGRAM_DESTROY(&max_header);
   APX_PROGRAM_DESTROY(&expected);
}

static void test_apx_program_encode_pack_header_uint16_size(CuTest* tc)
{
   uint8_t const expected_data1[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT16, 0u, 1u }; //256 encoded in little endian
   uint8_t const expected_data2[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT16, 0xFFu, 0xFFu}; //65535 encoded in little endian
   apx_program_t expected;
   apx_program_t min_header;
   apx_program_t max_header;
   APX_PROGRAM_CREATE(&expected);
   APX_PROGRAM_CREATE(&min_header);
   adt_bytearray_append(&expected, expected_data1, (uint32_t)sizeof(expected_data1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(&min_header, APX_PACK_PROGRAM, 256u, 0u, false));
   CuAssertTrue(tc, adt_bytearray_equals(&expected, &min_header));
   APX_PROGRAM_DESTROY(&min_header);
   adt_bytearray_clear(&expected);
   adt_bytearray_append(&expected, expected_data2, (uint32_t)sizeof(expected_data2));
   APX_PROGRAM_CREATE(&max_header);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(&max_header, APX_PACK_PROGRAM, 65535u, 0u, false));
   CuAssertTrue(tc, adt_bytearray_equals(&expected, &max_header));
   APX_PROGRAM_DESTROY(&max_header);
   APX_PROGRAM_DESTROY(&expected);
}

static void test_apx_program_encode_pack_header_uint32_size(CuTest* tc)
{
   uint8_t const expected_data1[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT32, 0u, 0, 1u, 0 }; //65536 encoded in little endian
   uint8_t const expected_data2[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT32, 0xFFu, 0xFFu, 0xFFu, 0xFFu }; //UINT32_MAX
   apx_program_t expected;
   apx_program_t min_header;
   apx_program_t max_header;
   APX_PROGRAM_CREATE(&expected);
   APX_PROGRAM_CREATE(&min_header);
   adt_bytearray_append(&expected, expected_data1, (uint32_t)sizeof(expected_data1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(&min_header, APX_PACK_PROGRAM, 65536, 0u, false));
   CuAssertTrue(tc, adt_bytearray_equals(&expected, &min_header));
   APX_PROGRAM_DESTROY(&min_header);
   adt_bytearray_clear(&expected);
   adt_bytearray_append(&expected, expected_data2, (uint32_t)sizeof(expected_data2));
   APX_PROGRAM_CREATE(&max_header);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(&max_header, APX_PACK_PROGRAM, UINT32_MAX, 0u, false));
   CuAssertTrue(tc, adt_bytearray_equals(&expected, &max_header));
   APX_PROGRAM_DESTROY(&max_header);
   APX_PROGRAM_DESTROY(&expected);
}

static void test_apx_program_encode_pack_header_uint8_element_size_uint8_queue_size(CuTest* tc)
{
   uint8_t const element_size = UINT8_SIZE;
   uint8_t const expected_data[] = { APX_VM_HEADER_FLAG_QUEUED_DATA | APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
      element_size * 10 + 1, //QueueSize=10, ElementSize=1, QueueStorageSize=1
      APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT), element_size };
   apx_program_t expected;
   apx_program_t header;
   uint32_t const queue_size = 10u;
   APX_PROGRAM_CREATE(&expected);
   APX_PROGRAM_CREATE(&header);
   adt_bytearray_append(&expected, expected_data, (uint32_t)sizeof(expected_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(&header, APX_PACK_PROGRAM, UINT8_SIZE, queue_size, false));
   CuAssertTrue(tc, adt_bytearray_equals(&expected, &header));
   APX_PROGRAM_DESTROY(&header);
   APX_PROGRAM_DESTROY(&expected);
}

static void test_apx_program_encode_pack_header_uint8_element_size_uint16_queue_size(CuTest* tc)
{
   uint8_t const element_size = UINT8_SIZE;
   uint32_t const queue_size = 4095u;
   uint8_t const expected_data[] = { APX_VM_HEADER_FLAG_QUEUED_DATA | APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT16,
      0x01, 0x10, //QueueSize=4095, ElementSize=1, QueueStorageSize=2 => TotalDataSize = 4097 = 4096+1
      APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U16 << APX_VM_INST_VARIANT_SHIFT), element_size };
   apx_program_t expected;
   apx_program_t header;

   APX_PROGRAM_CREATE(&expected);
   APX_PROGRAM_CREATE(&header);
   adt_bytearray_append(&expected, expected_data, (uint32_t)sizeof(expected_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(&header, APX_PACK_PROGRAM, UINT8_SIZE, queue_size, false));
   CuAssertTrue(tc, adt_bytearray_equals(&expected, &header));
   APX_PROGRAM_DESTROY(&header);
   APX_PROGRAM_DESTROY(&expected);
}

static void test_apx_program_decode_pack_header_uint8_size(CuTest* tc)
{
   uint8_t program_bytes[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, 0u };
   uint8_t const* next = NULL;
   uint8_t const* end = program_bytes + sizeof(program_bytes);
   apx_programHeader_t header;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_decode_header(program_bytes, end, &next, &header));
   CuAssertConstPtrEquals(tc, end, next);
   CuAssertUIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, 0, header.queue_length);
   CuAssertUIntEquals(tc, 0, header.element_size);
   CuAssertUIntEquals(tc, 0, header.data_size);

   program_bytes[1] = 255;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_decode_header(program_bytes, end, &next, &header));
   CuAssertConstPtrEquals(tc, end, next);
   CuAssertUIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, 0, header.queue_length);
   CuAssertUIntEquals(tc, 0, header.element_size);
   CuAssertUIntEquals(tc, 255, header.data_size);
}

static void test_apx_program_decode_pack_header_uint16_size(CuTest* tc)
{
   uint8_t program_bytes[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT16, 0u, 0u };
   uint8_t const* next = NULL;
   uint8_t const* end = program_bytes + sizeof(program_bytes);
   apx_programHeader_t header;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_decode_header(program_bytes, end, &next, &header));
   CuAssertConstPtrEquals(tc, end, next);
   CuAssertUIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, 0u, header.queue_length);
   CuAssertUIntEquals(tc, 0, header.element_size);
   CuAssertUIntEquals(tc, 0u, header.data_size);

   program_bytes[1] = 255u;
   program_bytes[2] = 255u;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_decode_header(program_bytes, end, &next, &header));
   CuAssertConstPtrEquals(tc, end, next);
   CuAssertUIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, 0, header.queue_length);
   CuAssertUIntEquals(tc, 0, header.element_size);
   CuAssertUIntEquals(tc, UINT16_MAX, header.data_size);
}

static void test_apx_program_decode_pack_header_uint32_size(CuTest* tc)
{
   uint8_t program_bytes[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT32, 0u, 0u, 0u, 0u };
   uint8_t const* next = NULL;
   uint8_t const* end = program_bytes + sizeof(program_bytes);
   apx_programHeader_t header;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_decode_header(program_bytes, end, &next, &header));
   CuAssertConstPtrEquals(tc, end, next);
   CuAssertUIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, 0, header.queue_length);
   CuAssertUIntEquals(tc, 0, header.element_size);
   CuAssertUIntEquals(tc, 0, header.data_size);

   program_bytes[1] = 255u;
   program_bytes[2] = 255u;
   program_bytes[3] = 255u;
   program_bytes[4] = 255u;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_decode_header(program_bytes, end, &next, &header));
   CuAssertConstPtrEquals(tc, end, next);
   CuAssertUIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, 0, header.queue_length);
   CuAssertUIntEquals(tc, 0, header.element_size);
   CuAssertUIntEquals(tc, UINT32_MAX, header.data_size);
}

static void test_apx_program_decode_pack_header_elem_size_2_queue_size_4(CuTest* tc)
{
   uint8_t const queue_size = 4;
   uint8_t const element_size = UINT16_SIZE;
   uint8_t program_bytes[] = { APX_VM_HEADER_FLAG_QUEUED_DATA | APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
         UINT8_SIZE + element_size * queue_size , APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT), element_size };
   uint8_t const* next = NULL;
   uint8_t const* end = program_bytes + sizeof(program_bytes);
   apx_programHeader_t header;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_decode_header(program_bytes, end, &next, &header));
   CuAssertConstPtrEquals(tc, end, next);
   CuAssertUIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, queue_size, header.queue_length);
   CuAssertUIntEquals(tc, element_size, header.element_size);
   CuAssertUIntEquals(tc, 9, header.data_size);
}

static void test_apx_program_decode_pack_header_elem_size_1_queue_size_1000(CuTest* tc)
{
   uint16_t const queue_size = 1000;
   uint8_t const element_size = UINT8_SIZE;
   uint8_t program_bytes[] = { APX_VM_HEADER_FLAG_QUEUED_DATA | APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT16,
        0xEA , 0x03, //1002 encoded in little-endian
        APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U16 << APX_VM_INST_VARIANT_SHIFT), element_size };
   uint8_t const* next = NULL;
   uint8_t const* end = program_bytes + sizeof(program_bytes);
   apx_programHeader_t header;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_decode_header(program_bytes, end, &next, &header));
   CuAssertConstPtrEquals(tc, end, next);
   CuAssertUIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, queue_size, header.queue_length);
   CuAssertUIntEquals(tc, element_size, header.element_size);
   CuAssertUIntEquals(tc, 1002, header.data_size);
}
