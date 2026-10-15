/*****************************************************************************
 * \file      testsuite_extended_file_info.c
 * \author    Conny Gustafsson
 * \date      2026-10-15
 * \brief     Unit tests for extended file info
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/extended_file_info.h"
#include "apx/file_info.h"
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
static void test_create_extended_file(CuTest *tc);
static void test_create_from_file_info(CuTest *tc);
static void test_clone_and_assign(CuTest *tc);
static void test_data_state_transitions(CuTest *tc);
static void test_name_ends_with_and_base_name(CuTest *tc);
static void test_encode_decode_publish_remote_file(CuTest *tc);
static void test_extended_file_info_signature(CuTest *tc);
static void test_encode_decode_publish_signed_remote_file(CuTest *tc);
static void test_extended_file_info_sha1_digest(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_extended_file_info(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_create_extended_file);
  SUITE_ADD_TEST(suite, test_create_from_file_info);
  SUITE_ADD_TEST(suite, test_clone_and_assign);
  SUITE_ADD_TEST(suite, test_data_state_transitions);
  SUITE_ADD_TEST(suite, test_name_ends_with_and_base_name);
  SUITE_ADD_TEST(suite, test_encode_decode_publish_remote_file);
  SUITE_ADD_TEST(suite, test_extended_file_info_signature);
  SUITE_ADD_TEST(suite, test_encode_decode_publish_signed_remote_file);
  SUITE_ADD_TEST(suite, test_extended_file_info_sha1_digest);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_extended_file(CuTest *tc)
{
  rmf_extended_file_info_t *info =
    rmf_extended_file_info_make_fixed("TestNode.apx", 40u, 0x1000u, 5u, APX_DATA_STATE_WAITING_FOR_FILE_DATA);
  CuAssertPtrNotNull(tc, info);
  CuAssertStrEquals(tc, "TestNode.apx", rmf_extended_file_info_name(info));
  CuAssertUIntEquals(tc, 5u, rmf_extended_file_info_connection_id(info));
  CuAssertUIntEquals(tc, 0x1000u, rmf_extended_file_info_address(info));
  CuAssertUIntEquals(tc, 40u, rmf_extended_file_info_size(info));
  CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, rmf_extended_file_info_rmf_file_type(info));
  CuAssertUIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, rmf_extended_file_info_data_state(info));
  CuAssertUIntEquals(tc, RMF_DIGEST_TYPE_NONE, rmf_extended_file_info_digest_type(info));
  rmf_extended_file_info_delete(info);
}

static void test_create_from_file_info(CuTest *tc)
{
  rmf_file_info_t *file_info = rmf_file_info_make_fixed("TestNode.out", 12u, 0x2000u);
  rmf_extended_file_info_t extended;
  apx_error_t rc;

  CuAssertPtrNotNull(tc, file_info);
  rc = rmf_extended_file_info_create_from_file_info(
    &extended, 3u, file_info, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  CuAssertStrEquals(tc, "TestNode.out", rmf_extended_file_info_name(&extended));
  CuAssertUIntEquals(tc, 3u, rmf_extended_file_info_connection_id(&extended));
  CuAssertUIntEquals(tc, 0x2000u, rmf_extended_file_info_address(&extended));
  CuAssertUIntEquals(tc, 12u, rmf_extended_file_info_size(&extended));
  CuAssertUIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, rmf_extended_file_info_data_state(&extended));

  rmf_extended_file_info_destroy(&extended);
  rmf_file_info_delete(file_info);
}

static void test_clone_and_assign(CuTest *tc)
{
  rmf_extended_file_info_t *original =
    rmf_extended_file_info_make_fixed("Sensor.in", 8u, 0x3000u, 7u, APX_DATA_STATE_SYNCHRONIZED);
  rmf_extended_file_info_t *cloned = rmf_extended_file_info_clone(original);
  rmf_extended_file_info_t assigned;

  CuAssertPtrNotNull(tc, original);
  CuAssertPtrNotNull(tc, cloned);
  CuAssertStrEquals(tc, "Sensor.in", rmf_extended_file_info_name(cloned));
  CuAssertUIntEquals(tc, 7u, rmf_extended_file_info_connection_id(cloned));
  CuAssertUIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, rmf_extended_file_info_data_state(cloned));

  apx_error_t rc = rmf_extended_file_info_create(
    &assigned, 0u, 0u, 0u, NULL, RMF_FILE_TYPE_FIXED, APX_DATA_STATE_INIT, RMF_DIGEST_TYPE_NONE, NULL);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  rc = rmf_extended_file_info_assign(&assigned, original);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  CuAssertStrEquals(tc, "Sensor.in", rmf_extended_file_info_name(&assigned));
  CuAssertUIntEquals(tc, 7u, rmf_extended_file_info_connection_id(&assigned));

  rmf_extended_file_info_destroy(&assigned);
  rmf_extended_file_info_delete(cloned);
  rmf_extended_file_info_delete(original);
}

static void test_data_state_transitions(CuTest *tc)
{
  rmf_extended_file_info_t info;
  apx_data_state_t parsed_state;
  apx_error_t rc = rmf_extended_file_info_create(&info, 1u, 0x1000u, 20u, "Test.apx", RMF_FILE_TYPE_FIXED,
    APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, RMF_DIGEST_TYPE_NONE, NULL);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  CuAssertUIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, rmf_extended_file_info_data_state(&info));

  rmf_extended_file_info_set_data_state(&info, APX_DATA_STATE_WAITING_FOR_FILE_DATA);
  CuAssertUIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, rmf_extended_file_info_data_state(&info));

  rmf_extended_file_info_set_data_state(&info, APX_DATA_STATE_SYNCHRONIZED);
  CuAssertUIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, rmf_extended_file_info_data_state(&info));

  CuAssertTrue(tc, apx_value_to_data_state(4u, &parsed_state));
  CuAssertUIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, parsed_state);
  CuAssertFalse(tc, apx_value_to_data_state(99u, &parsed_state));

  rmf_extended_file_info_destroy(&info);
}

static void test_name_ends_with_and_base_name(CuTest *tc)
{
  rmf_extended_file_info_t *info =
    rmf_extended_file_info_make_fixed("MyNode.apx", 50u, 0x1000u, 2u, APX_DATA_STATE_INIT);
  char base_buf[16];

  CuAssertPtrNotNull(tc, info);
  CuAssertTrue(tc, rmf_extended_file_info_name_ends_with(info, ".apx"));
  CuAssertFalse(tc, rmf_extended_file_info_name_ends_with(info, ".out"));

  char *base = rmf_extended_file_info_base_name(info);
  CuAssertPtrNotNull(tc, base);
  CuAssertStrEquals(tc, "MyNode", base);
  free(base);

  rmf_extended_file_info_copy_base_name(info, base_buf, sizeof(base_buf));
  CuAssertStrEquals(tc, "MyNode", base_buf);

  rmf_extended_file_info_delete(info);
}

static void test_encode_decode_publish_remote_file(CuTest *tc)
{
  uint8_t buffer[128];
  rmf_extended_file_info_t original;
  rmf_extended_file_info_t decoded;

  apx_error_t rc = rmf_extended_file_info_create(&original, 42u, 0x4000u, 100u, "Engine.out", RMF_FILE_TYPE_FIXED,
    APX_DATA_STATE_SYNCHRONIZED, RMF_DIGEST_TYPE_NONE, NULL);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);

  apx_size_t encoded_len = rmf_encode_publish_remote_file_cmd(buffer, sizeof(buffer), &original);
  CuAssertTrue(tc, encoded_len > 0u);

  apx_size_t decoded_len = rmf_decode_publish_remote_file_cmd(buffer, encoded_len, &decoded);
  CuAssertUIntEquals(tc, encoded_len, decoded_len);
  CuAssertUIntEquals(tc, 42u, rmf_extended_file_info_connection_id(&decoded));
  CuAssertUIntEquals(tc, 0x4000u, rmf_extended_file_info_address(&decoded));
  CuAssertUIntEquals(tc, 100u, rmf_extended_file_info_size(&decoded));
  CuAssertUIntEquals(tc, APX_DATA_STATE_SYNCHRONIZED, rmf_extended_file_info_data_state(&decoded));
  CuAssertStrEquals(tc, "Engine.out", rmf_extended_file_info_name(&decoded));

  rmf_extended_file_info_destroy(&original);
  rmf_extended_file_info_destroy(&decoded);
}

static void test_extended_file_info_signature(CuTest *tc)
{
  uint8_t dummy_sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  for (size_t i = 0; i < sizeof(dummy_sig); i++)
  {
    dummy_sig[i] = (uint8_t)(i + 1);
  }

  // 1. make_fixed_with_signature
  rmf_extended_file_info_t *info = rmf_extended_file_info_make_fixed_with_signature("SignedNode.apx", 512u, 0x1000u,
    10u, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_sig);
  CuAssertPtrNotNull(tc, info);
  CuAssertTrue(tc, rmf_extended_file_info_is_signed(info));
  CuAssertIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, rmf_extended_file_info_signature_type(info));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, rmf_extended_file_info_signature_data(info), sizeof(dummy_sig)));

  // 2. Clone preserves signature
  rmf_extended_file_info_t *cloned = rmf_extended_file_info_clone(info);
  CuAssertPtrNotNull(tc, cloned);
  CuAssertTrue(tc, rmf_extended_file_info_is_signed(cloned));
  CuAssertIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, rmf_extended_file_info_signature_type(cloned));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, rmf_extended_file_info_signature_data(cloned), sizeof(dummy_sig)));
  rmf_extended_file_info_delete(cloned);

  // 3. Assign preserves signature
  rmf_extended_file_info_t assigned;
  rmf_extended_file_info_create(
    &assigned, 0u, 0u, 0u, NULL, RMF_FILE_TYPE_FIXED, APX_DATA_STATE_INIT, RMF_DIGEST_TYPE_NONE, NULL);
  rmf_extended_file_info_assign(&assigned, info);
  CuAssertTrue(tc, rmf_extended_file_info_is_signed(&assigned));
  CuAssertIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, rmf_extended_file_info_signature_type(&assigned));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, rmf_extended_file_info_signature_data(&assigned), sizeof(dummy_sig)));
  rmf_extended_file_info_destroy(&assigned);

  // 4. create_from_file_info preserves signature
  rmf_file_info_t *file_info =
    rmf_file_info_make_fixed_with_signature("SignedNode.apx", 512u, 0x1000u, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_sig);
  CuAssertPtrNotNull(tc, file_info);
  rmf_extended_file_info_t from_file;
  apx_error_t rc = rmf_extended_file_info_create_from_file_info(
    &from_file, 10u, file_info, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  CuAssertTrue(tc, rmf_extended_file_info_is_signed(&from_file));
  CuAssertIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, rmf_extended_file_info_signature_type(&from_file));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, rmf_extended_file_info_signature_data(&from_file), sizeof(dummy_sig)));

  rmf_extended_file_info_destroy(&from_file);
  rmf_file_info_delete(file_info);
  rmf_extended_file_info_delete(info);
}

static void test_encode_decode_publish_signed_remote_file(CuTest *tc)
{
  uint8_t buffer[256];
  uint8_t dummy_sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  for (size_t i = 0; i < sizeof(dummy_sig); i++)
  {
    dummy_sig[i] = (uint8_t)(0xAA ^ (uint8_t)i);
  }

  rmf_extended_file_info_t *original = rmf_extended_file_info_make_fixed_with_signature(
    "Motor.apx", 1024u, 0x8000u, 3u, APX_DATA_STATE_WAITING_FOR_FILE_DATA, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_sig);
  CuAssertPtrNotNull(tc, original);

  apx_size_t encoded_len = rmf_encode_publish_signed_remote_file_cmd(buffer, sizeof(buffer), original);
  CuAssertTrue(tc, encoded_len >= RMF_SIGNED_EXTENDED_FILE_INFO_HEADER_SIZE);

  rmf_extended_file_info_t decoded;
  apx_size_t decoded_len = rmf_decode_publish_signed_remote_file_cmd(buffer, encoded_len, &decoded);
  CuAssertUIntEquals(tc, encoded_len, decoded_len);
  CuAssertUIntEquals(tc, 3u, rmf_extended_file_info_connection_id(&decoded));
  CuAssertUIntEquals(tc, 0x8000u, rmf_extended_file_info_address(&decoded));
  CuAssertUIntEquals(tc, 1024u, rmf_extended_file_info_size(&decoded));
  CuAssertUIntEquals(tc, APX_DATA_STATE_WAITING_FOR_FILE_DATA, rmf_extended_file_info_data_state(&decoded));
  CuAssertStrEquals(tc, "Motor.apx", rmf_extended_file_info_name(&decoded));
  CuAssertTrue(tc, rmf_extended_file_info_is_signed(&decoded));
  CuAssertIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, rmf_extended_file_info_signature_type(&decoded));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, rmf_extended_file_info_signature_data(&decoded), sizeof(dummy_sig)));

  rmf_extended_file_info_destroy(&decoded);
  rmf_extended_file_info_delete(original);
}

static void test_extended_file_info_sha1_digest(CuTest *tc)
{
  uint8_t buffer[256];
  uint8_t dummy_sha1[RMF_SHA1_SIZE];
  for (size_t i = 0; i < sizeof(dummy_sha1); i++)
  {
    dummy_sha1[i] = (uint8_t)(i + 0x10);
  }

  rmf_extended_file_info_t *info = rmf_extended_file_info_make_fixed_with_digest("LegacyNode.apx", 256u, 0x2000u, 7u,
    APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, RMF_DIGEST_TYPE_SHA1, dummy_sha1);
  CuAssertPtrNotNull(tc, info);
  CuAssertIntEquals(tc, RMF_DIGEST_TYPE_SHA1, rmf_extended_file_info_digest_type(info));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sha1, rmf_extended_file_info_digest_data(info), RMF_SHA1_SIZE));

  apx_size_t encoded_len = rmf_encode_publish_remote_file_cmd(buffer, sizeof(buffer), info);
  CuAssertTrue(tc, encoded_len >= RMF_EXTENDED_FILE_INFO_HEADER_SIZE);

  rmf_extended_file_info_t decoded;
  apx_size_t decoded_len = rmf_decode_publish_remote_file_cmd(buffer, encoded_len, &decoded);
  CuAssertUIntEquals(tc, encoded_len, decoded_len);
  CuAssertUIntEquals(tc, 7u, rmf_extended_file_info_connection_id(&decoded));
  CuAssertIntEquals(tc, RMF_DIGEST_TYPE_SHA1, rmf_extended_file_info_digest_type(&decoded));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sha1, rmf_extended_file_info_digest_data(&decoded), RMF_SHA1_SIZE));

  rmf_extended_file_info_destroy(&decoded);
  rmf_extended_file_info_delete(info);
}
