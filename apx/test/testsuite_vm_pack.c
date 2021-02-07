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
static void test_apx_vm_pack_uint8(CuTest* tc);
static void test_apx_vm_pack_uint16(CuTest* tc);
static void test_apx_vm_pack_uint32(CuTest* tc);
static void test_apx_vm_pack_uint64(CuTest* tc);
static void test_apx_vm_pack_int8(CuTest* tc);
static void test_apx_vm_pack_int16(CuTest* tc);
static void test_apx_vm_pack_int32(CuTest* tc);
static void test_apx_vm_pack_int64(CuTest* tc);
static void test_apx_vm_pack_bool(CuTest* tc);
static void test_apx_vm_pack_byte(CuTest* tc);
static void test_apx_vm_pack_char_string(CuTest* tc);
static void test_apx_vm_pack_record_u16_u8(CuTest* tc);
static void test_apx_vm_pack_array_of_record_u16_u8(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_vm_pack(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_vm_pack_uint8);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_uint16);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_uint32);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_uint64);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_int8);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_int16);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_int32);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_int64);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_bool);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_byte);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_char_string);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_record_u16_u8);
   SUITE_ADD_TEST(suite, test_apx_vm_pack_array_of_record_u16_u8);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_vm_pack_uint8(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[UINT8_SIZE] = { 0xffu };

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

   dtl_sv_set_u32(sv, 0u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   dtl_sv_set_u32(sv, 255u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 255u, buf[0]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_uint16(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[UINT16_SIZE] = { 0xffu, 0xffu };

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

   dtl_sv_set_u32(sv, 0u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   CuAssertUIntEquals(tc, 0u, buf[1]);
   dtl_sv_set_u32(sv, 65535u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 255u, buf[0]);
   CuAssertUIntEquals(tc, 255u, buf[1]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_uint32(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[UINT32_SIZE] = { 0xffu, 0xffu, 0xffu, 0xffu };

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

   dtl_sv_set_u32(sv, 0u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   CuAssertUIntEquals(tc, 0u, buf[1]);
   CuAssertUIntEquals(tc, 0u, buf[2]);
   CuAssertUIntEquals(tc, 0u, buf[3]);
   dtl_sv_set_u32(sv, 0xffffffffu);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 255u, buf[0]);
   CuAssertUIntEquals(tc, 255u, buf[1]);
   CuAssertUIntEquals(tc, 255u, buf[2]);
   CuAssertUIntEquals(tc, 255u, buf[3]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_uint64(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[UINT64_SIZE] = { 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu };

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

   dtl_sv_set_u32(sv, 0u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   CuAssertUIntEquals(tc, 0u, buf[1]);
   CuAssertUIntEquals(tc, 0u, buf[2]);
   CuAssertUIntEquals(tc, 0u, buf[3]);
   CuAssertUIntEquals(tc, 0u, buf[4]);
   CuAssertUIntEquals(tc, 0u, buf[5]);
   CuAssertUIntEquals(tc, 0u, buf[6]);
   CuAssertUIntEquals(tc, 0u, buf[7]);
   dtl_sv_set_u64(sv, 0xffffffffffffffffull);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 255u, buf[0]);
   CuAssertUIntEquals(tc, 255u, buf[1]);
   CuAssertUIntEquals(tc, 255u, buf[2]);
   CuAssertUIntEquals(tc, 255u, buf[3]);
   CuAssertUIntEquals(tc, 255u, buf[4]);
   CuAssertUIntEquals(tc, 255u, buf[5]);
   CuAssertUIntEquals(tc, 255u, buf[6]);
   CuAssertUIntEquals(tc, 255u, buf[7]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_int8(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[INT8_SIZE] = { 0xffu };

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

   dtl_sv_set_i32(sv, 0u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   dtl_sv_set_i32(sv, 127u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 127u, buf[0]);
   dtl_sv_set_i32(sv, -128);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0x80u, buf[0]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_int16(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[INT16_SIZE] = { 0xffu, 0xffu };

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

   dtl_sv_set_i32(sv, 0u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   CuAssertUIntEquals(tc, 0u, buf[1]);
   dtl_sv_set_i32(sv, 32767);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0xffu, buf[0]);
   CuAssertUIntEquals(tc, 0x7fu, buf[1]);
   dtl_sv_set_i32(sv, -32768);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0x00u, buf[0]);
   CuAssertUIntEquals(tc, 0x80u, buf[1]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_int32(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[INT32_SIZE] = { 0xffu, 0xffu, 0xffu, 0xffu };

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

   dtl_sv_set_i32(sv, 0u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   CuAssertUIntEquals(tc, 0u, buf[1]);
   CuAssertUIntEquals(tc, 0u, buf[2]);
   CuAssertUIntEquals(tc, 0u, buf[3]);
   dtl_sv_set_i32(sv, INT32_MAX);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0xffu, buf[0]);
   CuAssertUIntEquals(tc, 0xffu, buf[1]);
   CuAssertUIntEquals(tc, 0xffu, buf[2]);
   CuAssertUIntEquals(tc, 0x7fu, buf[3]);
   dtl_sv_set_i32(sv, INT32_MIN);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0x0u, buf[0]);
   CuAssertUIntEquals(tc, 0x0u, buf[1]);
   CuAssertUIntEquals(tc, 0x0u, buf[2]);
   CuAssertUIntEquals(tc, 0x80u, buf[3]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_int64(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[INT64_SIZE] = { 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu, 0xffu };

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

   dtl_sv_set_i32(sv, 0u);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   CuAssertUIntEquals(tc, 0u, buf[1]);
   CuAssertUIntEquals(tc, 0u, buf[2]);
   CuAssertUIntEquals(tc, 0u, buf[3]);
   CuAssertUIntEquals(tc, 0u, buf[4]);
   CuAssertUIntEquals(tc, 0u, buf[5]);
   CuAssertUIntEquals(tc, 0u, buf[6]);
   CuAssertUIntEquals(tc, 0u, buf[7]);
   dtl_sv_set_i64(sv, INT64_MAX);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0xffu, buf[0]);
   CuAssertUIntEquals(tc, 0xffu, buf[1]);
   CuAssertUIntEquals(tc, 0xffu, buf[2]);
   CuAssertUIntEquals(tc, 0xffu, buf[3]);
   CuAssertUIntEquals(tc, 0xffu, buf[4]);
   CuAssertUIntEquals(tc, 0xffu, buf[5]);
   CuAssertUIntEquals(tc, 0xffu, buf[6]);
   CuAssertUIntEquals(tc, 0x7fu, buf[7]);
   dtl_sv_set_i64(sv, INT64_MIN);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   CuAssertUIntEquals(tc, 0u, buf[1]);
   CuAssertUIntEquals(tc, 0u, buf[2]);
   CuAssertUIntEquals(tc, 0u, buf[3]);
   CuAssertUIntEquals(tc, 0u, buf[4]);
   CuAssertUIntEquals(tc, 0u, buf[5]);
   CuAssertUIntEquals(tc, 0u, buf[6]);
   CuAssertUIntEquals(tc, 0x80u, buf[7]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_bool(CuTest* tc)
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
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[UINT8_SIZE] = { 0xffu };

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

   dtl_sv_set_bool(sv, false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 0u, buf[0]);
   dtl_sv_set_bool(sv, true);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 1u, buf[0]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_apx_vm_pack_byte(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"B";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[UINT8_SIZE] = { 0xffu };
   uint8_t data[UINT8_SIZE] = { 0u };

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
   data[0] = 0x12;
   dtl_sv_set_bytearray_raw(sv, data, sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, data[0], buf[0]);
   data[0] = 0xaa;
   dtl_sv_set_bytearray_raw(sv, data, sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, data[0], buf[0]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_char_string(CuTest* tc)
{
   const char* apx_text =
      "APX/1.3\n"
      "N\"TestNode\"\n"
      "R\"TestPort\"a[4]";
   apx_parser_t parser;
   apx_istream_t stream;
   apx_node_t* node = NULL;
   apx_port_t* port = NULL;
   apx_compiler_t compiler;
   apx_error_t error_code = APX_NO_ERROR;
   apx_program_t* program;
   apx_vm_t* vm = apx_vm_new();
   dtl_sv_t* sv = dtl_sv_new();
   uint8_t buf[CHAR_SIZE*4];
   memset(buf, 0, sizeof(buf));
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
   dtl_sv_set_cstr(sv, "Test");
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)sv));
   CuAssertUIntEquals(tc, 'T', buf[0]);
   CuAssertUIntEquals(tc, 'e', buf[1]);
   CuAssertUIntEquals(tc, 's', buf[2]);
   CuAssertUIntEquals(tc, 't', buf[3]);

   apx_vm_delete(vm);
   dtl_dec_ref(sv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);

}

static void test_apx_vm_pack_record_u16_u8(CuTest* tc)
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
   dtl_hv_t* hv = dtl_hv_new();
   dtl_hv_set_cstr(hv, "First", (dtl_dv_t*)dtl_sv_make_u32(0x1234), false);
   dtl_hv_set_cstr(hv, "Second", (dtl_dv_t*)dtl_sv_make_u32(0x12), false);
   uint8_t buf[UINT16_SIZE + UINT8_SIZE];
   memset(buf, 0, sizeof(buf));
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

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)hv));
   CuAssertUIntEquals(tc, 0x34, buf[0]);
   CuAssertUIntEquals(tc, 0x12, buf[1]);
   CuAssertUIntEquals(tc, 0x12, buf[2]);

   apx_vm_delete(vm);
   dtl_dec_ref(hv);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}

static void test_apx_vm_pack_array_of_record_u16_u8(CuTest* tc)
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
   dtl_av_t* av = dtl_av_new();
   dtl_hv_t* hv1 = dtl_hv_new();
   dtl_hv_t* hv2 = dtl_hv_new();
   dtl_hv_set_cstr(hv1, "First", (dtl_dv_t*)dtl_sv_make_u32(0x1234), false);
   dtl_hv_set_cstr(hv1, "Second", (dtl_dv_t*)dtl_sv_make_u32(0x12), false);
   dtl_hv_set_cstr(hv2, "First", (dtl_dv_t*)dtl_sv_make_u32(0x1234), false);
   dtl_hv_set_cstr(hv2, "Second", (dtl_dv_t*)dtl_sv_make_u32(0x12), false);
   dtl_av_push(av, (dtl_dv_t*)hv1, false);
   dtl_av_push(av, (dtl_dv_t*)hv2, false);
   uint8_t buf[(UINT16_SIZE + UINT8_SIZE)*2];
   memset(buf, 0, sizeof(buf));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_select_program(vm, program));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_set_write_buffer(vm, buf, sizeof(buf)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_pack_value(vm, (dtl_dv_t*)av));
   CuAssertUIntEquals(tc, 0x34, buf[0]);
   CuAssertUIntEquals(tc, 0x12, buf[1]);
   CuAssertUIntEquals(tc, 0x12, buf[2]);
   CuAssertUIntEquals(tc, 0x34, buf[3]);
   CuAssertUIntEquals(tc, 0x12, buf[4]);
   CuAssertUIntEquals(tc, 0x12, buf[5]);

   apx_vm_delete(vm);
   dtl_dec_ref(av);
   APX_PROGRAM_DELETE(program);
   apx_compiler_destroy(&compiler);
   apx_node_delete(node);
   apx_parser_destroy(&parser);
   apx_istream_destroy(&stream);
}