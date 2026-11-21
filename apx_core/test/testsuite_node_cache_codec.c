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
#include "apx/node_cache_codec.h"
#include "apx/node_instance.h"
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
  CuAssertUIntEquals(tc, raw_size, apx_cache_reader_remaining(&reader));

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
  CuAssertUIntEquals(tc, 0u, apx_cache_reader_remaining(&reader));

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
  CuAssertUIntEquals(tc, 4u, apx_cache_reader_pos(&reader));
  CuAssertUIntEquals(tc, 2u, apx_cache_reader_remaining(&reader));

  // Reading u32 with only 2 bytes remaining fails without advancing cursor
  uint32_t v32 = 0u;
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_reader_read_u32_le(&reader, &v32));
  CuAssertUIntEquals(tc, 4u, apx_cache_reader_pos(&reader));

  // Overflow-sized slice fails cleanly
  uint8_t const *slice = NULL;
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_reader_read_slice(&reader, SIZE_MAX, &slice));
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_cache_reader_seek(&reader, 7u));

  // Exact slice read succeeds
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_reader_read_slice(&reader, 2u, &slice));
  CuAssertPtrNotNull(tc, slice);
  CuAssertUIntEquals(tc, 0x05u, slice[0]);
  CuAssertUIntEquals(tc, 0x06u, slice[1]);
  CuAssertUIntEquals(tc, 0u, apx_cache_reader_remaining(&reader));

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
  CuAssertUIntEquals(tc, 0u, apx_cache_writer_size(&writer));

  uint8_t *taken = NULL;
  size_t taken_size = 99u;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_take_buffer(&writer, &taken, &taken_size));
  CuAssertPtrEquals(tc, NULL, taken);
  CuAssertUIntEquals(tc, 0u, taken_size);

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_write_u32_le(&writer, 0x11223344u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_cache_writer_take_buffer(&writer, &taken, &taken_size));
  CuAssertPtrNotNull(tc, taken);
  CuAssertUIntEquals(tc, 4u, taken_size);
  CuAssertUIntEquals(tc, 0u, apx_cache_writer_size(&writer));
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
  CuAssertUIntEquals(tc, 0u, apx_cache_reader_remaining(&reader));

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
