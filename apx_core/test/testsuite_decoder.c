//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include "CuTest.h"
#include "apx/decoder.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_program_end_on_empty_program(CuTest* tc);
static void test_unpack_uint8(CuTest* tc);
static void test_pack_uint8(CuTest* tc);
static void test_unpack_uint16(CuTest* tc);
static void test_pack_uint16(CuTest* tc);
static void test_unpack_uint32(CuTest* tc);
static void test_pack_uint32(CuTest* tc);
static void test_unpack_uint64(CuTest* tc);
static void test_pack_uint64(CuTest* tc);
static void test_unpack_int8(CuTest* tc);
static void test_pack_int8(CuTest* tc);
static void test_unpack_int16(CuTest* tc);
static void test_pack_int16(CuTest* tc);
static void test_unpack_int32(CuTest* tc);
static void test_pack_int32(CuTest* tc);
static void test_unpack_int64(CuTest* tc);
static void test_pack_int64(CuTest* tc);
static void test_pack_bool(CuTest* tc);
static void test_pack_byte(CuTest* tc);
static void test_pack_char(CuTest* tc);
static void test_pack_char8(CuTest* tc);
static void test_pack_uint8_array_with_uint8_size(CuTest* tc);
static void test_pack_uint8_array_with_uint16_size(CuTest* tc);
static void test_pack_uint8_array_with_uint32_size(CuTest* tc);
static void test_pack_dynamic_uint8_array_with_uint8_size(CuTest* tc);
static void test_range_check_uint8(CuTest* tc);
static void test_range_check_uint16(CuTest* tc);
static void test_range_check_uint32(CuTest* tc);
static void test_range_check_uint64(CuTest* tc);
static void test_range_check_int8(CuTest* tc);
static void test_range_check_int16(CuTest* tc);
static void test_range_check_int32(CuTest* tc);
static void test_range_check_int64(CuTest* tc);
static void test_pack_record(CuTest* tc);
static void test_record_select(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_decoder(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_program_end_on_empty_program);
   SUITE_ADD_TEST(suite, test_unpack_uint8);
   SUITE_ADD_TEST(suite, test_pack_uint8);
   SUITE_ADD_TEST(suite, test_unpack_uint16);
   SUITE_ADD_TEST(suite, test_pack_uint16);
   SUITE_ADD_TEST(suite, test_unpack_uint32);
   SUITE_ADD_TEST(suite, test_pack_uint32);
   SUITE_ADD_TEST(suite, test_unpack_uint64);
   SUITE_ADD_TEST(suite, test_pack_uint64);
   SUITE_ADD_TEST(suite, test_unpack_int8);
   SUITE_ADD_TEST(suite, test_pack_int8);
   SUITE_ADD_TEST(suite, test_unpack_int16);
   SUITE_ADD_TEST(suite, test_pack_int16);
   SUITE_ADD_TEST(suite, test_unpack_int32);
   SUITE_ADD_TEST(suite, test_pack_int32);
   SUITE_ADD_TEST(suite, test_unpack_int64);
   SUITE_ADD_TEST(suite, test_pack_int64);
   SUITE_ADD_TEST(suite, test_pack_bool);
   SUITE_ADD_TEST(suite, test_pack_byte);
   SUITE_ADD_TEST(suite, test_pack_char);
   SUITE_ADD_TEST(suite, test_pack_char8);
   SUITE_ADD_TEST(suite, test_pack_uint8_array_with_uint8_size);
   SUITE_ADD_TEST(suite, test_pack_uint8_array_with_uint16_size);
   SUITE_ADD_TEST(suite, test_pack_uint8_array_with_uint32_size);
   SUITE_ADD_TEST(suite, test_pack_dynamic_uint8_array_with_uint8_size);
   SUITE_ADD_TEST(suite, test_range_check_uint8);
   SUITE_ADD_TEST(suite, test_range_check_uint16);
   SUITE_ADD_TEST(suite, test_range_check_uint32);
   SUITE_ADD_TEST(suite, test_range_check_uint64);
   SUITE_ADD_TEST(suite, test_range_check_int8);
   SUITE_ADD_TEST(suite, test_range_check_int16);
   SUITE_ADD_TEST(suite, test_range_check_int32);
   SUITE_ADD_TEST(suite, test_range_check_int64);
   SUITE_ADD_TEST(suite, test_pack_record);
   SUITE_ADD_TEST(suite, test_record_select);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_program_end_on_empty_program(CuTest* tc)
{
   uint8_t program[1] = { 0 };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, 0u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PROGRAM_END, operation_type);
   apx_vm_decoder_destroy(&decoder);
}

static void test_unpack_uint8(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_UNPACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = {APX_TYPE_CODE_NONE, 0u, false};
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT8, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_uint8(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t) sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT8, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_unpack_uint16(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_UNPACK | (APX_VM_VARIANT_UINT16 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT16, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_uint16(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT16 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT16, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_unpack_uint32(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_UNPACK | (APX_VM_VARIANT_UINT32 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT32, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_uint32(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT32 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT32, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_unpack_uint64(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_UNPACK | (APX_VM_VARIANT_UINT64 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT64, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_uint64(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT64 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT64, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_unpack_int8(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_UNPACK | (APX_VM_VARIANT_INT8 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_INT8, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_int8(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT8 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_INT8, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_unpack_int16(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_UNPACK | (APX_VM_VARIANT_INT16 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_INT16, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_int16(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT16 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_INT16, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_unpack_int32(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_UNPACK | (APX_VM_VARIANT_INT32 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_INT32, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_int32(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT32 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_INT32, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_unpack_int64(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_UNPACK | (APX_VM_VARIANT_INT64 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_UNPACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_INT64, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_int64(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_INT64 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_INT64, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_bool(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_BOOL << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_BOOL, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_byte(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_BYTE << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_BYTE, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_char(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_CHAR << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_CHAR, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_char8(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_CHAR8 << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_CHAR8, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_uint8_array_with_uint8_size(CuTest* tc)
{
   uint8_t program[3] = { APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
      APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
      0x12u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT8, info.type_code);
   CuAssertUIntEquals(tc, 0x12u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_uint8_array_with_uint16_size(CuTest* tc)
{
   uint8_t program[4] = { APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
      APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U16 << APX_VM_INST_VARIANT_SHIFT),
      0x34u, 0x12u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT8, info.type_code);
   CuAssertUIntEquals(tc, 0x1234u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_uint8_array_with_uint32_size(CuTest* tc)
{
   uint8_t program[6] = { APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
      APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U32 << APX_VM_INST_VARIANT_SHIFT),
      0x78u, 0x56u, 0x34u, 0x12u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT8, info.type_code);
   CuAssertUIntEquals(tc, 0x12345678u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_dynamic_uint8_array_with_uint8_size(CuTest* tc)
{
   uint8_t program[3] = { APX_VM_ARRAY_FLAG | APX_VM_OPCODE_PACK | (APX_VM_VARIANT_UINT8 << APX_VM_INST_VARIANT_SHIFT),
      APX_VM_DYN_ARRAY_FLAG | APX_VM_OPCODE_DATA_SIZE | (APX_VM_VARIANT_ARRAY_SIZE_U8 << APX_VM_INST_VARIANT_SHIFT),
      0x12u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_UINT8, info.type_code);
   CuAssertUIntEquals(tc, 0x12u, info.array_length);
   CuAssertTrue(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_range_check_uint8(CuTest* tc)
{
   uint8_t program[3] = {  APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_U8 << APX_VM_INST_VARIANT_SHIFT),
   0u,7u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_rangeCheckUInt32OperationInfo_t info = { 0u, 0u };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_UINT32, operation_type);
   apx_vm_decoder_range_check_info_uint32(&decoder, &info);
   CuAssertUIntEquals(tc, 0u, info.lower_limit);
   CuAssertUIntEquals(tc, 7u, info.upper_limit);
   apx_vm_decoder_destroy(&decoder);
}

static void test_range_check_uint16(CuTest* tc)
{
   uint8_t program[5] = { APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_U16 << APX_VM_INST_VARIANT_SHIFT),
   0u, 0u, 0xE7u, 0x03u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_rangeCheckUInt32OperationInfo_t info = { 0u, 0u };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_UINT32, operation_type);
   apx_vm_decoder_range_check_info_uint32(&decoder, &info);
   CuAssertUIntEquals(tc, 0u, info.lower_limit);
   CuAssertUIntEquals(tc, 999u, info.upper_limit);
   apx_vm_decoder_destroy(&decoder);
}

static void test_range_check_uint32(CuTest* tc)
{
   uint8_t program[9] = { APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_U32 << APX_VM_INST_VARIANT_SHIFT),
      0u, 0u, 0, 0, 0x9Fu, 0x86, 0x01, 0x00
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_rangeCheckUInt32OperationInfo_t info = { 0u, 0u };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_UINT32, operation_type);
   apx_vm_decoder_range_check_info_uint32(&decoder, &info);
   CuAssertUIntEquals(tc, 0u, info.lower_limit);
   CuAssertUIntEquals(tc, 99999u, info.upper_limit);
   apx_vm_decoder_destroy(&decoder);
}

static void test_range_check_uint64(CuTest* tc)
{
   uint8_t program[17] = { APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_U64 << APX_VM_INST_VARIANT_SHIFT),
   0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0x9Fu, 0x86u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_rangeCheckUInt64OperationInfo_t info = { 0u, 0u };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_UINT64, operation_type);
   apx_vm_decoder_range_check_info_uint64(&decoder, &info);
   CuAssertULIntEquals(tc, 0u, info.lower_limit);
   CuAssertULIntEquals(tc, 99999u, info.upper_limit);
   apx_vm_decoder_destroy(&decoder);
}

static void test_range_check_int8(CuTest* tc)
{
   uint8_t program[3] = { APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_S8 << APX_VM_INST_VARIANT_SHIFT),
   ((uint8_t)-10), 10u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_rangeCheckInt32OperationInfo_t info = { 0, 0 };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_INT32, operation_type);
   apx_vm_decoder_range_check_info_int32(&decoder, &info);
   CuAssertIntEquals(tc, -10, info.lower_limit);
   CuAssertIntEquals(tc, 10, info.upper_limit);
   apx_vm_decoder_destroy(&decoder);
}

static void test_range_check_int16(CuTest* tc)
{
   uint8_t program[5] = { APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_S16 << APX_VM_INST_VARIANT_SHIFT),
   0xF0u, 0xD8u, 0x10u, 0x27u
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_rangeCheckInt32OperationInfo_t info = { 0, 0 };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_INT32, operation_type);
   apx_vm_decoder_range_check_info_int32(&decoder, &info);
   CuAssertIntEquals(tc, -10000, info.lower_limit);
   CuAssertIntEquals(tc, 10000, info.upper_limit);
   apx_vm_decoder_destroy(&decoder);
}

static void test_range_check_int32(CuTest* tc)
{
   uint8_t program[9] = { APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_S32 << APX_VM_INST_VARIANT_SHIFT),
    0xF0, 0xD8, 0xFF, 0xFF, 0x10, 0x27, 0x00, 0x00
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_rangeCheckInt32OperationInfo_t info = { 0, 0 };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_INT32, operation_type);
   apx_vm_decoder_range_check_info_int32(&decoder, &info);
   CuAssertIntEquals(tc, -10000, info.lower_limit);
   CuAssertIntEquals(tc, 10000, info.upper_limit);
   apx_vm_decoder_destroy(&decoder);
}

static void test_range_check_int64(CuTest* tc)
{
   uint8_t program[17] = { APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_LIMIT_CHECK_S64 << APX_VM_INST_VARIANT_SHIFT),
     0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
   };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_rangeCheckInt64OperationInfo_t info = { 0, 0 };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RANGE_CHECK_INT64, operation_type);
   apx_vm_decoder_range_check_info_int64(&decoder, &info);
   CuAssertLIntEquals(tc, -1, info.lower_limit);
   CuAssertLIntEquals(tc, 1, info.upper_limit);
   apx_vm_decoder_destroy(&decoder);
}

static void test_pack_record(CuTest* tc)
{
   uint8_t program[1] = { APX_VM_OPCODE_PACK | (APX_VM_VARIANT_RECORD << APX_VM_INST_VARIANT_SHIFT) };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_packUnpackOperationInfo_t info = { APX_TYPE_CODE_NONE, 0u, false };
   apx_vm_decoder_create(&decoder);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PACK, operation_type);
   apx_vm_decoder_get_pack_unpack_info(&decoder, &info);
   CuAssertIntEquals(tc, APX_TYPE_CODE_RECORD, info.type_code);
   CuAssertUIntEquals(tc, 0u, info.array_length);
   CuAssertFalse(tc, info.is_dynamic_array);
   apx_vm_decoder_destroy(&decoder);
}

static void test_record_select(CuTest* tc)
{
   uint8_t program[7] = { APX_VM_OPCODE_DATA_CTRL | (APX_VM_VARIANT_RECORD_SELECT << APX_VM_INST_VARIANT_SHIFT),
   'F', 'i', 'r', 's', 't', '\0' };
   apx_vm_decoder_t decoder;
   apx_operationType_t operation_type = APX_OPERATION_TYPE_ARRAY_NEXT;
   apx_vm_decoder_create(&decoder);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_select_program(&decoder, program, (uint32_t)sizeof(program)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_RECORD_SELECT, operation_type);
   CuAssertStrEquals(tc, "First", apx_vm_decoder_get_field_name(&decoder));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_decoder_parse_next_operation(&decoder, &operation_type));
   CuAssertIntEquals(tc, APX_OPERATION_TYPE_PROGRAM_END, operation_type);
   apx_vm_decoder_destroy(&decoder);
}
