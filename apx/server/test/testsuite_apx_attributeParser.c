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

   //SUITE_ADD_TEST(suite, test_apx_attributesParser_parse);
   SUITE_ADD_TEST(suite, test_apx_attributeParser_parseInitValue);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_attributesParser_parse(CuTest* tc)
{
   apx_attributeParser_t parser;
   const char *test_data1 = "= {1, 2, 3}";
   apx_attributeParser_create(&parser);
   apx_attributeParser_parse(&parser,(const uint8_t*) test_data1, (const uint8_t*) (test_data1+strlen(test_data1)));
   apx_attributeParser_destroy(&parser);
}

static void test_apx_attributeParser_parseInitValue(CuTest* tc)
{
   const char *test_data1 = "7";
   const char *test_data2 = "65535";
   const char *test_data3 = "-1";
   const char *test_data4 = "{1,2,3}";
   uint32_t u32Value;
   int32_t s32Value;
   const uint8_t *pBegin;
   const uint8_t *pEnd;
   const uint8_t *pResult;
   dtl_dv_t *initValue = 0;
   dtl_av_t *av = 0;
   dtl_sv_t *sv;
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
   s32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 0));
   CuAssertIntEquals(tc, 1, s32Value);
   s32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 1));
   CuAssertIntEquals(tc, 2, s32Value);
   s32Value = dtl_sv_get_u32((dtl_sv_t*) *dtl_av_get(av, 2));
   CuAssertIntEquals(tc, 3, s32Value);
   dtl_av_delete(av);

   apx_attributeParser_destroy(&parser);

}
