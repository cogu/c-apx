/*****************************************************************************
* \file      testsuite_apx_vmSerializer.c
* \author    Conny Gustafsson
* \date      2019-08-11
* \brief     Test suite for apx_vmSerializer
*
* Copyright (c) 2019 Conny Gustafsson
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
#include "apx_vmSerializer.h"
#include "apx_vmdefs.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_vmSerializer_packU8(CuTest* tc);
static void test_apx_vmSerializer_packU16LE(CuTest* tc);
static void test_apx_vmSerializer_packU32LE(CuTest* tc);
//static void test_apx_vmSerializer_packS8(CuTest* tc);
static void test_apx_vmSerializer_packFixedStr(CuTest* tc);
static void test_apx_vmSerializer_packRecordU8(CuTest* tc);
static void test_apx_vmSerializer_packRecordStrU32(CuTest* tc);
static void test_apx_vmSerializer_packBytes(CuTest* tc);
static void test_apx_vmSerializer_packU8Array(CuTest* tc);
static void test_apx_vmSerializer_packDynU8Array(CuTest* tc);
/*static void test_apx_vmSerializer_packU16Array(CuTest* tc);
static void test_apx_vmSerializer_packU16DynArray(CuTest* tc);
static void test_apx_vmSerializer_packU32Array(CuTest* tc);
*/


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_vmSerializer(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU8);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU16LE);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU32LE);

   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packFixedStr);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packRecordU8);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packRecordStrU32);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packBytes);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU8Array);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packDynU8Array);
/*
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU16Array);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU16DynArray);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU32Array);
*/


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * Test pack u8
 */
static void test_apx_vmSerializer_packU8(CuTest* tc)
{
   uint8_t data[3];
   memset(&data[0], 0, sizeof(data));
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmSerializer_packU8(st, (uint8_t) 1u));
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU8(st, (uint8_t) 1u));
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU8(st, (uint8_t) 0x12));
   CuAssertUIntEquals(tc, 0x12, data[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU8(st, (uint8_t) 0xff));
   CuAssertUIntEquals(tc, 0xff, data[2]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmSerializer_packU8(st, (uint8_t) 0));
   apx_vmSerializer_delete(st);
}

/**
 * Test pack uint16
 */
static void test_apx_vmSerializer_packU16LE(CuTest* tc)
{
   uint8_t data[7];
   memset(&data[0], 0, sizeof(data));
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmSerializer_packU16(st, (uint16_t) 1u));
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU16(st, (uint16_t) 1u));
   CuAssertPtrEquals(tc, &data[2], apx_vmSerializer_getWritePtr(st));
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU16(st, (uint16_t) 0x1234));
   CuAssertPtrEquals(tc, &data[4], apx_vmSerializer_getWritePtr(st));
   CuAssertUIntEquals(tc, 0x34, data[2]);
   CuAssertUIntEquals(tc, 0x12, data[3]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU16(st, (uint16_t) 0xffff));
   CuAssertPtrEquals(tc, &data[6], apx_vmSerializer_getWritePtr(st));
   CuAssertUIntEquals(tc, 0xff, data[4]);
   CuAssertUIntEquals(tc, 0xff, data[5]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmSerializer_packU16(st, (uint16_t) 0));
   CuAssertPtrEquals(tc, &data[6], apx_vmSerializer_getWritePtr(st));
   apx_vmSerializer_delete(st);
}

/**
 * Test pack uint32
 */
static void test_apx_vmSerializer_packU32LE(CuTest* tc)
{
   uint8_t data[15];
   memset(&data[0], 0, sizeof(data));
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmSerializer_packU32(st, (uint32_t) 1u));
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU32(st, (uint32_t) 1u));
   CuAssertPtrEquals(tc, &data[4], apx_vmSerializer_getWritePtr(st));
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertUIntEquals(tc, 0, data[2]);
   CuAssertUIntEquals(tc, 0, data[3]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU32(st, (uint32_t) 0x12345678));
   CuAssertPtrEquals(tc, &data[8], apx_vmSerializer_getWritePtr(st));
   CuAssertUIntEquals(tc, 0x78, data[4]);
   CuAssertUIntEquals(tc, 0x56, data[5]);
   CuAssertUIntEquals(tc, 0x34, data[6]);
   CuAssertUIntEquals(tc, 0x12, data[7]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU32(st, (uint32_t) 0xffffffff));
   CuAssertPtrEquals(tc, &data[12], apx_vmSerializer_getWritePtr(st));
   CuAssertUIntEquals(tc, 0xff, data[8]);
   CuAssertUIntEquals(tc, 0xff, data[9]);
   CuAssertUIntEquals(tc, 0xff, data[10]);
   CuAssertUIntEquals(tc, 0xff, data[11]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmSerializer_packU32(st, (uint32_t) 0));
   CuAssertPtrEquals(tc, &data[12], apx_vmSerializer_getWritePtr(st));
   apx_vmSerializer_delete(st);
}
/*
static void test_apx_vmSerializer_packS8(CuTest* tc)
{
   uint8_t data[4];
   memset(&data[0], 0, sizeof(data));
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   CuAssertPtrNotNull(tc, st);
   apx_vmSerializer_start(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS8(st, (int8_t) INT8_MIN));
   CuAssertUIntEquals(tc, 0x80, data[0]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS8(st, (int8_t) -1));
   CuAssertUIntEquals(tc, 0xff, data[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS8(st, (int8_t) 0));
   CuAssertUIntEquals(tc, 0x0, data[2]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS8(st, (int8_t) INT8_MAX));
   CuAssertUIntEquals(tc, 0x7f, data[3]);

   apx_vmSerializer_delete(st);
}
*/

static void test_apx_vmSerializer_packFixedStr(CuTest* tc)
{
   uint8_t packedData[16]; //Assume that fixed string length is 16
   uint8_t verificationData[16] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', 0u, 0u, 0u, 0u, 0u};
   adt_str_t *str = adt_str_new_cstr("Hello World");
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   memset(&packedData[0], 0xff, sizeof(packedData));
   CuAssertPtrNotNull(tc, st);
   apx_vmSerializer_begin(st, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packFixedStr(st, str, 16));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(st));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));
   apx_vmSerializer_delete(st);
   adt_str_delete(str);
}

static void test_apx_vmSerializer_packRecordU8(CuTest* tc)
{
   uint8_t packedData[UINT8_SIZE*3];
   uint8_t verificationData[UINT8_SIZE*3] = {0xff, 0x12, 0xaa};
   const char *key1 = "Red";
   const char *key2 = "Green";
   const char *key3 = "Blue";
   uint8_t testDataRed = 0xff;
   uint8_t testDataGreen = 0x12;
   uint8_t testDataBlue = 0xaa;
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   dtl_hv_t *hv = dtl_hv_new();

   memset(&packedData[0], 0, sizeof(packedData));
   dtl_hv_set_cstr(hv, key1, (dtl_dv_t*) dtl_sv_make_u32(testDataRed), false);
   dtl_hv_set_cstr(hv, key2, (dtl_dv_t*) dtl_sv_make_u32(testDataGreen), false);
   dtl_hv_set_cstr(hv, key3, (dtl_dv_t*) dtl_sv_make_u32(testDataBlue), false);
   apx_vmSerializer_begin(st, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(st, (dtl_dv_t*) hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_recordSelect_cstr(st, key1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(st, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_recordSelect_cstr(st, key2));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(st, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_recordSelect_cstr(st, key3));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(st, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(st);
   dtl_hv_delete(hv);
}

static void test_apx_vmSerializer_packRecordStrU32(CuTest* tc)
{
   uint8_t packedData[12+UINT32_SIZE]; //Assume that maximum length of Name string is 12 bytes.
   uint8_t verificationData[12+UINT32_SIZE] = {'G', 'e', 'o', 'r', 'g', 'e', 0u, 0u, 0u, 0u, 0u, 0u, 0x78, 0x56, 0x34, 0x12};
   dtl_hv_t *hv = dtl_hv_new();
   const char *key1 = "Name";
   const char *key2 = "Id";
   const char *testName = "George";
   uint32_t testId = 0x12345678;

   memset(&packedData[0], 0xff, sizeof(packedData));
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   dtl_hv_set_cstr(hv, key1, (dtl_dv_t*) dtl_sv_make_cstr(testName), false);
   dtl_hv_set_cstr(hv, key2, (dtl_dv_t*) dtl_sv_make_u32(testId), false);
   apx_vmSerializer_begin(st, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(st, (dtl_dv_t*) hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_recordSelect_cstr(st, key1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsFixedStr(st, 12, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_recordSelect_cstr(st, key2));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU32(st, 0, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(st));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(st);
   dtl_hv_delete(hv);

}

static void test_apx_vmSerializer_packBytes(CuTest* tc)
{
   uint8_t packedData[8];
   const uint8_t bytes[8] = {82, 29, 14, 11, 0xff, 0xff, 0xff, 10};
   dtl_sv_t *sv;
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));
   sv = dtl_sv_make_bytes_raw(bytes, (uint32_t) sizeof(bytes));
   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsBytes(sr, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, bytes, sizeof(packedData)));

   apx_vmSerializer_delete(sr);
   dtl_dec_ref(sv);
}


static void test_apx_vmSerializer_packU8Array(CuTest* tc)
{
   uint8_t packedData[5];
   uint8_t verificationData[5] = {1u, 2u, 3u, 4u, 5u};
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(2), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(3), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(4), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(5), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(sr, 5, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(sr);
   dtl_av_delete(av);
}

static void test_apx_vmSerializer_packDynU8Array(CuTest* tc)
{
   uint8_t packedData[10];
   uint8_t verificationData[4] = {3u, 12, 19, 92};
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(12), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(19), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(92), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(sr, 10, APX_DYN_LEN_U8));
   CuAssertPtrEquals(tc, packedData+UINT8_SIZE+UINT8_SIZE*3, apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(verificationData)));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getAdjustedWritePtr(sr));
   apx_vmSerializer_delete(sr);
   dtl_av_delete(av);
}
/*
static void test_apx_vmSerializer_packU16Array(CuTest* tc)
{
   uint8_t packedData[4*UINT16_SIZE];
   uint8_t verificationData[4*UINT16_SIZE] = {0x00, 0x00, 0x34, 0x12, 0x10, 0xff, 0xff, 0xff};
                                              //0x0000   ,  0x1234     0xff10       0xffff
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x0000), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x1234), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xff10), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xffff), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU16(sr, 4, false));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(verificationData)));

   apx_vmSerializer_delete(sr);
   dtl_av_delete(av);
}

static void test_apx_vmSerializer_packU16DynArray(CuTest* tc)
{
   uint8_t packedData[10*UINT16_SIZE];
   uint8_t verificationData[2*UINT16_SIZE] = {0x00, 0x00, 0xff, 0xff};
                                              //0x0000    0xffff
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x0000), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xffff), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsArray(sr, APX_VARIANT_U16, 10, true));
   CuAssertPtrEquals(tc, packedData+UINT16_SIZE*2, apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(verificationData)));

   apx_vmSerializer_delete(sr);
   dtl_av_delete(av);
}

static void test_apx_vmSerializer_packU32Array(CuTest* tc)
{
   uint8_t packedData[3*UINT32_SIZE];
   uint8_t verificationData[3*UINT32_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff};
                                              //0x00000000          , 0x1234578             , 0xffffffff
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x00000000), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x12345678), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xffffffff), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsArray(sr, APX_VARIANT_U32, 3, false));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(verificationData)));

   apx_vmSerializer_delete(sr);
   dtl_av_delete(av);
}
*/
