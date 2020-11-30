//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx/connection_base.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_connectionBase_alloc(CuTest* tc);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_connectionBase(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_connectionBase_alloc);

   return suite;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_connectionBase_alloc(CuTest* tc)
{
   apx_connectionBase_t connection;
   uint8_t *ptr;
   size_t size;
   int i;
   apx_connectionBase_create(&connection, APX_SERVER_MODE, NULL);
   //allocate small objects
   for(i=1;i<SOA_SMALL_OBJECT_MAX_SIZE;i++)
   {
      char msg[20];
      size = i;
      ptr = apx_connectionBase_alloc(&connection, size);
      sprintf(msg, "size=%d", i);
      CuAssertPtrNotNullMsg(tc, msg, ptr);
      apx_connectionBase_free(&connection, ptr, size);
      apx_allocator_processAll(&connection.allocator);
   }
   //allocate some large objects
   size = 100;
   ptr = apx_connectionBase_alloc(&connection, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_connectionBase_free(&connection, ptr, size);
   size = 1000;
   ptr = apx_connectionBase_alloc(&connection, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_connectionBase_free(&connection, ptr, size);
   size = 10000;
   ptr = apx_connectionBase_alloc(&connection, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_connectionBase_free(&connection, ptr, size);
   apx_allocator_processAll(&connection.allocator);

   apx_connectionBase_destroy(&connection);
}

