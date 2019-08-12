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
static void test_apx_vmSerializer_packU8DynArrayHeader(CuTest* tc);
static void test_apx_vmSerializer_packU16DynArrayHeader(CuTest* tc);
static void test_apx_vmSerializer_packU32DynArrayHeader(CuTest* tc);
static void test_apx_vmSerializer_packFixedStr(CuTest* tc);
static void test_apx_vmSerializer_packRecordU8(CuTest* tc);
static void test_apx_vmSerializer_packRecordStrU32(CuTest* tc);
static void test_apx_vmSerializer_packU16FixedArray(CuTest* tc);

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

   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU8DynArrayHeader);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU16DynArrayHeader);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU32DynArrayHeader);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packFixedStr);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packRecordU8);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packRecordStrU32);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU16FixedArray);


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

/**
 * U8 data into U8 array
 */
static void test_apx_vmSerializer_packU8DynArrayHeader(CuTest* tc)
{
   uint8_t data[UINT8_SIZE];
   uint8_t verificationData[UINT8_SIZE];
   uint8_t array_len = 9;
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   memset(&data[0], 0, sizeof(data));
   verificationData[0] = array_len;

   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmSerializer_packU8DynArrayHeader(st, array_len));
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU8DynArrayHeader(st,array_len));
   CuAssertIntEquals(tc, 0, memcmp(data, verificationData, sizeof(data)));
   apx_vmSerializer_delete(st);
}

/**
 * U8 data into U16 array
 */
static void test_apx_vmSerializer_packU16DynArrayHeader(CuTest* tc)
{
   uint8_t data[UINT16_SIZE];
   uint8_t verificationData[UINT16_SIZE];
   uint16_t array_len = 400;
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   memset(&data[0], 0, sizeof(data));
   packLE(&verificationData[0], (uint32_t) array_len, (uint8_t) sizeof(array_len));

   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmSerializer_packU8DynArrayHeader(st, array_len));
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU16DynArrayHeader(st,array_len));
   CuAssertIntEquals(tc, 0, memcmp(data, verificationData, sizeof(data)));
   apx_vmSerializer_delete(st);
}

/**
 * U8 data into U32 array
 */
static void test_apx_vmSerializer_packU32DynArrayHeader(CuTest* tc)
{
   uint8_t data[UINT32_SIZE];
   uint8_t verificationData[UINT32_SIZE];
   uint32_t array_len = 282400;
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   memset(&data[0], 0, sizeof(data));
   packLE(&verificationData[0], (uint32_t) array_len, (uint8_t) sizeof(array_len));

   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmSerializer_packU8DynArrayHeader(st, array_len));
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packU32DynArrayHeader(st,array_len));
   CuAssertIntEquals(tc, 0, memcmp(data, verificationData, sizeof(data)));
   apx_vmSerializer_delete(st);
}

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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(st, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_recordSelect_cstr(st, key2));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(st, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_recordSelect_cstr(st, key3));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(st, true));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU32(st, true));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(st));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(st);
   dtl_hv_delete(hv);

}

static void test_apx_vmSerializer_packU16FixedArray(CuTest* tc)
{
   uint8_t packedData[4*UINT16_SIZE]; //Assume that array length is 4
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU16Array(sr, 4, false));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(sr);
   dtl_av_delete(av);
}
