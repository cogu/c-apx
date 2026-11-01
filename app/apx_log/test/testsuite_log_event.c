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
  CuAssertStrEquals(tc, "FILE_PUBLISH", apx_log_event_type_str(APX_LOG_EVENT_FILE_PUBLISH));
  CuAssertStrEquals(tc, "FILE_REVOKE", apx_log_event_type_str(APX_LOG_EVENT_FILE_REVOKE));
  CuAssertStrEquals(tc, "FILE_OPEN", apx_log_event_type_str(APX_LOG_EVENT_FILE_OPEN));
  CuAssertStrEquals(tc, "FILE_CLOSE", apx_log_event_type_str(APX_LOG_EVENT_FILE_CLOSE));
  CuAssertStrEquals(tc, "FILE_WRITE", apx_log_event_type_str(APX_LOG_EVENT_FILE_WRITE));
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
  fclose(f);
}

CuSuite *testsuite_log_event(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_log_event_parse_header);
  SUITE_ADD_TEST(suite, test_log_event_type_str);
  SUITE_ADD_TEST(suite, test_log_event_formatters);
  return suite;
}
