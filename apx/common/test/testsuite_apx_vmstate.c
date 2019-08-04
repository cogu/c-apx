/*****************************************************************************
* \file      testsuite_apx_vmstate.c
* \author    Conny Gustafsson
* \date      2019-03-10
* \brief     Description
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
#include "apx_vmstate.h"
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
static void test_apx_apx_vmstate_packU8(CuTest* tc);
static void test_apx_apx_vmstate_packU16LE(CuTest* tc);
static void test_apx_apx_vmstate_packU32LE(CuTest* tc);
static void test_apx_apx_vmstate_packS8(CuTest* tc);
static void test_apx_apx_vmstate_packU8DynArray(CuTest* tc);
static void test_apx_apx_vmstate_packU16DynArray(CuTest* tc);
static void test_apx_apx_vmstate_packU32DynArray(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_vmstate(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_apx_vmstate_packU8);
   SUITE_ADD_TEST(suite, test_apx_apx_vmstate_packU16LE);
   SUITE_ADD_TEST(suite, test_apx_apx_vmstate_packU32LE);
   SUITE_ADD_TEST(suite, test_apx_apx_vmstate_packU8DynArray);
   SUITE_ADD_TEST(suite, test_apx_apx_vmstate_packU16DynArray);
   SUITE_ADD_TEST(suite, test_apx_apx_vmstate_packU32DynArray);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * Test pack u8
 */
static void test_apx_apx_vmstate_packU8(CuTest* tc)
{
   uint8_t data[3];
   memset(&data[0], 0, sizeof(data));
   apx_vmstate_t *st = apx_vmstate_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmstate_packU8(st, (uint8_t) 1u));
   apx_vmstate_setWriteData(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU8(st, (uint8_t) 1u));
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU8(st, (uint8_t) 0x12));
   CuAssertUIntEquals(tc, 0x12, data[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU8(st, (uint8_t) 0xff));
   CuAssertUIntEquals(tc, 0xff, data[2]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmstate_packU8(st, (uint8_t) 0));
   apx_vmstate_delete(st);
}

/**
 * Test pack uint16
 */
static void test_apx_apx_vmstate_packU16LE(CuTest* tc)
{
   uint8_t data[7];
   memset(&data[0], 0, sizeof(data));
   apx_vmstate_t *st = apx_vmstate_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmstate_packU16(st, (uint16_t) 1u));
   apx_vmstate_setWriteData(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU16(st, (uint16_t) 1u));
   CuAssertPtrEquals(tc, &data[2], apx_vmstate_getWritePtr(st));
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU16(st, (uint16_t) 0x1234));
   CuAssertPtrEquals(tc, &data[4], apx_vmstate_getWritePtr(st));
   CuAssertUIntEquals(tc, 0x34, data[2]);
   CuAssertUIntEquals(tc, 0x12, data[3]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU16(st, (uint16_t) 0xffff));
   CuAssertPtrEquals(tc, &data[6], apx_vmstate_getWritePtr(st));
   CuAssertUIntEquals(tc, 0xff, data[4]);
   CuAssertUIntEquals(tc, 0xff, data[5]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmstate_packU16(st, (uint16_t) 0));
   CuAssertPtrEquals(tc, &data[6], apx_vmstate_getWritePtr(st));
   apx_vmstate_delete(st);
}

/**
 * Test pack uint32
 */
static void test_apx_apx_vmstate_packU32LE(CuTest* tc)
{
   uint8_t data[15];
   memset(&data[0], 0, sizeof(data));
   apx_vmstate_t *st = apx_vmstate_new();
   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmstate_packU32(st, (uint32_t) 1u));
   apx_vmstate_setWriteData(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU32(st, (uint32_t) 1u));
   CuAssertPtrEquals(tc, &data[4], apx_vmstate_getWritePtr(st));
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertUIntEquals(tc, 0, data[2]);
   CuAssertUIntEquals(tc, 0, data[3]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU32(st, (uint32_t) 0x12345678));
   CuAssertPtrEquals(tc, &data[8], apx_vmstate_getWritePtr(st));
   CuAssertUIntEquals(tc, 0x78, data[4]);
   CuAssertUIntEquals(tc, 0x56, data[5]);
   CuAssertUIntEquals(tc, 0x34, data[6]);
   CuAssertUIntEquals(tc, 0x12, data[7]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU32(st, (uint32_t) 0xffffffff));
   CuAssertPtrEquals(tc, &data[12], apx_vmstate_getWritePtr(st));
   CuAssertUIntEquals(tc, 0xff, data[8]);
   CuAssertUIntEquals(tc, 0xff, data[9]);
   CuAssertUIntEquals(tc, 0xff, data[10]);
   CuAssertUIntEquals(tc, 0xff, data[11]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmstate_packU32(st, (uint32_t) 0));
   CuAssertPtrEquals(tc, &data[12], apx_vmstate_getWritePtr(st));
   apx_vmstate_delete(st);
}

/**
 * U8 data into U8 array
 */
static void test_apx_apx_vmstate_packU8DynArray(CuTest* tc)
{
   uint8_t data[UINT8_SIZE];
   uint8_t verificationData[UINT8_SIZE];
   uint8_t array_len = 9;
   apx_vmstate_t *st = apx_vmstate_new();
   memset(&data[0], 0, sizeof(data));
   verificationData[0] = array_len;

   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmstate_packU8DynArray(st, array_len));
   apx_vmstate_setWriteData(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU8DynArray(st,array_len));
   CuAssertIntEquals(tc, 0, memcmp(data, verificationData, sizeof(data)));
   apx_vmstate_delete(st);
}

/**
 * U8 data into U16 array
 */
static void test_apx_apx_vmstate_packU16DynArray(CuTest* tc)
{
   uint8_t data[UINT16_SIZE];
   uint8_t verificationData[UINT16_SIZE];
   uint16_t array_len = 400;
   apx_vmstate_t *st = apx_vmstate_new();
   memset(&data[0], 0, sizeof(data));
   packLE(&verificationData[0], (uint32_t) array_len, (uint8_t) sizeof(array_len));

   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmstate_packU8DynArray(st, array_len));
   apx_vmstate_setWriteData(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU16DynArray(st,array_len));
   CuAssertIntEquals(tc, 0, memcmp(data, verificationData, sizeof(data)));
   apx_vmstate_delete(st);
}

/**
 * U8 data into U32 array
 */
static void test_apx_apx_vmstate_packU32DynArray(CuTest* tc)
{
   uint8_t data[UINT32_SIZE];
   uint8_t verificationData[UINT32_SIZE];
   uint32_t array_len = 282400;
   apx_vmstate_t *st = apx_vmstate_new();
   memset(&data[0], 0, sizeof(data));
   packLE(&verificationData[0], (uint32_t) array_len, (uint8_t) sizeof(array_len));

   CuAssertPtrNotNull(tc, st);
   CuAssertIntEquals(tc, APX_MISSING_BUFFER_ERROR, apx_vmstate_packU8DynArray(st, array_len));
   apx_vmstate_setWriteData(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmstate_packU32DynArray(st,array_len));
   CuAssertIntEquals(tc, 0, memcmp(data, verificationData, sizeof(data)));
   apx_vmstate_delete(st);
}
