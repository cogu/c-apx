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
#include "apx_error.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_node_finalize(CuTest* tc);
static void test_apx_node_createNodeWithoutTypeReferences(CuTest* tc);
static void test_apx_node_createNodeWithIdTypeReferences(CuTest* tc);
static void test_apx_node_createNodeWithNameTypeReferences(CuTest* tc);
static void test_apx_node_createNodeWithInvalidTypeReferenceId(CuTest* tc);
static void test_apx_node_createNodeWithInvalidTypeReferenceName(CuTest* tc);
static void test_apx_node_createNodeWithInvalidDataSignatureString(CuTest* tc);
static void test_apx_node_createNodeWithInvalidAttributeString(CuTest* tc);
static void test_apx_node_initValue_U8(CuTest* tc);
static void test_apx_node_initValue_U16(CuTest* tc);
static void test_apx_node_initValue_U32(CuTest* tc);
static void test_apx_node_initValue_S8(CuTest* tc);
static void test_apx_node_initValue_S16(CuTest* tc);
static void test_apx_node_initValue_S32(CuTest* tc);
static void test_apx_node_initValue_U8_array(CuTest* tc);
static void test_apx_node_initValue_U16_array(CuTest* tc);
static void test_apx_node_initValue_U32_array(CuTest* tc);
static void test_apx_node_initValue_S8_array(CuTest* tc);
static void test_apx_node_initValue_S16_array(CuTest* tc);
static void test_apx_node_initValue_S32_array(CuTest* tc);
static void test_apx_node_calcOutPortDataLen(CuTest* tc);
static void test_apx_node_calcInPortDataLen(CuTest* tc);


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

   SUITE_ADD_TEST(suite, test_apx_node_finalize);
   SUITE_ADD_TEST(suite, test_apx_node_createNodeWithoutTypeReferences);
   SUITE_ADD_TEST(suite, test_apx_node_createNodeWithIdTypeReferences);
   SUITE_ADD_TEST(suite, test_apx_node_createNodeWithNameTypeReferences);
   SUITE_ADD_TEST(suite, test_apx_node_createNodeWithInvalidTypeReferenceId);
   SUITE_ADD_TEST(suite, test_apx_node_createNodeWithInvalidTypeReferenceName);
   SUITE_ADD_TEST(suite, test_apx_node_createNodeWithInvalidDataSignatureString);
   SUITE_ADD_TEST(suite, test_apx_node_createNodeWithInvalidAttributeString);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U8);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U16);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U32);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S8);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S16);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S32);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U8_array);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U16_array);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_U32_array);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S8_array);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S16_array);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_S32_array);
   SUITE_ADD_TEST(suite, test_apx_node_calcOutPortDataLen);
   SUITE_ADD_TEST(suite, test_apx_node_calcInPortDataLen);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_node_finalize(CuTest* tc)
{
   apx_node_t node;
   int32_t errorLine;
   apx_port_t *port1;
   apx_port_t *port2;
   int32_t lineNumber=1;

   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "EnginesPeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"VehicleSpeed","T[0]","=65535", lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"EngineSpeed","T[1]","=65535", lineNumber++));
   port1 = apx_node_getRequirePort(&node, 0);
   port2 = apx_node_getRequirePort(&node, 1);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_ID, port1->dataSignature.dataElement->baseType);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_ID, port2->dataSignature.dataElement->baseType);
   CuAssertIntEquals(tc, 0, apx_port_getPackLen(port1));
   CuAssertIntEquals(tc, 0, apx_port_getPackLen(port2));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_PTR, port1->dataSignature.dataElement->baseType);
   CuAssertIntEquals(tc, APX_BASE_TYPE_REF_PTR, port2->dataSignature.dataElement->baseType);
   CuAssertIntEquals(tc, 2, apx_port_getPackLen(port1));
   CuAssertIntEquals(tc, 2, apx_port_getPackLen(port2));

   apx_node_destroy(&node);
}

static void test_apx_node_createNodeWithoutTypeReferences(CuTest* tc)
{
   apx_node_t node;
   int32_t lineNumber;
   apx_port_t *port;

   apx_node_create(&node, "TestNode");
   apx_node_createRequirePort(&node,"VehicleSpeed","S","=65535", 1);
   apx_node_createRequirePort(&node,"ParkBrakeFailure","C(0,3)", "=3", 2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &lineNumber));

   //verify individual ports
   port = apx_node_getRequirePort(&node, 0);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, 2, apx_dataSignature_calcPackLen(&port->dataSignature));
   CuAssertIntEquals(tc, 0, apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"VehicleSpeed\"S", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 1);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, 1, apx_dataSignature_calcPackLen(&port->dataSignature));
   CuAssertIntEquals(tc, 1 ,apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"ParkBrakeFailure\"C(0,3)", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 2);
   CuAssertPtrEquals(tc, 0, port);

   apx_node_destroy(&node);
}


static void test_apx_node_createNodeWithIdTypeReferences(CuTest* tc)
{
   apx_node_t node;
   int32_t errorLine;
   apx_port_t *port;
   int32_t lineNumber=1;

   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "EnginesPeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"VehicleSpeed","T[0]","=65535", lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"EngineSpeed","T[1]","=65535", lineNumber++));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));

   //verify individual ports
   port = apx_node_getRequirePort(&node, 0);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, 2, apx_dataSignature_calcPackLen(&port->dataSignature));
   CuAssertIntEquals(tc, 0, apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"VehicleSpeed\"S", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 1);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, 2, apx_dataSignature_calcPackLen(&port->dataSignature));
   CuAssertIntEquals(tc, 1 ,apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"EngineSpeed\"S", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 2);
   CuAssertPtrEquals(tc, 0, port);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);

}

static void test_apx_node_createNodeWithNameTypeReferences(CuTest* tc)
{
   apx_node_t node;
   int32_t errorLine;
   apx_port_t *port;
   int32_t lineNumber=1;
   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "EnginesPeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"VehicleSpeed","T[\"VehicleSpeed_T\"]","=65535", lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"EngineSpeed","T[\"EnginesPeed_T\"]","=65535", lineNumber++));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));

   //verify individual ports
   port = apx_node_getRequirePort(&node, 0);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, 2, apx_dataSignature_calcPackLen(&port->dataSignature));
   CuAssertIntEquals(tc, 0, apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"VehicleSpeed\"S", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 1);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, 2, apx_dataSignature_calcPackLen(&port->dataSignature));
   CuAssertIntEquals(tc, 1 ,apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"EngineSpeed\"S", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 2);
   CuAssertPtrEquals(tc, 0, port);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);

}

static void test_apx_node_createNodeWithInvalidTypeReferenceId(CuTest* tc)
{
   apx_node_t node;
   int32_t errorLine = 0;

   int32_t lineNumber=1;

   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"VehicleSpeed","T[0]","=65535", lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"EngineSpeed","T[1]","=65535", lineNumber++));
   CuAssertIntEquals(tc, APX_INVALID_TYPE_REF_ERROR, apx_node_finalize(&node, &errorLine));
   CuAssertIntEquals(tc, 3, errorLine);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);
}

static void test_apx_node_createNodeWithInvalidTypeReferenceName(CuTest* tc)
{
   apx_node_t node;
   int32_t errorLine = 0;

   int32_t lineNumber=1;

   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"VehicleSpeed","T[\"VehicleSpeed_T\"]","=65535", lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"EngineSpeed","T[\"EngineSpeed_T\"]","=65535", lineNumber++));
   CuAssertIntEquals(tc, APX_INVALID_TYPE_REF_ERROR, apx_node_finalize(&node, &errorLine));
   CuAssertIntEquals(tc, 3, errorLine);

   apx_node_destroy(&node);
}

static void test_apx_node_createNodeWithInvalidDataSignatureString(CuTest* tc)
{
   apx_node_t node;

   int32_t lineNumber=1;

   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrEquals(tc, NULL, apx_node_createRequirePort(&node,"VehicleSpeed","T[]","=65535", lineNumber));
   CuAssertIntEquals(tc, APX_INVALID_TYPE_REF_ERROR, apx_node_getLastError(&node));

   apx_node_destroy(&node);
}

static void test_apx_node_createNodeWithInvalidAttributeString(CuTest* tc)
{
   apx_node_t node;

   int32_t lineNumber=1;

   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrEquals(tc, NULL, apx_node_createRequirePort(&node,"VehicleSpeed","T[0]","65535", lineNumber));
   CuAssertIntEquals(tc, APX_INVALID_ATTRIBUTE_ERROR, apx_node_getLastError(&node));

   apx_node_destroy(&node);
}


static void test_apx_node_initValue_U8(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   adt_bytearray_t *initData;
   int32_t lineNumber=1;
   uint8_t *data;
   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U8Signal1","C","=0", lineNumber++);
   port2 = apx_node_createRequirePort(&node,"U8Signal2","C","=255", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &lineNumber));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
}

static void test_apx_node_initValue_U16(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   apx_port_t *port3;
   adt_bytearray_t *initData;
   uint8_t *data;
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U16Signal1","S","=0", lineNumber++);
   port2 = apx_node_createRequirePort(&node,"U16Signal2","S","=65535", lineNumber++);
   port3 = apx_node_createRequirePort(&node,"U16Signal3","S","=0x1234", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &lineNumber));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
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
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","L","=0", lineNumber++);
   port2 = apx_node_createRequirePort(&node,"U32Signal2","L","=4294967295", lineNumber++);
   port3 = apx_node_createRequirePort(&node,"U32Signal4","L","=0xFFFFFFFF", lineNumber++);
   port4 = apx_node_createRequirePort(&node,"U32Signal3","L","=0x12345678", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &lineNumber));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
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
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","c","=0", lineNumber++);
   port2 = apx_node_createRequirePort(&node,"U32Signal2","c","=-1", lineNumber++);
   port3 = apx_node_createRequirePort(&node,"U32Signal3","c","=-128", lineNumber++);
   port4 = apx_node_createRequirePort(&node,"U32Signal4","c","=-27", lineNumber++);
   port5 = apx_node_createRequirePort(&node,"U32Signal5","c","=27", lineNumber++);
   port6 = apx_node_createRequirePort(&node,"U32Signal6","c","=127", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &lineNumber));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
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
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","s","=0", lineNumber++);
   port2 = apx_node_createRequirePort(&node,"U32Signal2","s","=-1", lineNumber++);
   port3 = apx_node_createRequirePort(&node,"U32Signal3","s","=-32768", lineNumber++);
   port4 = apx_node_createRequirePort(&node,"U32Signal4","s","=-1234", lineNumber++);
   port5 = apx_node_createRequirePort(&node,"U32Signal5","s","=1234", lineNumber++);
   port6 = apx_node_createRequirePort(&node,"U32Signal6","s","=32767", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &lineNumber));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
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
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","l","=0", lineNumber++);
   port2 = apx_node_createRequirePort(&node,"U32Signal2","l","=-1", lineNumber++);
   port3 = apx_node_createRequirePort(&node,"U32Signal3","l","=-2147483648", lineNumber++);
   port4 = apx_node_createRequirePort(&node,"U32Signal4","l","=-123456789", lineNumber++);
   port5 = apx_node_createRequirePort(&node,"U32Signal5","l","=123456789", lineNumber++);
   port6 = apx_node_createRequirePort(&node,"U32Signal6","l","=2147483647", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &lineNumber));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U8_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   adt_bytearray_t *array;
   uint8_t *data;
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U8Signal1","C[4]","={1,2,3,4},P", lineNumber++);
   array = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, array);
   CuAssertIntEquals(tc, sizeof(uint8_t)*4, adt_bytearray_length(array));
   data = adt_bytearray_data(array);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertUIntEquals(tc, 2, data[1]);
   CuAssertUIntEquals(tc, 3, data[2]);
   CuAssertUIntEquals(tc, 4, data[3]);
   adt_bytearray_delete(array);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U16_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   adt_bytearray_t *array;
   uint8_t *data;
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U16Signal1","S[4]","={1,2,3,4}", lineNumber++);
   array = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, array);
   CuAssertIntEquals(tc, sizeof(uint16_t)*4, adt_bytearray_length(array));
   data = adt_bytearray_data(array);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertUIntEquals(tc, 2, data[2]);
   CuAssertUIntEquals(tc, 0, data[3]);
   CuAssertUIntEquals(tc, 3, data[4]);
   CuAssertUIntEquals(tc, 0, data[5]);
   CuAssertUIntEquals(tc, 4, data[6]);
   CuAssertUIntEquals(tc, 0, data[7]);
   adt_bytearray_delete(array);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U32_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   adt_bytearray_t *array;
   uint8_t *data;
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","L[4]","={1,2,3,4}", lineNumber++);
   port2 = apx_node_createRequirePort(&node,"U32Signal2","L[4]","={1,2,3}", lineNumber++); //missing one init-element
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   array = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, array);
   CuAssertIntEquals(tc, sizeof(uint32_t)*4, adt_bytearray_length(array));
   data = adt_bytearray_data(array);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 1, data[0]);
   CuAssertUIntEquals(tc, 0, data[1]);
   CuAssertUIntEquals(tc, 0, data[2]);
   CuAssertUIntEquals(tc, 0, data[3]);
   CuAssertUIntEquals(tc, 2, data[4]);
   CuAssertUIntEquals(tc, 0, data[5]);
   CuAssertUIntEquals(tc, 0, data[6]);
   CuAssertUIntEquals(tc, 0, data[7]);
   CuAssertUIntEquals(tc, 3, data[8]);
   CuAssertUIntEquals(tc, 0, data[9]);
   CuAssertUIntEquals(tc, 0, data[10]);
   CuAssertUIntEquals(tc, 0, data[11]);
   CuAssertUIntEquals(tc, 4, data[12]);
   CuAssertUIntEquals(tc, 0, data[13]);
   CuAssertUIntEquals(tc, 0, data[14]);
   CuAssertUIntEquals(tc, 0, data[15]);
   adt_bytearray_delete(array);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   array = apx_node_createPortInitData(&node, port2);
   CuAssertPtrEquals(tc, 0, array);
   CuAssertIntEquals(tc, APX_LENGTH_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S8_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   adt_bytearray_t *array;
   uint8_t *data;
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"S8Signal1","c[4]","={-1, -2, -3, -4}", lineNumber++);
   array = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, array);
   CuAssertIntEquals(tc, sizeof(uint8_t)*4, adt_bytearray_length(array));
   data = adt_bytearray_data(array);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0xFF, data[0]);
   CuAssertUIntEquals(tc, 0xFE, data[1]);
   CuAssertUIntEquals(tc, 0xFD, data[2]);
   CuAssertUIntEquals(tc, 0xFC, data[3]);
   adt_bytearray_delete(array);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S16_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   adt_bytearray_t *array;
   uint8_t *data;
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"S16Signal1","s[4]","={-1,-2,-3,-4}", lineNumber++);
   array = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, array);
   CuAssertIntEquals(tc, sizeof(uint16_t)*4, adt_bytearray_length(array));
   data = adt_bytearray_data(array);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0xFF, data[0]);
   CuAssertUIntEquals(tc, 0xFF, data[1]);
   CuAssertUIntEquals(tc, 0xFE, data[2]);
   CuAssertUIntEquals(tc, 0xFF, data[3]);
   CuAssertUIntEquals(tc, 0xFD, data[4]);
   CuAssertUIntEquals(tc, 0xFF, data[5]);
   CuAssertUIntEquals(tc, 0xFC, data[6]);
   CuAssertUIntEquals(tc, 0xFF, data[7]);
   adt_bytearray_delete(array);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S32_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   adt_bytearray_t *array;
   uint8_t *data;
   int32_t lineNumber=1;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U32Signal1","l[4]","={-1, 0, 0x12345678, 0x7FFFFFFF}", lineNumber++);

   array = apx_node_createPortInitData(&node, port1);
   CuAssertPtrNotNull(tc, array);
   CuAssertIntEquals(tc, sizeof(uint32_t)*4, adt_bytearray_length(array));
   data = adt_bytearray_data(array);
   CuAssertPtrNotNull(tc, data);
   CuAssertUIntEquals(tc, 0xFF, data[0]);
   CuAssertUIntEquals(tc, 0xFF, data[1]);
   CuAssertUIntEquals(tc, 0xFF, data[2]);
   CuAssertUIntEquals(tc, 0xFF, data[3]);
   CuAssertUIntEquals(tc, 0, data[4]);
   CuAssertUIntEquals(tc, 0, data[5]);
   CuAssertUIntEquals(tc, 0, data[6]);
   CuAssertUIntEquals(tc, 0, data[7]);
   CuAssertUIntEquals(tc, 0x78, data[8]);
   CuAssertUIntEquals(tc, 0x56, data[9]);
   CuAssertUIntEquals(tc, 0x34, data[10]);
   CuAssertUIntEquals(tc, 0x12, data[11]);
   CuAssertUIntEquals(tc, 0xFF, data[12]);
   CuAssertUIntEquals(tc, 0xFF, data[13]);
   CuAssertUIntEquals(tc, 0xFF, data[14]);
   CuAssertUIntEquals(tc, 0x7F, data[15]);
   adt_bytearray_delete(array);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_getLastError(&node));
   apx_node_destroy(&node);
}

static void test_apx_node_calcOutPortDataLen(CuTest* tc)
{
   apx_node_t *node;
   int32_t lineNumber=1;
   int32_t errorLine;

   node = apx_node_new("TestNode");
   CuAssertPtrNotNull(tc, node);
   CuAssertPtrNotNull(tc, apx_node_createProvidePort(node, "U8Signal", "C(0,3)","=3", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createProvidePort(node, "U16Signal", "S","=65535", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createProvidePort(node, "U32Signal", "L","=0xFFFFFFFF", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createProvidePort(node, "StrSignal", "a[10]","=\"\"", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createProvidePort(node, "U8ArraySignal", "C[4]","={0,0,0,0}", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createProvidePort(node, "U16ArraySignal", "S[4]","={0,0,0,0}", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createProvidePort(node, "U32ArraySignal", "L[4]","={0,0,0,0}", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createProvidePort(node, "StructSignal", "{\"Name\"a[32]\"Id\"L}","={\"\",0xFFFFFFFF}", lineNumber++ ));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(node, &errorLine));

   CuAssertIntEquals(tc, 1+2+4+10+4+2*4+4*4+(32+4), apx_node_calcOutPortDataLen(node));

   apx_node_delete(node);
}

static void test_apx_node_calcInPortDataLen(CuTest* tc)
{
   apx_node_t *node;
   int32_t lineNumber=1;
   int32_t errorLine;

   node = apx_node_new("TestNode");
   CuAssertPtrNotNull(tc, node);
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(node, "U8Signal", "C(0,3)","=3", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(node, "U16Signal", "S","=65535", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(node, "U32Signal", "L","=0xFFFFFFFF", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(node, "StrSignal", "a[10]","=\"\"", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(node, "U8ArraySignal", "C[4]","={0,0,0,0}", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(node, "U16ArraySignal", "S[4]","={0,0,0,0}", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(node, "U32ArraySignal", "L[4]","={0,0,0,0}", lineNumber++ ));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(node, "StructSignal", "{\"Name\"a[32]\"Id\"L}","={\"\",0xFFFFFFFF}", lineNumber++ ));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(node, &errorLine));

   CuAssertIntEquals(tc, 1+2+4+10+4+2*4+4*4+(32+4), apx_node_calcInPortDataLen(node));

   apx_node_delete(node);

}
