//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "headerutil.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void testsuite_headerutil32(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_headerutil(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, testsuite_headerutil32);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void testsuite_headerutil32(CuTest* tc)
{
   uint8_t header[4];
   uint32_t value = 10000;
   uint8_t *pResult;
   const uint8_t *pResult2;
   pResult = headerutil_numEncode32(header, (uint32_t) sizeof(header), value);
   CuAssertPtrEquals(tc, header+4,pResult);
   CuAssertIntEquals(tc, 0x80, header[0]);
   CuAssertIntEquals(tc, 0x00, header[1]);
   CuAssertIntEquals(tc, 0x27, header[2]);
   CuAssertIntEquals(tc, 0x10, header[3]);
   pResult2=headerutil_numDecode32(header, header+4, &value);
   CuAssertPtrEquals(tc, header+4,(void*)pResult2);
   CuAssertUIntEquals(tc, 10000, value);

   value = 72;
   pResult = headerutil_numEncode32(header, (uint32_t) sizeof(header), value);
   CuAssertPtrEquals(tc, header+1,pResult);
   CuAssertIntEquals(tc, 72, header[0]);
   pResult2=headerutil_numDecode32(header, header+4, &value);
   CuAssertPtrEquals(tc, header+1,(void*)pResult2);
   CuAssertUIntEquals(tc, 72, value);

   value = 0;
   pResult = headerutil_numEncode32(header, (uint32_t) sizeof(header), value);
   CuAssertPtrEquals(tc, header+1,pResult);
   CuAssertIntEquals(tc, 0, header[0]);
   pResult2=headerutil_numDecode32(header, header+4, &value);
   CuAssertPtrEquals(tc, header+1,(void*)pResult2);
   CuAssertUIntEquals(tc, 0, value);

   value = HEADERUTIL32_MAX_NUM_LONG;
   pResult = headerutil_numEncode32(header, (uint32_t) sizeof(header), value);
   CuAssertPtrEquals(tc, header+4,pResult);
   CuAssertIntEquals(tc, 0xFF, header[0]);
   CuAssertIntEquals(tc, 0xFF, header[1]);
   CuAssertIntEquals(tc, 0xFF, header[2]);
   CuAssertIntEquals(tc, 0xFF, header[3]);
   pResult2=headerutil_numDecode32(header, header+4, &value);
   CuAssertPtrEquals(tc, header+4,(void*)pResult2);
   CuAssertUIntEquals(tc, HEADERUTIL32_MAX_NUM_LONG, value);
}

