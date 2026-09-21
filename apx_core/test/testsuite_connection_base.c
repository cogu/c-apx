/*****************************************************************************
* \file      testsuite_connection_base.c
* \author    Conny Gustafsson
* \date      2019-05-27
* \brief     Unit tests for connection base
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
static void test_apx_connection_base_alloc(CuTest* tc);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_connection_base(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_connection_base_alloc);

   return suite;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_connection_base_alloc(CuTest* tc)
{
   apx_connection_base_t connection;
   uint8_t *ptr;
   size_t size;
   int i;
   apx_connection_base_create(&connection, APX_SERVER_MODE, NULL);
   //allocate small objects
   for(i=1;i<SOA_SMALL_OBJECT_MAX_SIZE;i++)
   {
      char msg[20];
      size = i;
      ptr = apx_connection_base_alloc(&connection, size);
      sprintf(msg, "size=%d", i);
      CuAssertPtrNotNullMsg(tc, msg, ptr);
      apx_connection_base_free(&connection, ptr, size);
      apx_allocator_process_all(&connection.allocator);
   }
   //allocate some large objects
   size = 100;
   ptr = apx_connection_base_alloc(&connection, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_connection_base_free(&connection, ptr, size);
   size = 1000;
   ptr = apx_connection_base_alloc(&connection, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_connection_base_free(&connection, ptr, size);
   size = 10000;
   ptr = apx_connection_base_alloc(&connection, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_connection_base_free(&connection, ptr, size);
   apx_allocator_process_all(&connection.allocator);

   apx_connection_base_destroy(&connection);
}

