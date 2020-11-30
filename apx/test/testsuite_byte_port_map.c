//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stddef.h>
#include <stdio.h>
#include "CuTest.h"
#include "apx/byte_port_map.h"
#include "apx/node_info.h"
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


