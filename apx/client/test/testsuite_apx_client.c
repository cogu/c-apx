/*****************************************************************************
* \file      testsuite_apx_client.c
* \author    Conny Gustafsson
* \date      2020-01-27
* \brief     Unit tests for apx_client_t
*
* Copyright (c) 2020 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "apx_client.h"
#include "apx_clientEventListenerSpy.h"
#include "CuTest.h"
#include "pack.h"
#include "apx_util.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char *m_apx_definition1 = "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"VehicleSpeed\"S:=65535\n"
      "P\"EngineSpeed\"S:=65535\n"
      "\n";

static const char *m_apx_definition2 = "APX/1.2\n"
      "N\"TestNode2\"\n"
      "R\"VehicleSpeed\"S:=65535\n"
      "R\"EngineSpeed\"S:=65535\n"
      "\n";

static const char *m_apx_definition3 = "APX/1.2\n"
      "N\"TestNode3\"\n"
      "P\"U8Array\"C[3]:={0xFF, 0xFF, 0xFF}\n"
      "P\"U16Array\"S[3]:={0xFFFF, 0xFFFF, 0xFFFF}\n"
      "P\"U32Array\"L[3]:={0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}\n"
      "\n";

static const char *m_apx_definition4 = "APX/1.2\n"
      "N\"TestNode4\"\n"
      "R\"U8Array\"C[3]:={0xFF, 0xFF, 0xFF}\n"
      "R\"U16Array\"S[3]:={0xFFFF, 0xFFFF, 0xFFFF}\n"
      "R\"U32Array\"L[3]:={0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}\n"
      "\n";

#define ARRAY_LEN 3

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_client_create(CuTest* tc);
static void test_apx_client_buildNodeFromString1(CuTest* tc);
static void test_apx_client_buildNodeFromString2(CuTest* tc);
static void test_apx_client_registerEventHandler(CuTest* tc);
static void test_apx_client_portHandleWithoutDefiningNodeName1(CuTest* tc);
static void test_apx_client_portHandleWithoutDefiningNodeName2(CuTest* tc);
static void test_apx_client_writePortData_dtl_u16(CuTest* tc);
static void test_apx_client_writePortData_direct_u16(CuTest* tc);
static void test_apx_client_readPortData_dtl_u16(CuTest* tc);
static void test_apx_client_writePortData_dtl_u8_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_u8_fix_array(CuTest* tc);
static void test_apx_client_writePortData_dtl_u16_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_u16_fix_array(CuTest* tc);
static void test_apx_client_writePortData_dtl_u32_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_u32_fix_array(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_client(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_client_create);
   SUITE_ADD_TEST(suite, test_apx_client_buildNodeFromString1);
   SUITE_ADD_TEST(suite, test_apx_client_buildNodeFromString2);
   SUITE_ADD_TEST(suite, test_apx_client_registerEventHandler);
   SUITE_ADD_TEST(suite, test_apx_client_portHandleWithoutDefiningNodeName1);
   SUITE_ADD_TEST(suite, test_apx_client_portHandleWithoutDefiningNodeName2);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u16);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_direct_u16);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u16);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u8_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u8_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u16_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u16_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u32_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u32_fix_array);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_client_create(CuTest* tc)
{
   apx_client_t *client = apx_client_new();
   CuAssertPtrNotNull(tc, client);
   apx_client_delete(client);
}

static void test_apx_client_buildNodeFromString1(CuTest* tc)
{

   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   apx_nodeInstance_t *node = apx_client_getLastAttachedNode(client);
   CuAssertPtrNotNull(tc, node);
   apx_nodeInfo_t *nodeInfo = apx_nodeInstance_getNodeInfo(node);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertIntEquals(tc, 2, apx_nodeInfo_getNumProvidePorts(nodeInfo));
   CuAssertIntEquals(tc, 0, apx_nodeInfo_getNumRequirePorts(nodeInfo));
   apx_client_delete(client);
}

static void test_apx_client_buildNodeFromString2(CuTest* tc)
{

   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   apx_nodeInstance_t *node = apx_client_getLastAttachedNode(client);
   CuAssertPtrNotNull(tc, node);
   apx_nodeInfo_t *nodeInfo = apx_nodeInstance_getNodeInfo(node);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertIntEquals(tc, 0, apx_nodeInfo_getNumProvidePorts(nodeInfo));
   CuAssertIntEquals(tc, 2, apx_nodeInfo_getNumRequirePorts(nodeInfo));
   apx_client_delete(client);
}

static void test_apx_client_registerEventHandler(CuTest* tc)
{
   apx_client_t *client = apx_client_new();
   apx_clientEventListenerSpy_t *spy = apx_clientEventListenerSpy_new();
   CuAssertPtrNotNull(tc, client);
   CuAssertPtrNotNull(tc, spy);


   CuAssertIntEquals(tc, 0, apx_client_getNumEventListeners(client));
   void *handle = apx_clientEventListenerSpy_register(spy, client);
   CuAssertIntEquals(tc, 1, apx_client_getNumEventListeners(client));
   apx_client_unregisterEventListener(client, handle);
   CuAssertIntEquals(tc, 0, apx_client_getNumEventListeners(client));


   apx_client_delete(client);
   apx_clientEventListenerSpy_delete(spy);
}

static void test_apx_client_portHandleWithoutDefiningNodeName1(CuTest* tc)
{
   void *VehicleSpeedHandle;
   void *EngineSpeedHandle;
   apx_nodeInstance_t *node;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));

   node = apx_client_getLastAttachedNode(client);
   CuAssertPtrNotNull(tc, node);

   VehicleSpeedHandle = apx_client_getPortHandle(client, NULL, "VehicleSpeed");
   EngineSpeedHandle = apx_client_getPortHandle(client, NULL, "EngineSpeed");
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(node, 0), VehicleSpeedHandle);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(node, 1), EngineSpeedHandle);

   apx_client_delete(client);
}

static void test_apx_client_portHandleWithoutDefiningNodeName2(CuTest* tc)
{
   void *VehicleSpeedHandle;
   void *EngineSpeedHandle;
   apx_nodeInstance_t *node;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));

   node = apx_client_getLastAttachedNode(client);
   CuAssertPtrNotNull(tc, node);

   VehicleSpeedHandle = apx_client_getPortHandle(client, NULL, "VehicleSpeed");
   EngineSpeedHandle = apx_client_getPortHandle(client, NULL, "EngineSpeed");
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(node, 0), VehicleSpeedHandle);
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(node, 1), EngineSpeedHandle);

   apx_client_delete(client);

}

static void test_apx_client_writePortData_dtl_u16(CuTest* tc)
{
   void *VehicleSpeedHandle;
   void *EngineSpeedHandle;
   uint8_t rawData[UINT16_SIZE] = {0, 0};
   apx_nodeInstance_t *node;
   apx_client_t *client = apx_client_new();
   dtl_sv_t *sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   VehicleSpeedHandle = apx_client_getPortHandle(client, NULL, "VehicleSpeed");
   EngineSpeedHandle = apx_client_getPortHandle(client, NULL, "EngineSpeed");
   node = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(node, &rawData[0], 0u, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   dtl_sv_set_u32(sv, 0x1234);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, VehicleSpeedHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(node, &rawData[0], 0u, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x34, rawData[0]);
   CuAssertUIntEquals(tc, 0x12, rawData[1]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(node, &rawData[0], UINT16_SIZE, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   dtl_sv_set_u32(sv, 0x6218);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, EngineSpeedHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(node, &rawData[0], UINT16_SIZE, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x18, rawData[0]);
   CuAssertUIntEquals(tc, 0x62, rawData[1]);

   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*) sv);

}

static void test_apx_client_writePortData_direct_u16(CuTest* tc)
{
   void *VehicleSpeedHandle;
   void *EngineSpeedHandle;
   uint8_t rawData[UINT16_SIZE] = {0, 0};
   apx_nodeInstance_t *node;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   VehicleSpeedHandle = apx_client_getPortHandle(client, NULL, "VehicleSpeed");
   EngineSpeedHandle = apx_client_getPortHandle(client, NULL, "EngineSpeed");
   node = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(node, &rawData[0], 0u, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u16(client, VehicleSpeedHandle, 0x1234));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(node, &rawData[0], 0u, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x34, rawData[0]);
   CuAssertUIntEquals(tc, 0x12, rawData[1]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(node, &rawData[0], UINT16_SIZE, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u16(client, EngineSpeedHandle, 0x6218));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(node, &rawData[0], UINT16_SIZE, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x18, rawData[0]);
   CuAssertUIntEquals(tc, 0x62, rawData[1]);

   apx_client_delete(client);

}

static void test_apx_client_readPortData_dtl_u16(CuTest* tc)
{
   void *VehicleSpeedHandle;
   void *EngineSpeedHandle;
   uint8_t rawData[UINT16_SIZE] = {0, 0};
   apx_nodeInstance_t *node;
   dtl_dv_t *dv = 0;
   bool ok = false;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   VehicleSpeedHandle = apx_client_getPortHandle(client, NULL, "VehicleSpeed");
   EngineSpeedHandle = apx_client_getPortHandle(client, NULL, "EngineSpeed");
   CuAssertPtrNotNull(tc, VehicleSpeedHandle);
   CuAssertPtrNotNull(tc, EngineSpeedHandle);
   node = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, VehicleSpeedHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 65535, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;
   packLE(rawData, 0x1234, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(node, rawData, 0u, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, VehicleSpeedHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(rawData, 0x6218, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(node, rawData, UINT16_SIZE, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, EngineSpeedHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x6218, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);

   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_u8_fix_array(CuTest* tc)
{
   const uint32_t offset = 0u;
   apx_nodeInstance_t *nodeInstance;
   void *u8ArrayHandle;
   uint8_t rawData[UINT8_SIZE*ARRAY_LEN] = {0, 0, 0};
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition3));
   u8ArrayHandle = apx_client_getProvidePortHandleById(client, NULL, 0u);
   CuAssertPtrNotNull(tc, u8ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*ARRAY_LEN));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x12), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xff), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, u8ArrayHandle, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*ARRAY_LEN));
   CuAssertUIntEquals(tc, 0x0, rawData[0]);
   CuAssertUIntEquals(tc, 0x12, rawData[1]);
   CuAssertUIntEquals(tc, 0xff, rawData[2]);

   apx_client_delete(client);
   dtl_dv_dec_ref((dtl_dv_t*) av);
}

static void test_apx_client_readPortData_dtl_u8_fix_array(CuTest* tc)
{
   const uint32_t offset = 0u;
   apx_nodeInstance_t *nodeInstance;
   void *u8ArrayHandle;
   uint8_t rawData[UINT8_SIZE*ARRAY_LEN] = {0, 0, 0};
   apx_client_t *client = apx_client_new();
   dtl_dv_t *dv;
   dtl_av_t *av;
   dtl_sv_t *sv;

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition4));
   u8ArrayHandle = apx_client_getRequirePortHandleById(client, NULL, 0u);
   CuAssertPtrNotNull(tc, u8ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   rawData[0] = 0x00;
   rawData[1] = 0x12;
   rawData[2] = 0xff;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, u8ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, ARRAY_LEN, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0x00, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0xff, dtl_sv_to_u32(sv, NULL));

   dtl_dv_dec_ref(dv);
   apx_client_delete(client);

}

static void test_apx_client_writePortData_dtl_u16_fix_array(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE*ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *u16ArrayHandle;
   uint8_t rawData[UINT16_SIZE*ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();
   memset(rawData, 0, sizeof(rawData));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition3));
   u16ArrayHandle = apx_client_getProvidePortHandleById(client, NULL, 1u);
   CuAssertPtrNotNull(tc, u16ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*ARRAY_LEN));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   CuAssertUIntEquals(tc, 0xFF, rawData[3]);
   CuAssertUIntEquals(tc, 0xFF, rawData[4]);
   CuAssertUIntEquals(tc, 0xFF, rawData[5]);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x0000), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x1234), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xffff), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, u16ArrayHandle, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*ARRAY_LEN));
   CuAssertUIntEquals(tc, 0, rawData[0]);
   CuAssertUIntEquals(tc, 0, rawData[1]);
   CuAssertUIntEquals(tc, 0x34, rawData[2]);
   CuAssertUIntEquals(tc, 0x12, rawData[3]);
   CuAssertUIntEquals(tc, 0xff, rawData[4]);
   CuAssertUIntEquals(tc, 0xff, rawData[5]);

   apx_client_delete(client);
   dtl_dv_dec_ref((dtl_dv_t*) av);
}

static void test_apx_client_readPortData_dtl_u16_fix_array(CuTest* tc)
{
   dtl_dv_t *dv;
   dtl_av_t *av;
   dtl_sv_t *sv;
   const uint32_t offset = UINT8_SIZE*ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *u16ArrayHandle;
   uint8_t rawData[UINT16_SIZE*ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   memset(rawData, 0, sizeof(rawData));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition4));
   u16ArrayHandle = apx_client_getRequirePortHandleById(client, NULL, 1u);
   CuAssertPtrNotNull(tc, u16ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   rawData[0] = 0x00;
   rawData[1] = 0x00;
   rawData[2] = 0x34;
   rawData[3] = 0x12;
   rawData[4] = 0xff;
   rawData[5] = 0xff;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, u16ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, ARRAY_LEN, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0x0000, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0xffff, dtl_sv_to_u32(sv, NULL));

   dtl_dv_dec_ref(dv);
   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_u32_fix_array(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE*ARRAY_LEN + UINT16_SIZE*ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *u32ArrayHandle;
   uint8_t rawData[UINT32_SIZE*ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();
   memset(rawData, 0, sizeof(rawData));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition3));
   u32ArrayHandle = apx_client_getProvidePortHandleById(client, NULL, 2u);
   CuAssertPtrNotNull(tc, u32ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*ARRAY_LEN));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   CuAssertUIntEquals(tc, 0xFF, rawData[3]);
   CuAssertUIntEquals(tc, 0xFF, rawData[4]);
   CuAssertUIntEquals(tc, 0xFF, rawData[5]);
   CuAssertUIntEquals(tc, 0xFF, rawData[6]);
   CuAssertUIntEquals(tc, 0xFF, rawData[7]);
   CuAssertUIntEquals(tc, 0xFF, rawData[8]);
   CuAssertUIntEquals(tc, 0xFF, rawData[9]);
   CuAssertUIntEquals(tc, 0xFF, rawData[10]);
   CuAssertUIntEquals(tc, 0xFF, rawData[11]);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x00000000), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x12345678), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xffffffff), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, u32ArrayHandle, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*ARRAY_LEN));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);
   CuAssertUIntEquals(tc, 0x00, rawData[2]);
   CuAssertUIntEquals(tc, 0x00, rawData[3]);
   CuAssertUIntEquals(tc, 0x78, rawData[4]);
   CuAssertUIntEquals(tc, 0x56, rawData[5]);
   CuAssertUIntEquals(tc, 0x34, rawData[6]);
   CuAssertUIntEquals(tc, 0x12, rawData[7]);
   CuAssertUIntEquals(tc, 0xFF, rawData[8]);
   CuAssertUIntEquals(tc, 0xFF, rawData[9]);
   CuAssertUIntEquals(tc, 0xFF, rawData[10]);
   CuAssertUIntEquals(tc, 0xFF, rawData[11]);

   apx_client_delete(client);
   dtl_dv_dec_ref((dtl_dv_t*) av);
}

static void test_apx_client_readPortData_dtl_u32_fix_array(CuTest* tc)
{
   dtl_dv_t *dv;
   dtl_av_t *av;
   dtl_sv_t *sv;
   const uint32_t offset = UINT8_SIZE*ARRAY_LEN+UINT16_SIZE*ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *u32ArrayHandle;
   uint8_t rawData[UINT32_SIZE*ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   memset(rawData, 0, sizeof(rawData));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition4));
   u32ArrayHandle = apx_client_getRequirePortHandleById(client, NULL, 2u);
   CuAssertPtrNotNull(tc, u32ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   rawData[0] = 0x00;
   rawData[1] = 0x00;
   rawData[2] = 0x00;
   rawData[3] = 0x00;
   rawData[4] = 0x78;
   rawData[5] = 0x56;
   rawData[6] = 0x34;
   rawData[7] = 0x12;
   rawData[8] = 0xff;
   rawData[9] = 0xff;
   rawData[10] = 0xff;
   rawData[11] = 0xff;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, u32ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, ARRAY_LEN, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0x00000000, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0x12345678, dtl_sv_to_u32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32(sv, NULL));

   dtl_dv_dec_ref(dv);
   apx_client_delete(client);
}
