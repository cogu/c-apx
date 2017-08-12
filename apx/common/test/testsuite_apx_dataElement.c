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
static void test_apx_dataElement_pack_U8(CuTest* tc);
static void test_apx_dataElement_pack_U16(CuTest* tc);
static void test_apx_dataElement_pack_U32(CuTest* tc);
static void test_apx_dataElement_pack_S8(CuTest* tc);
static void test_apx_dataElement_pack_S16(CuTest* tc);
static void test_apx_dataElement_pack_S32(CuTest* tc);
static void test_apx_dataElement_pack_U8_array(CuTest* tc);
static void test_apx_dataElement_pack_U16_array(CuTest* tc);
static void test_apx_dataElement_pack_U32_array(CuTest* tc);
static void test_apx_dataElement_pack_S8_array(CuTest* tc);
static void test_apx_dataElement_pack_S16_array(CuTest* tc);
static void test_apx_dataElement_pack_S32_array(CuTest* tc);
static void test_apx_dataElement_pack_string(CuTest *tc);
static void test_apx_dataElement_pack_pair(CuTest *tc);
static void test_apx_dataElement_pack_deep(CuTest *tc);


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

   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_U8);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_U16);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_U32);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_S8);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_S16);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_S32);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_U8_array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_U16_array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_U32_array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_S8_array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_S16_array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_S32_array);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_string);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_pair);
   SUITE_ADD_TEST(suite, test_apx_dataElement_pack_deep);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_dataElement_pack_U8(CuTest* tc)
{
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_sv_t *sv;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT8, 0);
   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, 1);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   sv = dtl_sv_make_i32(10);
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) sv);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 10);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_sv_delete(sv);
}


static void test_apx_dataElement_pack_U16(CuTest* tc)
{
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_sv_t *sv;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT16, 0);
   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint16_t));
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   sv = dtl_sv_make_i32(0x1234);
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) sv);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 0x34);
   CuAssertIntEquals(tc, pBegin[1], 0x12);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_sv_delete(sv);
}

static void test_apx_dataElement_pack_U32(CuTest* tc)
{
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_sv_t *sv;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT32, 0);
   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint32_t));
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   sv = dtl_sv_make_u32(0xFFFE1234);
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) sv);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 0x34);
   CuAssertIntEquals(tc, pBegin[1], 0x12);
   CuAssertIntEquals(tc, pBegin[2], 0xFE);
   CuAssertIntEquals(tc, pBegin[3], 0xFF);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_sv_delete(sv);
}

static void test_apx_dataElement_pack_S8(CuTest* tc)
{
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_sv_t *sv;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_SINT8, 0);
   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, 1);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   sv = dtl_sv_make_i32(-123);
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) sv);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], ((uint8_t) -123));
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_sv_delete(sv);
}

static void test_apx_dataElement_pack_S16(CuTest* tc)
{
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_sv_t *sv;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_SINT16, 0);
   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint16_t));
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   sv = dtl_sv_make_i32(-1234);
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) sv);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 0x2E);
   CuAssertIntEquals(tc, pBegin[1], 0xFB);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_sv_delete(sv);
}

static void test_apx_dataElement_pack_S32(CuTest* tc)
{
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_sv_t *sv;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT32, 0);
   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint32_t));
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   sv = dtl_sv_make_i32(-12345678L);
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) sv);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 0xB2);
   CuAssertIntEquals(tc, pBegin[1], 0x9E);
   CuAssertIntEquals(tc, pBegin[2], 0x43);
   CuAssertIntEquals(tc, pBegin[3], 0xFF);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_sv_delete(sv);
}

static void test_apx_dataElement_pack_U8_array(CuTest* tc)
{
   const int arrayLen = 9;
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_av_t *av;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT8, 0);
   apx_dataElement_setArrayLen(&dataElement, arrayLen);

   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint8_t)*arrayLen);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   av = dtl_av_new();
   CuAssertPtrNotNull(tc, av);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(1));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(2));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(3));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(4));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(5));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(6));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(7));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(8));

   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, 0, pResult); //one element still missing
   CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_getLastError());
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(9));
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 1);
   CuAssertIntEquals(tc, pBegin[1], 2);
   CuAssertIntEquals(tc, pBegin[2], 3);
   CuAssertIntEquals(tc, pBegin[3], 4);
   CuAssertIntEquals(tc, pBegin[4], 5);
   CuAssertIntEquals(tc, pBegin[5], 6);
   CuAssertIntEquals(tc, pBegin[6], 7);
   CuAssertIntEquals(tc, pBegin[7], 8);
   CuAssertIntEquals(tc, pBegin[8], 9);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_av_delete(av);
}

static void test_apx_dataElement_pack_U16_array(CuTest* tc)
{
   const int arrayLen = 9;
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_av_t *av;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT16, 0);
   apx_dataElement_setArrayLen(&dataElement, arrayLen);

   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint16_t)*arrayLen);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   av = dtl_av_new();
   CuAssertPtrNotNull(tc, av);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(1));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(2));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(3));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(4));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(5));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(6));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(7));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(8));

   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, 0, pResult); //one element still missing
   CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_getLastError());
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(9));
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 1);
   CuAssertIntEquals(tc, pBegin[1], 0);
   CuAssertIntEquals(tc, pBegin[2], 2);
   CuAssertIntEquals(tc, pBegin[3], 0);
   CuAssertIntEquals(tc, pBegin[4], 3);
   CuAssertIntEquals(tc, pBegin[5], 0);
   CuAssertIntEquals(tc, pBegin[6], 4);
   CuAssertIntEquals(tc, pBegin[7], 0);
   CuAssertIntEquals(tc, pBegin[8], 5);
   CuAssertIntEquals(tc, pBegin[9], 0);
   CuAssertIntEquals(tc, pBegin[10], 6);
   CuAssertIntEquals(tc, pBegin[11], 0);
   CuAssertIntEquals(tc, pBegin[12], 7);
   CuAssertIntEquals(tc, pBegin[13], 0);
   CuAssertIntEquals(tc, pBegin[14], 8);
   CuAssertIntEquals(tc, pBegin[15], 0);
   CuAssertIntEquals(tc, pBegin[16], 9);
   CuAssertIntEquals(tc, pBegin[17], 0);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_av_delete(av);
}

static void test_apx_dataElement_pack_U32_array(CuTest* tc)
{
   const int arrayLen = 3;
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_av_t *av;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT32, 0);
   apx_dataElement_setArrayLen(&dataElement, arrayLen);

   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint32_t)*arrayLen);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   av = dtl_av_new();
   CuAssertPtrNotNull(tc, av);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(1));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(2));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(3));

   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 1);
   CuAssertIntEquals(tc, pBegin[1], 0);
   CuAssertIntEquals(tc, pBegin[2], 0);
   CuAssertIntEquals(tc, pBegin[3], 0);
   CuAssertIntEquals(tc, pBegin[4], 2);
   CuAssertIntEquals(tc, pBegin[5], 0);
   CuAssertIntEquals(tc, pBegin[6], 0);
   CuAssertIntEquals(tc, pBegin[7], 0);
   CuAssertIntEquals(tc, pBegin[8], 3);
   CuAssertIntEquals(tc, pBegin[9], 0);
   CuAssertIntEquals(tc, pBegin[10], 0);
   CuAssertIntEquals(tc, pBegin[11], 0);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_av_delete(av);
}

static void test_apx_dataElement_pack_S8_array(CuTest* tc)
{
   const int arrayLen = 5;
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_av_t *av;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_SINT8, 0);
   apx_dataElement_setArrayLen(&dataElement, arrayLen);

   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint8_t)*arrayLen);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   av = dtl_av_new();
   CuAssertPtrNotNull(tc, av);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-2));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-3));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-4));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-5));
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 0xFF);
   CuAssertIntEquals(tc, pBegin[1], 0xFE);
   CuAssertIntEquals(tc, pBegin[2], 0xFD);
   CuAssertIntEquals(tc, pBegin[3], 0xFC);
   CuAssertIntEquals(tc, pBegin[4], 0xFB);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_av_delete(av);
}

static void test_apx_dataElement_pack_S16_array(CuTest* tc)
{
   const int arrayLen = 5;
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_av_t *av;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT16, 0);
   apx_dataElement_setArrayLen(&dataElement, arrayLen);

   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint16_t)*arrayLen);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   av = dtl_av_new();
   CuAssertPtrNotNull(tc, av);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-2));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-3));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-4));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-5));

   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 0xFF);
   CuAssertIntEquals(tc, pBegin[1], 0xFF);
   CuAssertIntEquals(tc, pBegin[2], 0xFE);
   CuAssertIntEquals(tc, pBegin[3], 0xFF);
   CuAssertIntEquals(tc, pBegin[4], 0xFD);
   CuAssertIntEquals(tc, pBegin[5], 0xFF);
   CuAssertIntEquals(tc, pBegin[6], 0xFC);
   CuAssertIntEquals(tc, pBegin[7], 0xFF);
   CuAssertIntEquals(tc, pBegin[8], 0xFB);
   CuAssertIntEquals(tc, pBegin[9], 0xFF);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_av_delete(av);
}

static void test_apx_dataElement_pack_S32_array(CuTest* tc)
{
   const int arrayLen = 3;
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_av_t *av;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_UINT32, 0);
   apx_dataElement_setArrayLen(&dataElement, arrayLen);

   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint32_t)*arrayLen);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   av = dtl_av_new();
   CuAssertPtrNotNull(tc, av);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-2));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-3));

   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 0xFF);
   CuAssertIntEquals(tc, pBegin[1], 0xFF);
   CuAssertIntEquals(tc, pBegin[2], 0xFF);
   CuAssertIntEquals(tc, pBegin[3], 0xFF);
   CuAssertIntEquals(tc, pBegin[4], 0xFE);
   CuAssertIntEquals(tc, pBegin[5], 0xFF);
   CuAssertIntEquals(tc, pBegin[6], 0xFF);
   CuAssertIntEquals(tc, pBegin[7], 0xFF);
   CuAssertIntEquals(tc, pBegin[8], 0xFD);
   CuAssertIntEquals(tc, pBegin[9], 0xFF);
   CuAssertIntEquals(tc, pBegin[10], 0xFF);
   CuAssertIntEquals(tc, pBegin[11], 0xFF);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_av_delete(av);
}

static void test_apx_dataElement_pack_string(CuTest *tc)
{
   const int arrayLen = 13;
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_sv_t *sv;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_STRING, 0);
   apx_dataElement_setArrayLen(&dataElement, arrayLen);

   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint8_t)*arrayLen);

   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);

   sv = dtl_sv_make_cstr("");
   memset(pBegin, 0,  sizeof(uint8_t)*arrayLen);

   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) sv);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 0);
   CuAssertIntEquals(tc, pBegin[1], 0);
   CuAssertIntEquals(tc, pBegin[2], 0);
   CuAssertIntEquals(tc, pBegin[3], 0);
   CuAssertIntEquals(tc, pBegin[4], 0);
   CuAssertIntEquals(tc, pBegin[5], 0);
   CuAssertIntEquals(tc, pBegin[6], 0);
   CuAssertIntEquals(tc, pBegin[7], 0);
   CuAssertIntEquals(tc, pBegin[8], 0);
   CuAssertIntEquals(tc, pBegin[9], 0);
   CuAssertIntEquals(tc, pBegin[10], 0);
   CuAssertIntEquals(tc, pBegin[11], 0);
   CuAssertIntEquals(tc, pBegin[12], 0);
   dtl_sv_set_cstr(sv, "Hello World!");
   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) sv);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, 'H', pBegin[0]);
   CuAssertIntEquals(tc, 'e', pBegin[1]);
   CuAssertIntEquals(tc, 'l', pBegin[2]);
   CuAssertIntEquals(tc, 'l', pBegin[3]);
   CuAssertIntEquals(tc, 'o', pBegin[4]);
   CuAssertIntEquals(tc, ' ', pBegin[5]);
   CuAssertIntEquals(tc, 'W', pBegin[6]);
   CuAssertIntEquals(tc, 'o', pBegin[7]);
   CuAssertIntEquals(tc, 'r', pBegin[8]);
   CuAssertIntEquals(tc, 'l', pBegin[9]);
   CuAssertIntEquals(tc, 'd', pBegin[10]);
   CuAssertIntEquals(tc, '!', pBegin[11]);
   CuAssertIntEquals(tc, 0, pBegin[12]);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_sv_delete(sv);
}

static void test_apx_dataElement_pack_pair(CuTest *tc)
{
   apx_dataElement_t dataElement;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_av_t *av;

   apx_dataElement_create(&dataElement, APX_BASE_TYPE_RECORD, 0);
   apx_dataElement_appendChild(&dataElement, apx_dataElement_new(APX_BASE_TYPE_UINT16, 0));
   apx_dataElement_appendChild(&dataElement, apx_dataElement_new(APX_BASE_TYPE_UINT16, 0));

   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, sizeof(uint16_t)*2);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   av = dtl_av_new();
   CuAssertPtrNotNull(tc, av);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(7));
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(15));

   pResult = apx_dataElement_pack_dv(&dataElement, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, pBegin[0], 7);
   CuAssertIntEquals(tc, pBegin[1], 0);
   CuAssertIntEquals(tc, pBegin[2], 15);
   CuAssertIntEquals(tc, pBegin[3], 0);
   apx_dataElement_destroy(&dataElement);
   adt_bytearray_delete(array);
   dtl_av_delete(av);

}

static void test_apx_dataElement_pack_deep(CuTest *tc)
{
   apx_dataElement_t *rootElem;
   apx_dataElement_t *childElem;
   apx_dataElement_t *grandChildElem;
   adt_bytearray_t *array;
   uint8_t *pBegin;
   uint8_t *pEnd;
   uint8_t *pResult;
   dtl_av_t *av;
   dtl_av_t *av1;

   rootElem = apx_dataElement_new(APX_BASE_TYPE_RECORD, 0);
   childElem = apx_dataElement_new(APX_BASE_TYPE_RECORD, 0);

   apx_dataElement_appendChild(childElem, apx_dataElement_new(APX_BASE_TYPE_UINT8, 0));
   grandChildElem = apx_dataElement_new(APX_BASE_TYPE_STRING, 0);
   apx_dataElement_setArrayLen(grandChildElem, 9);
   apx_dataElement_appendChild(childElem, grandChildElem);
   apx_dataElement_appendChild(rootElem, apx_dataElement_new(APX_BASE_TYPE_UINT32, 0));
   apx_dataElement_appendChild(rootElem, childElem);


   array = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(array, 4+1+9);
   pBegin = adt_bytearray_data(array);
   pEnd = pBegin + adt_bytearray_length(array);
   av1 = dtl_av_new();
   CuAssertPtrNotNull(tc, av1);
   dtl_av_push(av1, (dtl_dv_t*) dtl_sv_make_i32(3));
   dtl_av_push(av1, (dtl_dv_t*) dtl_sv_make_cstr("        "));
   av = dtl_av_new();
   CuAssertPtrNotNull(tc, av);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xFFFFFFFF));
   dtl_av_push(av, (dtl_dv_t*) av1);

   pResult = apx_dataElement_pack_dv(rootElem, pBegin, pEnd, (dtl_dv_t*) av);
   CuAssertPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, 0xFF, pBegin[0]);
   CuAssertIntEquals(tc, 0xFF, pBegin[1]);
   CuAssertIntEquals(tc, 0xFF, pBegin[2]);
   CuAssertIntEquals(tc, 0xFF, pBegin[3]);
   CuAssertIntEquals(tc, 3, pBegin[4]);
   CuAssertIntEquals(tc, ' ',pBegin[5]);
   CuAssertIntEquals(tc, ' ',pBegin[6]);
   CuAssertIntEquals(tc, ' ',pBegin[7]);
   CuAssertIntEquals(tc, ' ',pBegin[8]);
   CuAssertIntEquals(tc, ' ',pBegin[9]);
   CuAssertIntEquals(tc, ' ',pBegin[10]);
   CuAssertIntEquals(tc, ' ',pBegin[11]);
   CuAssertIntEquals(tc, ' ',pBegin[12]);
   CuAssertIntEquals(tc, '\0',pBegin[13]);
   apx_dataElement_delete(rootElem);
   adt_bytearray_delete(array);
   dtl_av_delete(av);

}
