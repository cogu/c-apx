/*****************************************************************************
 * \file      testsuite_file_info.c
 * \author    Conny Gustafsson
 * \date      2021-02-04
 * \brief     Unit tests for file info
 *
 * Copyright (c) 2021-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/file_info.h"
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
static void test_create_local_file(CuTest *tc);
static void test_create_signed_file(CuTest *tc);
static void test_name_ends_with(CuTest *tc);
static void test_base_name(CuTest *tc);
static void test_encode_decode_publish_file(CuTest *tc);
static void test_encode_decode_publish_signed_file(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite *testsuite_file_info(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_create_local_file);
  SUITE_ADD_TEST(suite, test_create_signed_file);
  SUITE_ADD_TEST(suite, test_name_ends_with);
  SUITE_ADD_TEST(suite, test_base_name);
  SUITE_ADD_TEST(suite, test_encode_decode_publish_file);
  SUITE_ADD_TEST(suite, test_encode_decode_publish_signed_file);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_local_file(CuTest *tc)
{
  rmf_file_info_t *info = rmf_file_info_make_fixed("TestNode.apx", 40, RMF_INVALID_ADDRESS);
  CuAssertPtrNotNull(tc, info);
  CuAssertStrEquals(tc, "TestNode.apx", rmf_file_info_name(info));
  CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, rmf_file_info_rmf_file_type(info));
  CuAssertUIntEquals(tc, 40u, rmf_file_info_size(info));
  CuAssertUIntEquals(tc, RMF_INVALID_ADDRESS, rmf_file_info_address(info));
  CuAssertUIntEquals(tc, RMF_DIGEST_TYPE_NONE, rmf_file_info_digest_type(info));
  rmf_file_info_delete(info);
}

static void test_name_ends_with(CuTest *tc)
{
  rmf_file_info_t *info1 = rmf_file_info_make_fixed("TestNode.apx", 40, RMF_INVALID_ADDRESS);
  CuAssertTrue(tc, rmf_file_info_name_ends_with(info1, ".apx"));
  CuAssertFalse(tc, rmf_file_info_name_ends_with(info1, ".out"));
  CuAssertFalse(tc, rmf_file_info_name_ends_with(info1, ".in"));
  rmf_file_info_delete(info1);

  rmf_file_info_t *info2 = rmf_file_info_make_fixed("TestNode.out", 1, RMF_INVALID_ADDRESS);
  CuAssertFalse(tc, rmf_file_info_name_ends_with(info2, ".apx"));
  CuAssertTrue(tc, rmf_file_info_name_ends_with(info2, ".out"));
  CuAssertFalse(tc, rmf_file_info_name_ends_with(info2, ".in"));
  rmf_file_info_delete(info2);

  rmf_file_info_t *info3 = rmf_file_info_make_fixed("TestNode.in", 1, RMF_INVALID_ADDRESS);
  CuAssertFalse(tc, rmf_file_info_name_ends_with(info3, ".apx"));
  CuAssertFalse(tc, rmf_file_info_name_ends_with(info3, ".out"));
  CuAssertTrue(tc, rmf_file_info_name_ends_with(info3, ".in"));
  rmf_file_info_delete(info3);
}


static void test_base_name(CuTest *tc)
{
  rmf_file_info_t *info1 = rmf_file_info_make_fixed("TestNode.apx", 40, RMF_INVALID_ADDRESS);
  rmf_file_info_t *info2 = rmf_file_info_make_fixed("TestNode.out", 1, RMF_INVALID_ADDRESS);
  rmf_file_info_t *info3 = rmf_file_info_make_fixed("TestNode.in", 1, RMF_INVALID_ADDRESS);
  char *base_name1 = rmf_file_info_base_name(info1);
  char *base_name2 = rmf_file_info_base_name(info2);
  char *base_name3 = rmf_file_info_base_name(info3);
  CuAssertStrEquals(tc, "TestNode", base_name1);
  CuAssertStrEquals(tc, "TestNode", base_name2);
  CuAssertStrEquals(tc, "TestNode", base_name3);
  free(base_name1);
  free(base_name2);
  free(base_name3);
  rmf_file_info_delete(info1);
  rmf_file_info_delete(info2);
  rmf_file_info_delete(info3);
}

static void test_create_signed_file(CuTest *tc)
{
  uint8_t dummy_sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  uint32_t i;
  for (i = 0; i < RMF_SIGNATURE_SIZE_ECDSA_P256; i++)
  {
    dummy_sig[i] = (uint8_t)(i + 1u);
  }
  rmf_file_info_t *info =
    rmf_file_info_make_fixed_with_signature("TestNode.apx", 40, 0x1000u, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_sig);
  CuAssertPtrNotNull(tc, info);
  CuAssertStrEquals(tc, "TestNode.apx", rmf_file_info_name(info));
  CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, rmf_file_info_rmf_file_type(info));
  CuAssertUIntEquals(tc, 40u, rmf_file_info_size(info));
  CuAssertUIntEquals(tc, 0x1000u, rmf_file_info_address(info));
  CuAssertUIntEquals(tc, RMF_DIGEST_TYPE_NONE, rmf_file_info_digest_type(info));
  CuAssertTrue(tc, rmf_file_info_is_signed(info));
  CuAssertUIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, rmf_file_info_signature_type(info));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, rmf_file_info_signature_data(info), RMF_SIGNATURE_SIZE_ECDSA_P256));

  rmf_file_info_t *clone = rmf_file_info_clone(info);
  CuAssertPtrNotNull(tc, clone);
  CuAssertTrue(tc, rmf_file_info_is_signed(clone));
  CuAssertUIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, rmf_file_info_signature_type(clone));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, rmf_file_info_signature_data(clone), RMF_SIGNATURE_SIZE_ECDSA_P256));

  rmf_file_info_delete(clone);
  rmf_file_info_delete(info);
}

static void test_encode_decode_publish_file(CuTest *tc)
{
  uint8_t buf[256];
  rmf_file_info_t *src_info = rmf_file_info_make_fixed("MyNode.apx", 128, 0x2000u);
  CuAssertPtrNotNull(tc, src_info);
  apx_size_t encoded_size = rmf_encode_publish_file_cmd(buf, (apx_size_t)sizeof(buf), src_info);
  CuAssertTrue(tc, encoded_size > 0);

  rmf_file_info_t *dst_info = rmf_file_info_make_empty();
  CuAssertPtrNotNull(tc, dst_info);
  apx_size_t decoded_size = rmf_decode_publish_file_cmd(buf, encoded_size, dst_info);
  CuAssertTrue(tc, decoded_size > 0);
  CuAssertUIntEquals(tc, 0x2000u, rmf_file_info_address(dst_info));
  CuAssertUIntEquals(tc, 128u, rmf_file_info_size(dst_info));
  CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, rmf_file_info_rmf_file_type(dst_info));
  CuAssertStrEquals(tc, "MyNode.apx", rmf_file_info_name(dst_info));
  CuAssertFalse(tc, rmf_file_info_is_signed(dst_info));

   // Attempting to decode standard publish file with decode_publish_signed_file_cmd should fail
  rmf_file_info_t *invalid_info = rmf_file_info_make_empty();
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_signed_file_cmd(buf, encoded_size, invalid_info));
  rmf_file_info_delete(invalid_info);

  rmf_file_info_delete(dst_info);
  rmf_file_info_delete(src_info);
}

static void test_encode_decode_publish_signed_file(CuTest *tc)
{
  uint8_t dummy_sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  uint8_t buf[256];
  uint32_t i;
  for (i = 0; i < RMF_SIGNATURE_SIZE_ECDSA_P256; i++)
  {
    dummy_sig[i] = (uint8_t)(0xA0u + (i & 0x0Fu));
  }
  rmf_file_info_t *src_info =
    rmf_file_info_make_fixed_with_signature("SecureNode.apx", 512, 0x4000u, RMF_SIGNATURE_TYPE_ECDSA_P256, dummy_sig);
  CuAssertPtrNotNull(tc, src_info);

  apx_size_t encoded_size = rmf_encode_publish_signed_file_cmd(buf, (apx_size_t)sizeof(buf), src_info);
  CuAssertTrue(tc, encoded_size == RMF_SIGNED_FILE_INFO_HEADER_SIZE + strlen("SecureNode.apx") + 1u);

   // Verify opcode in header
  uint32_t cmd_type = unpackLE(buf, UINT32_SIZE);
  CuAssertUIntEquals(tc, RMF_CMD_PUBLISH_SIGNED_FILE_MSG, cmd_type);

  rmf_file_info_t *dst_info = rmf_file_info_make_empty();
  CuAssertPtrNotNull(tc, dst_info);
  apx_size_t decoded_size = rmf_decode_publish_signed_file_cmd(buf, encoded_size, dst_info);
  CuAssertTrue(tc, decoded_size > 0);
  CuAssertUIntEquals(tc, 0x4000u, rmf_file_info_address(dst_info));
  CuAssertUIntEquals(tc, 512u, rmf_file_info_size(dst_info));
  CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, rmf_file_info_rmf_file_type(dst_info));
  CuAssertStrEquals(tc, "SecureNode.apx", rmf_file_info_name(dst_info));
  CuAssertTrue(tc, rmf_file_info_is_signed(dst_info));
  CuAssertUIntEquals(tc, RMF_SIGNATURE_TYPE_ECDSA_P256, rmf_file_info_signature_type(dst_info));
  CuAssertIntEquals(tc, 0, memcmp(dummy_sig, rmf_file_info_signature_data(dst_info), RMF_SIGNATURE_SIZE_ECDSA_P256));

   // Attempting to decode signed publish file with legacy decode_publish_file_cmd should fail
  rmf_file_info_t *invalid_info = rmf_file_info_make_empty();
  CuAssertUIntEquals(tc, 0u, rmf_decode_publish_file_cmd(buf, encoded_size, invalid_info));
  rmf_file_info_delete(invalid_info);

  rmf_file_info_delete(dst_info);
  rmf_file_info_delete(src_info);
}
