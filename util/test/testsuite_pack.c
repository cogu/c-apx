//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "pack.h"
#ifdef USE_PLATFORM_TYPES
#include "Platform_Types.h"
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_pack_unpack_LE64(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_pack(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_pack_unpack_LE64);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_pack_unpack_LE64(CuTest* tc)
{
#ifdef USE_PLATFORM_TYPES
   uint64 value;
   uint64 result;
   uint8 buf[8];
#else
   uint64_t value;
   uint64_t result;
   uint8_t buf[8];
#endif
   uint32_t i;
   value = 0x0;
   packLE64(&buf[0], value);
   for (i=0; i<8; i++)
   {
      CuAssertUIntEquals(tc, 0, buf[i]);
   }
   result = unpackLE64(&buf[0]);
   CuAssertULIntEquals(tc, value, result);

   value = 0x0123456789ABCDEFUL;
   packLE64(&buf[0], value);
   CuAssertUIntEquals(tc, 0xEF, buf[0]);
   CuAssertUIntEquals(tc, 0xCD, buf[1]);
   CuAssertUIntEquals(tc, 0xAB, buf[2]);
   CuAssertUIntEquals(tc, 0x89, buf[3]);
   CuAssertUIntEquals(tc, 0x67, buf[4]);
   CuAssertUIntEquals(tc, 0x45, buf[5]);
   CuAssertUIntEquals(tc, 0x23, buf[6]);
   CuAssertUIntEquals(tc, 0x01, buf[7]);
   result = unpackLE64(&buf[0]);
   CuAssertULIntEquals(tc, value, result);

   value = 0xFFFFFFFFFFFFFFFFUL;
   packLE64(&buf[0], value);
   for (i=0; i<8; i++)
   {
      CuAssertUIntEquals(tc, 0xFF, buf[i]);
   }
   result = unpackLE64(&buf[0]);
   CuAssertULIntEquals(tc, value, result);
}

