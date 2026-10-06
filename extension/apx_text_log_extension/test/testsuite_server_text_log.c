/*****************************************************************************
 * \file      testsuite_server_text_log.c
 * \author    Conny Gustafsson
 * \date      2019-05-27
 * \brief     Unit tests for server text log
 *
 * Copyright (c) 2019-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/extension/server_text_log.h"
#include "apx/extension/server_text_log_extension.h"
#include "apx/extension/text_log_base.h"
#include "apx/server.h"
#include "dtl_hv.h"
#include "dtl_sv.h"
#include <stdio.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_extension_init_shutdown(CuTest *tc);
static void test_text_log_base_level_filtering(CuTest *tc);
static void test_text_log_base_timestamp(CuTest *tc);
static void test_extension_configure_log_level_and_timestamp(CuTest *tc);
static void test_extension_configure_invalid_log_level(CuTest *tc);
static void test_server_text_log_properties(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_server_text_log_extension(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_extension_init_shutdown);
  SUITE_ADD_TEST(suite, test_text_log_base_level_filtering);
  SUITE_ADD_TEST(suite, test_text_log_base_timestamp);
  SUITE_ADD_TEST(suite, test_extension_configure_log_level_and_timestamp);
  SUITE_ADD_TEST(suite, test_extension_configure_invalid_log_level);
  SUITE_ADD_TEST(suite, test_server_text_log_properties);
  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_extension_init_shutdown(CuTest *tc)
{
  apx_server_t apx_server;
  dtl_hv_t *extension_cfg = NULL;
  apx_server_create(&apx_server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_text_log_extension_register(&apx_server, (dtl_dv_t *)extension_cfg));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_start(&apx_server));
  apx_server_destroy(&apx_server);
}

static void test_text_log_base_level_filtering(CuTest *tc)
{
  apx_text_log_base_t log;
  FILE *tmp = tmpfile();
  char buf[256];
  CuAssertPtrNotNull(tc, tmp);

  apx_text_log_base_create(&log);
  CuAssertIntEquals(tc, APX_LOG_LEVEL_INFO, apx_text_log_base_get_log_level(&log));
  CuAssertTrue(tc, apx_text_log_base_get_timestamp_enabled(&log) == false);

  log.file = tmp;
  log.fileEnabled = true;

  // Log DEBUG message: should be filtered out by default INFO level
  apx_text_log_base_log(&log, APX_LOG_LEVEL_DEBUG, "TEST", "Debug message");
  fflush(tmp);
  CuAssertIntEquals(tc, 0, (int)ftell(tmp));

  // Log INFO message: should be written
  apx_text_log_base_log(&log, APX_LOG_LEVEL_INFO, "TEST", "Info message");
  fflush(tmp);
  CuAssertTrue(tc, ftell(tmp) > 0);

  CuAssertIntEquals(tc, 0, fseek(tmp, 0, SEEK_SET));
  memset(buf, 0, sizeof(buf));
  CuAssertPtrNotNull(tc, fgets(buf, sizeof(buf), tmp));
  CuAssertStrEquals(tc, "[INFO] [TEST] Info message\n", buf);

  // Set level to WARNING: INFO should now be dropped, WARNING should pass
  apx_text_log_base_set_log_level(&log, APX_LOG_LEVEL_WARNING);
  CuAssertIntEquals(tc, APX_LOG_LEVEL_WARNING, apx_text_log_base_get_log_level(&log));

  apx_text_log_base_close_all(&log);
  tmp = tmpfile();
  CuAssertPtrNotNull(tc, tmp);
  log.file = tmp;
  log.fileEnabled = true;

  apx_text_log_base_log(&log, APX_LOG_LEVEL_INFO, "TEST", "Dropped info");
  fflush(tmp);
  CuAssertIntEquals(tc, 0, (int)ftell(tmp));

  apx_text_log_base_logf(&log, APX_LOG_LEVEL_WARNING, "TEST", "Warning code %d", 123);
  fflush(tmp);
  CuAssertTrue(tc, ftell(tmp) > 0);

  CuAssertIntEquals(tc, 0, fseek(tmp, 0, SEEK_SET));
  memset(buf, 0, sizeof(buf));
  CuAssertPtrNotNull(tc, fgets(buf, sizeof(buf), tmp));
  CuAssertStrEquals(tc, "[WARNING] [TEST] Warning code 123\n", buf);

  // Set level to DEBUG: DEBUG should now pass
  apx_text_log_base_set_log_level(&log, APX_LOG_LEVEL_DEBUG);
  apx_text_log_base_close_all(&log);
  tmp = tmpfile();
  CuAssertPtrNotNull(tc, tmp);
  log.file = tmp;
  log.fileEnabled = true;

  apx_text_log_base_log(&log, APX_LOG_LEVEL_DEBUG, "TEST", "Debug allowed");
  fflush(tmp);
  CuAssertIntEquals(tc, 0, fseek(tmp, 0, SEEK_SET));
  memset(buf, 0, sizeof(buf));
  CuAssertPtrNotNull(tc, fgets(buf, sizeof(buf), tmp));
  CuAssertStrEquals(tc, "[DEBUG] [TEST] Debug allowed\n", buf);

  apx_text_log_base_destroy(&log);
}

static void test_text_log_base_timestamp(CuTest *tc)
{
  apx_text_log_base_t log;
  FILE *tmp = tmpfile();
  char buf[256];
  CuAssertPtrNotNull(tc, tmp);

  apx_text_log_base_create(&log);
  apx_text_log_base_set_timestamp_enabled(&log, true);
  CuAssertTrue(tc, apx_text_log_base_get_timestamp_enabled(&log) == true);

  log.file = tmp;
  log.fileEnabled = true;

  apx_text_log_base_log(&log, APX_LOG_LEVEL_ERROR, "SYS", "Something went wrong");
  fflush(tmp);

  CuAssertIntEquals(tc, 0, fseek(tmp, 0, SEEK_SET));
  memset(buf, 0, sizeof(buf));
  CuAssertPtrNotNull(tc, fgets(buf, sizeof(buf), tmp));

  // Should format as: [<YYYY-MM-DD HH:MM:SS.mmm>] [ERROR] [SYS] Something went wrong\n
  CuAssertIntEquals(tc, '[', buf[0]);
  CuAssertPtrNotNull(tc, strstr(buf, "] [ERROR] [SYS] Something went wrong\n"));

  apx_text_log_base_destroy(&log);
}

static void test_extension_configure_log_level_and_timestamp(CuTest *tc)
{
  apx_server_t apx_server;
  dtl_hv_t *cfg = dtl_hv_new();
  CuAssertPtrNotNull(tc, cfg);

  dtl_hv_set_cstr(cfg, "log-level", (dtl_dv_t *)dtl_sv_make_cstr("DEBUG"), false);
  dtl_hv_set_cstr(cfg, "use-timestamp", (dtl_dv_t *)dtl_sv_make_bool(true), false);

  apx_server_create(&apx_server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_text_log_extension_register(&apx_server, (dtl_dv_t *)cfg));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_start(&apx_server));
  apx_server_destroy(&apx_server);
  dtl_dec_ref((dtl_dv_t *)cfg);
}

static void test_extension_configure_invalid_log_level(CuTest *tc)
{
  apx_server_t apx_server;
  dtl_hv_t *cfg = dtl_hv_new();
  CuAssertPtrNotNull(tc, cfg);

  dtl_hv_set_cstr(cfg, "log-level", (dtl_dv_t *)dtl_sv_make_cstr("UNKNOWN_LEVEL"), false);

  apx_server_create(&apx_server);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_server_text_log_extension_register(&apx_server, (dtl_dv_t *)cfg));
  CuAssertIntEquals(tc, APX_VALUE_RANGE_ERROR, apx_server_start(&apx_server));
  apx_server_destroy(&apx_server);
  dtl_dec_ref((dtl_dv_t *)cfg);
}

static void test_server_text_log_properties(CuTest *tc)
{
  apx_server_text_log_t server_log;

  apx_server_text_log_create(&server_log, NULL);
  CuAssertIntEquals(tc, APX_LOG_LEVEL_INFO, apx_server_text_log_get_log_level(&server_log));
  CuAssertTrue(tc, apx_server_text_log_get_timestamp_enabled(&server_log) == false);

  apx_server_text_log_set_log_level(&server_log, APX_LOG_LEVEL_DEBUG);
  CuAssertIntEquals(tc, APX_LOG_LEVEL_DEBUG, apx_server_text_log_get_log_level(&server_log));

  apx_server_text_log_set_timestamp_enabled(&server_log, true);
  CuAssertTrue(tc, apx_server_text_log_get_timestamp_enabled(&server_log) == true);

  apx_server_text_log_destroy(&server_log);
}
