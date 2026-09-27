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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/remotefile.h"
#include "apx/error.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_low_address_encode(CuTest* tc);
static void test_low_address_decode(CuTest* tc);
static void test_high_address_encode(CuTest* tc);
static void test_high_address_decode(CuTest* tc);
static void test_encode_accept_header(CuTest* tc);
static void test_decode_accept_header(CuTest* tc);
static void test_encode_connection_create_without_tag(CuTest* tc);
static void test_decode_connection_create_without_tag(CuTest* tc);
static void test_encode_connection_create_with_tag(CuTest* tc);
static void test_decode_connection_create_with_tag(CuTest* tc);
static void test_encode_connection_state(CuTest* tc);
static void test_decode_connection_state(CuTest* tc);
static void test_encode_remote_file_publish(CuTest* tc);
static void test_decode_remote_file_publish(CuTest* tc);
static void test_encode_remote_file_state(CuTest* tc);
static void test_decode_remote_file_state(CuTest* tc);
static void test_encode_remote_file_request(CuTest* tc);
static void test_decode_remote_file_request(CuTest* tc);
static void test_encode_nack_cmd(CuTest* tc);
static void test_decode_nack_cmd(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_remotefile(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_low_address_encode);
   SUITE_ADD_TEST(suite, test_low_address_decode);
   SUITE_ADD_TEST(suite, test_high_address_encode);
   SUITE_ADD_TEST(suite, test_high_address_decode);
   SUITE_ADD_TEST(suite, test_encode_accept_header);
   SUITE_ADD_TEST(suite, test_decode_accept_header);
   SUITE_ADD_TEST(suite, test_encode_connection_create_without_tag);
   SUITE_ADD_TEST(suite, test_decode_connection_create_without_tag);
   SUITE_ADD_TEST(suite, test_encode_connection_state);
   SUITE_ADD_TEST(suite, test_decode_connection_state);
   SUITE_ADD_TEST(suite, test_encode_remote_file_publish);
   SUITE_ADD_TEST(suite, test_decode_remote_file_publish);
   SUITE_ADD_TEST(suite, test_encode_remote_file_state);
   SUITE_ADD_TEST(suite, test_decode_remote_file_state);
   SUITE_ADD_TEST(suite, test_encode_remote_file_request);
   SUITE_ADD_TEST(suite, test_decode_remote_file_request);
   SUITE_ADD_TEST(suite, test_encode_nack_cmd);
   SUITE_ADD_TEST(suite, test_decode_nack_cmd);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_low_address_encode(CuTest* tc)
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

static void test_low_address_decode(CuTest* tc)
{
   uint8_t buffer[UINT16_SIZE] = { 0, 0 };
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

static void test_high_address_encode(CuTest* tc)
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

static void test_high_address_decode(CuTest* tc)
{
   uint8_t buffer[UINT32_SIZE] = { 0x80u, 0x0u, 0x40u , 0x0u };
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

static void test_encode_accept_header(CuTest* tc)
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
   CuAssertUIntEquals(tc, (unsigned int)sizeof(actual), rmf_encode_header_accepted(actual, (apx_size_t)sizeof(actual), connection_id));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(expected)));
   connection_id = 0xFFFFFF;
   packLE(&expected[RMF_CMD_TYPE_SIZE], connection_id, UINT32_SIZE);
   CuAssertUIntEquals(tc, (unsigned int)sizeof(actual), rmf_encode_header_accepted(actual, (apx_size_t)sizeof(actual), connection_id));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(expected)));
}

static void test_decode_accept_header(CuTest* tc)
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
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_decode_header_accepted(buffer + RMF_CMD_TYPE_SIZE, buffer + sizeof(buffer), &connection_id));
   CuAssertUIntEquals(tc, 0u, connection_id);
   packLE(&buffer[RMF_CMD_TYPE_SIZE], 0x12345678, UINT32_SIZE);
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_decode_header_accepted(buffer + RMF_CMD_TYPE_SIZE, buffer + sizeof(buffer), &connection_id));
   CuAssertUIntEquals(tc, 0x12345678, connection_id);
}

static void test_encode_connection_create_without_tag(CuTest* tc)
{
   uint8_t actual[RMF_CMD_TYPE_SIZE + UINT32_SIZE + UINT8_SIZE + CHAR_SIZE];
   uint8_t expected[RMF_CMD_TYPE_SIZE + UINT32_SIZE + UINT8_SIZE + CHAR_SIZE] = {
      (uint8_t)RMF_CMD_CONNECTION_CREATE,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      0x00,
      APX_CONNECTION_STATE_CONNECTING,
      0x00
   };
   uint32_t connection_id = 0u;
   uint8_t connection_state = APX_CONNECTION_STATE_CONNECTING;
   CuAssertUIntEquals(tc, (unsigned int)sizeof(actual), rmf_encode_connection_create(actual,
      (apx_size_t)sizeof(actual), connection_id, connection_state, NULL));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(expected)));
   connection_id = 0xFFFFFF;
   packLE(&expected[RMF_CMD_TYPE_SIZE], connection_id, UINT32_SIZE);
   CuAssertUIntEquals(tc, (unsigned int)sizeof(actual), rmf_encode_connection_create(actual,
      (apx_size_t)sizeof(actual), connection_id, connection_state, NULL));
   CuAssertIntEquals(tc, 0, memcmp(actual, expected, sizeof(expected)));
}

static void test_decode_connection_create_without_tag(CuTest* tc)
{
   (void)tc;
}

static void test_decode_connection_create_with_tag(CuTest* tc)
{
   (void)tc;
}

static void test_encode_connection_state(CuTest* tc)
{
   (void)tc;
}

static void test_decode_connection_state(CuTest* tc)
{
   (void)tc;
}

static void test_encode_remote_file_publish(CuTest* tc)
{
   (void)tc;
}

static void test_decode_remote_file_publish(CuTest* tc)
{
   (void)tc;
}

static void test_encode_remote_file_state(CuTest* tc)
{
   (void)tc;
}

static void test_decode_remote_file_state(CuTest* tc)
{
   (void)tc;
}

static void test_encode_remote_file_request(CuTest* tc)
{
   (void)tc;
}

static void test_decode_remote_file_request(CuTest* tc)
{
   (void)tc;
}

static void test_encode_nack_cmd(CuTest* tc)
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
   char const* node_name = "TestNode1";
   apx_size_t expected_size = RMF_CMD_NACK_SIZE + (apx_size_t)strlen(node_name) + 1u;
   CuAssertUIntEquals(tc, expected_size, rmf_encode_nack_cmd(buffer, sizeof(buffer), (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, node_name));
   CuAssertUIntEquals(tc, RMF_CMD_NACK_MSG, unpackLE(buffer, UINT32_SIZE));
   CuAssertUIntEquals(tc, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, unpackLE(buffer + RMF_CMD_TYPE_SIZE, UINT32_SIZE));
   CuAssertStrEquals(tc, node_name, (char const*)(buffer + RMF_CMD_NACK_SIZE));

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

static void test_decode_nack_cmd(CuTest* tc)
{
   uint8_t full_cmd[64];
   uint32_t error_code = 0xFFFFFFFF;
   char const* name = NULL;

   // NULL / invalid arguments
   CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(NULL, full_cmd + sizeof(full_cmd), &error_code, &name));
   CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(full_cmd, NULL, &error_code, &name));
   CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(full_cmd, full_cmd, &error_code, &name));
   CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(full_cmd, full_cmd + sizeof(full_cmd), NULL, &name));

   // Decode legacy 8-byte command (cmd_type + error_code, no string)
   packLE(full_cmd, RMF_CMD_NACK_MSG, UINT32_SIZE);
   packLE(full_cmd + RMF_CMD_TYPE_SIZE, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, UINT32_SIZE);

   CuAssertUIntEquals(tc, RMF_CMD_NACK_SIZE, rmf_decode_nack_cmd(full_cmd, full_cmd + RMF_CMD_NACK_SIZE, &error_code, &name));
   CuAssertUIntEquals(tc, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, error_code);
   CuAssertPtrEquals(tc, NULL, (void*)name);

   // Decode full command with node name string
   char const* test_name = "EngineController";
   apx_size_t enc_size = rmf_encode_nack_cmd(full_cmd, sizeof(full_cmd), (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, test_name);
   error_code = 0;
   name = NULL;
   CuAssertUIntEquals(tc, enc_size, rmf_decode_nack_cmd(full_cmd, full_cmd + enc_size, &error_code, &name));
   CuAssertUIntEquals(tc, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, error_code);
   CuAssertPtrNotNull(tc, name);
   CuAssertStrEquals(tc, test_name, name);

   // Decode payload only (4-byte error_code)
   error_code = 0;
   name = (char const*)0x1;
   CuAssertUIntEquals(tc, UINT32_SIZE, rmf_decode_nack_cmd(full_cmd + RMF_CMD_TYPE_SIZE, full_cmd + RMF_CMD_NACK_SIZE, &error_code, &name));
   CuAssertUIntEquals(tc, (uint32_t)APX_SIGNATURE_VERIFICATION_ERROR, error_code);
   CuAssertPtrEquals(tc, NULL, (void*)name);

   // Too short buffer (< 4 bytes)
   CuAssertUIntEquals(tc, 0u, rmf_decode_nack_cmd(full_cmd, full_cmd + 3, &error_code, &name));
}

