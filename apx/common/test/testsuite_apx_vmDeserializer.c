/*****************************************************************************
* \file      testsuite_apx_vmDeserializer.c
* \author    Conny Gustafsson
* \date      2019-10-03
* \brief     Test suite for apx_vmDeserializer
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
#include "apx_vmDeserializer.h"
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
static void test_apx_vmDeserializer_unpackU8(CuTest* tc);
static void test_apx_vmDeserializer_unpackU16LE(CuTest* tc);
static void test_apx_vmDeserializer_unpackU32LE(CuTest* tc);
static void test_apx_vmDeserializer_unpackFixedStrAscii(CuTest* tc);
static void test_apx_vmDeserializer_unpackFixedStrUtf8(CuTest* tc);
static void test_apx_vmDeserializer_unpackBytes(CuTest* tc);
static void test_apx_vmDeserializer_unpackU8Scalar(CuTest* tc);
static void test_apx_vmDeserializer_unpackU8Array(CuTest* tc);
static void test_apx_vmDeserializer_unpackU8Record(CuTest* tc);
static void test_apx_vmDeserializer_unpackRecordStrU32(CuTest* tc);
static void test_apx_vmDeserializer_unpackBytesValue(CuTest* tc);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_vmDeserializer(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackU8);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackU16LE);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackU32LE);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackFixedStrAscii);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackFixedStrUtf8);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackU8Scalar);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackU8Array);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackU8Record);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackRecordStrU32);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackBytes);
   SUITE_ADD_TEST(suite, test_apx_vmDeserializer_unpackBytesValue);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_vmDeserializer_unpackU8(CuTest* tc)
{
   uint8_t data[3*1] = {0x00, 0x12, 0xff};
   uint8_t u8Value = 0u;
   apx_vmDeserializer_t *sr = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, sr);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmDeserializer_unpackU8(sr, &u8Value));
   apx_vmDeserializer_begin(sr, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8(sr, &u8Value));
   CuAssertUIntEquals(tc, 0x00, u8Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8(sr, &u8Value));
   CuAssertUIntEquals(tc, 0x12, u8Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8(sr, &u8Value));
   CuAssertUIntEquals(tc, 0xff, u8Value);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmDeserializer_unpackU8(sr, &u8Value));
   apx_vmDeserializer_delete(sr);
}
static void test_apx_vmDeserializer_unpackU16LE(CuTest* tc)
{
   uint8_t data[3*2] = {0x00, 0x00, 0x34, 0x12, 0xff, 0xff};
   uint16_t u16Value = 0u;
   apx_vmDeserializer_t *sr = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, sr);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmDeserializer_unpackU16(sr, &u16Value));
   apx_vmDeserializer_begin(sr, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU16(sr, &u16Value));
   CuAssertUIntEquals(tc, 0x0000, u16Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU16(sr, &u16Value));
   CuAssertUIntEquals(tc, 0x1234, u16Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU16(sr, &u16Value));
   CuAssertUIntEquals(tc, 0xffff, u16Value);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmDeserializer_unpackU16(sr, &u16Value));
   apx_vmDeserializer_delete(sr);
}

static void test_apx_vmDeserializer_unpackU32LE(CuTest* tc)
{
   uint8_t data[3*4] = {0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff};
   uint32_t u32Value = 0u;
   apx_vmDeserializer_t *sr = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, sr);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmDeserializer_unpackU32(sr, &u32Value));
   apx_vmDeserializer_begin(sr, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU32(sr, &u32Value));
   CuAssertUIntEquals(tc, 0x00000000, u32Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU32(sr, &u32Value));
   CuAssertUIntEquals(tc, 0x12345678, u32Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU32(sr, &u32Value));
   CuAssertUIntEquals(tc, 0xffffffff, u32Value);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmDeserializer_unpackU32(sr, &u32Value));
   apx_vmDeserializer_delete(sr);
}

#define PACKED_DATA_SIZE 16
static void test_apx_vmDeserializer_unpackFixedStrAscii(CuTest* tc)
{
   uint8_t packedData[PACKED_DATA_SIZE] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', 0u, 0u, 0u, 0u, 0u};
   const char *verificationString = "Hello World";
   adt_str_t *str = adt_str_new();
   apx_vmDeserializer_t *sr = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, sr);
   apx_vmDeserializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackFixedStr(sr, str, PACKED_DATA_SIZE));
   CuAssertConstPtrEquals(tc, packedData+sizeof(packedData), apx_vmDeserializer_getReadPtr(sr));
   CuAssertIntEquals(tc, 11, adt_str_length(str));
   CuAssertIntEquals(tc, ADT_STR_ENCODING_ASCII, adt_str_getEncoding(str));
   CuAssertStrEquals(tc, verificationString, adt_str_cstr(str));
   apx_vmDeserializer_delete(sr);
   adt_str_delete(str);
}

static void test_apx_vmDeserializer_unpackFixedStrUtf8(CuTest* tc)
{
   uint8_t packedData[PACKED_DATA_SIZE] = {'K', 0303, 0266, 'p', 'e', 'n', 'h', 'a','m', 'n', 0u, 0u, 0u, 0u, 0u, 0u};
   const char *verificationString = "K\303\266penhamn"; //Köpenhamn
   adt_str_t *str = adt_str_new();
   apx_vmDeserializer_t *sr = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, sr);
   apx_vmDeserializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackFixedStr(sr, str, PACKED_DATA_SIZE));
   CuAssertConstPtrEquals(tc, packedData+sizeof(packedData), apx_vmDeserializer_getReadPtr(sr));
   CuAssertIntEquals(tc, ADT_STR_ENCODING_UTF8, adt_str_getEncoding(str));
   CuAssertIntEquals(tc, 9, adt_str_length(str));
   CuAssertStrEquals(tc, verificationString, adt_str_cstr(str));
   apx_vmDeserializer_delete(sr);
   adt_str_delete(str);
}
#undef PACKED_DATA_SIZE

static void test_apx_vmDeserializer_unpackU8Scalar(CuTest* tc)
{
   uint8_t packedData[UINT8_SIZE*2] = {0u, 255u};
   dtl_dv_t *dv = 0;
   apx_vmDeserializer_t *sr = apx_vmDeserializer_new();
   apx_vmDeserializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8Value(sr, 0u, APX_DYN_LEN_NONE));
   CuAssertConstPtrEquals(tc, packedData+UINT8_SIZE, apx_vmDeserializer_getReadPtr(sr));
   dv = apx_vmDeserializer_getValue(sr, false);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8Value(sr, 0u, APX_DYN_LEN_NONE));
   CuAssertConstPtrEquals(tc, packedData+UINT8_SIZE*2, apx_vmDeserializer_getReadPtr(sr));
   dv = apx_vmDeserializer_getValue(sr, false);
   CuAssertUIntEquals(tc, 255u, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));
   apx_vmDeserializer_delete(sr);
}

static void test_apx_vmDeserializer_unpackU8Array(CuTest* tc)
{
   const uint32_t arrayLen = 5u;
   uint8_t packedData[UINT8_SIZE*5] = {0x11, 0x22, 0x33, 0x44, 0x55};
   dtl_dv_t *dv = 0;
   dtl_av_t *av;

   uint32_t i;
   apx_vmDeserializer_t *sr = apx_vmDeserializer_new();
   apx_vmDeserializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8Value(sr, arrayLen, APX_DYN_LEN_NONE));
   CuAssertConstPtrEquals(tc, packedData+UINT8_SIZE*arrayLen, apx_vmDeserializer_getReadPtr(sr));
   dv = apx_vmDeserializer_getValue(sr, false);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, arrayLen, dtl_av_length(av));
   for (i=0; i<arrayLen; i++)
   {
      dtl_sv_t *sv = (dtl_sv_t*) dtl_av_value(av, i);
      CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
      CuAssertUIntEquals(tc, packedData[i], dtl_sv_to_u32(sv, NULL));
   }
   apx_vmDeserializer_delete(sr);
}

static void test_apx_vmDeserializer_unpackU8Record(CuTest* tc)
{
   uint8_t packedData[UINT8_SIZE*3] = {0xff, 0x12, 0xaa};
   const char *keyRed = "Red";
   const char *keyGreen = "Green";
   const char *keyBlue = "Blue";
   uint8_t verificationDataRed = 0xff;
   uint8_t verificationDataGreen = 0x12;
   uint8_t verificationDataBlue = 0xaa;
   apx_vmDeserializer_t *ds = apx_vmDeserializer_new();
   dtl_dv_t *dv = 0;
   dtl_hv_t *hv;
   dtl_sv_t *sv;

   apx_vmDeserializer_begin(ds, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_createRecordValue(ds, 0u, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_setRecordKey_cstr(ds, keyRed));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8Value(ds, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_setRecordKey_cstr(ds, keyGreen));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8Value(ds, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_setRecordKey_cstr(ds, keyBlue));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8Value(ds, 0, APX_DYN_LEN_NONE));
   dv = apx_vmDeserializer_getValue(ds, false);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(dv));
   hv = (dtl_hv_t*) dv;
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, keyRed);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, verificationDataRed, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, keyGreen);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, verificationDataGreen, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, keyBlue);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, verificationDataBlue, dtl_sv_to_u32(sv, NULL));
   apx_vmDeserializer_delete(ds);
}

static void test_apx_vmDeserializer_unpackRecordStrU32(CuTest* tc)
{
   uint8_t packedData[12+UINT32_SIZE] = {'G', 'e', 'o', 'r', 'g', 'e', 0u, 0u, 0u, 0u, 0u, 0u, 0x78, 0x56, 0x34, 0x12};
   const char *key1 = "Name";
   const char *key2 = "Id";
   const char *nameVerificationData = "George";
   const uint32_t idVerificationData = 0x12345678;

   apx_vmDeserializer_t *ds = apx_vmDeserializer_new();
   dtl_dv_t *dv = 0;
   dtl_hv_t *hv;
   dtl_sv_t *sv;

   apx_vmDeserializer_begin(ds, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_createRecordValue(ds, 0u, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_setRecordKey_cstr(ds, key1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackStrValue(ds, 12u, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_setRecordKey_cstr(ds, key2));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU32Value(ds, 0, APX_DYN_LEN_NONE));
   CuAssertConstPtrEquals(tc, packedData+16, apx_vmDeserializer_getReadPtr(ds));
   dv = apx_vmDeserializer_getValue(ds, false);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(dv));
   hv = (dtl_hv_t*) dv;
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, key1);
   CuAssertPtrNotNull(tc, sv);
   CuAssertStrEquals(tc, nameVerificationData, dtl_sv_to_cstr(sv));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, key2);
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, idVerificationData, dtl_sv_to_u32(sv, NULL));
   apx_vmDeserializer_delete(ds);
}

static void test_apx_vmDeserializer_unpackBytes(CuTest* tc)
{
   uint8_t packedData[4] = {0x01, 0x02, 0x03, 0x04};
   apx_vmDeserializer_t *ds = apx_vmDeserializer_new();
   adt_bytes_t *bytes = 0;
   const uint8_t *data;

   apx_vmDeserializer_begin(ds, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackBytes(ds, &bytes, 4u));
   CuAssertPtrNotNull(tc, bytes);
   data = adt_bytes_constData(bytes);
   CuAssertUIntEquals(tc, packedData[0], data[0]);
   CuAssertUIntEquals(tc, packedData[1], data[1]);
   CuAssertUIntEquals(tc, packedData[2], data[2]);
   CuAssertUIntEquals(tc, packedData[3], data[3]);
   adt_bytes_delete(bytes);
   apx_vmDeserializer_delete(ds);
}

static void test_apx_vmDeserializer_unpackBytesValue(CuTest* tc)
{
   dtl_dv_t *dv = 0;
   dtl_sv_t *sv;
   const adt_bytes_t *bytes = 0;
   const uint8_t *data;
   int i;
   uint8_t packedData[8] = {0xaa, 0xbb, 0xcd, 0x92, 0x00, 0x18, 0x26, 0x08};
   apx_vmDeserializer_t *ds = apx_vmDeserializer_new();

   apx_vmDeserializer_begin(ds, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackBytesValue(ds, 8, APX_DYN_LEN_NONE));
   CuAssertConstPtrEquals(tc, packedData+8, apx_vmDeserializer_getReadPtr(ds));
   dv = apx_vmDeserializer_getValue(ds, false);
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   sv = (dtl_sv_t*) dv;
   CuAssertIntEquals(tc, DTL_SV_BYTES, dtl_sv_type(sv));
   bytes = dtl_sv_get_bytes(sv);
   CuAssertPtrNotNull(tc, bytes);
   data = adt_bytes_constData(bytes);
   for(i=0; i < 8; i++)
   {
      CuAssertUIntEquals(tc, packedData[i], data[i]);
   }
   apx_vmDeserializer_delete(ds);
}
