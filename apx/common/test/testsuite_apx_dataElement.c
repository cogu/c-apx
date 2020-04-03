/*****************************************************************************
* \file      testsuite_apx_dataElement.c
* \author    Conny Gustafsson
* \date      my_date
* \brief     unit tests for apx_dataElement_t
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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_dataElement.h"
#include "apx_dataType.h"
#include "adt_bytearray.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_apx_dataElement_create_typeReferenceId(CuTest *tc);
static void test_apx_dataElement_create_typeReferenceName(CuTest *tc);
static void test_apx_dataElement_create_typeReferencePtr(CuTest *tc);
static void test_apx_dataElement_create_U8DynArrayU8(CuTest *tc);
static void test_apx_dataElement_create_U8DynArrayU16(CuTest *tc);
static void test_apx_dataElement_create_U8DynArrayU32(CuTest *tc);
static void test_apx_dataElement_createProperInitValueU8(CuTest* tc);
static void test_apx_dataElement_createProperInitValueU16(CuTest* tc);
static void test_apx_dataElement_createProperInitValueU32(CuTest* tc);
static void test_apx_dataElement_createProperInitValueS8(CuTest* tc);
static void test_apx_dataElement_createProperInitValueS16(CuTest* tc);
static void test_apx_dataElement_createProperInitValueS32(CuTest* tc);
static void test_apx_dataElement_createProperInitValueU8Array(CuTest* tc);
static void test_apx_dataElement_createProperInitValueU16Array(CuTest* tc);
static void test_apx_dataElement_createProperInitValueU32Array(CuTest* tc);
static void test_apx_dataElement_createProperInitValueS8Array(CuTest* tc);
static void test_apx_dataElement_createProperInitValueS16Array(CuTest* tc);
static void test_apx_dataElement_createProperInitValueS32Array(CuTest* tc);
static void test_apx_dataElement_createProperInitValueString(CuTest* tc);
static void test_apx_dataElement_createProperInitValueRecord_U8(CuTest* tc);
static void test_apx_dataElement_createProperInitValueRecord_U16U8U8(CuTest* tc);
static void test_apx_dataElement_createProperInitValueRecord_U16ArrayU8Array(CuTest* tc);
static void test_apx_dataElement_createProperInitValueRecord_StringU32(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_dataElement(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_dataElement_create_typeReferenceId);
   SUITE_ADD_TEST(suite, test_apx_dataElement_create_typeReferenceName);
   SUITE_ADD_TEST(suite, test_apx_dataElement_create_typeReferencePtr);
   SUITE_ADD_TEST(suite, test_apx_dataElement_create_U8DynArrayU8);
   SUITE_ADD_TEST(suite, test_apx_dataElement_create_U8DynArrayU16);
   SUITE_ADD_TEST(suite, test_apx_dataElement_create_U8DynArrayU32);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueU8);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueU16);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueU32);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueS8);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueS16);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueS32);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueU8Array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueU16Array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueU32Array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueS8Array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueS16Array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueS32Array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueString);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueRecord_U8);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueRecord_U16U8U8);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueRecord_U16ArrayU8Array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_createProperInitValueRecord_StringU32);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_dataElement_create_typeReferenceId(CuTest *tc)
{
   apx_dataElement_t *element;
   element = apx_dataElement_new(APX_BASE_TYPE_REF_ID, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setTypeReferenceId(element, 64);
   CuAssertIntEquals(tc, 64, apx_dataElement_getTypeReferenceId(element));
   apx_dataElement_delete(element);
}

static void test_apx_dataElement_create_typeReferenceName(CuTest *tc)
{
   apx_dataElement_t *element;
   element = apx_dataElement_new(APX_BASE_TYPE_REF_NAME, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setTypeReferenceName(element, "typeName");
   CuAssertStrEquals(tc, "typeName", apx_dataElement_getTypeReferenceName(element));
   apx_dataElement_delete(element);
}

static void test_apx_dataElement_create_typeReferencePtr(CuTest *tc)
{
   apx_error_t err;
   apx_dataElement_t *element;
   apx_datatype_t *dataType;
   element = apx_dataElement_new(APX_BASE_TYPE_REF_PTR, NULL);
   CuAssertPtrNotNull(tc, element);
   dataType = apx_datatype_new("TypeName_T","C(0,1)", NULL, 0, &err);
   apx_dataElement_setTypeReferencePtr(element, dataType);
   CuAssertPtrEquals(tc, dataType, apx_dataElement_getTypeReferencePtr(element));
   apx_dataElement_delete(element);
   apx_datatype_delete(dataType);
}

static void test_apx_dataElement_create_U8DynArrayU8(CuTest *tc)
{

   apx_dataElement_t *element;

   const uint32_t arrayLen = 10;
   apx_size_t packLen;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT8, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);
   apx_dataElement_setDynamicArray(element);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataElement_calcPackLen(element, &packLen));
   CuAssertIntEquals(tc, UINT8_SIZE+UINT8_SIZE*arrayLen, packLen);
   apx_dataElement_delete(element);

}

static void test_apx_dataElement_create_U8DynArrayU16(CuTest *tc)
{

   apx_dataElement_t *element;

   const uint32_t arrayLen = 256;
   apx_size_t packLen;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT8, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);
   apx_dataElement_setDynamicArray(element);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataElement_calcPackLen(element, &packLen));
   CuAssertIntEquals(tc, UINT16_SIZE+UINT8_SIZE*arrayLen, packLen);
   apx_dataElement_delete(element);

}

static void test_apx_dataElement_create_U8DynArrayU32(CuTest *tc)
{

   apx_dataElement_t *element;

   const uint32_t arrayLen = 65536;
   apx_size_t packLen;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT8, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);
   apx_dataElement_setDynamicArray(element);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataElement_calcPackLen(element, &packLen));
   CuAssertIntEquals(tc, UINT32_SIZE+UINT8_SIZE*arrayLen, packLen);
   apx_dataElement_delete(element);

}

static void test_apx_dataElement_createProperInitValueU8(CuTest* tc)
{
   apx_dataElement_t *element;
   dtl_sv_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT8, NULL);
   CuAssertPtrNotNull(tc, element);
   initValue = dtl_sv_make_u32(255);
   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(properInitValue));
   sv = (dtl_sv_t*) properInitValue;
   CuAssertUIntEquals(tc,255, dtl_sv_to_u32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueU16(CuTest* tc)
{
   apx_dataElement_t *element;
   dtl_sv_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT16, NULL);
   CuAssertPtrNotNull(tc, element);
   initValue = dtl_sv_make_u32(65535);
   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(properInitValue));
   sv = (dtl_sv_t*) properInitValue;
   CuAssertUIntEquals(tc, 65535, dtl_sv_to_u32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueU32(CuTest* tc)
{
   apx_dataElement_t *element;
   dtl_sv_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT32, NULL);
   CuAssertPtrNotNull(tc, element);
   initValue = dtl_sv_make_u32(0xffffffff);
   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(properInitValue));
   sv = (dtl_sv_t*) properInitValue;
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueS8(CuTest* tc)
{
   apx_dataElement_t *element;
   dtl_sv_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_SINT8, NULL);
   CuAssertPtrNotNull(tc, element);
   initValue = dtl_sv_make_i32(-1);
   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(properInitValue));
   sv = (dtl_sv_t*) properInitValue;
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueS16(CuTest* tc)
{
   apx_dataElement_t *element;
   dtl_sv_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_SINT16, NULL);
   CuAssertPtrNotNull(tc, element);
   initValue = dtl_sv_make_i32(-1);
   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(properInitValue));
   sv = (dtl_sv_t*) properInitValue;
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueS32(CuTest* tc)
{
   apx_dataElement_t *element;
   dtl_sv_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_SINT32, NULL);
   CuAssertPtrNotNull(tc, element);
   initValue = dtl_sv_make_i32(-1);
   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(properInitValue));
   sv = (dtl_sv_t*) properInitValue;
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueU8Array(CuTest* tc)
{
   const int arrayLen = 3;
   apx_dataElement_t *element;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_av_t *av;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT8, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);

   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0x12), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0xff), false);

   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);

   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(properInitValue));
   av = (dtl_av_t*) properInitValue;
   CuAssertUIntEquals(tc, 3, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xff, dtl_sv_to_u32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueU16Array(CuTest* tc)
{
   const int arrayLen = 3;
   apx_dataElement_t *element;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_av_t *av;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT16, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);

   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0x1234), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0xffff), false);

   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);

   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(properInitValue));
   av = (dtl_av_t*) properInitValue;
   CuAssertUIntEquals(tc, 3, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xffff, dtl_sv_to_u32(sv, NULL));

   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueU32Array(CuTest* tc)
{
   const int arrayLen = 3;
   apx_dataElement_t *element;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_av_t *av;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_UINT32, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);

   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0x12345678), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0xffffffff), false);

   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);

   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(properInitValue));
   av = (dtl_av_t*) properInitValue;
   CuAssertUIntEquals(tc, 3, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0x12345678, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32(sv, NULL));

   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueS8Array(CuTest* tc)
{
   const int arrayLen = 4;
   apx_dataElement_t *element;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_av_t *av;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_SINT8, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);

   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(-128), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(0), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(127), false);

   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);

   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(properInitValue));
   av = (dtl_av_t*) properInitValue;
   CuAssertUIntEquals(tc, 4, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertIntEquals(tc, -128, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 127, dtl_sv_to_i32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueS16Array(CuTest* tc)
{
   const int arrayLen = 4;
   apx_dataElement_t *element;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_av_t *av;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_SINT16, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);

   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(-32768), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(0), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(32767), false);

   CuAssertPtrNotNull(tc, initValue);
   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);

   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(properInitValue));
   av = (dtl_av_t*) properInitValue;
   CuAssertUIntEquals(tc, 4, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertIntEquals(tc, -32768, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 32767, dtl_sv_to_i32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueS32Array(CuTest* tc)
{
   const int arrayLen = 4;
   apx_dataElement_t *element;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_av_t *av;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;
   element = apx_dataElement_new(APX_BASE_TYPE_SINT32, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_setArrayLen(element, arrayLen);

   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(-2147483648), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(0), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_i32(2147483647), false);

   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);

   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(properInitValue));
   av = (dtl_av_t*) properInitValue;
   CuAssertUIntEquals(tc, 4, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -2147483648, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, 2147483647, dtl_sv_to_i32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueString(CuTest* tc)
{
   const int arrayLen = 13;
   apx_dataElement_t *element;
   dtl_sv_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;

   element = apx_dataElement_new(APX_BASE_TYPE_STRING, NULL);
   apx_dataElement_setArrayLen(element, arrayLen);
   initValue = dtl_sv_make_cstr("Hello World!");
   CuAssertPtrNotNull(tc, initValue);

   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(properInitValue));

   sv = (dtl_sv_t*) properInitValue;
   CuAssertIntEquals(tc, DTL_SV_STR, dtl_sv_type(sv));
   CuAssertStrEquals(tc, "Hello World!", dtl_sv_to_cstr(sv));

   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);

}

static void test_apx_dataElement_createProperInitValueRecord_U8(CuTest* tc)
{
   apx_dataElement_t *element;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_hv_t *hv;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;


   element = apx_dataElement_new(APX_BASE_TYPE_RECORD, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_appendChild(element, apx_dataElement_new(APX_BASE_TYPE_UINT8, "Item"));
   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0xff), false);

   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(properInitValue));
   hv = (dtl_hv_t*) properInitValue;
   CuAssertIntEquals(tc, 1, dtl_hv_length(hv));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Item");
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xff, dtl_sv_to_u32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueRecord_U16U8U8(CuTest* tc)
{
   apx_dataElement_t *element;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_hv_t *hv;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;


   element = apx_dataElement_new(APX_BASE_TYPE_RECORD, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_appendChild(element, apx_dataElement_new(APX_BASE_TYPE_UINT16, "Id"));
   apx_dataElement_appendChild(element, apx_dataElement_new(APX_BASE_TYPE_UINT8, "FTB"));
   apx_dataElement_appendChild(element, apx_dataElement_new(APX_BASE_TYPE_UINT8, "Status"));
   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0xffff), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0xff), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0xff), false);

   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(properInitValue));
   hv = (dtl_hv_t*) properInitValue;
   CuAssertIntEquals(tc, 3, dtl_hv_length(hv));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Id");
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xffff, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "FTB");
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xff, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Status");
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xff, dtl_sv_to_u32(sv, NULL));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

/**
 * This tests init value for a sort of mapping object. The mapping consists of an U16 raw value table and an equally lengthed
 * U8 lookup table (which contains the mapped values)
 */
static void test_apx_dataElement_createProperInitValueRecord_U16ArrayU8Array(CuTest* tc)
{
   const int32_t arrayLen = 6;
   apx_dataElement_t *element;
   apx_dataElement_t *innerElement;
   dtl_av_t *initValue;
   dtl_av_t *u16InnerInitValue;
   dtl_av_t *u8InnerInitValue;
   dtl_dv_t *properInitValue;
   dtl_hv_t *hv;
   dtl_av_t *av;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;


   element = apx_dataElement_new(APX_BASE_TYPE_RECORD, NULL);
   CuAssertPtrNotNull(tc, element);
   innerElement = apx_dataElement_new(APX_BASE_TYPE_UINT16, "X-AXIS");
   apx_dataElement_setArrayLen(innerElement, arrayLen);
   apx_dataElement_appendChild(element, innerElement);
   innerElement = apx_dataElement_new(APX_BASE_TYPE_UINT8, "Y-AXIS");
   apx_dataElement_setArrayLen(innerElement, arrayLen);
   apx_dataElement_appendChild(element, innerElement);
   initValue = dtl_av_new();
   u16InnerInitValue = dtl_av_new();
   u8InnerInitValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   CuAssertPtrNotNull(tc, u16InnerInitValue);
   CuAssertPtrNotNull(tc, u8InnerInitValue);
   dtl_av_push(u16InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(0), false);
   dtl_av_push(u16InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(1000), false);
   dtl_av_push(u16InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(2000), false);
   dtl_av_push(u16InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(3000), false);
   dtl_av_push(u16InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(4000), false);
   dtl_av_push(u16InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(6000), false);

   dtl_av_push(u8InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(0), false);
   dtl_av_push(u8InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(20), false);
   dtl_av_push(u8InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(60), false);
   dtl_av_push(u8InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(80), false);
   dtl_av_push(u8InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(90), false);
   dtl_av_push(u8InnerInitValue, (dtl_dv_t*) dtl_sv_make_u32(120), false);

   dtl_av_push(initValue, (dtl_dv_t*) u16InnerInitValue, false);
   dtl_av_push(initValue, (dtl_dv_t*) u8InnerInitValue, false);


   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(properInitValue));
   hv = (dtl_hv_t*) properInitValue;
   CuAssertIntEquals(tc, 2, dtl_hv_length(hv));
   av = (dtl_av_t*) dtl_hv_get_cstr(hv, "X-AXIS");
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, arrayLen, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 1000, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 2000, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 3000, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 4);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 4000, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 5);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 6000, dtl_sv_to_u32(sv, NULL));

   av = (dtl_av_t*) dtl_hv_get_cstr(hv, "Y-AXIS");
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, arrayLen, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 20, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 60, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 3);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 80, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 4);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 90, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 5);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 120, dtl_sv_to_u32(sv, NULL));

   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}

static void test_apx_dataElement_createProperInitValueRecord_StringU32(CuTest* tc)
{
   const int32_t maxStrLen = 32;
   apx_dataElement_t *element;
   apx_dataElement_t *innerElement;
   dtl_av_t *initValue;
   dtl_dv_t *properInitValue;
   dtl_hv_t *hv;
   dtl_sv_t *sv;
   apx_error_t result = APX_NO_ERROR;


   element = apx_dataElement_new(APX_BASE_TYPE_RECORD, NULL);
   CuAssertPtrNotNull(tc, element);
   apx_dataElement_appendChild(element, apx_dataElement_new(APX_BASE_TYPE_UINT32, "UserId"));
   innerElement = apx_dataElement_new(APX_BASE_TYPE_STRING, "UserName");
   apx_dataElement_setArrayLen(innerElement, maxStrLen);
   apx_dataElement_appendChild(element, innerElement);

   initValue = dtl_av_new();
   CuAssertPtrNotNull(tc, initValue);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_u32(0xffffffff), false);
   dtl_av_push(initValue, (dtl_dv_t*) dtl_sv_make_cstr(""), false);

   properInitValue = apx_dataElement_makeProperInitValueFromDynamicValue(element, (dtl_dv_t*) initValue, &result);
   CuAssertIntEquals(tc, APX_NO_ERROR, result);
   CuAssertPtrNotNull(tc, properInitValue);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(properInitValue));
   hv = (dtl_hv_t*) properInitValue;
   CuAssertIntEquals(tc, 2, dtl_hv_length(hv));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "UserId");
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(sv));
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "UserName");
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_STR, dtl_sv_type(sv));
   CuAssertStrEquals(tc, "", dtl_sv_to_cstr(sv));
   apx_dataElement_delete(element);
   dtl_dec_ref(initValue);
   dtl_dec_ref(properInitValue);
}
