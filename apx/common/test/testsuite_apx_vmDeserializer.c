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
static void test_apx_vmSerializer_unpackFixedStrAscii(CuTest* tc);
static void test_apx_vmSerializer_unpackFixedStrUtf8(CuTest* tc);


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
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_unpackFixedStrAscii);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_unpackFixedStrUtf8);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_vmDeserializer_unpackU8(CuTest* tc)
{
   uint8_t data[3*1] = {0x00, 0x12, 0xff};
   uint8_t u8Value = 0u;
   apx_vmDeserializer_t *st = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmDeserializer_unpackU8(st, &u8Value));
   apx_vmDeserializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8(st, &u8Value));
   CuAssertUIntEquals(tc, 0x00, u8Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8(st, &u8Value));
   CuAssertUIntEquals(tc, 0x12, u8Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU8(st, &u8Value));
   CuAssertUIntEquals(tc, 0xff, u8Value);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmDeserializer_unpackU8(st, &u8Value));
   apx_vmDeserializer_delete(st);
}
static void test_apx_vmDeserializer_unpackU16LE(CuTest* tc)
{
   uint8_t data[3*2] = {0x00, 0x00, 0x34, 0x12, 0xff, 0xff};
   uint16_t u16Value = 0u;
   apx_vmDeserializer_t *st = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmDeserializer_unpackU16(st, &u16Value));
   apx_vmDeserializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU16(st, &u16Value));
   CuAssertUIntEquals(tc, 0x0000, u16Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU16(st, &u16Value));
   CuAssertUIntEquals(tc, 0x1234, u16Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU16(st, &u16Value));
   CuAssertUIntEquals(tc, 0xffff, u16Value);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmDeserializer_unpackU16(st, &u16Value));
   apx_vmDeserializer_delete(st);
}

static void test_apx_vmDeserializer_unpackU32LE(CuTest* tc)
{
   uint8_t data[3*4] = {0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff};
   uint32_t u32Value = 0u;
   apx_vmDeserializer_t *st = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmDeserializer_unpackU32(st, &u32Value));
   apx_vmDeserializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU32(st, &u32Value));
   CuAssertUIntEquals(tc, 0x00000000, u32Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU32(st, &u32Value));
   CuAssertUIntEquals(tc, 0x12345678, u32Value);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackU32(st, &u32Value));
   CuAssertUIntEquals(tc, 0xffffffff, u32Value);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmDeserializer_unpackU32(st, &u32Value));
   apx_vmDeserializer_delete(st);
}

#define PACKED_DATA_SIZE 16
static void test_apx_vmSerializer_unpackFixedStrAscii(CuTest* tc)
{
   uint8_t packedData[PACKED_DATA_SIZE] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', 0u, 0u, 0u, 0u, 0u};
   const char *verificationString = "Hello World";
   adt_str_t *str = adt_str_new();
   apx_vmDeserializer_t *st = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, st);
   apx_vmDeserializer_begin(st, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackFixedStr(st, str, PACKED_DATA_SIZE));
   CuAssertConstPtrEquals(tc, packedData+sizeof(packedData), apx_vmDeserializer_getReadPtr(st));
   CuAssertIntEquals(tc, 11, adt_str_length(str));
   CuAssertIntEquals(tc, ADT_STR_ENCODING_ASCII, adt_str_getEncoding(str));
   CuAssertStrEquals(tc, verificationString, adt_str_cstr(str));
   apx_vmDeserializer_delete(st);
   adt_str_delete(str);
}

static void test_apx_vmSerializer_unpackFixedStrUtf8(CuTest* tc)
{
   uint8_t packedData[PACKED_DATA_SIZE] = {'K', 0303, 0266, 'p', 'e', 'n', 'h', 'a','m', 'n', 0u, 0u, 0u, 0u, 0u, 0u};
   const char *verificationString = "K\303\266penhamn"; //Köpenhamn
   adt_str_t *str = adt_str_new();
   apx_vmDeserializer_t *st = apx_vmDeserializer_new();
   CuAssertPtrNotNull(tc, st);
   apx_vmDeserializer_begin(st, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmDeserializer_unpackFixedStr(st, str, PACKED_DATA_SIZE));
   CuAssertConstPtrEquals(tc, packedData+sizeof(packedData), apx_vmDeserializer_getReadPtr(st));
   CuAssertIntEquals(tc, ADT_STR_ENCODING_UTF8, adt_str_getEncoding(str));
   CuAssertIntEquals(tc, 9, adt_str_length(str));
   CuAssertStrEquals(tc, verificationString, adt_str_cstr(str));
   apx_vmDeserializer_delete(st);
   adt_str_delete(str);
}
#undef PACKED_DATA_SIZE
