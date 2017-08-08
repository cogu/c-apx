//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_attributeParser.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_attributesParser_parse(CuTest* tc);
static void test_apx_attributeParser_parseInitValue(CuTest* tc);
static void test_apx_attributeParser_parseSingleAttribute(CuTest* tc);
static void test_apx_attributeParser_parseQueueLength(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_apx_attributesParser(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_attributesParser_parse);
   SUITE_ADD_TEST(suite, test_apx_attributeParser_parseInitValue);
   SUITE_ADD_TEST(suite, test_apx_attributeParser_parseSingleAttribute);
   SUITE_ADD_TEST(suite, test_apx_attributeParser_parseQueueLength);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_attributesParser_parse(CuTest* tc)
{
   const char *test_data1 = "=3";
   const char *test_data2 = "=3, P";
   const char *test_data3 = "P";
   const char *test_data4 = "Q[10]";
   const char *test_data5 = "P, {{255, 0}, \"\"}"; //incorrect, it's missing the '=' character
   const char *test_data6 = "P, ={{255, 0}, \"\"}"; //correct
   const char *test_data = 0;
   const uint8_t *pBegin = 0;
   const uint8_t *pEnd = 0;
   const uint8_t *pResult = 0;
   apx_attributeParser_t parser;
   apx_portAttributes_t attr;
   dtl_sv_t *sv;
   int32_t lastError;
   const uint8_t *pErrorNext;


   test_data = test_data1;
   apx_portAttributes_create(&attr, test_data);
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   apx_attributeParser_create(&parser);
   CuAssertPtrEquals(tc, 0, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == false);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);

   apx_attributeParser_parse(&parser, pBegin, pEnd, &attr);

   CuAssertPtrNotNull(tc, attr.initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(attr.initValue));
   sv = (dtl_sv_t*) attr.initValue;
   CuAssertUIntEquals(tc, 3, dtl_sv_get_u32(sv));
   CuAssertTrue(tc, attr.isParameter == false);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);
   apx_portAttributes_destroy(&attr);

   test_data = test_data2;
   apx_portAttributes_create(&attr, test_data);
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   apx_attributeParser_create(&parser);
   CuAssertPtrEquals(tc, 0, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == false);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);

   apx_attributeParser_parse(&parser, pBegin, pEnd, &attr);

   CuAssertPtrNotNull(tc, attr.initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(attr.initValue));
   sv = (dtl_sv_t*) attr.initValue;
   CuAssertUIntEquals(tc, 3, dtl_sv_get_u32(sv));
   CuAssertTrue(tc, attr.isParameter == true);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);
   apx_portAttributes_destroy(&attr);

   test_data = test_data3;
   apx_portAttributes_create(&attr, test_data);
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   apx_attributeParser_create(&parser);
   CuAssertPtrEquals(tc, 0, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == false);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);

   apx_attributeParser_parse(&parser, pBegin, pEnd, &attr);

   CuAssertPtrEquals(tc, 0, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == true);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);
   apx_portAttributes_destroy(&attr);

   test_data = test_data4;
   apx_portAttributes_create(&attr, test_data);
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   apx_attributeParser_create(&parser);
   CuAssertPtrEquals(tc, 0, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == false);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);

   apx_attributeParser_parse(&parser, pBegin, pEnd, &attr);

   CuAssertPtrEquals(tc, 0, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == false);
   CuAssertTrue(tc, attr.isQueued == true);
   CuAssertIntEquals(tc, 10, attr.queueLen);
   apx_portAttributes_destroy(&attr);

   test_data = test_data5;
   apx_portAttributes_create(&attr, test_data);
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   apx_attributeParser_create(&parser);
   CuAssertPtrEquals(tc, 0, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == false);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);

   pResult = apx_attributeParser_parse(&parser, pBegin, pEnd, &attr);

   CuAssertConstPtrEquals(tc, 0, pResult);
   lastError = apx_attributeParser_getLastError(&parser, &pErrorNext);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, lastError);
   CuAssertConstPtrEquals(tc, pBegin + 3, pErrorNext);
   apx_portAttributes_destroy(&attr);


   test_data = test_data6;
   apx_portAttributes_create(&attr, test_data);
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   apx_attributeParser_create(&parser);
   CuAssertPtrEquals(tc, 0, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == false);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);

   apx_attributeParser_parse(&parser, pBegin, pEnd, &attr);

   CuAssertPtrNotNull(tc, attr.initValue);
   CuAssertTrue(tc, attr.isParameter == true);
   CuAssertTrue(tc, attr.isQueued == false);
   CuAssertIntEquals(tc, -1, attr.queueLen);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(attr.initValue));

   {
      dtl_dv_t *dv;
      dtl_av_t *av = (dtl_av_t*) attr.initValue;
      CuAssertIntEquals(tc, 2, dtl_av_length(av));
      dv = *dtl_av_get(av, 0);
      CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
      {
         dtl_av_t *av0 = (dtl_av_t*) dv;
         CuAssertIntEquals(tc, 2, dtl_av_length(av0));
         dv = *dtl_av_get(av0, 0);
         CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
         CuAssertUIntEquals(tc, 255, dtl_sv_get_u32( (dtl_sv_t*) dv));
         dv = *dtl_av_get(av0, 1);
         CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
         CuAssertUIntEquals(tc, 0, dtl_sv_get_u32( (dtl_sv_t*) dv));
      }
      dv = *dtl_av_get(av, 1);
      CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
      {
         const char *str;
         sv = (dtl_sv_t*) dv;
         CuAssertIntEquals(tc, DTL_SV_CSTR, dtl_sv_type(sv));
         str = dtl_sv_get_cstr(sv);
         CuAssertUIntEquals(tc, 0, strlen(str));
      }

   }
   apx_portAttributes_destroy(&attr);

   apx_attributeParser_destroy(&parser);
}

static void test_apx_attributeParser_parseSingleAttribute(CuTest* tc)
{
   const char *test_data1 = "=3 , P"; //init value
   const char *test_data = 0;
   const uint8_t *pBegin = 0;
   const uint8_t *pEnd = 0;
   const uint8_t *pResult = 0;
   apx_attributeParser_t parser;
   apx_portAttributes_t attr;

   test_data = test_data1;
   apx_portAttributes_create(&attr, test_data);
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   apx_attributeParser_create(&parser);
   CuAssertPtrEquals(tc, 0, attr.initValue);
   pResult = apx_attributeParser_parseSingleAttribute(&parser, pBegin, pEnd, &attr);
   CuAssertConstPtrEquals(tc, pBegin+2, pResult);
   CuAssertPtrNotNull(tc, attr.initValue);
   CuAssertTrue(tc, !attr.isParameter);
   pResult = apx_attributeParser_parseSingleAttribute(&parser, pBegin+5, pEnd, &attr);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertTrue(tc, attr.isParameter);
   apx_portAttributes_destroy(&attr);
   apx_attributeParser_destroy(&parser);

}

static void test_apx_attributeParser_parseInitValue(CuTest* tc)
{
   const char *test_data1 = "7";
   const char *test_data2 = "65535";
   const char *test_data3 = "-1";
   const char *test_data4 = "{1,2,3}";
   const char *test_data5 = "{1, 2, 3}";
   const char *test_data6 = "{ 1, 2, 3 }";
   const char *test_data7 = "{ {1, 2, {3, 4}}, {5,6,7}, 8}";
   const char *test_data8 = "\"\"";
   const char *test_data9 = "\"test\"";
   const char *test_data10 = "{\"test1\", {\"test2\", 3, 4}}";

   uint32_t u32Value;
   int32_t s32Value;
   const uint8_t *pBegin;
   const uint8_t *pEnd;
   const uint8_t *pResult;
   dtl_dv_t *initValue = 0;
   dtl_av_t *av = 0;
   dtl_sv_t *sv;
   const char *str;
   apx_attributeParser_t parser;

   apx_attributeParser_create(&parser);
   pBegin = (const uint8_t*) test_data1;
   pEnd = pBegin + strlen(test_data1);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(sv));
   u32Value = dtl_sv_get_u32(sv);
   CuAssertUIntEquals(tc, 7, u32Value);
   dtl_sv_delete(sv);

   pBegin = (const uint8_t*) test_data2;
   pEnd = pBegin + strlen(test_data2);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(sv));
   u32Value = dtl_sv_get_u32(sv);
   CuAssertUIntEquals(tc, 65535, u32Value);
   dtl_sv_delete(sv);

   pBegin = (const uint8_t*) test_data3;
   pEnd = pBegin + strlen(test_data3);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   s32Value = dtl_sv_get_i32(sv);
   CuAssertIntEquals(tc, -1, s32Value);
   dtl_sv_delete(sv);

   pBegin = (const uint8_t*) test_data4;
   pEnd = pBegin + strlen(test_data4);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertPtrEquals(tc, (void*) pEnd, (void*)pResult);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 0));
   CuAssertIntEquals(tc, 1, u32Value);
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 1));
   CuAssertIntEquals(tc, 2, u32Value);
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 2));
   CuAssertIntEquals(tc, 3, u32Value);
   dtl_av_delete(av);

   pBegin = (const uint8_t*) test_data5;
   pEnd = pBegin + strlen(test_data5);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertConstPtrEquals(tc,  pEnd, pResult);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 0));
   CuAssertIntEquals(tc, 1, u32Value);
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 1));
   CuAssertIntEquals(tc, 2, u32Value);
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 2));
   CuAssertIntEquals(tc, 3, u32Value);
   dtl_av_delete(av);

   pBegin = (const uint8_t*) test_data6;
   pEnd = pBegin + strlen(test_data6);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 0));
   CuAssertIntEquals(tc, 1, u32Value);
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 1));
   CuAssertIntEquals(tc, 2, u32Value);
   u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 2));
   CuAssertIntEquals(tc, 3, u32Value);
   dtl_av_delete(av);

   pBegin = (const uint8_t*) test_data7;
   pEnd = pBegin + strlen(test_data7);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   {
      dtl_dv_t *dv;
      dv = *dtl_av_get(av, 0);
      CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
      {
         dtl_av_t *av0;
         av0 = (dtl_av_t*) dv;
         CuAssertIntEquals(tc, 3, dtl_av_length(av0));
         u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av0, 0));
         CuAssertIntEquals(tc, 1, u32Value);
         u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av0, 1));
         CuAssertIntEquals(tc, 2, u32Value);
         dv = *dtl_av_get(av0, 2);
         CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
         {
            dtl_av_t *av02 = (dtl_av_t*) dv;
            CuAssertIntEquals(tc, 2, dtl_av_length(av02));
            u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av02, 0));
            CuAssertUIntEquals(tc, 3, u32Value);
            u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av02, 1));
            CuAssertUIntEquals(tc, 4, u32Value);
         }
      }
      dv = *dtl_av_get(av, 1);
      CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
      {
         dtl_av_t *av1 = (dtl_av_t*) dv;
         CuAssertIntEquals(tc, 3, dtl_av_length(av1));
         u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av1, 0));
         CuAssertUIntEquals(tc, 5, u32Value);
         u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av1, 1));
         CuAssertUIntEquals(tc, 6, u32Value);
         u32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av1, 2));
         CuAssertIntEquals(tc, 7, u32Value);
      }
      dv = *dtl_av_get(av, 2);
      CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
      {
         dtl_sv_t *sv2 = (dtl_sv_t*) dv;
         u32Value = dtl_sv_get_u32(sv2);
         CuAssertUIntEquals(tc, 8, u32Value);
      }
   }
   dtl_av_delete(av);


   pBegin = (const uint8_t*) test_data8;
   pEnd = pBegin + strlen(test_data8);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, DTL_SV_CSTR, dtl_sv_type(sv));
   str = dtl_sv_get_cstr(sv);
   CuAssertUIntEquals(tc, 0, strlen(str));
   dtl_sv_delete(sv);

   pBegin = (const uint8_t*) test_data9;
   pEnd = pBegin + strlen(test_data9);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, DTL_SV_CSTR, dtl_sv_type(sv));
   str = dtl_sv_get_cstr(sv);
   CuAssertUIntEquals(tc, 4, strlen(str));
   CuAssertIntEquals(tc, 0, strcmp(str, "test"));
   dtl_sv_delete(sv);


   pBegin = (const uint8_t*) test_data10;
   pEnd = pBegin + strlen(test_data10);
   pResult = apx_attributeParser_parseInitValue(&parser, pBegin, pEnd, &initValue);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 2, dtl_av_length(av));
   {
      dtl_dv_t *dv;
      dv = *dtl_av_get(av, 0);
      CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
      sv = (dtl_sv_t*) dv;
      CuAssertIntEquals(tc, DTL_SV_CSTR, dtl_sv_type(sv));
      str = dtl_sv_get_cstr(sv);
      CuAssertIntEquals(tc, 0, strcmp(str, "test1"));
      dv = *dtl_av_get(av, 1);
      CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
      {
         dtl_av_t *av1 = (dtl_av_t*) dv;
         CuAssertIntEquals(tc, 3, dtl_av_length(av1));
         dv = *dtl_av_get(av1, 0);
         CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
         sv = (dtl_sv_t*) dv;
         CuAssertIntEquals(tc, DTL_SV_CSTR, dtl_sv_type(sv));
         str = dtl_sv_get_cstr(sv);
         CuAssertIntEquals(tc, 0, strcmp(str, "test2"));
         dv = *dtl_av_get(av1, 1);
         CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
         sv = (dtl_sv_t*) dv;
         CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(sv));
         CuAssertUIntEquals(tc, 3, dtl_sv_get_u32(sv));
         dv = *dtl_av_get(av1, 2);
         CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
         sv = (dtl_sv_t*) dv;
         CuAssertIntEquals(tc, DTL_SV_U32, dtl_sv_type(sv));
         CuAssertUIntEquals(tc, 4, dtl_sv_get_u32(sv));
      }
   }
   dtl_av_delete(av);
   apx_attributeParser_destroy(&parser);

}

static void test_apx_attributeParser_parseQueueLength(CuTest* tc)
{
   const char *test_data1 = "[1]";
   const char *test_data2 = "[120]";
   const char *test_data3 = "[0]";
   const char *test_data4 = "[]";
   const char *test_data5 = "[-1]";
   const char *test_data = 0;
   const uint8_t *pBegin = 0;
   const uint8_t *pEnd = 0;
   const uint8_t *pResult = 0;
   int32_t lastError;
   const uint8_t *pErrorNext;
   apx_attributeParser_t parser;
   apx_portAttributes_t attr;

   apx_attributeParser_create(&parser);

   apx_portAttributes_create(&attr, test_data);
   test_data = test_data1;
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   CuAssertIntEquals(tc, -1, attr.queueLen);
   pResult = apx_attributeParser_parseQueueLength(&parser, pBegin, pEnd, &attr);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, 1, attr.queueLen);
   apx_portAttributes_destroy(&attr);

   apx_portAttributes_create(&attr, test_data);
   test_data = test_data2;
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   CuAssertIntEquals(tc, -1, attr.queueLen);
   pResult = apx_attributeParser_parseQueueLength(&parser, pBegin, pEnd, &attr);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertIntEquals(tc, 120, attr.queueLen);
   apx_portAttributes_destroy(&attr);

   apx_portAttributes_create(&attr, test_data);
   test_data = test_data3;
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   pResult = apx_attributeParser_parseQueueLength(&parser, pBegin, pEnd, &attr);
   CuAssertConstPtrEquals(tc, 0, pResult);
   lastError = apx_attributeParser_getLastError(&parser, &pErrorNext);
   CuAssertIntEquals(tc, APX_INVALID_VALUE_ERROR, lastError);
   CuAssertPtrEquals(tc, (void*) (pBegin+1), (void*) pErrorNext);
   apx_portAttributes_destroy(&attr);

   apx_portAttributes_create(&attr, test_data);
   test_data = test_data4;
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   pResult = apx_attributeParser_parseQueueLength(&parser, pBegin, pEnd, &attr);
   CuAssertConstPtrEquals(tc, 0, pResult);
   lastError = apx_attributeParser_getLastError(&parser, &pErrorNext);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, lastError);
   CuAssertConstPtrEquals(tc, (pBegin+1), pErrorNext);
   apx_portAttributes_destroy(&attr);

   apx_portAttributes_create(&attr, test_data);
   test_data = test_data5;
   pBegin = (const uint8_t*)test_data, pEnd = pBegin+strlen(test_data);
   pResult = apx_attributeParser_parseQueueLength(&parser, pBegin, pEnd, &attr);
   CuAssertConstPtrEquals(tc, 0,pResult);
   lastError = apx_attributeParser_getLastError(&parser, &pErrorNext);
   CuAssertIntEquals(tc, APX_PARSE_ERROR, lastError);
   CuAssertConstPtrEquals(tc, (pBegin+1), pErrorNext);
   apx_portAttributes_destroy(&attr);

   apx_attributeParser_destroy(&parser);
}
