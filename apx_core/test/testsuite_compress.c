/*****************************************************************************
 * \file      testsuite_compress.c
 * \author    Conny Gustafsson
 * \date      2026-11-04
 * \brief     Unit tests for APX compression and decompression codec
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/compress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char *m_sample_apx_text = "APX/1.3\n"
                                       "N\"EngineSpeedNode\"\n"
                                       "T\"EngineSpeed_T\"S\n"
                                       "P\"EngineSpeed\"T[0]:=0\n"
                                       "P\"VehicleSpeed\"S:=0\n";

// Golden vector generated with Python standard library zlib.compress()
// Input: "APX/1.3\nN\"GoldenNode\"\nP\"Speed\"S:=0\n" (35 bytes)
static const uint8_t m_golden_compressed[43] = {0x78, 0x9c, 0x73, 0x0c, 0x88, 0xd0, 0x37, 0xd4, 0x33, 0xe6, 0xf2, 0x53,
  0x72, 0xcf, 0xcf, 0x49, 0x49, 0xcd, 0xf3, 0xcb, 0x4f, 0x49, 0x55, 0xe2, 0x0a, 0x50, 0x0a, 0x2e, 0x48, 0x4d, 0x4d,
  0x51, 0x0a, 0xb6, 0xb2, 0x35, 0xe0, 0x02, 0x00, 0xaf, 0xd6, 0x09, 0xb9};
static const char *m_golden_expected_text = "APX/1.3\nN\"GoldenNode\"\nP\"Speed\"S:=0\n";

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_compress_backend_name(CuTest *tc);
static void test_compress_bound(CuTest *tc);
static void test_compress_invalid_args(CuTest *tc);
static void test_decompress_invalid_args(CuTest *tc);
static void test_compress_decompress_empty(CuTest *tc);
static void test_compress_decompress_small_text(CuTest *tc);
static void test_compress_decompress_apx_definition(CuTest *tc);
static void test_compress_decompress_repetitive(CuTest *tc);
static void test_compress_decompress_binary_with_zeros(CuTest *tc);
static void test_compress_insufficient_output_buffer(CuTest *tc);
static void test_decompress_insufficient_output_buffer(CuTest *tc);
static void test_decompress_bounded_max_output_size(CuTest *tc);
static void test_decompress_truncated_stream(CuTest *tc);
static void test_decompress_trailing_garbage(CuTest *tc);
static void test_decompress_corrupted_header(CuTest *tc);
static void test_decompress_corrupted_payload(CuTest *tc);
static void test_decompress_corrupted_adler32(CuTest *tc);
static void test_decompress_golden_vector(CuTest *tc);
static void test_compress_levels(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

CuSuite *testsuite_apx_compress(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_compress_backend_name);
  SUITE_ADD_TEST(suite, test_compress_bound);
  SUITE_ADD_TEST(suite, test_compress_invalid_args);
  SUITE_ADD_TEST(suite, test_decompress_invalid_args);
  SUITE_ADD_TEST(suite, test_compress_decompress_empty);
  SUITE_ADD_TEST(suite, test_compress_decompress_small_text);
  SUITE_ADD_TEST(suite, test_compress_decompress_apx_definition);
  SUITE_ADD_TEST(suite, test_compress_decompress_repetitive);
  SUITE_ADD_TEST(suite, test_compress_decompress_binary_with_zeros);
  SUITE_ADD_TEST(suite, test_compress_insufficient_output_buffer);
  SUITE_ADD_TEST(suite, test_decompress_insufficient_output_buffer);
  SUITE_ADD_TEST(suite, test_decompress_bounded_max_output_size);
  SUITE_ADD_TEST(suite, test_decompress_truncated_stream);
  SUITE_ADD_TEST(suite, test_decompress_trailing_garbage);
  SUITE_ADD_TEST(suite, test_decompress_corrupted_header);
  SUITE_ADD_TEST(suite, test_decompress_corrupted_payload);
  SUITE_ADD_TEST(suite, test_decompress_corrupted_adler32);
  SUITE_ADD_TEST(suite, test_decompress_golden_vector);
  SUITE_ADD_TEST(suite, test_compress_levels);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_compress_backend_name(CuTest *tc)
{
  const char *name = apx_compress_backend_name();
  CuAssertPtrNotNull(tc, name);
  int is_valid = (strcmp(name, "zlib") == 0) || (strcmp(name, "miniz") == 0);
  CuAssertIntEquals(tc, 1, is_valid);
}

static void test_compress_bound(CuTest *tc)
{
  size_t b0 = apx_compress_bound(0);
  size_t b100 = apx_compress_bound(100);
  size_t b1000 = apx_compress_bound(1000);

  CuAssertTrue(tc, b0 >= 6);
  CuAssertTrue(tc, b100 > 100);
  CuAssertTrue(tc, b1000 > 1000);
  CuAssertTrue(tc, b1000 > b100);
}

static void test_compress_invalid_args(CuTest *tc)
{
  uint8_t dst[64];
  size_t dst_len = sizeof(dst);
  uint8_t src[16] = {1, 2, 3};

  // NULL dst
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_compress(NULL, &dst_len, src, sizeof(src)));

  // NULL dst_len
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_compress(dst, NULL, src, sizeof(src)));

  // NULL src with src_len > 0
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_compress(dst, &dst_len, NULL, sizeof(src)));

  // Invalid levels
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_compress_level(dst, &dst_len, src, sizeof(src), -2));
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_compress_level(dst, &dst_len, src, sizeof(src), 10));
}

static void test_decompress_invalid_args(CuTest *tc)
{
  uint8_t dst[64];
  size_t dst_len = sizeof(dst);
  uint8_t src[16] = {0x78, 0x01, 0x01, 0x00, 0x00, 0xff};

  // NULL dst_len
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_decompress(dst, NULL, src, sizeof(src), 0));

  // NULL dst when *dst_len > 0
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_decompress(NULL, &dst_len, src, sizeof(src), 0));

  // NULL src with src_len > 0
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_decompress(dst, &dst_len, NULL, sizeof(src), 0));
}

static void test_compress_decompress_empty(CuTest *tc)
{
  uint8_t comp_buf[64];
  size_t comp_len = sizeof(comp_buf);

  // Compress empty input (0 bytes)
  apx_error_t err = apx_compress(comp_buf, &comp_len, NULL, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertTrue(tc, comp_len >= 6); // zlib header + Adler32
  CuAssertIntEquals(tc, 0x78, comp_buf[0]); // CMF: deflate

  // Decompress back
  uint8_t decomp_buf[64];
  size_t decomp_len = sizeof(decomp_buf);
  err = apx_decompress(decomp_buf, &decomp_len, comp_buf, comp_len, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertIntEquals(tc, 0, (int)decomp_len);
}

static void test_compress_decompress_small_text(CuTest *tc)
{
  const char *msg = "Hello, APX RemoteFile!";
  size_t msg_len = strlen(msg);

  uint8_t comp_buf[128];
  size_t comp_len = sizeof(comp_buf);

  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)msg, msg_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertTrue(tc, comp_len > 0);

  uint8_t decomp_buf[128];
  size_t decomp_len = sizeof(decomp_buf);
  err = apx_decompress(decomp_buf, &decomp_len, comp_buf, comp_len, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertIntEquals(tc, (int)msg_len, (int)decomp_len);
  CuAssertIntEquals(tc, 0, memcmp(msg, decomp_buf, msg_len));
}

static void test_compress_decompress_apx_definition(CuTest *tc)
{
  size_t orig_len = strlen(m_sample_apx_text);
  size_t bound = apx_compress_bound(orig_len);
  uint8_t *comp_buf = (uint8_t *)malloc(bound);
  CuAssertPtrNotNull(tc, comp_buf);

  size_t comp_len = bound;
  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)m_sample_apx_text, orig_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertTrue(tc, comp_len < bound);

  uint8_t *decomp_buf = (uint8_t *)malloc(orig_len + 16);
  CuAssertPtrNotNull(tc, decomp_buf);
  size_t decomp_len = orig_len + 16;

  err = apx_decompress(decomp_buf, &decomp_len, comp_buf, comp_len, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertIntEquals(tc, (int)orig_len, (int)decomp_len);
  CuAssertIntEquals(tc, 0, memcmp(m_sample_apx_text, decomp_buf, orig_len));

  free(comp_buf);
  free(decomp_buf);
}

static void test_compress_decompress_repetitive(CuTest *tc)
{
  size_t orig_len = 8192;
  uint8_t *orig_buf = (uint8_t *)malloc(orig_len);
  CuAssertPtrNotNull(tc, orig_buf);
  memset(orig_buf, 'A', orig_len);

  size_t bound = apx_compress_bound(orig_len);
  uint8_t *comp_buf = (uint8_t *)malloc(bound);
  CuAssertPtrNotNull(tc, comp_buf);
  size_t comp_len = bound;

  apx_error_t err = apx_compress(comp_buf, &comp_len, orig_buf, orig_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  // Repetitive data should compress drastically (less than 100 bytes)
  CuAssertTrue(tc, comp_len < 100);

  uint8_t *decomp_buf = (uint8_t *)malloc(orig_len);
  CuAssertPtrNotNull(tc, decomp_buf);
  size_t decomp_len = orig_len;

  err = apx_decompress(decomp_buf, &decomp_len, comp_buf, comp_len, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertIntEquals(tc, (int)orig_len, (int)decomp_len);
  CuAssertIntEquals(tc, 0, memcmp(orig_buf, decomp_buf, orig_len));

  free(orig_buf);
  free(comp_buf);
  free(decomp_buf);
}

static void test_compress_decompress_binary_with_zeros(CuTest *tc)
{
  uint8_t binary_data[32] = {0, 1, 0, 2, 0, 3, 255, 0, 128, 64, 0, 0, 0, 42};
  size_t orig_len = sizeof(binary_data);

  uint8_t comp_buf[128];
  size_t comp_len = sizeof(comp_buf);

  apx_error_t err = apx_compress(comp_buf, &comp_len, binary_data, orig_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  uint8_t decomp_buf[128];
  size_t decomp_len = sizeof(decomp_buf);

  err = apx_decompress(decomp_buf, &decomp_len, comp_buf, comp_len, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertIntEquals(tc, (int)orig_len, (int)decomp_len);
  CuAssertIntEquals(tc, 0, memcmp(binary_data, decomp_buf, orig_len));
}

static void test_compress_insufficient_output_buffer(CuTest *tc)
{
  const char *msg = "This string cannot fit in 2 compressed bytes";
  uint8_t comp_buf[2];
  size_t comp_len = sizeof(comp_buf);

  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)msg, strlen(msg));
  CuAssertIntEquals(tc, APX_BUFFER_FULL_ERROR, err);
}

static void test_decompress_insufficient_output_buffer(CuTest *tc)
{
  const char *msg = "A moderately long test message to verify buffer full on decompression";
  size_t msg_len = strlen(msg);

  uint8_t comp_buf[128];
  size_t comp_len = sizeof(comp_buf);
  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)msg, msg_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  // Provide only 10 bytes of destination buffer
  uint8_t small_dst[10];
  size_t small_len = sizeof(small_dst);
  err = apx_decompress(small_dst, &small_len, comp_buf, comp_len, 0);
  CuAssertIntEquals(tc, APX_BUFFER_FULL_ERROR, err);
}

static void test_decompress_bounded_max_output_size(CuTest *tc)
{
  const char *msg = "Bounded decompression protection against zip bombs";
  size_t msg_len = strlen(msg); // 50 bytes

  uint8_t comp_buf[128];
  size_t comp_len = sizeof(comp_buf);
  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)msg, msg_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  uint8_t dst[128];
  size_t dst_len = sizeof(dst);

  // max_output_size smaller than uncompressed length (e.g. 20 < 50)
  err = apx_decompress(dst, &dst_len, comp_buf, comp_len, 20);
  CuAssertIntEquals(tc, APX_LENGTH_ERROR, err);

  // max_output_size exactly equal to uncompressed length
  dst_len = sizeof(dst);
  err = apx_decompress(dst, &dst_len, comp_buf, comp_len, msg_len);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertIntEquals(tc, (int)msg_len, (int)dst_len);
  CuAssertIntEquals(tc, 0, memcmp(msg, dst, msg_len));

  // max_output_size larger than uncompressed length
  dst_len = sizeof(dst);
  err = apx_decompress(dst, &dst_len, comp_buf, comp_len, msg_len + 50);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertIntEquals(tc, (int)msg_len, (int)dst_len);
}

static void test_decompress_truncated_stream(CuTest *tc)
{
  const char *msg = "Truncation test for compressed streams";
  uint8_t comp_buf[128];
  size_t comp_len = sizeof(comp_buf);
  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)msg, strlen(msg));
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  uint8_t dst[128];
  size_t dst_len = sizeof(dst);

  // Cut off the last 4 bytes (Adler-32 checksum)
  err = apx_decompress(dst, &dst_len, comp_buf, comp_len - 4, 0);
  CuAssertTrue(tc, (err == APX_UNEXPECTED_END_ERROR) || (err == APX_DECOMPRESSION_ERROR));

  // Cut off significantly more (halfway through DEFLATE stream)
  dst_len = sizeof(dst);
  err = apx_decompress(dst, &dst_len, comp_buf, comp_len / 2, 0);
  CuAssertTrue(tc, (err == APX_UNEXPECTED_END_ERROR) || (err == APX_DECOMPRESSION_ERROR));
}

static void test_decompress_trailing_garbage(CuTest *tc)
{
  const char *msg = "Checking rejection of trailing concatenated data";
  uint8_t comp_buf[128];
  size_t comp_len = sizeof(comp_buf) - 8;
  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)msg, strlen(msg));
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  // Append 4 trailing garbage bytes
  comp_buf[comp_len] = 0xDE;
  comp_buf[comp_len + 1] = 0xAD;
  comp_buf[comp_len + 2] = 0xBE;
  comp_buf[comp_len + 3] = 0xEF;

  uint8_t dst[128];
  size_t dst_len = sizeof(dst);
  err = apx_decompress(dst, &dst_len, comp_buf, comp_len + 4, 0);
  CuAssertIntEquals(tc, APX_UNEXPECTED_DATA_ERROR, err);
}

static void test_decompress_corrupted_header(CuTest *tc)
{
  uint8_t valid_stream[16];
  size_t comp_len = sizeof(valid_stream);
  apx_error_t err = apx_compress(valid_stream, &comp_len, NULL, 0);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  uint8_t dst[64];
  size_t dst_len = sizeof(dst);

  // Case 1: Corrupted CMF (compression method != 8)
  uint8_t bad_cmf[16];
  memcpy(bad_cmf, valid_stream, comp_len);
  bad_cmf[0] = 0x79; // method 9
  err = apx_decompress(dst, &dst_len, bad_cmf, comp_len, 0);
  CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR, err);

  // Case 2: Corrupted CINFO (window size > 7)
  uint8_t bad_cinfo[16];
  memcpy(bad_cinfo, valid_stream, comp_len);
  bad_cinfo[0] = 0x88; // CINFO = 8
  err = apx_decompress(dst, &dst_len, bad_cinfo, comp_len, 0);
  CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR, err);

  // Case 3: Header checksum mismatch ((CMF*256 + FLG) % 31 != 0)
  uint8_t bad_chk[16];
  memcpy(bad_chk, valid_stream, comp_len);
  bad_chk[1] ^= 0x01;
  err = apx_decompress(dst, &dst_len, bad_chk, comp_len, 0);
  CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR, err);

  // Case 4: Gzip magic header (0x1f, 0x8b)
  uint8_t gzip_header[16] = {0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};
  dst_len = sizeof(dst);
  err = apx_decompress(dst, &dst_len, gzip_header, sizeof(gzip_header), 0);
  CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR, err);

  // Case 5: Preset dictionary bit (FDICT, bit 5) set
  // Compute valid FLG with bit 5 set such that (CMF * 256 + FLG) % 31 == 0
  uint8_t fdict_header[16];
  memcpy(fdict_header, valid_stream, comp_len);
  uint8_t cmf = 0x78;
  uint16_t base = (uint16_t)cmf * 256u;
  // We want bit 5 set: FLG has form 0x20 | fcheck
  uint8_t flg = 0x20;
  uint8_t rem = (uint8_t)((base + flg) % 31u);
  if (rem != 0)
  {
    flg += (31u - rem);
  }
  fdict_header[0] = cmf;
  fdict_header[1] = flg;
  dst_len = sizeof(dst);
  err = apx_decompress(dst, &dst_len, fdict_header, comp_len, 0);
  CuAssertIntEquals(tc, APX_INVALID_HEADER_ERROR, err);
}

static void test_decompress_corrupted_payload(CuTest *tc)
{
  const char *msg = "Testing payload corruption during decompression of zlib stream";
  uint8_t comp_buf[128];
  size_t comp_len = sizeof(comp_buf);
  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)msg, strlen(msg));
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  // Corrupt a byte in the payload area (byte 6)
  comp_buf[6] ^= 0xFF;

  uint8_t dst[128];
  size_t dst_len = sizeof(dst);
  err = apx_decompress(dst, &dst_len, comp_buf, comp_len, 0);
  CuAssertTrue(tc, (err == APX_DECOMPRESSION_ERROR) || (err == APX_CHECKSUM_ERROR));
}

static void test_decompress_corrupted_adler32(CuTest *tc)
{
  const char *msg = "Testing Adler32 checksum mismatch";
  uint8_t comp_buf[128];
  size_t comp_len = sizeof(comp_buf);
  apx_error_t err = apx_compress(comp_buf, &comp_len, (const uint8_t *)msg, strlen(msg));
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  // Corrupt the last byte (part of Adler-32)
  comp_buf[comp_len - 1] ^= 0x55;

  uint8_t dst[128];
  size_t dst_len = sizeof(dst);
  err = apx_decompress(dst, &dst_len, comp_buf, comp_len, 0);
  CuAssertTrue(tc, (err == APX_CHECKSUM_ERROR) || (err == APX_DECOMPRESSION_ERROR));
}

static void test_decompress_golden_vector(CuTest *tc)
{
  uint8_t dst[128];
  size_t dst_len = sizeof(dst);
  size_t expected_len = strlen(m_golden_expected_text);

  apx_error_t err = apx_decompress(dst, &dst_len, m_golden_compressed, sizeof(m_golden_compressed), 0);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);
  CuAssertIntEquals(tc, (int)expected_len, (int)dst_len);
  CuAssertIntEquals(tc, 0, memcmp(m_golden_expected_text, dst, expected_len));
}

static void test_compress_levels(CuTest *tc)
{
  const char *msg = "Testing compression levels 0, 1, 6, 9 to verify interoperability";
  size_t msg_len = strlen(msg);
  int levels[] = {0, 1, 6, 9};

  for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); i++)
  {
    uint8_t comp[256];
    size_t comp_len = sizeof(comp);

    apx_error_t err = apx_compress_level(comp, &comp_len, (const uint8_t *)msg, msg_len, levels[i]);
    CuAssertIntEquals(tc, APX_NO_ERROR, err);

    uint8_t decomp[256];
    size_t decomp_len = sizeof(decomp);
    err = apx_decompress(decomp, &decomp_len, comp, comp_len, 0);
    CuAssertIntEquals(tc, APX_NO_ERROR, err);
    CuAssertIntEquals(tc, (int)msg_len, (int)decomp_len);
    CuAssertIntEquals(tc, 0, memcmp(msg, decomp, msg_len));
  }
}
