//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_router.h"
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
static void test_apx_router_create(CuTest* tc);
//static int create_test_nodes(apx_node_t *nodeList, apx_nodeInfo_t *nodeInfoList, apx_port_t **ports, int maxNumNodes);
//static void destroy_test_nodes(apx_node_t *nodeList, apx_nodeInfo_t *nodeInfoList, int numNodes);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_router(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_router_create);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_router_create(CuTest* tc)
{

   apx_node_t *apx_node[5];
   apx_nodeInfo_t nodeInfoList[5];
   apx_parser_t parser;
   apx_router_t router;
   int32_t i;
   adt_ary_t *connectors;

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

   for (i=0;i<5;i++)
   {
      apx_nodeInfo_create(&nodeInfoList[i],apx_node[i]);
   }

   apx_router_create(&router);

   apx_router_attachNodeInfo(&router,&nodeInfoList[0]);
   apx_router_attachNodeInfo(&router,&nodeInfoList[2]);
   apx_router_attachNodeInfo(&router,&nodeInfoList[3]);
   apx_router_attachNodeInfo(&router,&nodeInfoList[4]);
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[0].provideConnectors,0); //test1/WheelBasedVehicleSpeed (current provider)
   if (connectors != 0)
   {
      int32_t numConnectors = adt_ary_length(connectors);
      CuAssertIntEquals(tc,3,numConnectors);
   }

   apx_router_attachNodeInfo(&router,&nodeInfoList[1]);
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[0].provideConnectors,0); //test1/WheelBasedVehicleSpeed (not connected)
   if (connectors != 0)
   {
      int32_t numConnectors = adt_ary_length(connectors);
      CuAssertIntEquals(tc,0,numConnectors);
   }
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[1].provideConnectors,0); //test2/WheelBasedVehicleSpeed (new provider)
   if (connectors != 0)
   {
      int32_t numConnectors = adt_ary_length(connectors);
      CuAssertIntEquals(tc,3,numConnectors);
   }
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[0].provideConnectors,1); //test1/PS_CabTiltLockWarning
   CuAssertPtrNotNull(tc,connectors);
   CuAssertIntEquals(tc,1,adt_ary_length(connectors));


   //Verify port connectors for NodeInfo2
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[1].provideConnectors,0); //test2/WheelBasedVehicleSpeed (this should be connected)
   CuAssertPtrNotNull(tc,connectors);
   CuAssertIntEquals(tc,3,adt_ary_length(connectors));

   //try to detach test2
   apx_router_detachNodeInfo(&router, &nodeInfoList[1]); //detach the overrider node of WheelBasedVehicleSpeed
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[1].provideConnectors,0); //test2/WheelBasedVehicleSpeed (not connected)
   if (connectors != 0)
   {
      int32_t numConnectors = adt_ary_length(connectors);
      CuAssertIntEquals(tc,0,numConnectors);
   }
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[0].provideConnectors,0); //test1/WheelBasedVehicleSpeed (new provider)
   if (connectors != 0)
   {
      int32_t numConnectors = adt_ary_length(connectors);
      CuAssertIntEquals(tc,3,numConnectors);
   }
   //try to attach test2 again
   apx_router_attachNodeInfo(&router, &nodeInfoList[1]); //this will override WheelBasedVehicleSpeed again
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[0].provideConnectors,0); //test1/WheelBasedVehicleSpeed (not connected)
   if (connectors != 0)
   {
      int32_t numConnectors = adt_ary_length(connectors);
      CuAssertIntEquals(tc,0,numConnectors);
   }
   connectors = (adt_ary_t*) *adt_ary_get(&nodeInfoList[1].provideConnectors,0); //test2/WheelBasedVehicleSpeed (new provider)
   if (connectors != 0)
   {
      int32_t numConnectors = adt_ary_length(connectors);
      CuAssertIntEquals(tc,3,numConnectors);
   }


   apx_router_destroy(&router);

   for(i=0;i<5;i++)
   {
      apx_nodeInfo_destroy(&nodeInfoList[i]);
   }
   apx_parser_destroy(&parser);
}




