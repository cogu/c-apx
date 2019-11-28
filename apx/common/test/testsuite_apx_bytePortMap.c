/*****************************************************************************
* \file      testsuite_apx_bytePortMap.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Unit tests for apx_bytePortMap
*
* Copyright (c) 2018 Conny Gustafsson
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
#include <stddef.h>
#include <stdio.h>
#include "CuTest.h"
#include "apx_bytePortMap.h"
#include "apx_nodeInfo.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define ERROR_SIZE 15

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_bytePortMap_createClientBytePortMap(CuTest* tc);
static void test_apx_bytePortMap_createServerBytePortMap(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_bytePortMap(void)
{
   CuSuite* suite = CuSuiteNew();

//   SUITE_ADD_TEST(suite, test_apx_bytePortMap_createClientBytePortMap);
//   SUITE_ADD_TEST(suite, test_apx_bytePortMap_createServerBytePortMap);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_bytePortMap_createClientBytePortMap(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"ComplexPort\"{\"Left\"L\"Right\"L}\n"
         "R\"U8Port\"C\n"
         "R\"U16Port\"S\n"
         "R\"NamePort\"a[21]\n";
   int32_t i;
   apx_portId_t expected[32] =
   {     0, 0, 0, 0, 0, 0, 0, 0,
         1,
         2, 2,
         3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
         3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
   };
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(apx_text, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   const apx_bytePortMap_t *bytePortMap = apx_nodeInfo_getClientBytePortMap(nodeInfo);
   CuAssertPtrNotNull(tc, nodeInfo);

   CuAssertUIntEquals(tc, 32, apx_bytePortMap_length(bytePortMap));
   for(i=0;i<32;i++)
   {
      char msg[ERROR_SIZE];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected[i], apx_bytePortMap_lookup(bytePortMap, i));
   }

   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_bytePortMap_createServerBytePortMap(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "P\"ComplexPort\"{\"Left\"L\"Right\"L}\n"
         "P\"U8Port\"C\n"
         "P\"U16Port\"S\n"
         "P\"NamePort\"a[21]\n";
   int32_t i;
   apx_portId_t expected[32] =
   {     0, 0, 0, 0, 0, 0, 0, 0,
         1,
         2, 2,
         3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
         3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
   };
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(apx_text, APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   const apx_bytePortMap_t *bytePortMap = apx_nodeInfo_getServerBytePortMap(nodeInfo);
   CuAssertPtrNotNull(tc, nodeInfo);

   CuAssertUIntEquals(tc, 32, apx_bytePortMap_length(bytePortMap));
   for(i=0;i<32;i++)
   {
      char msg[ERROR_SIZE];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected[i], apx_bytePortMap_lookup(bytePortMap, i));
   }

   apx_nodeInfo_delete(nodeInfo);

}


