/*****************************************************************************
 * \file      testsuite_log_file.c
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     Unit tests for .apxlog container file reader and writer
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "CuTest.h"
#include "apx_log_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

static const char *s_test_log_file = "test_run.apxlog";

static void test_log_file_write_and_read(CuTest *tc)
{
  remove(s_test_log_file);

  apx_log_writer_t writer;
  apx_log_writer_create(&writer);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_writer_open(&writer, s_test_log_file));

  uint64_t const start_ns = 1700000000000000000ULL;
  uint64_t const end_ns = 1700000005000000000ULL;
  apx_log_writer_set_start_time(&writer, start_ns);

  // Record 1: 10 bytes (<= 127 bytes, 1-byte NumHeader32)
  uint8_t rec1[10];
  for (int i = 0; i < 10; ++i)
  {
    rec1[i] = (uint8_t)(i + 1);
  }
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_writer_write_record(&writer, rec1, sizeof(rec1)));

  // Record 2: 200 bytes (> 127 bytes, 4-byte NumHeader32)
  uint8_t rec2[200];
  for (int i = 0; i < 200; ++i)
  {
    rec2[i] = (uint8_t)(0xAA ^ (uint8_t)i);
  }
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_writer_write_record(&writer, rec2, sizeof(rec2)));

  // Record 3: 1 byte
  uint8_t rec3[1] = {0x42};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_writer_write_record(&writer, rec3, sizeof(rec3)));

  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_writer_finalize(&writer, end_ns));
  CuAssertUIntEquals(tc, 3u, (uint32_t)writer.header.record_count);
  CuAssertTrue(tc, (writer.header.flags & APX_LOG_FLAG_FINALIZED) != 0u);

  apx_log_writer_close(&writer);
  apx_log_writer_destroy(&writer);

  // Now read back with reader
  apx_log_reader_t reader;
  apx_log_reader_create(&reader);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_reader_open(&reader, s_test_log_file));

  CuAssertUIntEquals(tc, 1u, reader.header.format_major);
  CuAssertUIntEquals(tc, 0u, reader.header.format_minor);
  CuAssertUIntEquals(tc, 64u, reader.header.header_size);
  CuAssertTrue(tc, (reader.header.flags & APX_LOG_FLAG_FINALIZED) != 0u);
  CuAssertTrue(tc, reader.header.capture_start_unix_ns == start_ns);
  CuAssertTrue(tc, reader.header.capture_end_unix_ns == end_ns);
  CuAssertUIntEquals(tc, 3u, (uint32_t)reader.header.record_count);
  CuAssertUIntEquals(tc, 64u, (uint32_t)reader.header.data_offset);
  CuAssertTrue(tc, reader.file_size > 64u);

  // Read Record 1
  uint8_t *data = NULL;
  uint32_t size = 0u;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_reader_read_next_record(&reader, &data, &size));
  CuAssertPtrNotNull(tc, data);
  CuAssertUIntEquals(tc, 10u, size);
  CuAssertTrue(tc, memcmp(data, rec1, 10u) == 0);
  free(data);
  data = NULL;

  // Read Record 2
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_reader_read_next_record(&reader, &data, &size));
  CuAssertPtrNotNull(tc, data);
  CuAssertUIntEquals(tc, 200u, size);
  CuAssertTrue(tc, memcmp(data, rec2, 200u) == 0);
  free(data);
  data = NULL;

  // Read Record 3
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_reader_read_next_record(&reader, &data, &size));
  CuAssertPtrNotNull(tc, data);
  CuAssertUIntEquals(tc, 1u, size);
  CuAssertTrue(tc, memcmp(data, rec3, 1u) == 0);
  free(data);
  data = NULL;

  // Read EOF
  CuAssertIntEquals(tc, APX_LOG_EOF_ERROR, apx_log_reader_read_next_record(&reader, &data, &size));
  CuAssertTrue(tc, data == NULL);

  apx_log_reader_close(&reader);
  apx_log_reader_destroy(&reader);

  remove(s_test_log_file);
}

static void test_log_file_invalid(CuTest *tc)
{
  apx_log_reader_t reader;
  apx_log_reader_create(&reader);

  // Non-existent file
  CuAssertIntEquals(tc, APX_FILE_NOT_FOUND_ERROR, apx_log_reader_open(&reader, "nonexistent_file.apxlog"));

  // Create invalid magic file
  const char *bad_file = "test_bad.apxlog";
  FILE *f = fopen(bad_file, "wb");
  CuAssertPtrNotNull(tc, f);
  uint8_t junk[64];
  memset(junk, 0x55, sizeof(junk));
  fwrite(junk, 1, sizeof(junk), f);
  fclose(f);

  CuAssertIntEquals(tc, APX_INVALID_FILE_ERROR, apx_log_reader_open(&reader, bad_file));
  remove(bad_file);

  apx_log_reader_destroy(&reader);
}

CuSuite *testsuite_log_file(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_log_file_write_and_read);
  SUITE_ADD_TEST(suite, test_log_file_invalid);
  return suite;
}
