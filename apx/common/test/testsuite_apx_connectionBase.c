/*****************************************************************************
* \file      testsuite_apx_connectionBase.c
* \author    Conny Gustafsson
* \date      2020-16-02
* \brief     Description
*
* Copyright (c) 2020 Conny Gustafsson
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
#include "apx_connectionBase.h"
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

