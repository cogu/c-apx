

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "bstr.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_bstr_toUnsignedLong_base10(CuTest* tc);
static void test_bstr_toUnsignedLong_base16(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_bstr(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_bstr_toUnsignedLong_base10);
   SUITE_ADD_TEST(suite, test_bstr_toUnsignedLong_base16);

   return suite;
}
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


static void test_bstr_toUnsignedLong_base10(CuTest* tc)
{
   const char *test_data1 = "123456789";
   const char *test_data2 = "0";
   const char *test_data3 = "4294967295";
   const char *test_data = 0;
   const uint8_t *pBegin;
   const uint8_t *pEnd;
   const uint8_t *pResult = 0;
   unsigned long value;


   test_data = test_data1;
   pBegin = (const uint8_t*) test_data, pEnd = pBegin+strlen(test_data);
   pResult = bstr_toUnsignedLong(pBegin, pEnd, 10, &value);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertUIntEquals(tc, 123456789, value);

   test_data = test_data2;
   pBegin = (const uint8_t*) test_data, pEnd = pBegin+strlen(test_data);
   pResult = bstr_toUnsignedLong(pBegin, pEnd, 10, &value);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertUIntEquals(tc, 0, value);

   test_data = test_data3;
   pBegin = (const uint8_t*) test_data, pEnd = pBegin+strlen(test_data);
   pResult = bstr_toUnsignedLong(pBegin, pEnd, 10, &value);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertUIntEquals(tc, 4294967295UL, value);

}

static void test_bstr_toUnsignedLong_base16(CuTest* tc)
{
   const char *test_data1 = "75BCD15";
   const char *test_data2 = "0";
   const char *test_data3 = "FFFFFFFF";
   const char *test_data = 0;
   const uint8_t *pBegin;
   const uint8_t *pEnd;
   const uint8_t *pResult = 0;
   unsigned long value;


   test_data = test_data1;
   pBegin = (const uint8_t*) test_data, pEnd = pBegin+strlen(test_data);
   pResult = bstr_toUnsignedLong(pBegin, pEnd, 16, &value);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertUIntEquals(tc, 123456789, value);

   test_data = test_data2;
   pBegin = (const uint8_t*) test_data, pEnd = pBegin+strlen(test_data);
   pResult = bstr_toUnsignedLong(pBegin, pEnd, 16, &value);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertUIntEquals(tc, 0, value);

   test_data = test_data3;
   pBegin = (const uint8_t*) test_data, pEnd = pBegin+strlen(test_data);
   pResult = bstr_toUnsignedLong(pBegin, pEnd, 16, &value);
   CuAssertConstPtrEquals(tc, pEnd, pResult);
   CuAssertUIntEquals(tc, 4294967295UL, value);

}
