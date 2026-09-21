/*****************************************************************************
* \file      testsuite_computation.c
* \author    Conny Gustafsson
* \date      2021-02-04
* \brief     Unit tests for APX computation
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <string.h>
#include "CuTest.h"
#include "apx/computation.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_value_table_to_string(CuTest* tc);
static void test_rational_scaling_to_string(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_computation(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_value_table_to_string);
   SUITE_ADD_TEST(suite, test_rational_scaling_to_string);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_value_table_to_string(CuTest* tc)
{
   adt_ary_t* values = adt_ary_new(adt_str_vdelete);
   apx_value_table_t* vt = apx_value_table_new();
   CuAssertPtrNotNull(tc, vt);
   adt_ary_push(values, adt_str_new_cstr("Off"));
   adt_ary_push(values, adt_str_new_cstr("On"));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_value_table_move_values(vt, values));
   apx_value_table_set_range_unsigned(vt, 0, 1);
   adt_str_t* str = apx_value_table_to_string(vt);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "VT(0,1,\"Off\",\"On\")", adt_str_cstr(str));
   adt_str_delete(str);
   apx_value_table_delete(vt);
   adt_ary_delete(values);
}

static void test_rational_scaling_to_string(CuTest* tc)
{
   apx_rational_scaling_t* rs = apx_rational_scaling_new(0.0, 4, 10, "Percent");
   CuAssertPtrNotNull(tc, rs);
   apx_rational_scaling_set_range_unsigned(rs, 0, 250);
   adt_str_t* str = apx_rational_scaling_to_string(rs);
   CuAssertPtrNotNull(tc, str);
   CuAssertStrEquals(tc, "RS(0,250,0.00000000,4,10,\"Percent\")", adt_str_cstr(str));
   adt_str_delete(str);
   apx_rational_scaling_delete(rs);
}
