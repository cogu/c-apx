/*****************************************************************************
* \file      testsuite_apx_datatype.c
* \author    Conny Gustafsson
* \date      2018-09-11
* \brief     Unit tests for apx_datatype_t
*
* Copyright (c) 2018 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include "CuTest.h"
#include "apx_dataType.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_datatype_create(CuTest *tc);
static void test_apx_datatype_createWithErrors(CuTest *tc);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_datatype(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_datatype_create);
   SUITE_ADD_TEST(suite, test_apx_datatype_createWithErrors);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_datatype_create(CuTest *tc)
{
   apx_error_t err;
   apx_datatype_t *datatype = apx_datatype_new("MyType", "C(0,3)", NULL, 4, &err);
   CuAssertPtrNotNull(tc, datatype);
   CuAssertIntEquals(tc, 4, apx_datatype_getLineNumber(datatype));
   apx_datatype_delete(datatype);

   datatype = apx_datatype_new("MyType", "T[2]", NULL, 4, &err);
   CuAssertPtrNotNull(tc, datatype);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_ID, datatype->dataSignature->dataElement->baseType);
   CuAssertIntEquals(tc, 2, datatype->dataSignature->dataElement->typeRef.id);
   apx_datatype_delete(datatype);

}

static void test_apx_datatype_createWithErrors(CuTest *tc)
{
   apx_error_t err;
   apx_datatype_t *datatype = apx_datatype_new("MyType", "Q", NULL, 1, &err);
   CuAssertPtrEquals(tc, NULL, datatype);
   CuAssertIntEquals(tc, APX_ELEMENT_TYPE_ERROR, err);

   datatype = apx_datatype_new("MyType", "T[2", NULL, 1, &err);
   CuAssertPtrEquals(tc, NULL, datatype);
   CuAssertIntEquals(tc, APX_UNMATCHED_BRACKET_ERROR, err);

}


