//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_routerPortMapEntry.h"
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
static void test_apx_routerPortMapEntry_create(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_routerPortMapEntry(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_routerPortMapEntry_create);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_routerPortMapEntry_create(CuTest* tc)
{
   apx_routerPortMapEntry_t portMapEntry;
   apx_node_t node1;
   apx_node_t node2;
   apx_node_t node3;
   apx_node_t node4;
   apx_node_t node5;

   apx_node_create(&node1, "test1");
   apx_node_create(&node2, "test2");
   apx_node_create(&node3, "test3");
   apx_node_create(&node4, "test4");
   apx_node_create(&node5, "test5");
   apx_node_createProvidePort(&node1, "WheelBasedVehicleSpeed", "S", 0);
   apx_node_createProvidePort(&node2, "WheelBasedVehicleSpeed", "S", 0);
   apx_node_createRequirePort(&node3, "WheelBasedVehicleSpeed", "S", 0);
   apx_node_createRequirePort(&node4, "WheelBasedVehicleSpeed", "S", 0);
   apx_node_createRequirePort(&node5, "WheelBasedVehicleSpeed", "S", 0);
   apx_routerPortMapEntry_create(&portMapEntry);

   apx_routerPortMapEntry_insertProvidePort(&portMapEntry,&node1,0);
   apx_routerPortMapEntry_insertProvidePort(&portMapEntry,&node2,0);
   apx_routerPortMapEntry_insertRequirePort(&portMapEntry,&node3,0);
   apx_routerPortMapEntry_insertRequirePort(&portMapEntry,&node4,0);
   apx_routerPortMapEntry_insertRequirePort(&portMapEntry,&node5,0);
   CuAssertIntEquals(tc, 2, adt_ary_length(&portMapEntry.providePorts));
   CuAssertIntEquals(tc, 3, adt_ary_length(&portMapEntry.requirePorts));
   //test that it prevents duplicate items
   apx_routerPortMapEntry_insertProvidePort(&portMapEntry,&node1,0);
   apx_routerPortMapEntry_insertProvidePort(&portMapEntry,&node2,0);
   apx_routerPortMapEntry_insertRequirePort(&portMapEntry,&node3,0);
   apx_routerPortMapEntry_insertRequirePort(&portMapEntry,&node4,0);
   apx_routerPortMapEntry_insertRequirePort(&portMapEntry,&node5,0);
   CuAssertIntEquals(tc, 2, adt_ary_length(&portMapEntry.providePorts));
   CuAssertIntEquals(tc, 3, adt_ary_length(&portMapEntry.requirePorts));

   apx_routerPortMapEntry_destroy(&portMapEntry);
   apx_node_destroy(&node1);
   apx_node_destroy(&node2);
   apx_node_destroy(&node3);
   apx_node_destroy(&node4);
   apx_node_destroy(&node5);

}


