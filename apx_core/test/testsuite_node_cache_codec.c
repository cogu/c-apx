/*****************************************************************************
 * \file      testsuite_node_cache_codec.c
 * \author    Conny Gustafsson
 * \date      2026-11-21
 * \brief     Unit tests for Level 2 binary cache (.apxc) codec primitives
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/crypto.h"
#include "apx/node_cache_codec.h"
#include "apx/node_instance.h"
#include "apx/node_manager.h"
#include "apx/port_instance.h"
#include "pack.h"
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_writer_and_reader_primitives_roundtrip(CuTest *tc);
static void test_writer_patch_u32_le_and_bounds(CuTest *tc);
static void test_reader_bounds_and_seek(CuTest *tc);
static void test_reader_str16_rejects_empty_and_embedded_nul(CuTest *tc);
static void test_writer_take_buffer_and_empty_writes(CuTest *tc);
static void test_port_descriptor_roundtrip(CuTest *tc);
static void test_port_descriptor_rejects_malformed(CuTest *tc);
static void test_program_roundtrip_and_validation(CuTest *tc);
static void test_full_node_roundtrip_server_and_client_modes(CuTest *tc);
static void test_apx_1_2_and_asymmetric_nodes(CuTest *tc);
static void test_embedded_def_text_raw_and_zlib(CuTest *tc);
static void test_adversarial_corruption_and_version_checks(CuTest *tc);
static void test_forward_compat_unknown_section_and_malformed_directory(CuTest *tc);

static void assert_nodes_equivalent(
  CuTest *tc, apx_node_instance_t const *expected, apx_node_instance_t const *actual, bool expect_vm_programs);
static void resign_apxc_trailer(CuTest *tc, uint8_t *buf, size_t size);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_node_cache_codec(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_writer_and_reader_primitives_roundtrip);
  SUITE_ADD_TEST(suite, test_writer_patch_u32_le_and_bounds);
  SUITE_ADD_TEST(suite, test_reader_bounds_and_seek);
  SUITE_ADD_TEST(suite, test_reader_str16_rejects_empty_and_embedded_nul);
  SUITE_ADD_TEST(suite, test_writer_take_buffer_and_empty_writes);
  SUITE_ADD_TEST(suite, test_port_descriptor_roundtrip);
  SUITE_ADD_TEST(suite, test_port_descriptor_rejects_malformed);
  SUITE_ADD_TEST(suite, test_program_roundtrip_and_validation);
  SUITE_ADD_TEST(suite, test_full_node_roundtrip_server_and_client_modes);
  SUITE_ADD_TEST(suite, test_apx_1_2_and_asymmetric_nodes);
  SUITE_ADD_TEST(suite, test_embedded_def_text_raw_and_zlib);
  SUITE_ADD_TEST(suite, test_adversarial_corruption_and_version_checks);
  SUITE_ADD_TEST(suite, test_forward_compat_unknown_section_and_malformed_directory);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_writer_and_reader_primitives_roundtrip(CuTest *tc)
{
  apx_cache_writer_t writer;
  apx_cache_writer_create(&writer);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_reserve(&writer, 64u));

  uint8_t const payload[5] = {0x10u, 0x20u, 0x30u, 0x40u, 0x50u};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_u8(&writer, 0xABu));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_u16_le(&writer, 0x1234u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_u32_le(&writer, 0x89ABCDEFu));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_bytes(&writer, payload, sizeof(payload)));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_str16(&writer, "WheelSpeed"));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_str16(&writer, ""));

  uint8_t const *raw = apx_cache_writer_data(&writer);
  size_t raw_size = apx_cache_writer_size(&writer);
  CuAssertPtrNotNull(tc, raw);
  // Verify exact little-endian wire layout for u8, u16, u32
  CuAssertUIntEquals(tc, 0xABu, raw[0]);
  CuAssertUIntEquals(tc, 0x34u, raw[1]);
  CuAssertUIntEquals(tc, 0x12u, raw[2]);
  CuAssertUIntEquals(tc, 0xEFu, raw[3]);
  CuAssertUIntEquals(tc, 0xCDu, raw[4]);
  CuAssertUIntEquals(tc, 0xABu, raw[5]);
  CuAssertUIntEquals(tc, 0x89u, raw[6]);

  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, raw, raw_size);
  CuAssertUIntEquals(tc, (unsigned int)raw_size, (unsigned int)apx_cache_reader_remaining(&reader));

  uint8_t v8 = 0u;
  uint16_t v16 = 0u;
  uint32_t v32 = 0u;
  uint8_t out_bytes[5] = {0u};
  char *str1 = NULL;
  char *str2 = NULL;

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_u8(&reader, &v8));
  CuAssertUIntEquals(tc, 0xABu, v8);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_u16_le(&reader, &v16));
  CuAssertUIntEquals(tc, 0x1234u, v16);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_u32_le(&reader, &v32));
  CuAssertUIntEquals(tc, 0x89ABCDEFu, v32);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_bytes(&reader, out_bytes, sizeof(out_bytes)));
  CuAssertIntEquals(tc, 0, memcmp(payload, out_bytes, sizeof(payload)));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_str16(&reader, false, &str1));
  CuAssertStrEquals(tc, "WheelSpeed", str1);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_str16(&reader, true, &str2));
  CuAssertStrEquals(tc, "", str2);
  CuAssertUIntEquals(tc, 0u, (unsigned int)apx_cache_reader_remaining(&reader));

  free(str1);
  free(str2);
  apx_cache_writer_destroy(&writer);
}

static void test_writer_patch_u32_le_and_bounds(CuTest *tc)
{
  apx_cache_writer_t writer;
  apx_cache_writer_create(&writer);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_u32_le(&writer, 0u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_u16_le(&writer, 0x55AAu));

  // Valid patch at offset 0
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_patch_u32_le(&writer, 0u, 0x01020304u));
  // Out-of-bounds patch (offset + 4 > 6)
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_writer_patch_u32_le(&writer, 3u, 0xDEADBEEFu));
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_writer_patch_u32_le(&writer, SIZE_MAX, 0u));

  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, apx_cache_writer_data(&writer), apx_cache_writer_size(&writer));
  uint32_t patched = 0u;
  uint16_t tail = 0u;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_u32_le(&reader, &patched));
  CuAssertUIntEquals(tc, 0x01020304u, patched);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_u16_le(&reader, &tail));
  CuAssertUIntEquals(tc, 0x55AAu, tail);

  apx_cache_writer_destroy(&writer);
}

static void test_reader_bounds_and_seek(CuTest *tc)
{
  uint8_t const buf[6] = {0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u};
  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, buf, sizeof(buf));

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_seek(&reader, 4u));
  CuAssertUIntEquals(tc, 4u, (unsigned int)apx_cache_reader_pos(&reader));
  CuAssertUIntEquals(tc, 2u, (unsigned int)apx_cache_reader_remaining(&reader));

  // Reading u32 with only 2 bytes remaining fails without advancing cursor
  uint32_t v32 = 0u;
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_reader_read_u32_le(&reader, &v32));
  CuAssertUIntEquals(tc, 4u, (unsigned int)apx_cache_reader_pos(&reader));

  // Overflow-sized slice fails cleanly
  uint8_t const *slice = NULL;
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_reader_read_slice(&reader, SIZE_MAX, &slice));
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_reader_seek(&reader, 7u));

  // Exact slice read succeeds
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_slice(&reader, 2u, &slice));
  CuAssertPtrNotNull(tc, slice);
  CuAssertUIntEquals(tc, 0x05u, slice[0]);
  CuAssertUIntEquals(tc, 0x06u, slice[1]);
  CuAssertUIntEquals(tc, 0u, (unsigned int)apx_cache_reader_remaining(&reader));

  // Zero-length slice at EOF succeeds
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_slice(&reader, 0u, &slice));
  CuAssertTrue(tc, slice == NULL);

  // Reading past EOF fails
  uint8_t v8 = 0u;
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_reader_read_u8(&reader, &v8));
}

static void test_reader_str16_rejects_empty_and_embedded_nul(CuTest *tc)
{
  // Empty string rejected when allow_empty == false
  uint8_t const empty_buf[2] = {0x00u, 0x00u};
  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, empty_buf, sizeof(empty_buf));
  char *str = NULL;
  CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_cache_reader_read_str16(&reader, false, &str));
  CuAssertPtrEquals(tc, NULL, str);

  // Embedded NUL byte rejected
  uint8_t const nul_buf[7] = {0x05u, 0x00u, 'a', 'b', '\0', 'c', 'd'};
  apx_cache_reader_create(&reader, nul_buf, sizeof(nul_buf));
  CuAssertIntEquals(tc, APX_INVALID_NAME_ERROR, apx_cache_reader_read_str16(&reader, false, &str));
  CuAssertPtrEquals(tc, NULL, str);

  // Truncated string length > remaining bytes rejected
  uint8_t const trunc_buf[4] = {0x0Au, 0x00u, 'h', 'i'};
  apx_cache_reader_create(&reader, trunc_buf, sizeof(trunc_buf));
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_reader_read_str16(&reader, false, &str));
  CuAssertPtrEquals(tc, NULL, str);
}

static void test_writer_take_buffer_and_empty_writes(CuTest *tc)
{
  apx_cache_writer_t writer;
  apx_cache_writer_create(&writer);

  // Zero-byte write with NULL pointer is allowed
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_bytes(&writer, NULL, 0u));
  CuAssertUIntEquals(tc, 0u, (unsigned int)apx_cache_writer_size(&writer));

  uint8_t *taken = NULL;
  size_t taken_size = 99u;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_take_buffer(&writer, &taken, &taken_size));
  CuAssertPtrEquals(tc, NULL, taken);
  CuAssertUIntEquals(tc, 0u, (unsigned int)taken_size);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_u32_le(&writer, 0x11223344u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_take_buffer(&writer, &taken, &taken_size));
  CuAssertPtrNotNull(tc, taken);
  CuAssertUIntEquals(tc, 4u, (unsigned int)taken_size);
  CuAssertUIntEquals(tc, 0u, (unsigned int)apx_cache_writer_size(&writer));
  CuAssertUIntEquals(tc, 0x44u, taken[0]);
  CuAssertUIntEquals(tc, 0x11u, taken[3]);

  free(taken);
  apx_cache_writer_destroy(&writer);
}

static void test_port_descriptor_roundtrip(CuTest *tc)
{
  apx_port_instance_t src0;
  apx_port_instance_t src1;
  apx_port_instance_t src2;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_port_instance_create_from_cache(
      &src0, NULL, APX_PROVIDE_PORT, 0u, "WheelSpeed", "\"WheelSpeed\"S", 0u, 2u, 0u, 0u, false, NULL, NULL));
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_port_instance_create_from_cache(
      &src1, NULL, APX_PROVIDE_PORT, 1u, "DynData", "\"DynData\"C[*]", 2u, 17u, 0u, 0u, true, NULL, NULL));
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_port_instance_create_from_cache(
      &src2, NULL, APX_PROVIDE_PORT, 2u, "Events", "\"Events\"L", 19u, 41u, 4u, 10u, false, NULL, NULL));

  apx_cache_writer_t writer;
  apx_cache_writer_create(&writer);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_encode_port_descriptor(&writer, &src0));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_encode_port_descriptor(&writer, &src1));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_encode_port_descriptor(&writer, &src2));

  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, apx_cache_writer_data(&writer), apx_cache_writer_size(&writer));

  apx_port_instance_t dst0;
  apx_port_instance_t dst1;
  apx_port_instance_t dst2;
  uint32_t next_off = 0u;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 0u, next_off, &dst0, &next_off));
  CuAssertUIntEquals(tc, 2u, next_off);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 1u, next_off, &dst1, &next_off));
  CuAssertUIntEquals(tc, 19u, next_off);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 2u, next_off, &dst2, &next_off));
  CuAssertUIntEquals(tc, 60u, next_off);
  CuAssertUIntEquals(tc, 0u, (unsigned int)apx_cache_reader_remaining(&reader));

  bool dyn = false;
  CuAssertStrEquals(tc, "WheelSpeed", apx_port_instance_name(&dst0));
  CuAssertStrEquals(tc, "\"WheelSpeed\"S", apx_port_instance_get_port_signature(&dst0, &dyn));
  CuAssertFalse(tc, dyn);
  CuAssertUIntEquals(tc, 0u, apx_port_instance_data_offset(&dst0));
  CuAssertUIntEquals(tc, 2u, apx_port_instance_data_size(&dst0));

  CuAssertStrEquals(tc, "DynData", apx_port_instance_name(&dst1));
  CuAssertStrEquals(tc, "\"DynData\"C[*]", apx_port_instance_get_port_signature(&dst1, &dyn));
  CuAssertTrue(tc, dyn);
  CuAssertUIntEquals(tc, 2u, apx_port_instance_data_offset(&dst1));
  CuAssertUIntEquals(tc, 17u, apx_port_instance_data_size(&dst1));

  CuAssertStrEquals(tc, "Events", apx_port_instance_name(&dst2));
  CuAssertUIntEquals(tc, 19u, apx_port_instance_data_offset(&dst2));
  CuAssertUIntEquals(tc, 41u, apx_port_instance_data_size(&dst2));
  CuAssertUIntEquals(tc, 4u, apx_port_instance_element_size(&dst2));
  CuAssertUIntEquals(tc, 10u, apx_port_instance_queue_length(&dst2));

  apx_port_instance_destroy(&src0);
  apx_port_instance_destroy(&src1);
  apx_port_instance_destroy(&src2);
  apx_port_instance_destroy(&dst0);
  apx_port_instance_destroy(&dst1);
  apx_port_instance_destroy(&dst2);
  apx_cache_writer_destroy(&writer);
}

static void test_port_descriptor_rejects_malformed(CuTest *tc)
{
  apx_port_instance_t src;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_port_instance_create_from_cache(
      &src, NULL, APX_PROVIDE_PORT, 0u, "SigA", "\"SigA\"C", 0u, 1u, 0u, 0u, false, NULL, NULL));

  apx_cache_writer_t writer;
  apx_cache_writer_create(&writer);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_encode_port_descriptor(&writer, &src));
  size_t len = apx_cache_writer_size(&writer);
  uint8_t *buf = (uint8_t *)malloc(len);
  CuAssertPtrNotNull(tc, buf);

  apx_port_instance_t dst;
  apx_cache_reader_t reader;
  uint32_t next_off = 0u;

  // Non-contiguous offset
  memcpy(buf, apx_cache_writer_data(&writer), len);
  cutil_pack_le(buf + 0, 5u, 4u);
  apx_cache_reader_create(&reader, buf, len);
  CuAssertIntEquals(
    tc, APX_LENGTH_ERROR, apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 0u, 0u, &dst, &next_off));

  // Zero data_size
  memcpy(buf, apx_cache_writer_data(&writer), len);
  cutil_pack_le(buf + 4, 0u, 4u);
  apx_cache_reader_create(&reader, buf, len);
  CuAssertIntEquals(
    tc, APX_LENGTH_ERROR, apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 0u, 0u, &dst, &next_off));

  // queue_length > 0 with element_size == 0
  memcpy(buf, apx_cache_writer_data(&writer), len);
  cutil_pack_le(buf + 12, 4u, 4u);
  apx_cache_reader_create(&reader, buf, len);
  CuAssertIntEquals(
    tc, APX_LENGTH_ERROR, apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 0u, 0u, &dst, &next_off));

  // Unknown flag bit
  memcpy(buf, apx_cache_writer_data(&writer), len);
  buf[16] = 0x02u;
  apx_cache_reader_create(&reader, buf, len);
  CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR,
    apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 0u, 0u, &dst, &next_off));

  // Non-zero reserved byte
  memcpy(buf, apx_cache_writer_data(&writer), len);
  buf[17] = 0xFFu;
  apx_cache_reader_create(&reader, buf, len);
  CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR,
    apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 0u, 0u, &dst, &next_off));

  // Embedded NUL in name
  memcpy(buf, apx_cache_writer_data(&writer), len);
  buf[23] = '\0';
  apx_cache_reader_create(&reader, buf, len);
  CuAssertIntEquals(tc, APX_INVALID_NAME_ERROR,
    apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 0u, 0u, &dst, &next_off));

  // Truncated descriptor
  memcpy(buf, apx_cache_writer_data(&writer), len);
  apx_cache_reader_create(&reader, buf, len - 1u);
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR,
    apx_cache_decode_port_descriptor(&reader, NULL, APX_PROVIDE_PORT, 0u, 0u, &dst, &next_off));

  free(buf);
  apx_port_instance_destroy(&src);
  apx_cache_writer_destroy(&writer);
}

static void test_program_roundtrip_and_validation(CuTest *tc)
{
  apx_program_t *prog = APX_PROGRAM_NEW();
  CuAssertPtrNotNull(tc, prog);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_program_encode_header(prog, APX_PACK_PROGRAM, 2u, 0u, false));
  CuAssertIntEquals(tc, ADT_NO_ERROR,
    adt_bytearray_push(prog, apx_program_encode_instruction(APX_VM_OPCODE_PACK, APX_VM_VARIANT_UINT16, false)));

  apx_cache_writer_t writer;
  apx_cache_writer_create(&writer);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_encode_program(&writer, prog));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_encode_program(&writer, NULL));

  apx_cache_reader_t reader;
  apx_cache_reader_create(&reader, apx_cache_writer_data(&writer), apx_cache_writer_size(&writer));

  apx_program_t *decoded = NULL;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_decode_program(&reader, false, &decoded));
  CuAssertPtrNotNull(tc, decoded);
  CuAssertTrue(tc, adt_bytearray_equals(prog, decoded));

  // Empty program allowed when allow_empty == true
  apx_program_t *empty_prog = NULL;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_decode_program(&reader, true, &empty_prog));
  CuAssertPtrEquals(tc, NULL, empty_prog);

  // Empty program rejected when allow_empty == false
  uint8_t const zero_len[4] = {0u, 0u, 0u, 0u};
  apx_cache_reader_create(&reader, zero_len, sizeof(zero_len));
  CuAssertIntEquals(tc, APX_INVALID_PROGRAM_ERROR, apx_cache_decode_program(&reader, false, &empty_prog));

  // Corrupt VM program header rejected
  uint8_t const bad_vm[8] = {0x04u, 0x00u, 0x00u, 0x00u, 0xFFu, 0xFFu, 0x00u, 0x00u};
  apx_cache_reader_create(&reader, bad_vm, sizeof(bad_vm));
  CuAssertIntEquals(tc, APX_INVALID_PROGRAM_ERROR, apx_cache_decode_program(&reader, false, &empty_prog));
  CuAssertPtrEquals(tc, NULL, empty_prog);

  // Attach decoded program to port_instance via apx_port_instance_set_programs and verify cleanup
  apx_port_instance_t port;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_port_instance_create_from_cache(
      &port, NULL, APX_PROVIDE_PORT, 0u, "P0", "\"P0\"S", 0u, 2u, 0u, 0u, false, NULL, NULL));
  apx_port_instance_set_programs(&port, decoded, NULL);
  CuAssertPtrEquals(tc, decoded, (void *)apx_port_instance_pack_program(&port));

  apx_port_instance_destroy(&port);
  APX_PROGRAM_DELETE(prog);
  apx_cache_writer_destroy(&writer);
}

static void assert_nodes_equivalent(
  CuTest *tc, apx_node_instance_t const *expected, apx_node_instance_t const *actual, bool expect_vm_programs)
{
  CuAssertIntEquals(tc, apx_node_instance_get_major_version(expected), apx_node_instance_get_major_version(actual));
  CuAssertIntEquals(tc, apx_node_instance_get_minor_version(expected), apx_node_instance_get_minor_version(actual));
  CuAssertUIntEquals(
    tc, apx_node_instance_get_num_provide_ports(expected), apx_node_instance_get_num_provide_ports(actual));
  CuAssertUIntEquals(
    tc, apx_node_instance_get_num_require_ports(expected), apx_node_instance_get_num_require_ports(actual));
  CuAssertUIntEquals(tc, apx_node_instance_get_provide_port_init_data_size(expected),
    apx_node_instance_get_provide_port_init_data_size(actual));
  CuAssertUIntEquals(tc, apx_node_instance_get_require_port_init_data_size(expected),
    apx_node_instance_get_require_port_init_data_size(actual));

  apx_size_t prov_size = apx_node_instance_get_provide_port_init_data_size(expected);
  if (prov_size > 0u)
  {
    CuAssertIntEquals(tc, 0,
      memcmp(apx_node_instance_get_provide_port_init_data(expected),
        apx_node_instance_get_provide_port_init_data(actual), (size_t)prov_size));
  }
  apx_size_t req_size = apx_node_instance_get_require_port_init_data_size(expected);
  if (req_size > 0u)
  {
    CuAssertIntEquals(tc, 0,
      memcmp(apx_node_instance_get_require_port_init_data(expected),
        apx_node_instance_get_require_port_init_data(actual), (size_t)req_size));
  }

  for (apx_size_t i = 0u; i < apx_node_instance_get_num_provide_ports(expected); i++)
  {
    apx_port_instance_t *pe = apx_node_instance_get_provide_port(expected, (apx_port_id_t)i);
    apx_port_instance_t *pa = apx_node_instance_get_provide_port(actual, (apx_port_id_t)i);
    bool de = false;
    bool da = false;
    CuAssertStrEquals(tc, apx_port_instance_name(pe), apx_port_instance_name(pa));
    CuAssertStrEquals(tc, apx_port_instance_get_port_signature(pe, &de), apx_port_instance_get_port_signature(pa, &da));
    CuAssertIntEquals(tc, (int)de, (int)da);
    CuAssertUIntEquals(tc, apx_port_instance_data_offset(pe), apx_port_instance_data_offset(pa));
    CuAssertUIntEquals(tc, apx_port_instance_data_size(pe), apx_port_instance_data_size(pa));
    CuAssertUIntEquals(tc, apx_port_instance_element_size(pe), apx_port_instance_element_size(pa));
    CuAssertUIntEquals(tc, apx_port_instance_queue_length(pe), apx_port_instance_queue_length(pa));
    if (expect_vm_programs)
    {
      CuAssertPtrNotNull(tc, (void *)apx_port_instance_pack_program(pa));
      CuAssertTrue(tc, adt_bytearray_equals(apx_port_instance_pack_program(pe), apx_port_instance_pack_program(pa)));
    }
    else
    {
      CuAssertPtrEquals(tc, NULL, (void *)apx_port_instance_pack_program(pa));
    }
    CuAssertPtrEquals(tc, NULL, (void *)apx_port_instance_unpack_program(pa));
  }

  for (apx_size_t i = 0u; i < apx_node_instance_get_num_require_ports(expected); i++)
  {
    apx_port_instance_t *re = apx_node_instance_get_require_port(expected, (apx_port_id_t)i);
    apx_port_instance_t *ra = apx_node_instance_get_require_port(actual, (apx_port_id_t)i);
    bool de = false;
    bool da = false;
    CuAssertStrEquals(tc, apx_port_instance_name(re), apx_port_instance_name(ra));
    CuAssertStrEquals(tc, apx_port_instance_get_port_signature(re, &de), apx_port_instance_get_port_signature(ra, &da));
    CuAssertIntEquals(tc, (int)de, (int)da);
    CuAssertUIntEquals(tc, apx_port_instance_data_offset(re), apx_port_instance_data_offset(ra));
    CuAssertUIntEquals(tc, apx_port_instance_data_size(re), apx_port_instance_data_size(ra));
    CuAssertUIntEquals(tc, apx_port_instance_element_size(re), apx_port_instance_element_size(ra));
    CuAssertUIntEquals(tc, apx_port_instance_queue_length(re), apx_port_instance_queue_length(ra));
    if (expect_vm_programs)
    {
      CuAssertPtrNotNull(tc, (void *)apx_port_instance_pack_program(ra));
      CuAssertPtrNotNull(tc, (void *)apx_port_instance_unpack_program(ra));
      CuAssertTrue(tc, adt_bytearray_equals(apx_port_instance_pack_program(re), apx_port_instance_pack_program(ra)));
      CuAssertTrue(
        tc, adt_bytearray_equals(apx_port_instance_unpack_program(re), apx_port_instance_unpack_program(ra)));
    }
    else
    {
      CuAssertPtrEquals(tc, NULL, (void *)apx_port_instance_pack_program(ra));
      CuAssertPtrEquals(tc, NULL, (void *)apx_port_instance_unpack_program(ra));
    }
  }
}

static void resign_apxc_trailer(CuTest *tc, uint8_t *buf, size_t size)
{
  CuAssertTrue(tc, size >= (size_t)APXC_MIN_FILE_SIZE);
  uint8_t hash[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256(buf, size - RMF_SHA256_SIZE, hash));
  memcpy(buf + size - RMF_SHA256_SIZE, hash, RMF_SHA256_SIZE);
}

static void test_full_node_roundtrip_server_and_client_modes(CuTest *tc)
{
  char const *apx_text = "APX/1.3\n"
                         "N\"RichNode\"\n"
                         "T\"Mode_T\"C(0,3):VT(\"Off\",\"Init\",\"Run\",\"Fault\")\n"
                         "T\"Vec_T\"{\"x\"s\"y\"s\"z\"l}\n"
                         "P\"VehicleSpeed\"S:=65535\n"
                         "P\"Mode\"T[\"Mode_T\"]:=2\n"
                         "P\"FixedArr\"C[4]:={1,2,3,4}\n"
                         "P\"DynBuf\"C[16*]:={}\n"
                         "P\"Position\"T[\"Vec_T\"]:={-10,20,-300}\n"
                         "R\"Command\"L:=42\n"
                         "R\"RxDyn\"S[8*]:={}\n";
  size_t apx_len = strlen(apx_text);
  uint8_t sha256[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx_text, apx_len, sha256));

  apx_node_manager_t nm;
  apx_node_manager_create(&nm, APX_SERVER_MODE);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(&nm, apx_text));
  apx_node_instance_t *compiled = apx_node_manager_get_last_attached(&nm);
  CuAssertPtrNotNull(tc, compiled);

  uint8_t *encoded = NULL;
  size_t encoded_size = 0u;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_cache_encode_binary(compiled, sha256, (uint32_t)apx_len, NULL, false, true, &encoded, &encoded_size));
  CuAssertPtrNotNull(tc, encoded);
  CuAssertTrue(tc, encoded_size >= (size_t)APXC_MIN_FILE_SIZE);

  // Validate container without decoding
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_cache_validate_binary(encoded, encoded_size, "RichNode", sha256));

  // Decode in server mode (skip VM programs)
  apx_node_instance_t *restored_srv = apx_node_instance_new(APX_SERVER_MODE, "RichNode");
  CuAssertPtrNotNull(tc, restored_srv);
  uint8_t *embedded_text = NULL;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_cache_decode_binary(
      encoded, encoded_size, "RichNode", sha256, (uint32_t)apx_len, false, restored_srv, &embedded_text));
  CuAssertPtrEquals(tc, NULL, embedded_text);
  assert_nodes_equivalent(tc, compiled, restored_srv, false);

  // Finalize derived structures (node_data, byte_port_map, connector_table)
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_instance_init_empty_node_data(restored_srv));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_instance_finalize_node_data(restored_srv));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_instance_create_byte_port_map(restored_srv));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_instance_build_connector_table(restored_srv));
  CuAssertTrue(tc, apx_node_instance_has_provide_port_count_data(restored_srv));
  CuAssertTrue(tc, apx_node_instance_has_require_port_count_data(restored_srv));
  for (apx_size_t off = 0u; off < apx_node_instance_get_provide_port_init_data_size(compiled); off++)
  {
    CuAssertUIntEquals(tc, apx_node_instance_lookup_provide_port_id(compiled, off),
      apx_node_instance_lookup_provide_port_id(restored_srv, off));
  }

  // Decode with VM programs enabled (future client mode reuse)
  apx_node_instance_t *restored_cli = apx_node_instance_new(APX_CLIENT_MODE, "RichNode");
  CuAssertPtrNotNull(tc, restored_cli);
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_cache_decode_binary(
      encoded, encoded_size, "RichNode", sha256, (uint32_t)apx_len, true, restored_cli, NULL));
  assert_nodes_equivalent(tc, compiled, restored_cli, true);

  apx_node_instance_delete(restored_cli);
  apx_node_instance_delete(restored_srv);
  free(encoded);
  apx_node_manager_destroy(&nm);
}

static void test_apx_1_2_and_asymmetric_nodes(CuTest *tc)
{
  char const *cases[4] = {
    "APX/1.2\nN\"LegacyNode\"\nP\"A\"C:=7\nR\"B\"S:=9\n",
    "APX/1.3\nN\"ProvideOnly\"\nP\"Out1\"L:=123456\n",
    "APX/1.3\nN\"RequireOnly\"\nR\"In1\"S:=99\n",
    "APX/1.3\nN\"EmptyNode\"\n",
  };
  char const *names[4] = {"LegacyNode", "ProvideOnly", "RequireOnly", "EmptyNode"};

  for (size_t idx = 0u; idx < 4u; idx++)
  {
    size_t len = strlen(cases[idx]);
    uint8_t sha256[RMF_SHA256_SIZE];
    CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)cases[idx], len, sha256));

    apx_node_manager_t nm;
    apx_node_manager_create(&nm, APX_SERVER_MODE);
    CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(&nm, cases[idx]));
    apx_node_instance_t *compiled = apx_node_manager_get_last_attached(&nm);
    CuAssertPtrNotNull(tc, compiled);

    uint8_t *encoded = NULL;
    size_t encoded_size = 0u;
    CuAssertIntEquals(tc, APX_NO_ERROR,
      apx_node_cache_encode_binary(compiled, sha256, (uint32_t)len, NULL, false, true, &encoded, &encoded_size));

    apx_node_instance_t *restored = apx_node_instance_new(APX_SERVER_MODE, names[idx]);
    CuAssertIntEquals(tc, APX_NO_ERROR,
      apx_node_cache_decode_binary(encoded, encoded_size, names[idx], sha256, (uint32_t)len, false, restored, NULL));
    assert_nodes_equivalent(tc, compiled, restored, false);
    if (idx == 0u)
    {
      CuAssertFalse(tc, apx_node_instance_has_provide_port_count_data(restored));
      CuAssertFalse(tc, apx_node_instance_has_require_port_count_data(restored));
    }

    apx_node_instance_delete(restored);
    free(encoded);
    apx_node_manager_destroy(&nm);
  }
}

static void test_embedded_def_text_raw_and_zlib(CuTest *tc)
{
  char const *apx_text = "APX/1.3\n"
                         "N\"PrewarmNode\"\n"
                         "P\"Sig0\"S:=100\n"
                         "P\"Sig1\"S:=200\n"
                         "P\"Sig2\"S:=300\n";
  size_t apx_len = strlen(apx_text);
  uint8_t sha256[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx_text, apx_len, sha256));

  apx_node_manager_t nm;
  apx_node_manager_create(&nm, APX_SERVER_MODE);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(&nm, apx_text));
  apx_node_instance_t *compiled = apx_node_manager_get_last_attached(&nm);
  CuAssertPtrNotNull(tc, compiled);

  for (int mode = 0; mode < 2; mode++)
  {
    bool compress = (mode == 1);
    uint8_t *encoded = NULL;
    size_t encoded_size = 0u;
    CuAssertIntEquals(tc, APX_NO_ERROR,
      apx_node_cache_encode_binary(
        compiled, sha256, (uint32_t)apx_len, (uint8_t const *)apx_text, compress, true, &encoded, &encoded_size));

    apx_node_instance_t *restored = apx_node_instance_new(APX_SERVER_MODE, "PrewarmNode");
    uint8_t *embedded_text = NULL;
    CuAssertIntEquals(tc, APX_NO_ERROR,
      apx_node_cache_decode_binary(
        encoded, encoded_size, "PrewarmNode", sha256, (uint32_t)apx_len, false, restored, &embedded_text));
    CuAssertPtrNotNull(tc, embedded_text);
    CuAssertStrEquals(tc, apx_text, (char const *)embedded_text);

    free(embedded_text);
    apx_node_instance_delete(restored);
    free(encoded);
  }
  apx_node_manager_destroy(&nm);
}

static void test_adversarial_corruption_and_version_checks(CuTest *tc)
{
  char const *apx_text = "APX/1.3\nN\"GuardNode\"\nP\"Out\"S:=5\nR\"In\"C:=3\n";
  size_t apx_len = strlen(apx_text);
  uint8_t sha256[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx_text, apx_len, sha256));

  apx_node_manager_t nm;
  apx_node_manager_create(&nm, APX_SERVER_MODE);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(&nm, apx_text));
  apx_node_instance_t *compiled = apx_node_manager_get_last_attached(&nm);

  uint8_t *good = NULL;
  size_t good_size = 0u;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_cache_encode_binary(compiled, sha256, (uint32_t)apx_len, NULL, false, true, &good, &good_size));

  uint8_t *mut = (uint8_t *)malloc(good_size);
  CuAssertPtrNotNull(tc, mut);
  apx_node_instance_t *dst = apx_node_instance_new(APX_SERVER_MODE, "GuardNode");

  // Truncated below minimum size and by 1 byte
  CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_node_cache_validate_binary(good, 16u, "GuardNode", sha256));
  CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_node_cache_validate_binary(good, good_size - 1u, "GuardNode", sha256));

  // Bad magic
  memcpy(mut, good, good_size);
  mut[0] = 'X';
  CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR, apx_node_cache_validate_binary(mut, good_size, "GuardNode", sha256));

  // Version / ABI mismatches return APX_VERSION_ERROR
  memcpy(mut, good, good_size);
  mut[4] = APXC_FORMAT_MAJOR + 1u;
  CuAssertIntEquals(tc, APX_VERSION_ERROR, apx_node_cache_validate_binary(mut, good_size, "GuardNode", sha256));

  memcpy(mut, good, good_size);
  mut[6] = APXC_VM_MAJOR + 1u;
  CuAssertIntEquals(tc, APX_VERSION_ERROR, apx_node_cache_validate_binary(mut, good_size, "GuardNode", sha256));

  memcpy(mut, good, good_size);
  cutil_pack_le(mut + 8, APXC_COMPILER_ABI_VERSION + 1u, 2u);
  CuAssertIntEquals(tc, APX_VERSION_ERROR, apx_node_cache_validate_binary(mut, good_size, "GuardNode", sha256));

  // Bit flip in payload and in trailer caught by SHA-256
  memcpy(mut, good, good_size);
  mut[good_size - RMF_SHA256_SIZE - 1u] ^= 0x01u;
  CuAssertIntEquals(tc, APX_CHECKSUM_ERROR, apx_node_cache_validate_binary(mut, good_size, "GuardNode", sha256));

  memcpy(mut, good, good_size);
  mut[good_size - 1u] ^= 0x80u;
  CuAssertIntEquals(tc, APX_CHECKSUM_ERROR, apx_node_cache_validate_binary(mut, good_size, "GuardNode", sha256));

  // Identity mismatches (name, sha256, logical_def_size)
  CuAssertIntEquals(tc, APX_INVALID_NAME_ERROR, apx_node_cache_validate_binary(good, good_size, "OtherNode", sha256));
  uint8_t bad_sha[RMF_SHA256_SIZE];
  memcpy(bad_sha, sha256, RMF_SHA256_SIZE);
  bad_sha[0] ^= 0xFFu;
  CuAssertIntEquals(tc, APX_CHECKSUM_ERROR, apx_node_cache_validate_binary(good, good_size, "GuardNode", bad_sha));
  CuAssertIntEquals(tc, APX_LENGTH_ERROR,
    apx_node_cache_decode_binary(good, good_size, "GuardNode", sha256, (uint32_t)apx_len + 1u, false, dst, NULL));

  // Corrupt port descriptor inside re-signed file -> verify decode cleans up dst with zero leaks
  memcpy(mut, good, good_size);
  // First section payload starts at header(72) + name(9) + dir(6 * 12 = 72) = 153; corrupt data_size at +4 to 0
  size_t sec0_off = (size_t)cutil_unpack_le(mut + APXC_HEADER_SIZE + 9u + 4u, 4u);
  cutil_pack_le(mut + sec0_off + 4u, 0u, 4u);
  resign_apxc_trailer(tc, mut, good_size);
  CuAssertIntEquals(tc, APX_LENGTH_ERROR,
    apx_node_cache_decode_binary(mut, good_size, "GuardNode", sha256, (uint32_t)apx_len, false, dst, NULL));
  CuAssertUIntEquals(tc, 0u, apx_node_instance_get_num_provide_ports(dst));
  CuAssertUIntEquals(tc, 0u, apx_node_instance_get_num_require_ports(dst));
  CuAssertPtrEquals(tc, NULL, (void *)apx_node_instance_get_provide_port_init_data(dst));

  apx_node_instance_delete(dst);
  free(mut);
  free(good);
  apx_node_manager_destroy(&nm);
}

static void test_forward_compat_unknown_section_and_malformed_directory(CuTest *tc)
{
  // Start from an EmptyNode (0 ports, 0 sections) and append an unknown optional section 0x0009
  char const *apx_text = "APX/1.3\nN\"ExtNode\"\n";
  size_t apx_len = strlen(apx_text);
  uint8_t sha256[RMF_SHA256_SIZE];
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_crypto_sha256((uint8_t const *)apx_text, apx_len, sha256));

  apx_node_manager_t nm;
  apx_node_manager_create(&nm, APX_SERVER_MODE);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_manager_build_node(&nm, apx_text));
  apx_node_instance_t *compiled = apx_node_manager_get_last_attached(&nm);

  uint8_t *base = NULL;
  size_t base_size = 0u;
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_cache_encode_binary(compiled, sha256, (uint32_t)apx_len, NULL, false, false, &base, &base_size));

  // Build extended buffer with 1 unknown section (type 0x0009, 4 payload bytes)
  size_t name_end = APXC_HEADER_SIZE + strlen("ExtNode");
  size_t ext_size = name_end + APXC_SECTION_ENTRY_SIZE + 4u + RMF_SHA256_SIZE;
  uint8_t *ext = (uint8_t *)calloc(1u, ext_size);
  CuAssertPtrNotNull(tc, ext);
  memcpy(ext, base, name_end);
  cutil_pack_le(ext + 12u, 1u, 2u); // section_count = 1
  cutil_pack_le(ext + 16u, (uint32_t)ext_size, 4u); // total_file_size
  uint8_t *dir = ext + name_end;
  cutil_pack_le(dir + 0u, 0x0009u, 2u); // unknown optional section type
  cutil_pack_le(dir + 2u, 0u, 2u);
  cutil_pack_le(dir + 4u, (uint32_t)(name_end + APXC_SECTION_ENTRY_SIZE), 4u);
  cutil_pack_le(dir + 8u, 4u, 4u);
  resign_apxc_trailer(tc, ext, ext_size);

  apx_node_instance_t *dst = apx_node_instance_new(APX_SERVER_MODE, "ExtNode");
  CuAssertIntEquals(tc, APX_NO_ERROR,
    apx_node_cache_decode_binary(ext, ext_size, "ExtNode", sha256, (uint32_t)apx_len, false, dst, NULL));
  apx_node_instance_delete(dst);

  // Out-of-bounds section size (integer overflow check)
  cutil_pack_le(dir + 8u, UINT32_MAX, 4u);
  resign_apxc_trailer(tc, ext, ext_size);
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_node_cache_validate_binary(ext, ext_size, "ExtNode", sha256));

  // Section offset pointing inside header/directory
  cutil_pack_le(dir + 4u, 0u, 4u);
  cutil_pack_le(dir + 8u, 4u, 4u);
  resign_apxc_trailer(tc, ext, ext_size);
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_node_cache_validate_binary(ext, ext_size, "ExtNode", sha256));

  free(ext);
  free(base);
  apx_node_manager_destroy(&nm);
}
