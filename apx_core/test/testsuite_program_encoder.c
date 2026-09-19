/*****************************************************************************
* \file      testsuite_program_encoder.c
* \author    Conny Gustafsson
* \date      2021-01-15
* \brief     Unit tests for APX VM 2.1 program encoder
*
* Copyright (c) 2021 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "CuTest.h"
#include "apx/program_encoder.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_encode_instruction_pack_uint8(CuTest* tc);
static void test_encode_instruction_pack_uint16(CuTest* tc);
static void test_encode_instruction_pack_uint32(CuTest* tc);
static void test_encode_instruction_pack_uint64(CuTest* tc);
static void test_encode_instruction_pack_uint8_array(CuTest* tc);
static void test_encode_instruction_pack_record(CuTest* tc);
static void test_encode_instruction_pack_record_array(CuTest* tc);
static void test_encode_instruction_unpack_uint8(CuTest* tc);
static void test_encode_instruction_unpack_uint16(CuTest* tc);
static void test_encode_instruction_unpack_uint32(CuTest* tc);
static void test_encode_instruction_unpack_uint64(CuTest* tc);
static void test_encode_instruction_unpack_uint8_array(CuTest* tc);
static void test_encode_instruction_unpack_record(CuTest* tc);
static void test_encode_instruction_unpack_record_array(CuTest* tc);
static void test_encode_array_size_uint8(CuTest* tc);
static void test_encode_array_size_uint16(CuTest* tc);
static void test_encode_array_size_dynamic_uint32(CuTest* tc);
static void test_encode_limit_values_uint8(CuTest* tc);
static void test_encode_limit_values_int16(CuTest* tc);
static void test_encode_simple_pack_program_header(CuTest* tc);
static void test_encode_simple_unpack_program_header(CuTest* tc);
static void test_encode_queued_program_header(CuTest* tc);
static void test_encode_field_name(CuTest* tc);
static void test_get_program(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_program_encoder(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_encode_instruction_pack_uint8);
   SUITE_ADD_TEST(suite, test_encode_instruction_pack_uint16);
   SUITE_ADD_TEST(suite, test_encode_instruction_pack_uint32);
   SUITE_ADD_TEST(suite, test_encode_instruction_pack_uint64);
   SUITE_ADD_TEST(suite, test_encode_instruction_pack_uint8_array);
   SUITE_ADD_TEST(suite, test_encode_instruction_pack_record);
   SUITE_ADD_TEST(suite, test_encode_instruction_pack_record_array);
   SUITE_ADD_TEST(suite, test_encode_instruction_unpack_uint8);
   SUITE_ADD_TEST(suite, test_encode_instruction_unpack_uint16);
   SUITE_ADD_TEST(suite, test_encode_instruction_unpack_uint32);
   SUITE_ADD_TEST(suite, test_encode_instruction_unpack_uint64);
   SUITE_ADD_TEST(suite, test_encode_instruction_unpack_uint8_array);
   SUITE_ADD_TEST(suite, test_encode_instruction_unpack_record);
   SUITE_ADD_TEST(suite, test_encode_instruction_unpack_record_array);
   SUITE_ADD_TEST(suite, test_encode_array_size_uint8);
   SUITE_ADD_TEST(suite, test_encode_array_size_uint16);
   SUITE_ADD_TEST(suite, test_encode_array_size_dynamic_uint32);
   SUITE_ADD_TEST(suite, test_encode_limit_values_uint8);
   SUITE_ADD_TEST(suite, test_encode_limit_values_int16);
   SUITE_ADD_TEST(suite, test_encode_simple_pack_program_header);
   SUITE_ADD_TEST(suite, test_encode_simple_unpack_program_header);
   SUITE_ADD_TEST(suite, test_encode_queued_program_header);
   SUITE_ADD_TEST(suite, test_encode_field_name);
   SUITE_ADD_TEST(suite, test_get_program);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_encode_instruction_pack_uint8(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT8, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00000000u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_pack_uint16(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT16, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00000001u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_pack_uint32(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT32, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00000010u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_pack_uint64(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT64, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00000011u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_pack_uint8_array(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT8, true));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b10000000u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_pack_record(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_RECORD, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00001010u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_pack_record_array(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_RECORD, true));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b10001010u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_unpack_uint8(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_UINT8, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00010000u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_unpack_uint16(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_UINT16, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00010001u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_unpack_uint32(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_UINT32, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00010010u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_unpack_uint64(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_UINT64, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00010011u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_unpack_uint8_array(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_UINT8, true));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b10010000u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_unpack_record(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_RECORD, false));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b00011010u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_instruction_unpack_record_array(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_RECORD, true));
   CuAssertUIntEquals(tc, 1u, adt_bytearray_length(&encoder.buffer));
   CuAssertUIntEquals(tc, 0b10011010u, adt_bytearray_data(&encoder.buffer)[0]);
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_array_size_uint8(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_array_size(&encoder, 10u, false));
   uint8_t const expected[] = { 0b00100000u, 10u };
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.buffer, expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_array_size_uint16(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_array_size(&encoder, 1000u, false));
   uint8_t const expected[] = { 0b00100001u, 0xE8u, 0x03u }; // 1000 in little-endian
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.buffer, expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_array_size_dynamic_uint32(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_array_size(&encoder, 100000u, true));
   uint8_t const expected[] = { 0b10100010u, 0xA0u, 0x86u, 0x01u, 0x00u }; // 100000 in little-endian
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.buffer, expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_limit_values_uint8(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_limit_check_instruction(&encoder, APX_VM_VARIANT_LIMIT_CHECK_U8, 0, 255, false));
   uint8_t const expected[] = {
      (APX_VM_OPCODE_DATA_CTRL << APX_VM_INST_OPCODE_SHIFT) | APX_VM_VARIANT_LIMIT_CHECK_U8,
      0u,
      255u
   };
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.buffer, expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_limit_values_int16(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_limit_check_instruction(&encoder, APX_VM_VARIANT_LIMIT_CHECK_S16, -100, 100, true));
   uint8_t const expected[] = {
      APX_VM_INST_FLAG | (APX_VM_OPCODE_DATA_CTRL << APX_VM_INST_OPCODE_SHIFT) | APX_VM_VARIANT_LIMIT_CHECK_S16,
      0x9Cu, 0xFFu, // -100 in 16-bit signed LE
      0x64u, 0x00u  // 100 in 16-bit signed LE
   };
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.buffer, expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_simple_pack_program_header(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_program_header(&encoder, APX_PACK_PROGRAM, 10u, 0u, false));
   uint8_t const expected[] = {
      APX_VM_HEADER_VERSION_MAJOR,
      APX_VM_HEADER_VERSION_MINOR,
      APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
      10u
   };
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.header, expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_simple_unpack_program_header(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_program_header(&encoder, APX_UNPACK_PROGRAM, 1000u, 0u, true));
   uint8_t const expected[] = {
      APX_VM_HEADER_VERSION_MAJOR,
      APX_VM_HEADER_VERSION_MINOR,
      APX_VM_HEADER_PROG_TYPE_UNPACK | APX_VM_HEADER_FLAG_DYNAMIC_DATA | APX_VM_VARIANT_UINT16,
      0xE8u, 0x03u
   };
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.header, expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_queued_program_header(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   // elem_size = 2, queue_size = 4 => total max_data_size = 1 + (2 * 4) = 9
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_program_header(&encoder, APX_PACK_PROGRAM, 2u, 4u, false));
   uint8_t const expected[] = {
      APX_VM_HEADER_VERSION_MAJOR,
      APX_VM_HEADER_VERSION_MINOR,
      APX_VM_HEADER_FLAG_QUEUED_DATA | APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
      9u,
      (APX_VM_OPCODE_DATA_SIZE << APX_VM_INST_OPCODE_SHIFT) | APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U8,
      2u
   };
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.header, expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_encode_field_name(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_programEncoder_create(&encoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_field_name(&encoder, "EngineSpeed"));
   uint8_t const expected[] = "EngineSpeed";
   CuAssertUIntEquals(tc, (uint32_t)sizeof(expected), adt_bytearray_length(&encoder.buffer));
   CuAssertTrue(tc, adt_bytearray_data_equals(&encoder.buffer, (uint8_t const*)expected, sizeof(expected)));
   apx_programEncoder_destroy(&encoder);
}

static void test_get_program(CuTest* tc)
{
   apx_programEncoder_t encoder;
   apx_program_t program;
   apx_programEncoder_create(&encoder);
   APX_PROGRAM_CREATE(&program);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_program_header(&encoder, APX_PACK_PROGRAM, 1u, 0u, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT8, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_get_program(&encoder, &program));

   uint8_t const expected[] = {
      APX_VM_HEADER_VERSION_MAJOR,
      APX_VM_HEADER_VERSION_MINOR,
      APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
      1u,
      0b00000000u
   };
   CuAssertTrue(tc, adt_bytearray_data_equals(&program, expected, sizeof(expected)));

   APX_PROGRAM_DESTROY(&program);
   apx_programEncoder_destroy(&encoder);
}
