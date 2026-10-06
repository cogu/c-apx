/*****************************************************************************
 * \file      testsuite_util.c
 * \author    Conny Gustafsson
 * \date      2020-04-22
 * \brief     Unit tests for apx_util
 *
 * Copyright (c) 2020-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/util.h"
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
static void test_apx_strerror(CuTest *tc);
static void test_apx_log_level_conversions(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_apx_util(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_apx_strerror);
  SUITE_ADD_TEST(suite, test_apx_log_level_conversions);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_strerror(CuTest *tc)
{
  CuAssertStrEquals(tc, "No error", apx_strerror(APX_NO_ERROR));
  CuAssertStrEquals(tc, "No such file or directory", apx_strerror(APX_FILE_NOT_FOUND_ERROR));
  CuAssertStrEquals(tc, "Parse error", apx_strerror(APX_PARSE_ERROR));
  CuAssertStrEquals(tc, "Invalid argument", apx_strerror(APX_INVALID_ARGUMENT_ERROR));
  CuAssertStrEquals(tc, "Out of memory", apx_strerror(APX_MEM_ERROR));
  CuAssertStrEquals(tc, "Not a directory", apx_strerror(APX_NOT_A_DIRECTORY_ERROR));
  CuAssertStrEquals(tc, "Unknown error", apx_strerror(9999));
}

static void test_apx_log_level_conversions(CuTest *tc)
{
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_CRITICAL, (int)apx_log_level_from_string("CRITICAL"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_CRITICAL, (int)apx_log_level_from_string("critical"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_ERROR, (int)apx_log_level_from_string("ERROR"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_ERROR, (int)apx_log_level_from_string("error"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_WARNING, (int)apx_log_level_from_string("WARNING"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_WARNING, (int)apx_log_level_from_string("warning"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_WARNING, (int)apx_log_level_from_string("WARN"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_WARNING, (int)apx_log_level_from_string("warn"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_INFO, (int)apx_log_level_from_string("INFO"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_INFO, (int)apx_log_level_from_string("info"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_DEBUG, (int)apx_log_level_from_string("DEBUG"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_DEBUG, (int)apx_log_level_from_string("debug"));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_INVALID, (int)apx_log_level_from_string(NULL));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_INVALID, (int)apx_log_level_from_string(""));
  CuAssertIntEquals(tc, (int)APX_LOG_LEVEL_INVALID, (int)apx_log_level_from_string("UNKNOWN"));

  CuAssertStrEquals(tc, "CRITICAL", apx_log_level_to_string(APX_LOG_LEVEL_CRITICAL));
  CuAssertStrEquals(tc, "ERROR", apx_log_level_to_string(APX_LOG_LEVEL_ERROR));
  CuAssertStrEquals(tc, "WARNING", apx_log_level_to_string(APX_LOG_LEVEL_WARNING));
  CuAssertStrEquals(tc, "INFO", apx_log_level_to_string(APX_LOG_LEVEL_INFO));
  CuAssertStrEquals(tc, "DEBUG", apx_log_level_to_string(APX_LOG_LEVEL_DEBUG));
  CuAssertStrEquals(tc, "UNKNOWN", apx_log_level_to_string(APX_LOG_LEVEL_INVALID));
}
