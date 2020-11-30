//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/vm_serializer.h"
#include "apx/vm_defs.h"
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
static void test_apx_vmSerializer_packS8(CuTest* tc);
static void test_apx_vmSerializer_packS16(CuTest* tc);
static void test_apx_vmSerializer_packS32(CuTest* tc);
static void test_apx_vmSerializer_packValueAsString(CuTest* tc);
static void test_apx_vmSerializer_packRecordU8(CuTest* tc);
static void test_apx_vmSerializer_packRecordStrU32(CuTest* tc);
static void test_apx_vmSerializer_packBytes(CuTest* tc);
static void test_apx_vmSerializer_packU8Array(CuTest* tc);
static void test_apx_vmSerializer_packU16LEArray(CuTest* tc);
static void test_apx_vmSerializer_packU32LEArray(CuTest* tc);
static void test_apx_vmSerializer_packDynU8Array(CuTest* tc);
static void test_apx_vmSerializer_packS8Array(CuTest* tc);
static void test_apx_vmSerializer_packS16LEArray(CuTest* tc);
static void test_apx_vmSerializer_packS32LEArray(CuTest* tc);
static void test_apx_vmSerializer_selectRecordElement(CuTest* tc);



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
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packS8);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packS16);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packS32);

   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packValueAsString);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packRecordU8);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packRecordStrU32);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packBytes);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU8Array);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU16LEArray);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packU32LEArray);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packDynU8Array);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packS8Array);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packS16LEArray);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_packS32LEArray);
   SUITE_ADD_TEST(suite, test_apx_vmSerializer_selectRecordElement);


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

static void test_apx_vmSerializer_packS8(CuTest* tc)
{
   uint8_t data[4];
   memset(&data[0], 0, sizeof(data));
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   CuAssertPtrNotNull(tc, st);
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS8(st, (int8_t) -128));
   CuAssertIntEquals(tc, 0x80, data[0]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS8(st, (int8_t) -1));
   CuAssertIntEquals(tc, 0xff, data[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS8(st, (int8_t) 0));
   CuAssertIntEquals(tc, 0x00, data[2]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS8(st, (int8_t) 127));
   CuAssertIntEquals(tc, 0x7f, data[3]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmSerializer_packS8(st, 1));
   apx_vmSerializer_delete(st);
}

static void test_apx_vmSerializer_packS16(CuTest* tc)
{
   uint8_t data[4*UINT16_SIZE];
   memset(&data[0], 0, sizeof(data));
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   CuAssertPtrNotNull(tc, st);
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS16(st, (int16_t) -32768));
   CuAssertIntEquals(tc, 0x00, data[0]);
   CuAssertIntEquals(tc, 0x80, data[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS16(st, (int16_t) -1));
   CuAssertIntEquals(tc, 0xff, data[2]);
   CuAssertIntEquals(tc, 0xff, data[3]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS16(st, (int16_t) 0));
   CuAssertIntEquals(tc, 0x00, data[4]);
   CuAssertIntEquals(tc, 0x00, data[5]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS16(st, (int16_t) 32767));
   CuAssertIntEquals(tc, 0xff, data[6]);
   CuAssertIntEquals(tc, 0x7f, data[7]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmSerializer_packS16(st, (int16_t) 1));
   apx_vmSerializer_delete(st);
}

static void test_apx_vmSerializer_packS32(CuTest* tc)
{
   uint8_t data[4*UINT32_SIZE];
   memset(&data[0], 0, sizeof(data));
   apx_vmSerializer_t *st = apx_vmSerializer_new();
   CuAssertPtrNotNull(tc, st);
   apx_vmSerializer_begin(st, &data[0], sizeof(data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS32(st, (int32_t) INT32_MIN));
   CuAssertIntEquals(tc, 0x00, data[0]);
   CuAssertIntEquals(tc, 0x00, data[1]);
   CuAssertIntEquals(tc, 0x00, data[2]);
   CuAssertIntEquals(tc, 0x80, data[3]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS32(st, (int32_t) -1));
   CuAssertIntEquals(tc, 0xff, data[4]);
   CuAssertIntEquals(tc, 0xff, data[5]);
   CuAssertIntEquals(tc, 0xff, data[6]);
   CuAssertIntEquals(tc, 0xff, data[7]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS32(st, (int32_t) 0));
   CuAssertIntEquals(tc, 0x00, data[8]);
   CuAssertIntEquals(tc, 0x00, data[9]);
   CuAssertIntEquals(tc, 0x00, data[10]);
   CuAssertIntEquals(tc, 0x00, data[11]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packS32(st, (int32_t) 2147483647));
   CuAssertIntEquals(tc, 0xff, data[12]);
   CuAssertIntEquals(tc, 0xff, data[13]);
   CuAssertIntEquals(tc, 0xff, data[14]);
   CuAssertIntEquals(tc, 0x7f, data[15]);
   CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_vmSerializer_packS32(st, (int32_t) 1));
   apx_vmSerializer_delete(st);
}

static void test_apx_vmSerializer_packValueAsString(CuTest* tc)
{
   uint8_t packedData[16]; //Assume that fixed string length is 16
   uint8_t verificationData[16] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', 0u, 0u, 0u, 0u, 0u};
   dtl_sv_t *sv = dtl_sv_make_cstr("Hello World");
   apx_vmSerializer_t *sr = apx_vmSerializer_new();
   memset(&packedData[0], 0xff, sizeof(packedData));
   CuAssertPtrNotNull(tc, sr);
   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   apx_vmSerializer_setValue(sr, (dtl_dv_t*) sv);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsString(sr, 16, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));
   apx_vmSerializer_delete(sr);
   dtl_dec_ref(sv);
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
   apx_vmSerializer_t *sr = apx_vmSerializer_new();
   dtl_hv_t *hv = dtl_hv_new();

   memset(&packedData[0], 0, sizeof(packedData));
   dtl_hv_set_cstr(hv, key1, (dtl_dv_t*) dtl_sv_make_u32(testDataRed), false);
   dtl_hv_set_cstr(hv, key2, (dtl_dv_t*) dtl_sv_make_u32(testDataGreen), false);
   dtl_hv_set_cstr(hv, key3, (dtl_dv_t*) dtl_sv_make_u32(testDataBlue), false);
   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_selectRecordElement_cstr(sr, key1, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(sr, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_selectRecordElement_cstr(sr, key2, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(sr, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_selectRecordElement_cstr(sr, key3, true));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU8(sr, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(sr);
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_selectRecordElement_cstr(st, key1, false));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsString(st, 12, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_selectRecordElement_cstr(st, key2, true));
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

static void test_apx_vmSerializer_packU16LEArray(CuTest* tc)
{
   uint8_t packedData[4];
   uint8_t verificationData[4] = {0x34, 0x12, 0x34, 0x12};
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x1234), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x1234), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU16(sr, 2, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(sr);
   dtl_av_delete(av);
}

static void test_apx_vmSerializer_packU32LEArray(CuTest* tc)
{
   uint8_t packedData[8];
   const uint8_t verificationData1[8] = {0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12};
   const uint8_t verificationData2[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x12345678), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x12345678), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU32(sr, 2, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData1, sizeof(packedData)));

   dtl_av_set(av, 0, (dtl_dv_t*) dtl_sv_make_u32(0xffffffff));
   dtl_av_set(av, 1, (dtl_dv_t*) dtl_sv_make_u32(0xffffffff));

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsU32(sr, 2, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData2, sizeof(packedData)));

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

static void test_apx_vmSerializer_packS8Array(CuTest* tc)
{
   uint8_t packedData[4*UINT8_SIZE];
   uint8_t verificationData[4*UINT8_SIZE] = {0x80, 0xff, 0x01, 0x7f};
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-128), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(127), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsS8(sr, 4, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(sr);
   dtl_dec_ref(av);
}

static void test_apx_vmSerializer_packS16LEArray(CuTest* tc)
{
   uint8_t packedData[4*UINT16_SIZE];
   uint8_t verificationData[4*UINT16_SIZE] = {0x00, 0x80, 0xff, 0xff, 0x01, 0x00, 0xff, 0x7f};
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-32768), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(32767), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsS16(sr, 4, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(sr);
   dtl_dec_ref(av);

}

static void test_apx_vmSerializer_packS32LEArray(CuTest* tc)
{
   uint8_t packedData[4*UINT32_SIZE];
   uint8_t verificationData[4*UINT32_SIZE] = {0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x7f};
   dtl_av_t *av = dtl_av_new();
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   memset(&packedData[0], 0xff, sizeof(packedData));

   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(INT32_MIN), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(2147483647), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_packValueAsS32(sr, 4, APX_DYN_LEN_NONE));
   CuAssertPtrEquals(tc, packedData+sizeof(packedData), apx_vmSerializer_getWritePtr(sr));
   CuAssertIntEquals(tc, 0, memcmp(packedData, verificationData, sizeof(packedData)));

   apx_vmSerializer_delete(sr);
   dtl_dec_ref(av);
}

static void test_apx_vmSerializer_selectRecordElement(CuTest* tc)
{
   dtl_hv_t *hv;
   uint8_t packedData[2*SINT8_SIZE];
   apx_vmWriteState_t *currentState;
   apx_vmWriteState_t *parentState;
   apx_vmSerializer_t *sr = apx_vmSerializer_new();

   hv = dtl_hv_new();

   dtl_hv_set_cstr(hv, "First", (dtl_dv_t*) dtl_sv_make_i32(5), false);
   dtl_hv_set_cstr(hv, "Second", (dtl_dv_t*) dtl_sv_make_i32(10), false);

   apx_vmSerializer_begin(sr, &packedData[0], sizeof(packedData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_setValue(sr, (dtl_dv_t*) hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_enterRecordValue(sr, 0, APX_DYN_LEN_NONE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_vmSerializer_selectRecordElement_cstr(sr, "First", false));
   currentState = apx_vmSerializer_getState(sr);
   CuAssertPtrNotNull(tc, currentState);
   parentState = currentState->parent;
   CuAssertPtrNotNull(tc, parentState);
   CuAssertStrEquals(tc, "First", adt_str_cstr(parentState->recordKey));
   CuAssertTrue(tc, !parentState->isLastElement);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(currentState->value.dv));
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(parentState->value.dv));

   apx_vmSerializer_delete(sr);
   dtl_dec_ref(hv);
}

