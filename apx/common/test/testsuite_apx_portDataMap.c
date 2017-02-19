//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_portDataMap.h"
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
static void test_apx_portDataMap_create(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_portDataMap(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_portDataMap_create);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portDataMap_create(CuTest* tc)
{
   apx_portDataMap_t dataMap;
   apx_parser_t parser;
   apx_node_t *node;
   apx_portDataMapEntry_t *entry;

   apx_portDataMap_create(&dataMap);
   apx_parser_create(&parser);
   node = apx_parser_parseFile(&parser, APX_TEST_DATA_PATH "test5.apx");
   CuAssertPtrNotNull(tc,node);
   apx_portDataMap_build(&dataMap,node,APX_REQUIRE_PORT);

   CuAssertIntEquals(tc,2,adt_ary_length(&dataMap.elements));

   entry = (apx_portDataMapEntry_t*) *(adt_ary_get(&dataMap.elements,0));
   CuAssertStrEquals(tc,"WheelBasedVehicleSpeed",entry->port->name);
   CuAssertIntEquals(tc,0,entry->offset);
   CuAssertIntEquals(tc,2,entry->length);

   entry = (apx_portDataMapEntry_t*) *(adt_ary_get(&dataMap.elements,1));
   CuAssertStrEquals(tc,"PS_CabTiltLockWarning",entry->port->name);
   CuAssertIntEquals(tc,2,entry->offset);
   CuAssertIntEquals(tc,1,entry->length);

   apx_parser_destroy(&parser);
   apx_portDataMap_destroy(&dataMap);
}


