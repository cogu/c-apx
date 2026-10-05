/*****************************************************************************
 * \file      testsuite_crypto.c
 * \author    Conny Gustafsson
 * \date      2026-09-26
 * \brief     Unit tests for APX cryptographic signing and verification
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
static void test_sha256(CuTest *tc);
static void test_keypair_generation(CuTest *tc);
static void test_sign_and_verify_roundtrip(CuTest *tc);
static void test_verify_tampered_data_fails(CuTest *tc);
static void test_verify_tampered_signature_fails(CuTest *tc);
static void test_verify_wrong_key_fails(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

CuSuite *testsuite_crypto(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_sha256);
  SUITE_ADD_TEST(suite, test_keypair_generation);
  SUITE_ADD_TEST(suite, test_sign_and_verify_roundtrip);
  SUITE_ADD_TEST(suite, test_verify_tampered_data_fails);
  SUITE_ADD_TEST(suite, test_verify_tampered_signature_fails);
  SUITE_ADD_TEST(suite, test_verify_wrong_key_fails);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_sha256(CuTest *tc)
{
   // SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
  static const uint8_t expected_empty[32] = {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8,
    0x99, 0x6f, 0xb9, 0x24, 0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8,
    0x55};
  uint8_t hash[32];
  apx_error_t rc = apx_crypto_sha256(NULL, 0, hash);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  CuAssertIntEquals(tc, 0, memcmp(expected_empty, hash, 32));

   // SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
  static const uint8_t expected_abc[32] = {0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d,
    0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
  rc = apx_crypto_sha256((const uint8_t *)"abc", 3, hash);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  CuAssertIntEquals(tc, 0, memcmp(expected_abc, hash, 32));
}

static void test_keypair_generation(CuTest *tc)
{
  char *priv_pem = NULL;
  char *pub_pem = NULL;

  apx_error_t rc = apx_crypto_generate_keypair_pem(&priv_pem, &pub_pem);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  CuAssertPtrNotNull(tc, priv_pem);
  CuAssertPtrNotNull(tc, pub_pem);

  CuAssertTrue(tc, strstr(priv_pem, "-----BEGIN") != NULL);
  CuAssertTrue(tc, strstr(pub_pem, "-----BEGIN") != NULL);

  free(priv_pem);
  free(pub_pem);
}

static void test_sign_and_verify_roundtrip(CuTest *tc)
{
  char *priv_pem = NULL;
  char *pub_pem = NULL;
  apx_error_t rc = apx_crypto_generate_keypair_pem(&priv_pem, &pub_pem);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);

  const char *apx_def = "APX/1.2\nN\"EngineNode\"\nP\"VehicleSpeed\"S:=0\n";
  size_t apx_def_len = strlen(apx_def);

  uint8_t sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  size_t sig_len = 0;

  rc = apx_crypto_sign_data(RMF_SIGNATURE_TYPE_ECDSA_P256, (const uint8_t *)priv_pem, strlen(priv_pem) + 1,
    (const uint8_t *)apx_def, apx_def_len, sig, sizeof(sig), &sig_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);
  CuAssertUIntEquals(tc, RMF_SIGNATURE_SIZE_ECDSA_P256, (uint32_t)sig_len);

  rc = apx_crypto_verify_signature(RMF_SIGNATURE_TYPE_ECDSA_P256, (const uint8_t *)pub_pem, strlen(pub_pem) + 1,
    (const uint8_t *)apx_def, apx_def_len, sig, sig_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, rc);

  free(priv_pem);
  free(pub_pem);
}

static void test_verify_tampered_data_fails(CuTest *tc)
{
  char *priv_pem = NULL;
  char *pub_pem = NULL;
  apx_crypto_generate_keypair_pem(&priv_pem, &pub_pem);

  char apx_def[] = "APX/1.2\nN\"EngineNode\"\nP\"VehicleSpeed\"S:=0\n";
  size_t apx_def_len = strlen(apx_def);

  uint8_t sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  size_t sig_len = 0;

  apx_crypto_sign_data(RMF_SIGNATURE_TYPE_ECDSA_P256, (const uint8_t *)priv_pem, strlen(priv_pem) + 1,
    (const uint8_t *)apx_def, apx_def_len, sig, sizeof(sig), &sig_len);

   // Tamper payload data
  apx_def[10] = 'X';

  apx_error_t rc = apx_crypto_verify_signature(RMF_SIGNATURE_TYPE_ECDSA_P256, (const uint8_t *)pub_pem,
    strlen(pub_pem) + 1, (const uint8_t *)apx_def, apx_def_len, sig, sig_len);
  CuAssertIntEquals(tc, APX_SIGNATURE_VERIFICATION_ERROR, rc);

  free(priv_pem);
  free(pub_pem);
}

static void test_verify_tampered_signature_fails(CuTest *tc)
{
  char *priv_pem = NULL;
  char *pub_pem = NULL;
  apx_crypto_generate_keypair_pem(&priv_pem, &pub_pem);

  const char *apx_def = "APX/1.2\nN\"EngineNode\"\nP\"VehicleSpeed\"S:=0\n";
  size_t apx_def_len = strlen(apx_def);

  uint8_t sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  size_t sig_len = 0;

  apx_crypto_sign_data(RMF_SIGNATURE_TYPE_ECDSA_P256, (const uint8_t *)priv_pem, strlen(priv_pem) + 1,
    (const uint8_t *)apx_def, apx_def_len, sig, sizeof(sig), &sig_len);

   // Tamper 1 byte in signature
  sig[5] ^= 0xFF;

  apx_error_t rc = apx_crypto_verify_signature(RMF_SIGNATURE_TYPE_ECDSA_P256, (const uint8_t *)pub_pem,
    strlen(pub_pem) + 1, (const uint8_t *)apx_def, apx_def_len, sig, sig_len);
  CuAssertIntEquals(tc, APX_SIGNATURE_VERIFICATION_ERROR, rc);

  free(priv_pem);
  free(pub_pem);
}

static void test_verify_wrong_key_fails(CuTest *tc)
{
  char *priv_pem1 = NULL;
  char *pub_pem1 = NULL;
  char *priv_pem2 = NULL;
  char *pub_pem2 = NULL;

  apx_crypto_generate_keypair_pem(&priv_pem1, &pub_pem1);
  apx_crypto_generate_keypair_pem(&priv_pem2, &pub_pem2);

  const char *apx_def = "APX/1.2\nN\"EngineNode\"\nP\"VehicleSpeed\"S:=0\n";
  size_t apx_def_len = strlen(apx_def);

  uint8_t sig[RMF_SIGNATURE_SIZE_ECDSA_P256];
  size_t sig_len = 0;

   // Sign with key 1
  apx_crypto_sign_data(RMF_SIGNATURE_TYPE_ECDSA_P256, (const uint8_t *)priv_pem1, strlen(priv_pem1) + 1,
    (const uint8_t *)apx_def, apx_def_len, sig, sizeof(sig), &sig_len);

   // Verify with key 2 public key
  apx_error_t rc = apx_crypto_verify_signature(RMF_SIGNATURE_TYPE_ECDSA_P256, (const uint8_t *)pub_pem2,
    strlen(pub_pem2) + 1, (const uint8_t *)apx_def, apx_def_len, sig, sig_len);
  CuAssertIntEquals(tc, APX_SIGNATURE_VERIFICATION_ERROR, rc);

  free(priv_pem1);
  free(pub_pem1);
  free(priv_pem2);
  free(pub_pem2);
}
