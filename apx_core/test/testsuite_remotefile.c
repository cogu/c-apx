/*****************************************************************************
 * \file      testsuite_remotefile.c
 * \author    Conny Gustafsson
 * \date      2017-02-20
 * \brief     Unit tests for remotefile
 *
 * Copyright (c) 2017-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/error.h"
#include "apx/remotefile.h"
#include "pack.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_low_address_encode(CuTest *tc);
static void test_low_address_decode(CuTest *tc);
static void test_high_address_encode(CuTest *tc);
static void test_high_address_decode(CuTest *tc);
static void test_encode_accept_header(CuTest *tc);
static void test_decode_accept_header(CuTest *tc);
static void test_encode_nack_cmd(CuTest *tc);
static void test_decode_nack_cmd(CuTest *tc);
static void test_close_file_cmd_encode(CuTest *tc);
static void test_stream_open_cmd_encode_decode(CuTest *tc);
static void test_stream_open_rsp_cmd_encode_decode(CuTest *tc);
static void test_stream_window_update_cmd_encode_decode(CuTest *tc);
static void test_stream_abort_cmd_encode_decode(CuTest *tc);
static void test_compression_type_conversion(CuTest *tc);
static void test_publish_compressed_file_cmd_encode_decode(CuTest *tc);
static void test_publish_signed_compressed_file_cmd_encode_decode(CuTest *tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite *testsuite_remotefile(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_low_address_encode);
  SUITE_ADD_TEST(suite, test_low_address_decode);
  SUITE_ADD_TEST(suite, test_high_address_encode);
  SUITE_ADD_TEST(suite, test_high_address_decode);
  SUITE_ADD_TEST(suite, test_encode_accept_header);
  SUITE_ADD_TEST(suite, test_decode_accept_header);
  SUITE_ADD_TEST(suite, test_encode_nack_cmd);
  SUITE_ADD_TEST(suite, test_decode_nack_cmd);
  SUITE_ADD_TEST(suite, test_close_file_cmd_encode);
  SUITE_ADD_TEST(suite, test_stream_open_cmd_encode_decode);
  SUITE_ADD_TEST(suite, test_stream_open_rsp_cmd_encode_decode);
  SUITE_ADD_TEST(suite, test_stream_window_update_cmd_encode_decode);
  SUITE_ADD_TEST(suite, test_stream_abort_cmd_encode_decode);
  SUITE_ADD_TEST(suite, test_compression_type_conversion);
  SUITE_ADD_TEST(suite, test_publish_compressed_file_cmd_encode_decode);
  SUITE_ADD_TEST(suite, test_publish_signed_compressed_file_cmd_encode_decode);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_low_address_encode(CuTest *tc)
{
  uint8_t buffer[UINT16_SIZE];
  CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_encode(buffer, sizeof(buffer), 0u, false));
  CuAssertUIntEquals(tc, 0x00, buffer[0]);
  CuAssertUIntEquals(tc, 0x00u, buffer[1]);
  CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_encode(buffer, sizeof(buffer), 0u, true));
  CuAssertUIntEquals(tc, 0x40u, buffer[0]);
  CuAssertUIntEquals(tc, 0x00u, buffer[1]);
  CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_LOW_ADDR_MAX, false));
  CuAssertUIntEquals(tc, 0x3Fu, buffer[0]);
  CuAssertUIntEquals(tc, 0xFFu, buffer[1]);
  CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_LOW_ADDR_MAX, true));
  CuAssertUIntEquals(tc, 0x7Fu, buffer[0]);
  CuAssertUIntEquals(tc, 0xFFu, buffer[1]);
}

static void test_low_address_decode(CuTest *tc)
{
  uint8_t buffer[UINT16_SIZE] = {0, 0};
  uint32_t address = 0u;
  bool more_bit = false;
  CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
  CuAssertUIntEquals(tc, 0u, address);
  CuAssertFalse(tc, more_bit);
  buffer[0] = 0x40;
  CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
  CuAssertUIntEquals(tc, 0u, address);
  CuAssertTrue(tc, more_bit);
  buffer[0] = 0x3F;
  buffer[1] = 0xFF;
  CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
  CuAssertUIntEquals(tc, RMF_LOW_ADDR_MAX, address);
  CuAssertFalse(tc, more_bit);
  buffer[0] = 0x7F;
  CuAssertUIntEquals(tc, UINT16_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
  CuAssertUIntEquals(tc, RMF_LOW_ADDR_MAX, address);
  CuAssertTrue(tc, more_bit);
}

static void test_high_address_encode(CuTest *tc)
{
  uint8_t buffer[UINT32_SIZE];
  CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_HIGH_ADDR_MIN, false));
  CuAssertUIntEquals(tc, 0x80u, buffer[0]);
  CuAssertUIntEquals(tc, 0x00u, buffer[1]);
  CuAssertUIntEquals(tc, 0x40u, buffer[2]);
  CuAssertUIntEquals(tc, 0x00u, buffer[3]);
  CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_HIGH_ADDR_MIN, true));
  CuAssertUIntEquals(tc, 0xC0u, buffer[0]);
  CuAssertUIntEquals(tc, 0x00u, buffer[1]);
  CuAssertUIntEquals(tc, 0x40u, buffer[2]);
  CuAssertUIntEquals(tc, 0x00u, buffer[3]);
  CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_HIGH_ADDR_MAX, false));
  CuAssertUIntEquals(tc, 0xBFu, buffer[0]);
  CuAssertUIntEquals(tc, 0xFFu, buffer[1]);
  CuAssertUIntEquals(tc, 0xFFu, buffer[2]);
  CuAssertUIntEquals(tc, 0xFFu, buffer[3]);
  CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_encode(buffer, sizeof(buffer), RMF_HIGH_ADDR_MAX, true));
  CuAssertUIntEquals(tc, 0xFFu, buffer[0]);
  CuAssertUIntEquals(tc, 0xFFu, buffer[1]);
  CuAssertUIntEquals(tc, 0xFFu, buffer[2]);
  CuAssertUIntEquals(tc, 0xFFu, buffer[3]);
}

static void test_high_address_decode(CuTest *tc)
{
  uint8_t buffer[UINT32_SIZE] = {0x80u, 0x0u, 0x40u, 0x0u};
  uint32_t address = 0u;
  bool more_bit = false;
  CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
  CuAssertUIntEquals(tc, RMF_HIGH_ADDR_MIN, address);
  CuAssertFalse(tc, more_bit);
  buffer[0] = 0xC0;
  CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
  CuAssertUIntEquals(tc, RMF_HIGH_ADDR_MIN, address);
  CuAssertTrue(tc, more_bit);
  buffer[0] = 0xBF;
  buffer[1] = 0xFF;
  buffer[2] = 0xFF;
  buffer[3] = 0xFF;
  CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
  CuAssertUIntEquals(tc, RMF_HIGH_ADDR_MAX, address);
  CuAssertFalse(tc, more_bit);
  buffer[0] = 0xFF;
  CuAssertUIntEquals(tc, UINT32_SIZE, rmf_address_decode(buffer, buffer + sizeof(buffer), &address, &more_bit));
  CuAssertUIntEquals(tc, RMF_HIGH_ADDR_MAX, address);
  CuAssertTrue(tc, more_bit);
}

static void test_encode_accept_header(CuTest *tc)
{
  uint8_t actual[RMF_CMD_TYPE_SIZE + UINT32_SIZE];
  uint8_t expected[RMF_CMD_TYPE_SIZE + UINT32_SIZE] = {
    (uint8_t)RMF_CMD_ACCEPT_HEADER,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
  };
  uint32_t connection_id = 0;
  CuAssertUIntEquals(
    tc, (unsigned int)sizeof(actual), rmf_encode_header_accepted(actual, (apx_size_t)sizeof(actual), connection_id));
  CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(expected)));
  connection_id = 0xFFFFFF;
  packLE(&expected[RMF_CMD_TYPE_SIZE], connection_id, UINT32_SIZE);
  CuAssertUIntEquals(
    tc, (unsigned int)sizeof(actual), rmf_encode_header_accepted(actual, (apx_size_t)sizeof(actual), connection_id));
  CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(expected)));
}

static void test_decode_accept_header(CuTest *tc)
{
  uint8_t buffer[RMF_CMD_TYPE_SIZE + UINT32_SIZE] = {
    (uint8_t)RMF_CMD_ACCEPT_HEADER,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
  };
  uint32_t connection_id = 0xFFFFFFFF;
  CuAssertUIntEquals(
    tc, UINT32_SIZE, rmf_decode_header_accepted(buffer + RMF_CMD_TYPE_SIZE, buffer + sizeof(buffer), &connection_id));
  CuAssertUIntEquals(tc, 0u, connection_id);
  packLE(&buffer[RMF_CMD_TYPE_SIZE], 0x12345678, UINT32_SIZE);
  CuAssertUIntEquals(
    tc, UINT32_SIZE, rmf_decode_header_accepted(buffer + RMF_CMD_TYPE_SIZE, buffer + sizeof(buffer), &connection_id));
  CuAssertUIntEquals(tc, 0x12345678, connection_id);
}

static void test_encode_nack_cmd(CuTest *tc)
{
  uint8_t buffer[64];
  uint8_t small_buf[RMF_CMD_NACK_SIZE];

   // Too small buffer returns 0
  CuAssertUIntEquals(tc, 0u, rmf_encode_nack_cmd(small_buf, sizeof(small_buf), 0u, NULL));
  CuAssertUIntEquals(tc, 0u, rmf_encode_nack_cmd(NULL, sizeof(buffer), 0u, NULL));

   // Encode APX_NO_ERROR (0) with NULL name (adds 1-byte null terminator)
  memset(buffer, 0xFF, sizeof(buffer));
  CuAssertUIntEquals(tc, RMF_CMD_NACK_SIZE + 1u, rmf_encode_nack_cmd(buffer, sizeof(buffer), 0u, NULL));
  CuAssertUIntEquals(tc, RMF_CMD_NACK_MSG, unpackLE(buffer, UINT32_SIZE));
  CuAssertUIntEquals(tc, 0u, unpackLE(buffer + RMF_CMD_TYPE_SIZE, UINT32_SIZE));
  CuAssertUIntEquals(tc, 0, buffer[RMF_CMD_NACK_SIZE]);

   // Encode APX_SIGNATURE_VERIFICATION_ERROR with node name
  memset(buffer, 0, sizeof(buffer));
  char const *node_name = "TestNode1";
  apx_size_t expected_size = RMF_CMD_NACK_SIZE + (apx_size_t)strlen(node_name) + 1u;
  CuAssertUIntEquals(tc, expected_size,
    rmf_encode_nack_cmd(buffer, sizeof(buffer), (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, node_name));
  CuAssertUIntEquals(tc, RMF_CMD_NACK_MSG, unpackLE(buffer, UINT32_SIZE));
  CuAssertUIntEquals(tc, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, unpackLE(buffer + RMF_CMD_TYPE_SIZE, UINT32_SIZE));
  CuAssertStrEquals(tc, node_name, (char const *)(buffer + RMF_CMD_NACK_SIZE));

   // Encode custom value with LE check
  memset(buffer, 0, sizeof(buffer));
  CuAssertUIntEquals(tc, RMF_CMD_NACK_SIZE + 1u, rmf_encode_nack_cmd(buffer, sizeof(buffer), 0x12345678, NULL));
  CuAssertUIntEquals(tc, (uint8_t)RMF_CMD_NACK_MSG, buffer[0]);
  CuAssertUIntEquals(tc, 0x00, buffer[1]);
  CuAssertUIntEquals(tc, 0x00, buffer[2]);
  CuAssertUIntEquals(tc, 0x00, buffer[3]);
  CuAssertUIntEquals(tc, 0x78, buffer[4]);
  CuAssertUIntEquals(tc, 0x56, buffer[5]);
  CuAssertUIntEquals(tc, 0x34, buffer[6]);
  CuAssertUIntEquals(tc, 0x12, buffer[7]);
  CuAssertUIntEquals(tc, 0, buffer[8]);
}

static void test_decode_nack_cmd(CuTest *tc)
{
  uint8_t full_cmd[64];
  uint32_t error_code = 0xFFFFFFFF;
  char const *name = NULL;

   // NULL / invalid arguments
  CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(NULL, full_cmd + sizeof(full_cmd), &error_code, &name));
  CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(full_cmd, NULL, &error_code, &name));
  CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(full_cmd, full_cmd, &error_code, &name));
  CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(full_cmd, full_cmd + sizeof(full_cmd), NULL, &name));

   // Decode legacy 8-byte command (cmd_type + error_code, no string)
  packLE(full_cmd, RMF_CMD_NACK_MSG, UINT32_SIZE);
  packLE(full_cmd + RMF_CMD_TYPE_SIZE, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, UINT32_SIZE);

  CuAssertUIntEquals(
    tc, RMF_CMD_NACK_SIZE, rmf_decode_nack_cmd(full_cmd, full_cmd + RMF_CMD_NACK_SIZE, &error_code, &name));
  CuAssertUIntEquals(tc, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, error_code);
  CuAssertPtrEquals(tc, NULL, (void *)name);

   // Decode full command with node name string
  char const *test_name = "EngineController";
  apx_size_t enc_size =
    rmf_encode_nack_cmd(full_cmd, sizeof(full_cmd), (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, test_name);
  error_code = 0;
  name = NULL;
  CuAssertUIntEquals(tc, enc_size, rmf_decode_nack_cmd(full_cmd, full_cmd + enc_size, &error_code, &name));
  CuAssertUIntEquals(tc, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, error_code);
  CuAssertPtrNotNull(tc, name);
  CuAssertStrEquals(tc, test_name, name);

   // Decode payload only (4-byte error_code)
  error_code = 0;
  name = (char const *)0x1;
  CuAssertUIntEquals(tc, UINT32_SIZE,
    rmf_decode_nack_cmd(full_cmd + RMF_CMD_TYPE_SIZE, full_cmd + RMF_CMD_NACK_SIZE, &error_code, &name));
  CuAssertUIntEquals(tc, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, error_code);
  CuAssertPtrEquals(tc, NULL, (void *)name);

   // Too short buffer (< 4 bytes)
  CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(full_cmd, full_cmd + 3, &error_code, &name));
}

static void test_close_file_cmd_encode(CuTest *tc)
{
  uint8_t buf[RMF_CMD_TYPE_SIZE + RMF_FILE_CLOSE_CMD_SIZE];
  CuAssertUIntEquals(tc, 0u, rmf_encode_close_file_cmd(NULL, sizeof(buf), 0x1000u));
  CuAssertUIntEquals(tc, 0u, rmf_encode_close_file_cmd(buf, sizeof(buf) - 1u, 0x1000u));
  CuAssertUIntEquals(tc, 0u, rmf_encode_close_file_cmd(buf, sizeof(buf), RMF_HIGH_ADDR_MAX + 1u));
  CuAssertUIntEquals(tc, sizeof(buf), rmf_encode_close_file_cmd(buf, sizeof(buf), 0x00102030u));
  CuAssertUIntEquals(tc, RMF_CMD_CLOSE_FILE_MSG, unpackLE(buf, UINT32_SIZE));
  CuAssertUIntEquals(tc, 0x00102030u, unpackLE(buf + UINT32_SIZE, UINT32_SIZE));

  CuAssertUIntEquals(tc, 0u, rmf_encode_revoke_file_cmd(NULL, sizeof(buf), 0x1000u));
  CuAssertUIntEquals(tc, 0u, rmf_encode_revoke_file_cmd(buf, sizeof(buf) - 1u, 0x1000u));
  CuAssertUIntEquals(tc, 0u, rmf_encode_revoke_file_cmd(buf, sizeof(buf), RMF_HIGH_ADDR_MAX + 1u));
  CuAssertUIntEquals(tc, sizeof(buf), rmf_encode_revoke_file_cmd(buf, sizeof(buf), 0x00102030u));
  CuAssertUIntEquals(tc, RMF_CMD_REVOKE_FILE_MSG, unpackLE(buf, UINT32_SIZE));
  CuAssertUIntEquals(tc, 0x00102030u, unpackLE(buf + UINT32_SIZE, UINT32_SIZE));
}

static void test_stream_open_cmd_encode_decode(CuTest *tc)
{
  uint8_t buf[RMF_STREAM_OPEN_CMD_SIZE];
  rmf_stream_open_cmd_t in_cmd = {
    .start_address = 0x20001000u,
    .max_block_size = 4096u,
    .window_blocks = 8u,
    .flags = 0u,
  };
  rmf_stream_open_cmd_t out_cmd;
  memset(&out_cmd, 0, sizeof(out_cmd));

  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_open_cmd(NULL, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_open_cmd(buf, sizeof(buf), NULL));
  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_open_cmd(buf, RMF_STREAM_OPEN_CMD_SIZE - 1u, &in_cmd));

  CuAssertUIntEquals(tc, RMF_STREAM_OPEN_CMD_SIZE, rmf_encode_stream_open_cmd(buf, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, RMF_CMD_STREAM_OPEN_MSG, unpackLE(buf, UINT32_SIZE));

  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_open_cmd(NULL, buf + sizeof(buf), &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_open_cmd(buf, NULL, &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_open_cmd(buf, buf + sizeof(buf), NULL));
  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_open_cmd(buf, buf + RMF_STREAM_OPEN_CMD_SIZE - 1u, &out_cmd));

  CuAssertUIntEquals(tc, RMF_STREAM_OPEN_CMD_SIZE, rmf_decode_stream_open_cmd(buf, buf + sizeof(buf), &out_cmd));
  CuAssertUIntEquals(tc, in_cmd.start_address, out_cmd.start_address);
  CuAssertUIntEquals(tc, in_cmd.max_block_size, out_cmd.max_block_size);
  CuAssertUIntEquals(tc, in_cmd.window_blocks, out_cmd.window_blocks);
  CuAssertUIntEquals(tc, in_cmd.flags, out_cmd.flags);

  // Wrong command type fails decode
  packLE(buf, RMF_CMD_OPEN_FILE_MSG, UINT32_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_open_cmd(buf, buf + sizeof(buf), &out_cmd));
}

static void test_stream_open_rsp_cmd_encode_decode(CuTest *tc)
{
  uint8_t buf[RMF_STREAM_OPEN_RSP_CMD_SIZE];
  rmf_stream_open_rsp_cmd_t in_cmd = {
    .start_address = 0x20001000u,
    .status = (uint32_t)APX_NO_ERROR,
    .block_size = 2048u,
    .window_blocks = 4u,
    .flags = 0u,
  };
  rmf_stream_open_rsp_cmd_t out_cmd;
  memset(&out_cmd, 0, sizeof(out_cmd));

  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_open_rsp_cmd(NULL, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_open_rsp_cmd(buf, sizeof(buf), NULL));
  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_open_rsp_cmd(buf, RMF_STREAM_OPEN_RSP_CMD_SIZE - 1u, &in_cmd));

  CuAssertUIntEquals(tc, RMF_STREAM_OPEN_RSP_CMD_SIZE, rmf_encode_stream_open_rsp_cmd(buf, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, RMF_CMD_STREAM_OPEN_RSP_MSG, unpackLE(buf, UINT32_SIZE));

  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_open_rsp_cmd(NULL, buf + sizeof(buf), &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_open_rsp_cmd(buf, buf + RMF_STREAM_OPEN_RSP_CMD_SIZE - 1u, &out_cmd));

  CuAssertUIntEquals(
    tc, RMF_STREAM_OPEN_RSP_CMD_SIZE, rmf_decode_stream_open_rsp_cmd(buf, buf + sizeof(buf), &out_cmd));
  CuAssertUIntEquals(tc, in_cmd.start_address, out_cmd.start_address);
  CuAssertUIntEquals(tc, in_cmd.status, out_cmd.status);
  CuAssertUIntEquals(tc, in_cmd.block_size, out_cmd.block_size);
  CuAssertUIntEquals(tc, in_cmd.window_blocks, out_cmd.window_blocks);
  CuAssertUIntEquals(tc, in_cmd.flags, out_cmd.flags);
}

static void test_stream_window_update_cmd_encode_decode(CuTest *tc)
{
  uint8_t buf[RMF_STREAM_WINDOW_UPDATE_CMD_SIZE];
  rmf_stream_window_update_cmd_t in_cmd = {
    .start_address = 0x20001000u,
    .consumed_block_count = 0x0102030405060708ULL,
  };
  rmf_stream_window_update_cmd_t out_cmd;
  memset(&out_cmd, 0, sizeof(out_cmd));

  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_window_update_cmd(NULL, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_window_update_cmd(buf, sizeof(buf), NULL));
  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_window_update_cmd(buf, RMF_STREAM_WINDOW_UPDATE_CMD_SIZE - 1u, &in_cmd));

  CuAssertUIntEquals(
    tc, RMF_STREAM_WINDOW_UPDATE_CMD_SIZE, rmf_encode_stream_window_update_cmd(buf, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, RMF_CMD_STREAM_WINDOW_UPDATE_MSG, unpackLE(buf, UINT32_SIZE));
  CuAssertUIntEquals(tc, 0x08u, buf[8]);
  CuAssertUIntEquals(tc, 0x01u, buf[15]);

  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_window_update_cmd(NULL, buf + sizeof(buf), &out_cmd));
  CuAssertUIntEquals(
    tc, 0u, rmf_decode_stream_window_update_cmd(buf, buf + RMF_STREAM_WINDOW_UPDATE_CMD_SIZE - 1u, &out_cmd));

  CuAssertUIntEquals(
    tc, RMF_STREAM_WINDOW_UPDATE_CMD_SIZE, rmf_decode_stream_window_update_cmd(buf, buf + sizeof(buf), &out_cmd));
  CuAssertUIntEquals(tc, in_cmd.start_address, out_cmd.start_address);
  CuAssertTrue(tc, in_cmd.consumed_block_count == out_cmd.consumed_block_count);
}

static void test_stream_abort_cmd_encode_decode(CuTest *tc)
{
  uint8_t buf[RMF_STREAM_ABORT_CMD_SIZE];
  rmf_stream_abort_cmd_t in_cmd = {
    .start_address = 0x20001000u,
    .status = (uint32_t)APX_FILE_TOO_LARGE_ERROR,
  };
  rmf_stream_abort_cmd_t out_cmd;
  memset(&out_cmd, 0, sizeof(out_cmd));

  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_abort_cmd(NULL, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_abort_cmd(buf, sizeof(buf), NULL));
  CuAssertUIntEquals(tc, 0u, rmf_encode_stream_abort_cmd(buf, RMF_STREAM_ABORT_CMD_SIZE - 1u, &in_cmd));

  CuAssertUIntEquals(tc, RMF_STREAM_ABORT_CMD_SIZE, rmf_encode_stream_abort_cmd(buf, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, RMF_CMD_STREAM_ABORT_MSG, unpackLE(buf, UINT32_SIZE));

  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_abort_cmd(NULL, buf + sizeof(buf), &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_stream_abort_cmd(buf, buf + RMF_STREAM_ABORT_CMD_SIZE - 1u, &out_cmd));

  CuAssertUIntEquals(tc, RMF_STREAM_ABORT_CMD_SIZE, rmf_decode_stream_abort_cmd(buf, buf + sizeof(buf), &out_cmd));
  CuAssertUIntEquals(tc, in_cmd.start_address, out_cmd.start_address);
  CuAssertUIntEquals(tc, in_cmd.status, out_cmd.status);
}

static void test_compression_type_conversion(CuTest *tc)
{
  rmf_compression_type_t compression_type = RMF_COMPRESSION_TYPE_NONE;
  CuAssertFalse(tc, rmf_value_to_compression_type(RMF_U16_COMPRESSION_TYPE_NONE, NULL));
  CuAssertTrue(tc, rmf_value_to_compression_type(RMF_U16_COMPRESSION_TYPE_NONE, &compression_type));
  CuAssertUIntEquals(tc, RMF_COMPRESSION_TYPE_NONE, compression_type);

  CuAssertTrue(tc, rmf_value_to_compression_type(RMF_U16_COMPRESSION_TYPE_ZLIB_DEFLATE, &compression_type));
  CuAssertUIntEquals(tc, RMF_COMPRESSION_TYPE_ZLIB_DEFLATE, compression_type);

  CuAssertFalse(tc, rmf_value_to_compression_type(2u, &compression_type));
  CuAssertFalse(tc, rmf_value_to_compression_type(0xFFFFu, &compression_type));
}

static void test_publish_compressed_file_cmd_encode_decode(CuTest *tc)
{
  uint8_t buf[RMF_CMD_AREA_SIZE];
  uint8_t dummy_digest[RMF_SHA256_SIZE];
  for (uint32_t i = 0u; i < RMF_SHA256_SIZE; ++i)
  {
    dummy_digest[i] = (uint8_t)(i + 1u);
  }

  rmf_publish_compressed_file_cmd_t in_cmd = {
    .start_address = 0x10000000u,
    .uncompressed_size = 5000u,
    .compressed_size = 650u,
    .file_type = RMF_FILE_TYPE_FIXED,
    .compression_type = RMF_COMPRESSION_TYPE_ZLIB_DEFLATE,
    .digest_type = RMF_DIGEST_TYPE_SHA256,
    .reserved = 0u,
    .name = "TestNode.apx",
  };
  memcpy(in_cmd.digest_data, dummy_digest, RMF_SHA256_SIZE);

  // Normal encode with SHA-256
  apx_size_t const expected_size = RMF_PUBLISH_COMPRESSED_FILE_HEADER_SIZE + strlen("TestNode.apx") + 1u;
  apx_size_t encoded_size = rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &in_cmd);
  CuAssertUIntEquals(tc, expected_size, encoded_size);

  // Verify wire layout
  CuAssertUIntEquals(tc, RMF_CMD_PUBLISH_COMPRESSED_FILE_MSG, unpackLE(buf, UINT32_SIZE));
  CuAssertUIntEquals(tc, 0x10000000u, unpackLE(buf + 4, UINT32_SIZE));
  CuAssertUIntEquals(tc, 5000u, unpackLE(buf + 8, UINT32_SIZE));
  CuAssertUIntEquals(tc, 650u, unpackLE(buf + 12, UINT32_SIZE));
  CuAssertUIntEquals(tc, RMF_U16_FILE_TYPE_FIXED, unpackLE(buf + 16, UINT16_SIZE));
  CuAssertUIntEquals(tc, RMF_U16_COMPRESSION_TYPE_ZLIB_DEFLATE, unpackLE(buf + 18, UINT16_SIZE));
  CuAssertUIntEquals(tc, RMF_U16_DIGEST_TYPE_SHA256, unpackLE(buf + 20, UINT16_SIZE));
  CuAssertUIntEquals(tc, 0u, unpackLE(buf + 22, UINT16_SIZE));
  CuAssertIntEquals(tc, 0, memcmp(buf + 24, dummy_digest, RMF_SHA256_SIZE));
  CuAssertStrEquals(tc, "TestNode.apx", (char const *)(buf + 56));

  // Normal decode with SHA-256
  rmf_publish_compressed_file_cmd_t out_cmd;
  memset(&out_cmd, 0, sizeof(out_cmd));
  apx_size_t decoded_size = rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd);
  CuAssertUIntEquals(tc, encoded_size, decoded_size);
  CuAssertUIntEquals(tc, in_cmd.start_address, out_cmd.start_address);
  CuAssertUIntEquals(tc, in_cmd.uncompressed_size, out_cmd.uncompressed_size);
  CuAssertUIntEquals(tc, in_cmd.compressed_size, out_cmd.compressed_size);
  CuAssertUIntEquals(tc, in_cmd.file_type, out_cmd.file_type);
  CuAssertUIntEquals(tc, in_cmd.compression_type, out_cmd.compression_type);
  CuAssertUIntEquals(tc, in_cmd.digest_type, out_cmd.digest_type);
  CuAssertUIntEquals(tc, 0u, out_cmd.reserved);
  CuAssertIntEquals(tc, 0, memcmp(out_cmd.digest_data, dummy_digest, RMF_SHA256_SIZE));
  CuAssertStrEquals(tc, "TestNode.apx", out_cmd.name);

  // Normal encode/decode with DIGEST_TYPE_NONE
  in_cmd.digest_type = RMF_DIGEST_TYPE_NONE;
  encoded_size = rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &in_cmd);
  CuAssertUIntEquals(tc, expected_size, encoded_size);
  CuAssertUIntEquals(tc, RMF_U16_DIGEST_TYPE_NONE, unpackLE(buf + 20, UINT16_SIZE));
  uint8_t zero_digest[RMF_SHA256_SIZE] = {0};
  CuAssertIntEquals(tc, 0, memcmp(buf + 24, zero_digest, RMF_SHA256_SIZE));

  memset(&out_cmd, 0, sizeof(out_cmd));
  decoded_size = rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd);
  CuAssertUIntEquals(tc, encoded_size, decoded_size);
  CuAssertUIntEquals(tc, RMF_DIGEST_TYPE_NONE, out_cmd.digest_type);
  CuAssertIntEquals(tc, 0, memcmp(out_cmd.digest_data, zero_digest, RMF_SHA256_SIZE));

  // Reset in_cmd
  in_cmd.digest_type = RMF_DIGEST_TYPE_SHA256;

  // Encode validation errors
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(NULL, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), NULL));
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, expected_size - 1u, &in_cmd));

  rmf_publish_compressed_file_cmd_t bad_cmd = in_cmd;
  bad_cmd.name = NULL;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));
  bad_cmd.name = "";
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.uncompressed_size = 0u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));
  bad_cmd.uncompressed_size = RMF_CMD_AREA_START_ADDRESS + 1u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.compressed_size = 0u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));
  bad_cmd.compressed_size = RMF_CMD_AREA_START_ADDRESS + 1u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.start_address = RMF_CMD_AREA_START_ADDRESS;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));
  bad_cmd.start_address = RMF_CMD_AREA_START_ADDRESS - 100u;
  bad_cmd.compressed_size = 101u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.file_type = RMF_FILE_TYPE_STREAM;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.compression_type = RMF_COMPRESSION_TYPE_NONE;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.digest_type = RMF_DIGEST_TYPE_SHA1;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  // Decode validation errors
  encoded_size = rmf_encode_publish_compressed_file_cmd(buf, sizeof(buf), &in_cmd);
  CuAssertTrue(tc, encoded_size > 0u);

  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(NULL, buf + encoded_size, &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, NULL, &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, NULL));
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size - 1u, &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + 10u, &out_cmd));

  // Corrupted opcode
  packLE(buf, RMF_CMD_PUBLISH_SIGNED_FILE_MSG, UINT32_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf, RMF_CMD_PUBLISH_COMPRESSED_FILE_MSG, UINT32_SIZE);

  // Non-zero reserved field
  packLE(buf + 22, 1u, UINT16_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 22, 0u, UINT16_SIZE);

  // Invalid uncompressed_size = 0
  packLE(buf + 8, 0u, UINT32_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 8, in_cmd.uncompressed_size, UINT32_SIZE);

  // Invalid compressed_size = 0
  packLE(buf + 12, 0u, UINT32_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 12, in_cmd.compressed_size, UINT32_SIZE);

  // Invalid file_type
  packLE(buf + 16, RMF_U16_FILE_TYPE_DYNAMIC8, UINT16_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 16, RMF_U16_FILE_TYPE_FIXED, UINT16_SIZE);

  // Invalid compression_type
  packLE(buf + 18, 0u, UINT16_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 18, RMF_U16_COMPRESSION_TYPE_ZLIB_DEFLATE, UINT16_SIZE);

  // Invalid digest_type (SHA1 = 1)
  packLE(buf + 20, 1u, UINT16_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 20, RMF_U16_DIGEST_TYPE_SHA256, UINT16_SIZE);

  // Empty name (null terminator at offset 56)
  uint8_t save_char = buf[56];
  buf[56] = 0u;
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  buf[56] = save_char;
}

static void test_publish_signed_compressed_file_cmd_encode_decode(CuTest *tc)
{
  uint8_t buf[RMF_CMD_AREA_SIZE];
  uint8_t dummy_sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  for (uint32_t i = 0u; i < RMF_SIGNATURE_SIZE_ECDSA_P256; ++i)
  {
    dummy_sig[i] = (uint8_t)(0xC0u + (i & 0x0Fu));
  }

  rmf_publish_signed_compressed_file_cmd_t in_cmd = {
    .start_address = 0x04000000u,
    .uncompressed_size = 12000u,
    .compressed_size = 1800u,
    .file_type = RMF_FILE_TYPE_FIXED,
    .compression_type = RMF_COMPRESSION_TYPE_ZLIB_DEFLATE,
    .signature_type = RMF_SIGNATURE_TYPE_ECDSA_P256,
    .reserved = 0u,
    .name = "SecureNode.apx",
  };
  memcpy(in_cmd.signature_data, dummy_sig, RMF_SIGNATURE_SIZE_ECDSA_P256);

  // Normal encode
  apx_size_t const expected_size = RMF_PUBLISH_SIGNED_COMPRESSED_FILE_HEADER_SIZE + strlen("SecureNode.apx") + 1u;
  apx_size_t encoded_size = rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &in_cmd);
  CuAssertUIntEquals(tc, expected_size, encoded_size);

  // Verify wire layout
  CuAssertUIntEquals(tc, RMF_CMD_PUBLISH_SIGNED_COMPRESSED_FILE_MSG, unpackLE(buf, UINT32_SIZE));
  CuAssertUIntEquals(tc, 0x04000000u, unpackLE(buf + 4, UINT32_SIZE));
  CuAssertUIntEquals(tc, 12000u, unpackLE(buf + 8, UINT32_SIZE));
  CuAssertUIntEquals(tc, 1800u, unpackLE(buf + 12, UINT32_SIZE));
  CuAssertUIntEquals(tc, RMF_U16_FILE_TYPE_FIXED, unpackLE(buf + 16, UINT16_SIZE));
  CuAssertUIntEquals(tc, RMF_U16_COMPRESSION_TYPE_ZLIB_DEFLATE, unpackLE(buf + 18, UINT16_SIZE));
  CuAssertUIntEquals(tc, RMF_U16_SIGNATURE_TYPE_ECDSA_P256, unpackLE(buf + 20, UINT16_SIZE));
  CuAssertUIntEquals(tc, 0u, unpackLE(buf + 22, UINT16_SIZE));
  CuAssertIntEquals(tc, 0, memcmp(buf + 24, dummy_sig, RMF_SIGNATURE_SIZE_ECDSA_P256));
  CuAssertStrEquals(tc, "SecureNode.apx", (char const *)(buf + 88));

  // Normal decode
  rmf_publish_signed_compressed_file_cmd_t out_cmd;
  memset(&out_cmd, 0, sizeof(out_cmd));
  apx_size_t decoded_size = rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd);
  CuAssertUIntEquals(tc, encoded_size, decoded_size);
  CuAssertUIntEquals(tc, in_cmd.start_address, out_cmd.start_address);
  CuAssertUIntEquals(tc, in_cmd.uncompressed_size, out_cmd.uncompressed_size);
  CuAssertUIntEquals(tc, in_cmd.compressed_size, out_cmd.compressed_size);
  CuAssertUIntEquals(tc, in_cmd.file_type, out_cmd.file_type);
  CuAssertUIntEquals(tc, in_cmd.compression_type, out_cmd.compression_type);
  CuAssertUIntEquals(tc, in_cmd.signature_type, out_cmd.signature_type);
  CuAssertUIntEquals(tc, 0u, out_cmd.reserved);
  CuAssertIntEquals(tc, 0, memcmp(out_cmd.signature_data, dummy_sig, RMF_SIGNATURE_SIZE_ECDSA_P256));
  CuAssertStrEquals(tc, "SecureNode.apx", out_cmd.name);

  // Encode validation errors
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(NULL, sizeof(buf), &in_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), NULL));
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, expected_size - 1u, &in_cmd));

  rmf_publish_signed_compressed_file_cmd_t bad_cmd = in_cmd;
  bad_cmd.name = NULL;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));
  bad_cmd.name = "";
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.uncompressed_size = 0u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));
  bad_cmd.uncompressed_size = RMF_CMD_AREA_START_ADDRESS + 1u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.compressed_size = 0u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));
  bad_cmd.compressed_size = RMF_CMD_AREA_START_ADDRESS + 1u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.start_address = RMF_CMD_AREA_START_ADDRESS;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));
  bad_cmd.start_address = RMF_CMD_AREA_START_ADDRESS - 100u;
  bad_cmd.compressed_size = 101u;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.file_type = RMF_FILE_TYPE_DYNAMIC16;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.compression_type = RMF_COMPRESSION_TYPE_NONE;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  bad_cmd = in_cmd;
  bad_cmd.signature_type = RMF_SIGNATURE_TYPE_NONE;
  CuAssertUIntEquals(tc, 0u, rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &bad_cmd));

  // Decode validation errors
  encoded_size = rmf_encode_publish_signed_compressed_file_cmd(buf, sizeof(buf), &in_cmd);
  CuAssertTrue(tc, encoded_size > 0u);

  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(NULL, buf + encoded_size, &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, NULL, &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, NULL));
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size - 1u, &out_cmd));
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + 20u, &out_cmd));

  // Corrupted opcode
  packLE(buf, RMF_CMD_PUBLISH_COMPRESSED_FILE_MSG, UINT32_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf, RMF_CMD_PUBLISH_SIGNED_COMPRESSED_FILE_MSG, UINT32_SIZE);

  // Non-zero reserved field
  packLE(buf + 22, 1u, UINT16_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 22, 0u, UINT16_SIZE);

  // Invalid uncompressed_size = 0
  packLE(buf + 8, 0u, UINT32_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 8, in_cmd.uncompressed_size, UINT32_SIZE);

  // Invalid compressed_size = 0
  packLE(buf + 12, 0u, UINT32_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 12, in_cmd.compressed_size, UINT32_SIZE);

  // Invalid file_type
  packLE(buf + 16, RMF_U16_FILE_TYPE_STREAM, UINT16_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 16, RMF_U16_FILE_TYPE_FIXED, UINT16_SIZE);

  // Invalid compression_type
  packLE(buf + 18, 0u, UINT16_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 18, RMF_U16_COMPRESSION_TYPE_ZLIB_DEFLATE, UINT16_SIZE);

  // Invalid signature_type
  packLE(buf + 20, 0u, UINT16_SIZE);
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  packLE(buf + 20, RMF_U16_SIGNATURE_TYPE_ECDSA_P256, UINT16_SIZE);

  // Empty name (null terminator at offset 88)
  uint8_t save_char = buf[88];
  buf[88] = 0u;
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_compressed_file_cmd(buf, buf + encoded_size, &out_cmd));
  buf[88] = save_char;
}
