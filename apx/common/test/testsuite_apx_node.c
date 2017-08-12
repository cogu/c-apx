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
static void test_apx_node_initValue_U16(CuTest* tc);
static void test_apx_node_initValue_U32(CuTest* tc);
static void test_apx_node_initValue_S8(CuTest* tc);
static void test_apx_node_initValue_S16(CuTest* tc);
static void test_apx_node_initValue_S32(CuTest* tc);
static void test_apx_node_initValue_U8Array(CuTest* tc);

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

   //SUITE_ADD_TEST(suite, test_apx_node_create);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U8);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U16);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U32);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S8);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S16);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S32);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U8Array);


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
   CuAssertIntEquals(tc, sizeof(uint8_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0, data[0]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port2);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint8_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 255, data[0]);
   adt_bytearray_delete(initData);

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U16(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   apx_port_t *port3;
   adt_bytearray_t *initData;
   uint8_t *data;
   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U16Signal1","S","=0");
   port2 = apx_node_createRequirePort(&node,"U16Signal2","S","=65535");
   port3 = apx_node_createRequirePort(&node,"U16Signal3","S","=0x1234");
   apx_node_finalize(&node);
   initData = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port2);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 255, data[0]);
   CuAssertUIntEquals(tc, 255, data[1]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port3);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0x34, data[0]);
   CuAssertUIntEquals(tc, 0x12, data[1]);
   adt_bytearray_delete(initData);

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U32(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   apx_port_t *port3;
   apx_port_t *port4;
   adt_bytearray_t *initData;
   uint8_t *data;
   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","L","=0");
   port2 = apx_node_createRequirePort(&node,"U32Signal2","L","=4294967295");
   port3 = apx_node_createRequirePort(&node,"U32Signal4","L","=0xFFFFFFFF");
   port4 = apx_node_createRequirePort(&node,"U32Signal3","L","=0x12345678");
   apx_node_finalize(&node);
   initData = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertUIntEquals(tc, 0, data[2]);
   CuAssertUIntEquals(tc, 0, data[3]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port2);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 255, data[0]);
   CuAssertUIntEquals(tc, 255, data[1]);
   CuAssertUIntEquals(tc, 255, data[2]);
   CuAssertUIntEquals(tc, 255, data[3]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port3);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 255, data[0]);
   CuAssertUIntEquals(tc, 255, data[1]);
   CuAssertUIntEquals(tc, 255, data[2]);
   CuAssertUIntEquals(tc, 255, data[3]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port4);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0x78, data[0]);
   CuAssertUIntEquals(tc, 0x56, data[1]);
   CuAssertUIntEquals(tc, 0x34, data[2]);
   CuAssertUIntEquals(tc, 0x12, data[3]);
   adt_bytearray_delete(initData);

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S8(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   apx_port_t *port3;
   apx_port_t *port4;
   apx_port_t *port5;
   apx_port_t *port6;
   adt_bytearray_t *initData;
   uint8_t *data;
   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","c","=0");
   port2 = apx_node_createRequirePort(&node,"U32Signal2","c","=-1");
   port3 = apx_node_createRequirePort(&node,"U32Signal3","c","=-128");
   port4 = apx_node_createRequirePort(&node,"U32Signal4","c","=-27");
   port5 = apx_node_createRequirePort(&node,"U32Signal5","c","=27");
   port6 = apx_node_createRequirePort(&node,"U32Signal6","c","=127");
   apx_node_finalize(&node);
   initData = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint8_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0, data[0]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port2);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint8_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) -1, data[0]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port3);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint8_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) -128, data[0]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port4);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint8_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) -27, data[0]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port5);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint8_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) 27, data[0]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port6);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint8_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) 127, data[0]);
   adt_bytearray_delete(initData);

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S16(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   apx_port_t *port3;
   apx_port_t *port4;
   apx_port_t *port5;
   apx_port_t *port6;
   adt_bytearray_t *initData;
   uint8_t *data;
   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","s","=0");
   port2 = apx_node_createRequirePort(&node,"U32Signal2","s","=-1");
   port3 = apx_node_createRequirePort(&node,"U32Signal3","s","=-32768");
   port4 = apx_node_createRequirePort(&node,"U32Signal4","s","=-1234");
   port5 = apx_node_createRequirePort(&node,"U32Signal5","s","=1234");
   port6 = apx_node_createRequirePort(&node,"U32Signal6","s","=32767");
   apx_node_finalize(&node);
   initData = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port2);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) 255, data[0]);
   CuAssertUIntEquals(tc, (uint8_t) 255, data[1]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port3);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) 0, data[0]);
   CuAssertUIntEquals(tc, (uint8_t) 0x80, data[1]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port4);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) 0x2E, data[0]);
   CuAssertUIntEquals(tc, (uint8_t) 0xFB, data[1]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port5);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) 0xD2, data[0]);
   CuAssertUIntEquals(tc, (uint8_t) 0x04, data[1]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port6);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint16_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, (uint8_t) 0xFF, data[0]);
   CuAssertUIntEquals(tc, (uint8_t) 0x7F, data[1]);
   adt_bytearray_delete(initData);

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S32(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   apx_port_t *port3;
   apx_port_t *port4;
   apx_port_t *port5;
   apx_port_t *port6;
   adt_bytearray_t *initData;
   uint8_t *data;
   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","l","=0");
   port2 = apx_node_createRequirePort(&node,"U32Signal2","l","=-1");
   port3 = apx_node_createRequirePort(&node,"U32Signal3","l","=-2147483648");
   port4 = apx_node_createRequirePort(&node,"U32Signal4","l","=-123456789");
   port5 = apx_node_createRequirePort(&node,"U32Signal5","l","=123456789");
   port6 = apx_node_createRequirePort(&node,"U32Signal6","l","=2147483647");
   apx_node_finalize(&node);
   initData = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertUIntEquals(tc, 0, data[2]);
   CuAssertUIntEquals(tc, 0, data[3]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port2);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 255, data[0]);
   CuAssertUIntEquals(tc, 255, data[1]);
   CuAssertUIntEquals(tc, 255, data[2]);
   CuAssertUIntEquals(tc, 255, data[3]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port3);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertUIntEquals(tc, 0, data[2]);
   CuAssertUIntEquals(tc, 0x80, data[3]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port4);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0xEB, data[0]);
   CuAssertUIntEquals(tc, 0x32, data[1]);
   CuAssertUIntEquals(tc, 0xA4, data[2]);
   CuAssertUIntEquals(tc, 0xF8, data[3]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port5);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0x15, data[0]);
   CuAssertUIntEquals(tc, 0xCD, data[1]);
   CuAssertUIntEquals(tc, 0x5B, data[2]);
   CuAssertUIntEquals(tc, 0x07, data[3]);
   adt_bytearray_delete(initData);

   initData = apx_node_createPortInitData(&node, port6);
   CuAssertPtrNotNull(tc, initData);
   CuAssertIntEquals(tc, sizeof(uint32_t), adt_bytearray_length(initData));
   data = adt_bytearray_data(initData);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0xFF, data[0]);
   CuAssertUIntEquals(tc, 0xFF, data[1]);
   CuAssertUIntEquals(tc, 0xFF, data[2]);
   CuAssertUIntEquals(tc, 0x7F, data[3]);
   adt_bytearray_delete(initData);

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U8Array(CuTest* tc)
{

}
