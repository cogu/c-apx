//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_nodeInfo.h"
#include "apx_parser.h"
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
static void test_apx_nodeInfo_create(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_nodeInfo(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeInfo_create);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeInfo_create(CuTest* tc)
{

   apx_node_t *node1;
   apx_node_t *node2;
   apx_node_t *node3;
   apx_node_t *node4;
   apx_node_t *node5;
   apx_nodeInfo_t nodeInfo1;
   apx_nodeInfo_t nodeInfo2;
   apx_nodeInfo_t nodeInfo3;
   apx_nodeInfo_t nodeInfo4;
   apx_nodeInfo_t nodeInfo5;
   apx_parser_t parser;



   apx_parser_create(&parser);
   node1 = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test1.apx");
   CuAssertPtrNotNull(tc,node1);
   CuAssertStrEquals(tc,"test1",node1->name);

   node2 = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test2.apx");
   CuAssertPtrNotNull(tc,node2);
   CuAssertStrEquals(tc,"test2",node2->name);

   node3 = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test3.apx");
   CuAssertPtrNotNull(tc,node3);
   CuAssertStrEquals(tc,"test3",node3->name);

   node4 = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test4.apx");
   CuAssertPtrNotNull(tc,node4);
   CuAssertStrEquals(tc,"test4",node4->name);

   node5 = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test5.apx");
   CuAssertPtrNotNull(tc,node5);
   CuAssertStrEquals(tc,"test5",node5->name);

   apx_nodeInfo_create(&nodeInfo1,node1);
   apx_nodeInfo_create(&nodeInfo2,node2);
   apx_nodeInfo_create(&nodeInfo3,node3);
   apx_nodeInfo_create(&nodeInfo4,node4);
   apx_nodeInfo_create(&nodeInfo5,node5);
   CuAssertIntEquals(tc,1,apx_portDataMap_getDataLen(&nodeInfo1.inDataMap));
   CuAssertIntEquals(tc,4,apx_portDataMap_getDataLen(&nodeInfo1.outDataMap));
   CuAssertIntEquals(tc,1,apx_portDataMap_getDataLen(&nodeInfo2.inDataMap));
   CuAssertIntEquals(tc,3,apx_portDataMap_getDataLen(&nodeInfo2.outDataMap));
   CuAssertIntEquals(tc,2,apx_portDataMap_getDataLen(&nodeInfo3.inDataMap));
   CuAssertIntEquals(tc,0,apx_portDataMap_getDataLen(&nodeInfo3.outDataMap));
   CuAssertIntEquals(tc,4,apx_portDataMap_getDataLen(&nodeInfo4.inDataMap));
   CuAssertIntEquals(tc,0,apx_portDataMap_getDataLen(&nodeInfo4.outDataMap));
   CuAssertIntEquals(tc,3,apx_portDataMap_getDataLen(&nodeInfo5.inDataMap));
   CuAssertIntEquals(tc,1,apx_portDataMap_getDataLen(&nodeInfo5.outDataMap));
   CuAssertPtrEquals(tc,&nodeInfo1,node1->nodeInfo);
   CuAssertPtrEquals(tc,&nodeInfo2,node2->nodeInfo);
   CuAssertPtrEquals(tc,&nodeInfo3,node3->nodeInfo);
   CuAssertPtrEquals(tc,&nodeInfo4,node4->nodeInfo);
   CuAssertPtrEquals(tc,&nodeInfo5,node5->nodeInfo);


   apx_nodeInfo_destroy(&nodeInfo1);
   apx_nodeInfo_destroy(&nodeInfo2);
   apx_nodeInfo_destroy(&nodeInfo3);
   apx_nodeInfo_destroy(&nodeInfo4);
   apx_nodeInfo_destroy(&nodeInfo5);
   apx_parser_destroy(&parser);

}


