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
static void test_apx_node_initValue_Record_U8(CuTest* tc);
static void test_apx_node_initValue_Record_U32String(CuTest* tc);


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
   SUITE_ADD_TEST(suite, test_apx_node_initValue_Record_U8);
   SUITE_ADD_TEST(suite, test_apx_node_initValue_Record_U32String);

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
   apx_size_t packLen = 0;

   apx_node_create(&node, "TestNode");
   apx_node_createRequirePort(&node,"VehicleSpeed","S","=65535", 1);
   apx_node_createRequirePort(&node,"ParkBrakeFailure","C(0,3)", "=3", 2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &lineNumber));

   //verify individual ports
   port = apx_node_getRequirePort(&node, 0);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(&port->dataSignature, &packLen));
   CuAssertIntEquals(tc, UINT16_SIZE, packLen);
   CuAssertIntEquals(tc, 0, apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"VehicleSpeed\"S", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 1);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(&port->dataSignature, &packLen));
   CuAssertIntEquals(tc, UINT8_SIZE, packLen);
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
   apx_size_t packLen = 0;

   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "EnginesPeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"VehicleSpeed","T[0]","=65535", lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"EngineSpeed","T[1]","=65535", lineNumber++));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));

   //verify individual ports
   port = apx_node_getRequirePort(&node, 0);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(&port->dataSignature, &packLen));
   CuAssertIntEquals(tc, UINT16_SIZE, packLen);

   CuAssertIntEquals(tc, 0, apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"VehicleSpeed\"S", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 1);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(&port->dataSignature, &packLen));
   CuAssertIntEquals(tc, UINT16_SIZE, packLen);
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
   apx_size_t packLen = 0;

   apx_node_create(&node, "TestNode");
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "VehicleSpeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createDataType(&node, "EnginesPeed_T", "S", NULL, lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"VehicleSpeed","T[\"VehicleSpeed_T\"]","=65535", lineNumber++));
   CuAssertPtrNotNull(tc, apx_node_createRequirePort(&node,"EngineSpeed","T[\"EnginesPeed_T\"]","=65535", lineNumber++));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));

   //verify individual ports
   port = apx_node_getRequirePort(&node, 0);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(&port->dataSignature, &packLen));
   CuAssertIntEquals(tc, UINT16_SIZE, packLen);

   CuAssertIntEquals(tc, 0, apx_port_getPortId(port));
   CuAssertStrEquals(tc, "\"VehicleSpeed\"S", apx_port_getDerivedPortSignature(port));

   port = apx_node_getRequirePort(&node, 1);
   CuAssertPtrNotNull(tc, port);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_dataSignature_calcPackLen(&port->dataSignature, &packLen));
   CuAssertIntEquals(tc, UINT16_SIZE, packLen);

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
   apx_port_t *port3;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   apx_node_create(&node,"TestNode");
   port1 = apx_node_createProvidePort(&node,"TestPort1","C","=0x0", lineNumber++);
   port2 = apx_node_createProvidePort(&node,"TestPort2","C","=0x12", lineNumber++);
   port3 = apx_node_createProvidePort(&node,"TestPort3","C","=0xff", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   dtl_sv_t *sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32(sv, NULL));

   initValue = apx_port_getProperInitValue(port2);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0x12u, dtl_sv_to_u32(sv, NULL));

   initValue = apx_port_getProperInitValue(port3);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0xff, dtl_sv_to_u32(sv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U16(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   apx_port_t *port3;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   apx_node_create(&node,"TestNode");
   port1 = apx_node_createProvidePort(&node,"TestPort1","S","=0x0", lineNumber++);
   port2 = apx_node_createProvidePort(&node,"TestPort2","S","=0x1234", lineNumber++);
   port3 = apx_node_createProvidePort(&node,"TestPort3","S","=0xffff", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   dtl_sv_t *sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32(sv, NULL));

   initValue = apx_port_getProperInitValue(port2);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0x1234u, dtl_sv_to_u32(sv, NULL));

   initValue = apx_port_getProperInitValue(port3);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0xffff, dtl_sv_to_u32(sv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U32(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   apx_port_t *port2;
   apx_port_t *port3;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   apx_node_create(&node,"TestNode");
   port1 = apx_node_createProvidePort(&node,"TestPort1","L","=0x0", lineNumber++);
   port2 = apx_node_createProvidePort(&node,"TestPort2","L","=0x12345678", lineNumber++);
   port3 = apx_node_createProvidePort(&node,"TestPort3","L","=0xffffffff", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   dtl_sv_t *sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32(sv, NULL));

   initValue = apx_port_getProperInitValue(port2);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0x12345678, dtl_sv_to_u32(sv, NULL));

   initValue = apx_port_getProperInitValue(port3);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32(sv, NULL));

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
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   apx_node_create(&node,"TestNode");
   port1 = apx_node_createProvidePort(&node,"TestPort1","c","=-127", lineNumber++);
   port2 = apx_node_createProvidePort(&node,"TestPort2","c","=-1", lineNumber++);
   port3 = apx_node_createProvidePort(&node,"TestPort3","c","=0", lineNumber++);
   port4 = apx_node_createProvidePort(&node,"TestPort4","c","=1", lineNumber++);
   port5 = apx_node_createProvidePort(&node,"TestPort5","c","=255", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));

   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   dtl_sv_t *sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, -127, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port2);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port3);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port4);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 1, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port5);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 255, dtl_sv_to_i32(sv, NULL));

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
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   apx_node_create(&node,"TestNode");
   port1 = apx_node_createProvidePort(&node,"TestPort1","s","=-32768", lineNumber++);
   port2 = apx_node_createProvidePort(&node,"TestPort2","s","=-1", lineNumber++);
   port3 = apx_node_createProvidePort(&node,"TestPort3","s","=0", lineNumber++);
   port4 = apx_node_createProvidePort(&node,"TestPort4","s","=1", lineNumber++);
   port5 = apx_node_createProvidePort(&node,"TestPort5","s","=32768", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));

   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   dtl_sv_t *sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, -32768, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port2);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port3);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port4);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 1, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port5);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 32768, dtl_sv_to_i32(sv, NULL));

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
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   apx_node_create(&node,"TestNode");
   port1 = apx_node_createProvidePort(&node,"TestPort1","l","=-2147483648", lineNumber++);
   port2 = apx_node_createProvidePort(&node,"TestPort2","l","=-1", lineNumber++);
   port3 = apx_node_createProvidePort(&node,"TestPort3","l","=0", lineNumber++);
   port4 = apx_node_createProvidePort(&node,"TestPort4","l","=1", lineNumber++);
   port5 = apx_node_createProvidePort(&node,"TestPort5","l","=2147483647", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));

   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   dtl_sv_t *sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, -2147483648, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port2);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port3);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port4);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 1, dtl_sv_to_i32(sv, NULL));

   initValue = apx_port_getProperInitValue(port5);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(initValue));
   sv = (dtl_sv_t*) initValue;
   CuAssertIntEquals(tc, 2147483647, dtl_sv_to_i32(sv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U8_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   dtl_av_t *av;
   dtl_dv_t *dv;


   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U8TestPort1","C[3]","={0,127,255}", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   dv = dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 127, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 255, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U16_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   dtl_av_t *av;
   dtl_dv_t *dv;


   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U16TestPort1","S[3]","={0,32767, 65535}", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   dv = dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 32767, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 65535, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_U32_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   dtl_av_t *av;
   dtl_dv_t *dv;


   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"U16TestPort1","L[3]","={0,0x7fffffff,0xffffffff},P", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   dv = dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 0, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 0x7fffffff, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32((dtl_sv_t*) dv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S8_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   dtl_av_t *av;
   dtl_dv_t *dv;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"S8Signal1","c[4]","={-128, 0, 64, 127}", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 4, dtl_av_length(av));
   dv = dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, -128, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 64, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 3);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 127, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S16_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   dtl_av_t *av;
   dtl_dv_t *dv;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"S16Signal1","s[4]","={-32768, 0, 16384, 32767}", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 4, dtl_av_length(av));
   dv = dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, -32768, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 16384, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 3);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 32767, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_S32_array(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   dtl_av_t *av;
   dtl_dv_t *dv;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"S16Signal1","l[4]","={-2147483648, 0, 1073741824, 2147483647}", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(initValue));
   av = (dtl_av_t*) initValue;
   CuAssertIntEquals(tc, 4, dtl_av_length(av));
   dv = dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, -2147483648, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 1073741824, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));
   dv = dtl_av_value(av, 3);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
   CuAssertIntEquals(tc, 2147483647, dtl_sv_to_i32((dtl_sv_t*) dv, NULL));

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_Record_U8(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   dtl_hv_t *hv;
   dtl_sv_t *sv;
   bool ok = false;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"RecordU8","{\"Items\"C}","={0}", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(initValue));
   hv = (dtl_hv_t*) initValue;
   CuAssertIntEquals(tc, 1, dtl_hv_length(hv));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Items");
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0u, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);

   apx_node_destroy(&node);
}

static void test_apx_node_initValue_Record_U32String(CuTest* tc)
{
   apx_node_t node;
   apx_port_t *port1;
   int32_t lineNumber=1;
   int32_t errorLine;
   dtl_dv_t *initValue;
   dtl_hv_t *hv;
   dtl_sv_t *sv;
   bool ok = false;

   apx_node_create(&node,"Test");
   port1 = apx_node_createRequirePort(&node,"RecordU32Str","{\"UserId\"L\"UserName\"a[32]}","={0xffffffff, \"Guest\"}", lineNumber++);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_finalize(&node, &errorLine));
   initValue = apx_port_getProperInitValue(port1);
   CuAssertPtrNotNull(tc, initValue);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(initValue));
   hv = (dtl_hv_t*) initValue;
   CuAssertIntEquals(tc, 2, dtl_hv_length(hv));
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "UserId");
   CuAssertPtrNotNull(tc, sv);
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32(sv, &ok));
   CuAssertTrue(tc, ok);
   sv = (dtl_sv_t*) dtl_hv_get_cstr(hv, "UserName");
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_STR, dtl_sv_type(sv));
   CuAssertStrEquals(tc, "Guest", dtl_sv_to_cstr(sv));

   apx_node_destroy(&node);
}
