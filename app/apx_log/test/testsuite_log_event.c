/*****************************************************************************
 * \file      testsuite_log_event.c
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     Unit tests for .apxlog event parser and formatters
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "CuTest.h"
#include "apx_log_event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

static void pack_le_u16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8u) & 0xFFu);
}

static void pack_le_u32(uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8u) & 0xFFu);
  p[2] = (uint8_t)((v >> 16u) & 0xFFu);
  p[3] = (uint8_t)((v >> 24u) & 0xFFu);
}

static void pack_le_u64(uint8_t *p, uint64_t v)
{
  pack_le_u32(p, (uint32_t)(v & 0xFFFFFFFFu));
  pack_le_u32(p + 4, (uint32_t)((v >> 32u) & 0xFFFFFFFFu));
}

static void test_log_event_parse_header(CuTest *tc)
{
  uint8_t buf[APX_LOG_EVENT_HEADER_SIZE];
  pack_le_u16(buf + 0, APX_LOG_EVENT_SNAPSHOT_END);
  pack_le_u16(buf + 2, APX_LOG_FLAG_SNAPSHOT);
  pack_le_u64(buf + 4, 42ULL);
  pack_le_u64(buf + 12, 1000000ULL);
  pack_le_u32(buf + 20, 5u);
  pack_le_u32(buf + 24, 10u);

  apx_log_event_t event;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_event_parse(&event, buf, sizeof(buf)));
  CuAssertUIntEquals(tc, APX_LOG_EVENT_SNAPSHOT_END, event.event_type);
  CuAssertUIntEquals(tc, APX_LOG_FLAG_SNAPSHOT, event.event_flags);
  CuAssertTrue(tc, event.sequence == 42ULL);
  CuAssertTrue(tc, event.timestamp_ns == 1000000ULL);
  CuAssertUIntEquals(tc, 5u, event.connection_id);
  CuAssertUIntEquals(tc, 10u, event.object_id);
  CuAssertUIntEquals(tc, 0u, event.payload_size);
  CuAssertPtrEquals(tc, NULL, (void *)event.payload);

  // Short buffer returns error
  CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_log_event_parse(&event, buf, 10u));
}

static void test_log_event_type_str(CuTest *tc)
{
  CuAssertStrEquals(tc, "STREAM_BEGIN", apx_log_event_type_str(APX_LOG_EVENT_STREAM_BEGIN));
  CuAssertStrEquals(tc, "SNAPSHOT_END", apx_log_event_type_str(APX_LOG_EVENT_SNAPSHOT_END));
  CuAssertStrEquals(tc, "CONNECTION_CREATE", apx_log_event_type_str(APX_LOG_EVENT_CONNECTION_CREATE));
  CuAssertStrEquals(tc, "CONNECTION_CLOSE", apx_log_event_type_str(APX_LOG_EVENT_CONNECTION_CLOSE));
  CuAssertStrEquals(tc, "NODE_ATTACH", apx_log_event_type_str(APX_LOG_EVENT_NODE_ATTACH));
  CuAssertStrEquals(tc, "NODE_DETACH", apx_log_event_type_str(APX_LOG_EVENT_NODE_DETACH));
  CuAssertStrEquals(tc, "NODE_DEFINITION_REJECTED", apx_log_event_type_str(APX_LOG_EVENT_NODE_DEFINITION_REJECTED));
  CuAssertStrEquals(tc, "FILE_PUBLISH", apx_log_event_type_str(APX_LOG_EVENT_FILE_PUBLISH));
  CuAssertStrEquals(tc, "FILE_REVOKE", apx_log_event_type_str(APX_LOG_EVENT_FILE_REVOKE));
  CuAssertStrEquals(tc, "FILE_OPEN", apx_log_event_type_str(APX_LOG_EVENT_FILE_OPEN));
  CuAssertStrEquals(tc, "FILE_CLOSE", apx_log_event_type_str(APX_LOG_EVENT_FILE_CLOSE));
  CuAssertStrEquals(tc, "FILE_WRITE", apx_log_event_type_str(APX_LOG_EVENT_FILE_WRITE));
  CuAssertStrEquals(tc, "PROVIDER_CHANGE", apx_log_event_type_str(APX_LOG_EVENT_PROVIDER_CHANGE));
  CuAssertStrEquals(tc, "EVENTS_DROPPED", apx_log_event_type_str(APX_LOG_EVENT_EVENTS_DROPPED));
  CuAssertStrEquals(tc, "STREAM_END", apx_log_event_type_str(APX_LOG_EVENT_STREAM_END));
  CuAssertStrEquals(tc, "UNKNOWN", apx_log_event_type_str(0x1234u));
}

static void test_log_event_formatters(CuTest *tc)
{
  uint8_t buf[APX_LOG_EVENT_HEADER_SIZE + 4u];
  pack_le_u16(buf + 0, APX_LOG_EVENT_STREAM_END);
  pack_le_u16(buf + 2, 0u);
  pack_le_u64(buf + 4, 10ULL);
  pack_le_u64(buf + 12, 500000000ULL);
  pack_le_u32(buf + 20, 0u);
  pack_le_u32(buf + 24, 0u);
  pack_le_u32(buf + 28, 0u); // status = 0 (APX_NO_ERROR)

  apx_log_event_t event;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_event_parse(&event, buf, sizeof(buf)));

  // Test text print to a temporary file
  FILE *f = tmpfile();
  CuAssertPtrNotNull(tc, f);
  apx_log_event_print_text(&event, f);
  apx_log_event_print_json(&event, f);

  // Also test PROVIDER_CHANGE event formatting
  uint8_t pbuf[APX_LOG_EVENT_HEADER_SIZE + 32u];
  memset(pbuf, 0, sizeof(pbuf));
  pack_le_u16(pbuf + 0, APX_LOG_EVENT_PROVIDER_CHANGE);
  pack_le_u16(pbuf + 2, 0u);
  pack_le_u64(pbuf + 4, 11ULL);
  pack_le_u64(pbuf + 12, 600000000ULL);
  pack_le_u32(pbuf + 20, 2u); // new_connection_id
  pack_le_u32(pbuf + 24, 0u); // new_port_index
  uint8_t *pl = pbuf + APX_LOG_EVENT_HEADER_SIZE;
  pack_le_u32(pl + 0, 1u); // old_connection_id
  pack_le_u32(pl + 4, 0u); // old_port_index
  pl[8] = 1u;              // reason = TAKEOVER
  pl[9] = 0u;
  pack_le_u16(pl + 10, 2u);
  memcpy(pl + 12, "N1", 2);
  pack_le_u16(pl + 14, 2u);
  memcpy(pl + 16, "N2", 2);
  pack_le_u16(pl + 18, 2u);
  memcpy(pl + 20, "P1", 2);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_event_parse(&event, pbuf, APX_LOG_EVENT_HEADER_SIZE + 22u));
  apx_log_event_print_text(&event, f);
  apx_log_event_print_json(&event, f);
  fclose(f);
}

static void test_definition_rejection_format(CuTest *tc)
{
  uint8_t buf[APX_LOG_EVENT_HEADER_SIZE + 26u + 8u] = {0u};
  pack_le_u16(buf, APX_LOG_EVENT_NODE_DEFINITION_REJECTED);
  uint8_t *payload = buf + APX_LOG_EVENT_HEADER_SIZE;
  pack_le_u32(payload, 0x02000000u);
  pack_le_u32(payload + 4u, 100u);
  pack_le_u32(payload + 8u, 500u);
  pack_le_u16(payload + 12u, 1u);
  pack_le_u16(payload + 14u, 3u);
  pack_le_u32(payload + 16u, APX_CHECKSUM_ERROR);
  pack_le_u32(payload + 20u, APX_PARSE_ERROR);
  pack_le_u16(payload + 24u, 8u);
  uint8_t const name[8] = {'N', 'o', 'd', 'e', '.', 'a', 'p', 'x'};
  memcpy(payload + 26u, name, sizeof(name));
  apx_log_event_t event;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_log_event_parse(&event, buf, sizeof(buf)));
  FILE *out = tmpfile();
  CuAssertPtrNotNull(tc, out);
  apx_log_event_print_text(&event, out);
  apx_log_event_print_json(&event, out);
  CuAssertIntEquals(tc, 0, fseek(out, 0, SEEK_SET));
  char output[2048];
  size_t const length = fread(output, 1u, sizeof(output) - 1u, out);
  output[length] = '\0';
  CuAssertTrue(tc, strstr(output, "encoding=zlib-deflate") != NULL);
  CuAssertTrue(tc, strstr(output, "stage=decompression") != NULL);
  CuAssertTrue(tc, strstr(output, "\"connection_id\"") != NULL);
  CuAssertTrue(tc, strstr(output, "\"error_code\"") != NULL);
  CuAssertTrue(tc, strstr(output, "\"response_code\"") != NULL);
  fclose(out);
  CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_log_event_parse(&event, buf, APX_LOG_EVENT_HEADER_SIZE + 25u));
  CuAssertIntEquals(tc, APX_VALUE_LENGTH_ERROR, apx_log_event_parse(&event, buf, sizeof(buf) - 1u));
}

CuSuite *testsuite_log_event(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_log_event_parse_header);
  SUITE_ADD_TEST(suite, test_log_event_type_str);
  SUITE_ADD_TEST(suite, test_log_event_formatters);
  SUITE_ADD_TEST(suite, test_definition_rejection_format);
  return suite;
}
