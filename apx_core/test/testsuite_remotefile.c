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


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_remotefile(void)
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

