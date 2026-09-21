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
#include <stdio.h>
#include <string.h>
#include "CuTest.h"
#include "apx/util.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_strerror(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_util(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_strerror);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_strerror(CuTest* tc)
{
   CuAssertStrEquals(tc, "No error", apx_strerror(APX_NO_ERROR));
   CuAssertStrEquals(tc, "No such file or directory", apx_strerror(APX_FILE_NOT_FOUND_ERROR));
   CuAssertStrEquals(tc, "Parse error", apx_strerror(APX_PARSE_ERROR));
   CuAssertStrEquals(tc, "Invalid argument", apx_strerror(APX_INVALID_ARGUMENT_ERROR));
   CuAssertStrEquals(tc, "Out of memory", apx_strerror(APX_MEM_ERROR));
   CuAssertStrEquals(tc, "Not a directory", apx_strerror(APX_NOT_A_DIRECTORY_ERROR));
   CuAssertStrEquals(tc, "Unknown error", apx_strerror(9999));
}
