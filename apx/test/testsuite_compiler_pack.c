//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/compiler.h"
#include "apx/parser.h"
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

static void test_apx_compiler_pack_uint8(CuTest* tc);
static void test_apx_compiler_pack_uint8_array(CuTest* tc);
static void test_apx_compiler_pack_uint8_with_range_check(CuTest* tc);
static void test_apx_compiler_pack_queued_uint8(CuTest* tc);
static void test_apx_compiler_pack_dynamic_uint8_array(CuTest* tc);
static void test_apx_compiler_pack_int8(CuTest* tc);
static void test_apx_compiler_pack_int8_array(CuTest* tc);
static void test_apx_compiler_pack_uint16(CuTest* tc);
static void test_apx_compiler_pack_uint16_array(CuTest* tc);
static void test_apx_compiler_pack_int16(CuTest* tc);
static void test_apx_compiler_pack_int16_array(CuTest* tc);
static void test_apx_compiler_pack_uint32(CuTest* tc);
static void test_apx_compiler_pack_uint32_array(CuTest* tc);
static void test_apx_compiler_pack_int32(CuTest* tc);
static void test_apx_compiler_pack_int32_array(CuTest* tc);
static void test_apx_compiler_pack_uint64(CuTest* tc);
static void test_apx_compiler_pack_uint64_array(CuTest* tc);
static void test_apx_compiler_pack_int64(CuTest* tc);
static void test_apx_compiler_pack_int64_array(CuTest* tc);
static void test_apx_compiler_pack_bool(CuTest* tc);
static void test_apx_compiler_pack_bool_array(CuTest* tc);
static void test_apx_compiler_pack_byte(CuTest* tc);
static void test_apx_compiler_pack_byte_array(CuTest* tc);
static void test_apx_compiler_pack_dynamic_byte_array(CuTest* tc);
static void test_apx_compiler_pack_char_string(CuTest* tc);
static void test_apx_compiler_pack_char8_string(CuTest* tc);
static void test_apx_compiler_pack_record_u8_u16(CuTest* tc);
static void test_apx_compiler_pack_uint8_reference(CuTest* tc);
static void test_apx_compiler_pack_record_reference_with_child_references(CuTest* tc);
static void test_apx_compiler_pack_array_of_records(CuTest* tc);
static void test_apx_compiler_pack_dynamic_array_of_records(CuTest* tc);
static void test_apx_compiler_pack_record_DYNU8_U16(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_compiler_pack(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint8);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint8_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint8_with_range_check);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_queued_uint8);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_dynamic_uint8_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_int8);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_int8_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint16);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint16_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_int16);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_int16_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint32);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint32_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_int32);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_int32_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint64);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint64_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_int64);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_int64_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_bool);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_bool_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_byte);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_byte_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_dynamic_byte_array);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_char_string);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_char8_string);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_record_u8_u16);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_uint8_reference);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_record_reference_with_child_references);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_array_of_records);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_dynamic_array_of_records);
   SUITE_ADD_TEST(suite, test_apx_compiler_pack_record_DYNU8_U16);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_compiler_pack_uint8(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U8Port\"C:=255";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT8_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT) };


   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t) sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_uint8_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U8Port\"C[2]:={255, 255}\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT8_SIZE * 2,
      APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
      APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
      2u
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_uint8_with_range_check(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U8Port\"C(0,3):=3\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT8_SIZE,
         APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_U8 << APX_VM_INST_VARIANT_SHIFT),
         0u,
         3u,
         APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_queued_uint8(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"U8QueuedSignal\"C:Q[10]\n";
   uint8_t const queue_length = 10u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_HEADER_FLAG_QUEUED_DATA | APX_VM_VARIANT_UINT8, UINT8_SIZE + UINT8_SIZE * queue_length,
         APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
         UINT8_SIZE,
         APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_provide_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_dynamic_uint8_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "P\"U8Port\"C[8*]\n";
   uint8_t const array_length = 8u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_HEADER_FLAG_DYNAMIC_DATA | APX_VM_VARIANT_UINT8, UINT8_SIZE + UINT8_SIZE * array_length,
         APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
         APX_VM_DYN_ARRAY_FLAG | APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
         array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_provide_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_int8(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"S8Port\"c:=0\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, INT8_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT8 << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_apx_compiler_pack_int8_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"S8Port\"c[2]:={-1, -1}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, INT8_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT8 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_uint16(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U16Port\"S:=65535\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT16_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT16 << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_uint16_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U16Port\"S[2]:={65535, 65535}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT16_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT16 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_int16(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"S16Port\"s:=-1\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, INT16_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT16 << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_int16_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"S16Port\"s[2]:={-1, -1}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, INT16_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT16 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_uint32(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U32Port\"L:=0\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT32_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT32 << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_apx_compiler_pack_uint32_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U32Port\"L[2]:={0, 0}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT32_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT32 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_int32(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"S32Port\"l:=-1\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, INT32_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT32 << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_int32_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"S32Port\"l[2]:={-1, -1}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, INT32_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT32 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_uint64(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U64Port\"Q:=0\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT64_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT64 << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_uint64_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"U64Port\"Q[2]:={0, 0}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT64_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT64 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_int64(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"S64Port\"q:=-1\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, INT64_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT64 << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_int64_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"S64Port\"q[2]:={-1, -1}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, INT64_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT64 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_bool(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"BoolPort\"b:=0\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, BOOL_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_BOOL << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_bool_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"BoolPort\"b[2]:={0, 0}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, BOOL_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_BOOL << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_byte(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"BytePort\"B:=255\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, BYTE_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_BYTE << APX_VM_INST_VARIANT_SHIFT) };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_byte_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"BytePort\"B[2]:={0, 0}\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, BYTE_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_BYTE << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_dynamic_byte_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"BytePort\"B[4095*]:={}\n";
   uint8_t const data_length_byte0 = 0x01;
   uint8_t const data_length_byte1 = 0x10u; //together these two bytes form 0xfff which is 4097
   uint8_t const array_length_byte0 = 0xff;
   uint8_t const array_length_byte1 = 0x0fu; //together these two bytes form 0xfff which is 4095
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_HEADER_FLAG_DYNAMIC_DATA | APX_VM_VARIANT_UINT16,
      data_length_byte0,
      data_length_byte1,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_BYTE << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_DYN_ARRAY_FLAG | APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U16 << APX_VM_INST_VARIANT_SHIFT),
   array_length_byte0,
   array_length_byte1
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_char_string(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"StringPort\"a[120]\n";
   uint8_t const array_length = 120u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
   CHAR_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_CHAR << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_char8_string(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"StringPort\"A[120]\n";
   uint8_t const array_length = 120u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
   CHAR8_SIZE * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_CHAR8 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
   array_length
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_record_u8_u16(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"RecordPort\"{\"First\"C\"Second\"S}:={255, 65535}\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
   UINT8_SIZE + UINT16_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_RECORD << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'F', 'i', 'r', 's', 't', '\0',
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_LAST_FIELD_FLAG | APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'S', 'e', 'c', 'o', 'n', 'd', '\0',
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT16 << APX_VM_INST_VARIANT_SHIFT),
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_uint8_reference(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"Type_T\"C(0,3):VT(\"Off\",\"On\",\"Error\",\"NotAvailable\")\n"
      "R\"UInt8Port\"T[0]:=3\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8, UINT8_SIZE,
         APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_U8 << APX_VM_INST_VARIANT_SHIFT),
         0u,
         3u,
         APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_record_reference_with_child_references(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "T\"FirstType_T\"C(0,3)\n"
      "T\"SecondType_T\"C(0,7)\n"
      "T\"RecordType_T\"{\"First\"T[0]\"Second\"T[1]}\n"
      "R\"RecordPort\"T[2]:={3,7}\n";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
   UINT8_SIZE + UINT8_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_RECORD << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'F', 'i', 'r', 's', 't', '\0',
   APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_U8 << APX_VM_INST_VARIANT_SHIFT),
   0u,
   3u,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_LAST_FIELD_FLAG | APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'S', 'e', 'c', 'o', 'n', 'd', '\0',
   APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_U8 << APX_VM_INST_VARIANT_SHIFT),
   0u,
   7u,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_array_of_records(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"RecordPort\"{\"Id\"S\"Value\"C}[2]:={ {0xFFFF,0}, {0xFFFF,0} }\n";
   uint8_t const array_length = 2u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
   (UINT16_SIZE + UINT8_SIZE) * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_RECORD << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8),
   array_length,
   APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'I', 'd','\0',
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT16 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_LAST_FIELD_FLAG | APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'V', 'a', 'l', 'u', 'e', '\0',
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_FLOW_CTRL | (APX_VM_VARIANT_ARRAY_NEXT << APX_VM_INST_VARIANT_SHIFT),
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_dynamic_array_of_records(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"RecordPort\"{\"Id\"S\"Value\"C}[10*]:={}\n";
   uint8_t const array_length = 10u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_HEADER_FLAG_DYNAMIC_DATA | APX_VM_VARIANT_UINT8,
   UINT8_SIZE + (UINT16_SIZE + UINT8_SIZE) * array_length,
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_RECORD << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_DYN_ARRAY_FLAG | APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8),
   array_length,
   APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'I', 'd','\0',
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT16 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_LAST_FIELD_FLAG | APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'V', 'a', 'l', 'u', 'e', '\0',
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_FLOW_CTRL | (APX_VM_VARIANT_ARRAY_NEXT << APX_VM_INST_VARIANT_SHIFT),
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_compiler_pack_record_DYNU8_U16(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"RecordPort\"{\"First\"C[8*]\"Second\"S}:={{}, 0xFFFF}\n";
   uint8_t const array_length = 8u;
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   uint8_t const expected[] = { APX_VM_HEADER_FLAG_DYNAMIC_DATA | APX_VM_HEADER_PROG_TYPE_PACK | APX_VM_VARIANT_UINT8,
   UINT8_SIZE + UINT8_SIZE * array_length + UINT16_SIZE,
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_RECORD << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'F', 'i', 'r', 's', 't', '\0',
   APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
   APX_VM_DYN_ARRAY_FLAG | APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8),
   array_length,
   APX_VM_LAST_FIELD_FLAG | APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'S', 'e', 'c', 'o', 'n', 'd', '\0',
   APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT16 << APX_VM_INST_VARIANT_SHIFT),
   };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_PACK_PROGRAM, &error_code);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertTrue(tc, adt_bytearray_data_equals(program, &expected[0], (uint32_t)sizeof(expected)));

   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}