//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/compiler.h"
#include "apx/parser.h"
#include "apx/vm.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_vm_unpack_uint8(CuTest* tc);
static void test_apx_vm_unpack_uint16(CuTest* tc);
static void test_apx_vm_unpack_uint32(CuTest* tc);
static void test_apx_vm_unpack_uint64(CuTest* tc);
static void test_apx_vm_unpack_int8(CuTest* tc);
static void test_apx_vm_unpack_int16(CuTest* tc);
static void test_apx_vm_unpack_int32(CuTest* tc);
static void test_apx_vm_unpack_int64(CuTest* tc);
static void test_apx_vm_unpack_uint8_with_range_check(CuTest* tc);
static void test_apx_vm_unpack_uint8_array(CuTest* tc);
static void test_apx_vm_unpack_uint8_array_with_range_check(CuTest* tc);
static void test_apx_vm_unpack_bool(CuTest* tc);
static void test_apx_vm_unpack_byte_array(CuTest* tc);
static void test_apx_vm_unpack_dynamic_byte_array(CuTest* tc);
static void test_apx_vm_unpack_char(CuTest* tc);
static void test_apx_vm_unpack_char_string(CuTest* tc);
static void test_apx_vm_unpack_char8_string(CuTest* tc);
static void test_apx_vm_unpack_record_u16_u8(CuTest* tc);
static void test_apx_vm_unpack_array_of_record_u16_u8(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_vm_unpack(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_vm_unpack_uint8);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_uint16);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_uint32);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_uint64);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_int8);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_int16);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_int32);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_int64);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_uint8_with_range_check);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_uint8_array);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_uint8_array_with_range_check);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_bool);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_byte_array);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_dynamic_byte_array);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_char);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_char_string);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_char8_string);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_record_u16_u8);
   SUITE_ADD_TEST(suite, test_apx_vm_unpack_array_of_record_u16_u8);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_vm_unpack_uint8(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"C";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT8_SIZE] = { 0x0u };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   buf[0] = 255;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 255u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_uint16(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"S";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT16_SIZE] = { 0x0u, 0x00 };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   buf[0] = buf[1] = 255;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, UINT16_MAX, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}


static void test_apx_vm_unpack_uint32(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"L";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT32_SIZE] = { 0x0u, 0x0u, 0x0u, 0x0u };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   buf[0] = buf[1] = buf[2] = buf[3] =  255;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, UINT32_MAX, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_uint64(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"Q";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT64_SIZE] = { 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertULIntEquals(tc, 0u, dtl_sv_to_u64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   memset(buf, 0xff, sizeof(buf));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertULIntEquals(tc, UINT64_MAX, dtl_sv_to_u64(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}


static void test_apx_vm_unpack_int8(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"c";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[INT8_SIZE] = { 0x7fu };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, INT8_MAX, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   buf[0] = 0x80;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, INT8_MIN, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_apx_vm_unpack_int16(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"s";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[INT16_SIZE] = { 0xffu, 0x7fu };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, INT16_MAX, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   memset(buf, 0, sizeof(buf));
   buf[1] = 0x80;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, INT16_MIN, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_int32(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"l";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[INT32_SIZE] = { 0xffu, 0xffu, 0xffu, 0x7fu };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, INT32_MAX, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   memset(buf, 0, sizeof(buf));
   buf[3] = 0x80;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, INT32_MIN, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_int64(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"q";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[INT64_SIZE] = { 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0x7fu };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertLIntEquals(tc, INT64_MAX, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   memset(buf, 0, sizeof(buf));
   buf[7] = 0x80;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertLIntEquals(tc, INT64_MIN, dtl_sv_to_i64(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_uint8_with_range_check(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"C(0,7)";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT8_SIZE] = { 0x0u };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   buf[0] = 7;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 7u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   buf[0] = 8;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_uint8_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"C[3]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_av_t* av = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   uint8_t buf[UINT8_SIZE * 3] = { 0x0u, 0x12u, 0xff };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&av));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type((dtl_dv_t*)av));
   child_sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)child_sv));
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)child_sv));
   CuAssertUIntEquals(tc, 0x12u, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)child_sv));
   CuAssertUIntEquals(tc, 0xffu, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_uint8_array_with_range_check(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"C(0,3)[3]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_av_t* av = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   uint8_t buf[UINT8_SIZE * 3 ] = { 0x3u, 0x3u, 0x3u };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&av));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type((dtl_dv_t*)av));
   CuAssertUIntEquals(tc, 3, dtl_av_length(av));
   child_sv = (dtl_sv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 3u, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 3u, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 3u, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);

   buf[2] = 4;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&av));

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_bool(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"b";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT8_SIZE] = { 0x0u };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, DTL_SV_BOOL, dtl_sv_type(sv));
   CuAssertFalse(tc, dtl_sv_to_bool(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   buf[0] = 1;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, DTL_SV_BOOL, dtl_sv_type(sv));
   CuAssertTrue(tc, dtl_sv_to_bool(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_byte_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"B[4]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   uint8_t buf[UINT8_SIZE * 4] = { 0x18u, 0x22u, 0x31u, 0x14u };
   apx_size_t array_length = 4u;
   adt_bytearray_t const *byte_array;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, DTL_SV_BYTEARRAY, dtl_sv_type(sv));
   byte_array = dtl_sv_get_bytearray(sv);
   CuAssertPtrNotNull(tc, byte_array);
   CuAssertIntEquals(tc, array_length, adt_bytearray_length(byte_array));
   CuAssertTrue(tc, adt_bytearray_data_equals(byte_array, buf, (uint32_t)sizeof(buf)));
   dtl_dec_ref(sv);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_dynamic_byte_array(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"B[10*]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   apx_size_t const array_length = 4u;
   uint8_t buf[UINT8_SIZE * 5] = { (uint8_t)array_length, 0x18u, 0x22u, 0x31u, 0x14u };
   adt_bytearray_t const* byte_array = NULL;

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, UINT8_SIZE * 5, (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, DTL_SV_BYTEARRAY, dtl_sv_type(sv));
   byte_array = dtl_sv_get_bytearray(sv);
   CuAssertPtrNotNull(tc, byte_array);
   CuAssertIntEquals(tc, array_length, adt_bytearray_length(byte_array));
   CuAssertTrue(tc, adt_bytearray_data_equals(byte_array, &buf[1], array_length));
   dtl_dec_ref(sv);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_char(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"a";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT8_SIZE] = { 'a' };

   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, DTL_SV_CHAR, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 'a', dtl_sv_to_char(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   buf[0] = 'z';
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, DTL_SV_CHAR, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 'z', dtl_sv_to_char(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_char_string(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"a[10]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT8_SIZE*10] = { 'H', 'e', 'l', 'l', 'o', '\0', '\0', '\0', '\0', '\0' };
   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, DTL_SV_STR, dtl_sv_type(sv));
   CuAssertStrEquals(tc, "Hello", dtl_sv_to_cstr(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_char8_string(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"A[10]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = NULL;
   bool ok = false;
   uint8_t buf[UINT8_SIZE * 10] = { 'H', 'e', 'l', 'l', 'o', '\0', '\0', '\0', '\0', '\0' };
   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&sv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)sv));
   CuAssertIntEquals(tc, DTL_SV_STR, dtl_sv_type(sv));
   CuAssertStrEquals(tc, "Hello", dtl_sv_to_cstr(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_record_u16_u8(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"{\"First\"S\"Second\"C}";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_hv_t* hv = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   uint8_t buf[UINT16_SIZE + UINT8_SIZE] = {0x34, 0x12, 0x12};
   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&hv));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, hv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type((dtl_dv_t*)hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(child_sv));
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(child_sv));
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(hv);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_unpack_array_of_record_u16_u8(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"{\"First\"S\"Second\"C}[2]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_av_t* av = NULL;
   dtl_hv_t* child_hv = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   uint8_t buf[(UINT16_SIZE + UINT8_SIZE) * 2] = { 0x34, 0x12, 0x12, 0x78, 0x56, 0x34 };
   apx_istream_create(&stream);
   apx_parser_create(&parser, &stream);
   CuAssertUIntEquals(tc, APX_NO_ERROR, apx_parser_parse_cstr(&parser, apx_text));
   node = apx_parser_take_last_node(&parser);
   CuAssertPtrNotNull(tc, node);
   port = apx_node_get_last_require_port(node);
   CuAssertPtrNotNull(tc, port);
   apx_compiler_create(&compiler);
   program = apx_compiler_compile_port(&compiler, port, APX_UNPACK_PROGRAM, &error_code);
   CuAssertPtrNotNull(tc, program);
   CuAssertIntEquals(tc, APX_NO_ERROR, error_code);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_read_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpack_value(vm, (dtl_dv_t**)&av));
   CuAssertUIntEquals(tc, (unsigned int)sizeof(buf), (unsigned int)apx_vm_get_bytes_read(vm));
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type((dtl_dv_t*)av));
   CuAssertIntEquals(tc, 2, dtl_av_length(av));
   child_hv = (dtl_hv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type((dtl_dv_t*)child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(child_sv));
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(child_sv));
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_hv = (dtl_hv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type((dtl_dv_t*)child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(child_sv));
   CuAssertUIntEquals(tc, 0x5678, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(child_sv));
   CuAssertUIntEquals(tc, 0x34, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(av);

   apx_vm_delete(vm);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}
