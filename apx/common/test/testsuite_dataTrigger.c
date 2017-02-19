//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_dataTrigger.h"
#include "apx_parser.h"
#include "apx_router.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#define APX_TEST_DATA_PATH "..\\..\\..\\apx\\common\\test\\data\\"
#else 
#define APX_TEST_DATA_PATH  "../../../apx/common/test/data/"
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_dataTriggerTable_create(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_dataTrigger(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_dataTriggerTable_create);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_dataTriggerTable_create(CuTest* tc)
{

   apx_node_t *apx_node[5];
   apx_nodeInfo_t apx_nodeInfo[5];
   apx_parser_t parser;
   apx_router_t router;
   int32_t i;

   apx_parser_create(&parser);
   apx_node[0] = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test1.apx");
   CuAssertPtrNotNull(tc,apx_node[0]);
   CuAssertStrEquals(tc,"test1",apx_node[0]->name);

   apx_node[1] = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test2.apx");
   CuAssertPtrNotNull(tc,apx_node[1]);
   CuAssertStrEquals(tc,"test2",apx_node[1]->name);

   apx_node[2] = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test3.apx");
   CuAssertPtrNotNull(tc,apx_node[2]);
   CuAssertStrEquals(tc,"test3",apx_node[2]->name);

   apx_node[3] = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test4.apx");
   CuAssertPtrNotNull(tc,apx_node[3]);
   CuAssertStrEquals(tc,"test4",apx_node[3]->name);

   apx_node[4] = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test5.apx");
   CuAssertPtrNotNull(tc,apx_node[4]);
   CuAssertStrEquals(tc,"test5",apx_node[4]->name);

   apx_nodeInfo_create(&apx_nodeInfo[0],apx_node[0]);
   apx_nodeInfo_create(&apx_nodeInfo[1],apx_node[1]);
   apx_nodeInfo_create(&apx_nodeInfo[2],apx_node[2]);
   apx_nodeInfo_create(&apx_nodeInfo[3],apx_node[3]);
   apx_nodeInfo_create(&apx_nodeInfo[4],apx_node[4]);
   apx_router_create(&router);

   for (i=0;i<5;i++)
   {
      apx_router_attachNodeInfo(&router,&apx_nodeInfo[i]);
   }



   apx_router_destroy(&router);

   for(i=0;i<5;i++)
   {
      apx_nodeInfo_destroy(&apx_nodeInfo[i]);
   }
   apx_parser_destroy(&parser);
}


