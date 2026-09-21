/*****************************************************************************
* \file      testsuite_program_decoder.c
* \author    Conny Gustafsson
* \date      2021-01-15
* \brief     Unit tests for APX VM 2.1 program decoder
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "CuTest.h"
#include "apx/program_encoder.h"
#include "apx/program_decoder.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_program_end_on_empty_program(CuTest* tc);
static void test_parse_simple_pack_header_uint8(CuTest* tc);
static void test_parse_simple_unpack_header_uint16(CuTest* tc);
static void test_parse_pack_header_uint32(CuTest* tc);
static void test_parse_queued_header(CuTest* tc);
static void test_parse_invalid_header_version(CuTest* tc);
static void test_parse_truncated_header(CuTest* tc);
static void test_decode_pack_scalar_types(CuTest* tc);
static void test_decode_unpack_scalar_types(CuTest* tc);
static void test_decode_fixed_array(CuTest* tc);
static void test_decode_dynamic_array(CuTest* tc);
static void test_decode_range_checks_unsigned(CuTest* tc);
static void test_decode_range_checks_signed(CuTest* tc);
static void test_decode_record_select_and_end(CuTest* tc);
static void test_decode_flow_ctrl_array_next(CuTest* tc);
static void test_decode_position_save_and_recall(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_program_decoder(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_program_end_on_empty_program);
   SUITE_ADD_TEST(suite, test_parse_simple_pack_header_uint8);
   SUITE_ADD_TEST(suite, test_parse_simple_unpack_header_uint16);
   SUITE_ADD_TEST(suite, test_parse_pack_header_uint32);
   SUITE_ADD_TEST(suite, test_parse_queued_header);
   SUITE_ADD_TEST(suite, test_parse_invalid_header_version);
   SUITE_ADD_TEST(suite, test_parse_truncated_header);
   SUITE_ADD_TEST(suite, test_decode_pack_scalar_types);
   SUITE_ADD_TEST(suite, test_decode_unpack_scalar_types);
   SUITE_ADD_TEST(suite, test_decode_fixed_array);
   SUITE_ADD_TEST(suite, test_decode_dynamic_array);
   SUITE_ADD_TEST(suite, test_decode_range_checks_unsigned);
   SUITE_ADD_TEST(suite, test_decode_range_checks_signed);
   SUITE_ADD_TEST(suite, test_decode_record_select_and_end);
   SUITE_ADD_TEST(suite, test_decode_flow_ctrl_array_next);
   SUITE_ADD_TEST(suite, test_decode_position_save_and_recall);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_program_end_on_empty_program(CuTest* tc)
{
   uint8_t program[1] = { 0 };
   apx_program_decoder_t decoder;
   apx_operation_type_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_programDecoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, program, 0u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PROGRAM_END, operation_type);
   apx_programDecoder_destroy(&decoder);
}

static void test_parse_simple_pack_header_uint8(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_program_header_t header;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_program_header(&encoder, APX_PACK_PROGRAM, 10u, 0u, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.header), adt_bytearray_length(&encoder.header)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_program_header(&decoder, &header));

   CuAssertIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertUIntEquals(tc, 10u, header.data_size);
   CuAssertFalse(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, 0u, header.element_size);
   CuAssertUIntEquals(tc, 0u, header.queue_length);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_parse_simple_unpack_header_uint16(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_program_header_t header;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_program_header(&encoder, APX_UNPACK_PROGRAM, 1000u, 0u, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.header), adt_bytearray_length(&encoder.header)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_program_header(&decoder, &header));

   CuAssertIntEquals(tc, APX_UNPACK_PROGRAM, header.program_type);
   CuAssertUIntEquals(tc, 1000u, header.data_size);
   CuAssertTrue(tc, header.has_dynamic_data);
   CuAssertUIntEquals(tc, 0u, header.element_size);
   CuAssertUIntEquals(tc, 0u, header.queue_length);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_parse_pack_header_uint32(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_program_header_t header;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_program_header(&encoder, APX_PACK_PROGRAM, 100000u, 0u, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.header), adt_bytearray_length(&encoder.header)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_program_header(&decoder, &header));

   CuAssertIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertUIntEquals(tc, 100000u, header.data_size);
   CuAssertFalse(tc, header.has_dynamic_data);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_parse_queued_header(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_program_header_t header;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   // elem_size = 2, queue_size = 4 => data_size = 1 + (2 * 4) = 9
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_program_header(&encoder, APX_PACK_PROGRAM, 2u, 4u, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.header), adt_bytearray_length(&encoder.header)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_program_header(&decoder, &header));

   CuAssertIntEquals(tc, APX_PACK_PROGRAM, header.program_type);
   CuAssertUIntEquals(tc, 9u, header.data_size);
   CuAssertUIntEquals(tc, 2u, header.element_size);
   CuAssertUIntEquals(tc, 4u, header.queue_length);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_parse_invalid_header_version(CuTest* tc)
{
   uint8_t bad_header[] = { '3', '0', 0x08, 10u };
   apx_program_decoder_t decoder;
   apx_program_header_t header;

   apx_programDecoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, bad_header, sizeof(bad_header)));
   CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR, apx_programDecoder_parse_program_header(&decoder, &header));
   apx_programDecoder_destroy(&decoder);
}

static void test_parse_truncated_header(CuTest* tc)
{
   uint8_t truncated[] = { '2', '1', 0x08 }; // Missing data_size byte
   apx_program_decoder_t decoder;
   apx_program_header_t header;

   apx_programDecoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, truncated, sizeof(truncated)));
   CuAssertIntEquals(tc, APX_PARSE_ERROR, apx_programDecoder_parse_program_header(&decoder, &header));
   apx_programDecoder_destroy(&decoder);
}

static void test_decode_pack_scalar_types(CuTest* tc)
{
   struct {
      uint8_t variant;
      apx_type_code_t expected_type;
   } test_cases[] = {
      { APX_VM_VARIANT_UINT8, APX_TYPE_CODE_UINT8 },
      { APX_VM_VARIANT_UINT16, APX_TYPE_CODE_UINT16 },
      { APX_VM_VARIANT_UINT32, APX_TYPE_CODE_UINT32 },
      { APX_VM_VARIANT_UINT64, APX_TYPE_CODE_UINT64 },
      { APX_VM_VARIANT_INT8, APX_TYPE_CODE_INT8 },
      { APX_VM_VARIANT_INT16, APX_TYPE_CODE_INT16 },
      { APX_VM_VARIANT_INT32, APX_TYPE_CODE_INT32 },
      { APX_VM_VARIANT_INT64, APX_TYPE_CODE_INT64 },
      { APX_VM_VARIANT_BOOL, APX_TYPE_CODE_BOOL },
      { APX_VM_VARIANT_BYTE, APX_TYPE_CODE_BYTE },
      { APX_VM_VARIANT_CHAR, APX_TYPE_CODE_CHAR },
      { APX_VM_VARIANT_CHAR8, APX_TYPE_CODE_CHAR8 },
      { APX_VM_VARIANT_RECORD, APX_TYPE_CODE_RECORD },
   };

   size_t const num_cases = sizeof(test_cases) / sizeof(test_cases[0]);
   for (size_t i = 0u; i < num_cases; ++i)
   {
      apx_program_encoder_t encoder;
      apx_program_decoder_t decoder;
      apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;
      apx_pack_unpack_operation_info_t info = { APX_TYPE_CODE_NONE, 0u, false };

      apx_programEncoder_create(&encoder);
      apx_programDecoder_create(&decoder);

      CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, test_cases[i].variant, false));
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
      CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, op_type);

      apx_programDecoder_get_pack_unpack_info(&decoder, &info);
      CuAssertIntEquals(tc, test_cases[i].expected_type, info.type_code);
      CuAssertUIntEquals(tc, 0u, info.array_length);
      CuAssertFalse(tc, info.is_dynamic_array);

      apx_programDecoder_destroy(&decoder);
      apx_programEncoder_destroy(&encoder);
   }
}

static void test_decode_unpack_scalar_types(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;
   apx_pack_unpack_operation_info_t info = { APX_TYPE_CODE_NONE, 0u, false };

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_UINT32, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, op_type);

   apx_programDecoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT32, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_decode_fixed_array(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;
   apx_pack_unpack_operation_info_t info = { APX_TYPE_CODE_NONE, 0u, false };

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT8, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_array_size(&encoder, 50u, false));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, op_type);

   apx_programDecoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT8, info.type_code);
   CuAssertUIntEquals(tc, 50u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_decode_dynamic_array(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;
   apx_pack_unpack_operation_info_t info = { APX_TYPE_CODE_NONE, 0u, false };

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_UNPACK, APX_VM_VARIANT_UINT16, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_array_size(&encoder, 2000u, true));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, op_type);

   apx_programDecoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT16, info.type_code);
   CuAssertUIntEquals(tc, 2000u, info.array_length);
   CuAssertTrue(tc, info.is_dynamic_array);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_decode_range_checks_unsigned(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   // Encode limit check uint32: lower = 10, upper = 50000, is_array = false
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_limit_check_instruction(&encoder, APX_VM_VARIANT_LIMIT_CHECK_U32, 10, 50000, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_UINT32, op_type);

   apx_range_check_uint32_operation_info_t info = { 0u, 0u };
   apx_programDecoder_range_check_info_uint32(&decoder, &info);
   CuAssertUIntEquals(tc, 10u, info.lower_limit);
   CuAssertUIntEquals(tc, 50000u, info.upper_limit);
   CuAssertFalse(tc, apx_programDecoder_is_array_limit(&decoder));

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_decode_range_checks_signed(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   // Encode limit check int16: lower = -1000, upper = 1000, is_array = true
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_limit_check_instruction(&encoder, APX_VM_VARIANT_LIMIT_CHECK_S16, -1000, 1000, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_INT32, op_type);

   apx_range_check_int32_operation_info_t info = { 0, 0 };
   apx_programDecoder_range_check_info_int32(&decoder, &info);
   CuAssertIntEquals(tc, -1000, info.lower_limit);
   CuAssertIntEquals(tc, 1000, info.upper_limit);
   CuAssertTrue(tc, apx_programDecoder_is_array_limit(&decoder));

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_decode_record_select_and_end(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   // First field: "Field1", is_first = true
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_DATA_CTRL, APX_VM_VARIANT_RECORD_SELECT, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_field_name(&encoder, "Field1"));

   // Second field: "Field2", is_first = false
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_DATA_CTRL, APX_VM_VARIANT_RECORD_SELECT, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_field_name(&encoder, "Field2"));

   // Record end
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_DATA_CTRL, APX_VM_VARIANT_RECORD_END, false));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));

   // Parse first field
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RECORD_SELECT, op_type);
   CuAssertStrEquals(tc, "Field1", apx_programDecoder_get_field_name(&decoder));
   CuAssertTrue(tc, apx_programDecoder_is_first_field(&decoder));

   // Parse second field
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RECORD_SELECT, op_type);
   CuAssertStrEquals(tc, "Field2", apx_programDecoder_get_field_name(&decoder));
   CuAssertFalse(tc, apx_programDecoder_is_first_field(&decoder));

   // Parse record end
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RECORD_END, op_type);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_decode_flow_ctrl_array_next(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_FLOW_CTRL, APX_VM_VARIANT_ARRAY_NEXT, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_ARRAY_NEXT, op_type);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}

static void test_decode_position_save_and_recall(CuTest* tc)
{
   apx_program_encoder_t encoder;
   apx_program_decoder_t decoder;
   apx_operation_type_t op_type = APX_OPERATION_TYPE_PROGRAM_END;
   apx_pack_unpack_operation_info_t info;

   apx_programEncoder_create(&encoder);
   apx_programDecoder_create(&decoder);

   // Encode: PACK UINT8, then PACK UINT16
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT8, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programEncoder_encode_instruction(&encoder, APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT16, false));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_select_program(&decoder, adt_bytearray_data(&encoder.buffer), adt_bytearray_length(&encoder.buffer)));
   CuAssertFalse(tc, apx_programDecoder_has_saved_program_position(&decoder));

   // Read first instruction (UINT8)
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, op_type);
   apx_programDecoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT8, info.type_code);

   // Save position before reading second instruction
   apx_programDecoder_save_program_position(&decoder);
   CuAssertTrue(tc, apx_programDecoder_has_saved_program_position(&decoder));

   // Read second instruction (UINT16)
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, op_type);
   apx_programDecoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT16, info.type_code);

   // End of program
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PROGRAM_END, op_type);

   // Recall position
   apx_programDecoder_recall_program_position(&decoder);

   // Reading next operation should again return second instruction (UINT16)
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_programDecoder_parse_next_operation(&decoder, &op_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, op_type);
   apx_programDecoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT16, info.type_code);

   apx_programDecoder_destroy(&decoder);
   apx_programEncoder_destroy(&encoder);
}
