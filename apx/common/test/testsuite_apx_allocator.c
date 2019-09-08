//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_allocator.h"
#include "apx_parser.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_allocator_create(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_allocator(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_allocator_create);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_allocator_create(CuTest* tc)
{
   uint8_t *data1;
   uint8_t *data2;
   uint8_t *data3;
   uint8_t *data4;
   uint8_t *data128;
   apx_allocator_t allocator;
   apx_allocator_create(&allocator,100);
   apx_allocator_start(&allocator);
   data1 = apx_allocator_alloc(&allocator,1);
   CuAssertPtrNotNull(tc,data1);
   data2 = apx_allocator_alloc(&allocator,2);
   CuAssertPtrNotNull(tc,data2);
   data3 = apx_allocator_alloc(&allocator,3);
   CuAssertPtrNotNull(tc,data3);
   data4 = apx_allocator_alloc(&allocator,4);
   data128 = apx_allocator_alloc(&allocator,128);
   CuAssertPtrNotNull(tc,data4);
   apx_allocator_free(&allocator,data1, 1);
   apx_allocator_free(&allocator,data2, 2);
   apx_allocator_free(&allocator,data3, 3);
   apx_allocator_free(&allocator,data4, 4);
   apx_allocator_free(&allocator,data128, 128);
   CuAssertIntEquals(tc, 5, apx_allocator_numPendingMessages(&allocator));
   apx_allocator_processAll(&allocator);
   apx_allocator_stop(&allocator);
   apx_allocator_destroy(&allocator);
}

