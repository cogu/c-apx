//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_node.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_node_create(CuTest* tc);
static void test_apx_node_initValue_U8(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_node(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_node_create);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U8);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_node_create(CuTest* tc)
{
   apx_node_t node;
   void **ptr;
   apx_port_t *port;
   apx_node_create(&node,"Dummy");
   apx_node_createRequirePort(&node,"WheelBasedVehicleSpeed","S",0);
   apx_node_createRequirePort(&node,"ParkBrakeActuationFalue","C(0,3)",0);
   apx_node_finalize(&node);
   CuAssertIntEquals(tc,2,adt_ary_length(&node.requirePortList));
   ptr = adt_ary_get(&node.requirePortList,0);
   CuAssertPtrNotNull(tc,ptr);
   port = (apx_port_t*) *ptr;
   CuAssertIntEquals(tc,2,apx_dataSignature_packLen(&port->derivedDsg));
   CuAssertIntEquals(tc,0,apx_port_getPortIndex(port));
   ptr = adt_ary_get(&node.requirePortList,1);
   CuAssertPtrNotNull(tc,ptr);
   port = (apx_port_t*) *ptr;
   CuAssertIntEquals(tc,1,apx_dataSignature_packLen(&port->derivedDsg));
   CuAssertIntEquals(tc,1,apx_port_getPortIndex(port));
   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U8(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   adt_bytearray_t *initData;
   uint8_t *data;
   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U8Signal1","C","=0");
   port2 = apx_node_createRequirePort(&node,"U8Signal2","C","=255");
   apx_node_finalize(&node);
   initData = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, 1, adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0, data[0]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port2);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, 1, adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 255, data[0]);
   adt_bytearray_delete(initData);

   apx_node_destroy(&node);
}

