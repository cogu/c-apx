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
