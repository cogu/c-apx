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
      "P\"U8Value\"C:=0xff\n"
      "P\"U16Value\"S:=0xffff\n"
      "P\"U32Value\"L:=0xffffffff\n"
      "\n";

static const char *m_apx_definition2 = "APX/1.2\n"
      "N\"TestNode2\"\n"
      "R\"U8Value\"C:=0xff\n"
      "R\"U16Value\"S:=0xffff\n"
      "R\"U32Value\"L:=0xffffffff\n"
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

static const char *m_apx_definition5 = "APX/1.2\n"
      "N\"TestNode5\"\n"
      "P\"S8Value\"c:=-1\n"
      "P\"S16Value\"s:=-1\n"
      "P\"S32Value\"l:=-1\n"
      "\n";

static const char *m_apx_definition6 = "APX/1.2\n"
      "N\"TestNode6\"\n"
      "R\"S8Value\"c:=-1\n"
      "R\"S16Value\"s:=-1\n"
      "R\"S32Value\"l:=-1\n"
      "\n";

static const char *m_apx_definition7 = "APX/1.2\n"
      "N\"TestNode7\"\n"
      "P\"S8Array\"c[4]:={-1, -1, -1, -1}\n"
      "P\"S16Array\"s[4]:={-1, -1, -1, -1}\n"
      "P\"S32Array\"l[4]:={-1, -1, -1, -1}\n"
      "\n";

static const char *m_apx_definition8 = "APX/1.2\n"
      "N\"TestNode8\"\n"
      "R\"S8Array\"c[4]:={-1, -1, -1, -1}\n"
      "R\"S16Array\"s[4]:={-1, -1, -1, -1}\n"
      "R\"S32Array\"l[4]:={-1, -1, -1, -1}\n"
      "\n";

static const char *m_apx_definition9 = "APX/1.2\n"
      "N\"TestNode9\"\n"
      "P\"ColorSetting\"{\"Red\"C\"Green\"C\"Blue\"C}:={0,0,0}\n"
      "\n";

static const char *m_apx_definition10 = "APX/1.2\n"
      "N\"TestNode9\"\n"
      "R\"ColorSetting\"{\"Red\"C\"Green\"C\"Blue\"C}:={0,0,0}\n"
      "\n";


#define UNSIGNED_ARRAY_LEN 3
#define SIGNED_ARRAY_LEN   4

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_client_create(CuTest* tc);
static void test_apx_client_buildNodeFromString1(CuTest* tc);
static void test_apx_client_buildNodeFromString2(CuTest* tc);
static void test_apx_client_registerEventHandler(CuTest* tc);
static void test_apx_client_portHandleWithoutDefiningNodeName1(CuTest* tc);
static void test_apx_client_portHandleWithoutDefiningNodeName2(CuTest* tc);

static void test_apx_client_writePortData_dtl_u8(CuTest* tc);
static void test_apx_client_readPortData_dtl_u8(CuTest* tc);
static void test_apx_client_writePortData_direct_u8(CuTest* tc);
static void test_apx_client_readPortData_direct_u8(CuTest* tc);

static void test_apx_client_writePortData_dtl_u16(CuTest* tc);
static void test_apx_client_readPortData_dtl_u16(CuTest* tc);
static void test_apx_client_writePortData_direct_u16(CuTest* tc);
static void test_apx_client_readPortData_direct_u16(CuTest* tc);

static void test_apx_client_writePortData_dtl_u32(CuTest* tc);
static void test_apx_client_readPortData_dtl_u32(CuTest* tc);
static void test_apx_client_writePortData_direct_u32(CuTest* tc);
static void test_apx_client_readPortData_direct_u32(CuTest* tc);

static void test_apx_client_writePortData_dtl_u8_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_u8_fix_array(CuTest* tc);
static void test_apx_client_writePortData_dtl_u16_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_u16_fix_array(CuTest* tc);
static void test_apx_client_writePortData_dtl_u32_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_u32_fix_array(CuTest* tc);

static void test_apx_client_writePortData_dtl_s8(CuTest* tc);
static void test_apx_client_readPortData_dtl_S8(CuTest* tc);
static void test_apx_client_writePortData_dtl_s16(CuTest* tc);
static void test_apx_client_readPortData_dtl_S16(CuTest* tc);
static void test_apx_client_writePortData_dtl_s32(CuTest* tc);
static void test_apx_client_readPortData_dtl_S32(CuTest* tc);

static void test_apx_client_writePortData_dtl_S8_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_s8_fix_array(CuTest* tc);
static void test_apx_client_writePortData_dtl_S16_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_s16_fix_array(CuTest* tc);
static void test_apx_client_writePortData_dtl_S32_fix_array(CuTest* tc);
static void test_apx_client_readPortData_dtl_s32_fix_array(CuTest* tc);

static void test_apx_client_writePortData_dtl_record(CuTest* tc);
static void test_apx_client_readPortData_dtl_record(CuTest* tc);



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
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u8);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u8);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_direct_u8);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_direct_u8);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u16);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u16);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_direct_u16);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_direct_u16);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u32);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u32);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_direct_u32);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_direct_u32);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u8_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u8_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u16_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u16_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_u32_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_u32_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_s8);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_S8);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_s16);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_S16);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_s32);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_S32);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_S8_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_s8_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_S16_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_s16_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_S32_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_s32_fix_array);
   SUITE_ADD_TEST(suite, test_apx_client_writePortData_dtl_record);
   SUITE_ADD_TEST(suite, test_apx_client_readPortData_dtl_record);


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
   CuAssertIntEquals(tc, 3, apx_nodeInfo_getNumProvidePorts(nodeInfo));
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
   CuAssertIntEquals(tc, 3, apx_nodeInfo_getNumRequirePorts(nodeInfo));
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
   void *U8ValueHandle;
   void *U16ValueHandle;
   void *U32ValueHandle;
   apx_nodeInstance_t *node;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));

   node = apx_client_getLastAttachedNode(client);
   CuAssertPtrNotNull(tc, node);

   U8ValueHandle = apx_client_getPortHandle(client, NULL, "U8Value");
   U16ValueHandle = apx_client_getPortHandle(client, NULL, "U16Value");
   U32ValueHandle = apx_client_getPortHandle(client, NULL, "U32Value");
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(node, 0), U8ValueHandle);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(node, 1), U16ValueHandle);
   CuAssertPtrEquals(tc, apx_nodeInstance_getProvidePortRef(node, 2), U32ValueHandle);

   apx_client_delete(client);
}

static void test_apx_client_portHandleWithoutDefiningNodeName2(CuTest* tc)
{
   void *U8ValueHandle;
   void *U16ValueHandle;
   void *U32ValueHandle;
   apx_nodeInstance_t *node;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));

   node = apx_client_getLastAttachedNode(client);
   CuAssertPtrNotNull(tc, node);

   U8ValueHandle = apx_client_getPortHandle(client, NULL, "U8Value");
   U16ValueHandle = apx_client_getPortHandle(client, NULL, "U16Value");
   U32ValueHandle = apx_client_getPortHandle(client, NULL, "U32Value");
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(node, 0), U8ValueHandle);
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(node, 1), U16ValueHandle);
   CuAssertPtrEquals(tc, apx_nodeInstance_getRequirePortRef(node, 2), U32ValueHandle);
   apx_client_delete(client);

}

static void test_apx_client_writePortData_dtl_u8(CuTest* tc)
{
   const uint32_t offset = 0;
   void *U8ValueHandle;
   uint8_t rawData[UINT8_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   dtl_sv_t *sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   U8ValueHandle = apx_client_getPortHandle(client, NULL, "U8Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);

   dtl_sv_set_u32(sv, 0x00);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U8ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);

   dtl_sv_set_u32(sv, 0x12);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U8ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x12, rawData[0]);

   dtl_sv_set_u32(sv, 0xff);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U8ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0xff, rawData[0]);


   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*) sv);

}


static void test_apx_client_readPortData_dtl_u8(CuTest* tc)
{
   const uint32_t offset = 0u;
   void *U8ValueHandle;
   uint8_t rawData[UINT8_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   dtl_dv_t *dv = 0;
   bool ok = false;
   apx_client_t *client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   U8ValueHandle = apx_client_getPortHandle(client, NULL, "U8Value");
   CuAssertPtrNotNull(tc, U8ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U8ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 255, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(rawData, 0x12, UINT8_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U8ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(rawData, 0x00, UINT8_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U8ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x00, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);

   apx_client_delete(client);
}

static void test_apx_client_writePortData_direct_u8(CuTest* tc)
{
   uint32_t offset = 0u;
   void *U8ValueHandle;
   uint8_t rawData[UINT8_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   U8ValueHandle = apx_client_getPortHandle(client, NULL, "U8Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u8(client, U8ValueHandle, 0x00));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u8(client, U8ValueHandle, 0x12));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[1]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u8(client, U8ValueHandle, 0xff));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);

   apx_client_delete(client);

}

static void test_apx_client_readPortData_direct_u8(CuTest* tc)
{
   const uint32_t offset = 0u;
   void *U8ValueHandle;
   uint8_t rawData[UINT8_SIZE] = {0};
   uint8_t value;
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   U8ValueHandle = apx_client_getPortHandle(client, NULL, "U8Value");
   CuAssertPtrNotNull(tc, U8ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u8(client, U8ValueHandle, &value));
   CuAssertUIntEquals(tc, 255, value);

   packLE(rawData, 0x12, UINT8_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u8(client, U8ValueHandle, &value));
   CuAssertUIntEquals(tc, 0x12, value);

   packLE(rawData, 0x00, UINT8_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u8(client, U8ValueHandle, &value));
   CuAssertUIntEquals(tc, 0, value);

   apx_client_delete(client);
}


static void test_apx_client_writePortData_dtl_u16(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE;
   void *U16ValueHandle;
   uint8_t rawData[UINT16_SIZE] = {0, 0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   dtl_sv_t *sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   U16ValueHandle = apx_client_getPortHandle(client, NULL, "U16Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);

   dtl_sv_set_u32(sv, 0x0000);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U16ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);

   dtl_sv_set_u32(sv, 0x1234);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U16ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x34, rawData[0]);
   CuAssertUIntEquals(tc, 0x12, rawData[1]);

   dtl_sv_set_u32(sv, 0xffff);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U16ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xff, rawData[0]);
   CuAssertUIntEquals(tc, 0xff, rawData[1]);


   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*) sv);

}


static void test_apx_client_readPortData_dtl_u16(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE;
   void *U16ValueHandle;
   uint8_t rawData[UINT16_SIZE] = {0, 0};
   apx_nodeInstance_t *nodeInstance;
   dtl_dv_t *dv = 0;
   bool ok = false;
   apx_client_t *client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   U16ValueHandle = apx_client_getPortHandle(client, NULL, "U16Value");
   CuAssertPtrNotNull(tc, U16ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U16ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 65535, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(rawData, 0x1234, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U16ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(rawData, 0x0000, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U16ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x0000, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);

   apx_client_delete(client);
}

static void test_apx_client_writePortData_direct_u16(CuTest* tc)
{
   uint32_t offset = UINT8_SIZE;
   void *U16ValueHandle;

   uint8_t rawData[UINT16_SIZE] = {0, 0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   U16ValueHandle = apx_client_getPortHandle(client, NULL, "U16Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u16(client, U16ValueHandle, 0x0000));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u16(client, U16ValueHandle, 0x1234));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x34, rawData[0]);
   CuAssertUIntEquals(tc, 0x12, rawData[1]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u16(client, U16ValueHandle, 0xffff));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);

   apx_client_delete(client);

}

static void test_apx_client_readPortData_direct_u16(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE;
   void *U16ValueHandle;
   uint8_t rawData[UINT16_SIZE] = {0, 0};
   uint16_t value;
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   U16ValueHandle = apx_client_getPortHandle(client, NULL, "U16Value");
   CuAssertPtrNotNull(tc, U16ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u16(client, U16ValueHandle, &value));
   CuAssertUIntEquals(tc, 65535, value);

   packLE(rawData, 0x1234, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u16(client, U16ValueHandle, &value));
   CuAssertUIntEquals(tc, 0x1234, value);

   packLE(rawData, 0x0000, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u16(client, U16ValueHandle, &value));
   CuAssertUIntEquals(tc, 0, value);

   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_u32(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE+UINT16_SIZE;
   void *U32ValueHandle;
   uint8_t rawData[UINT32_SIZE] = {0, 0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   dtl_sv_t *sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   U32ValueHandle = apx_client_getPortHandle(client, NULL, "U32Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);

   dtl_sv_set_u32(sv, 0x00000000);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U32ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);

   dtl_sv_set_u32(sv, 0x12345678);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U32ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0x78, rawData[0]);
   CuAssertUIntEquals(tc, 0x56, rawData[1]);
   CuAssertUIntEquals(tc, 0x34, rawData[2]);
   CuAssertUIntEquals(tc, 0x12, rawData[3]);

   dtl_sv_set_u32(sv, 0xffffffff);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, U32ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0xff, rawData[0]);
   CuAssertUIntEquals(tc, 0xff, rawData[1]);
   CuAssertUIntEquals(tc, 0xff, rawData[2]);
   CuAssertUIntEquals(tc, 0xff, rawData[3]);


   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*) sv);

}


static void test_apx_client_readPortData_dtl_u32(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE+UINT16_SIZE;
   void *U32ValueHandle;
   uint8_t rawData[UINT32_SIZE] = {0, 0};
   apx_nodeInstance_t *nodeInstance;
   dtl_dv_t *dv = 0;
   bool ok = false;
   apx_client_t *client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   U32ValueHandle = apx_client_getPortHandle(client, NULL, "U32Value");
   CuAssertPtrNotNull(tc, U32ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U32ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0xffffffff, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(rawData, 0x12345678, UINT32_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT32_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U32ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x12345678, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(rawData, 0x00000000, UINT32_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT32_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, U32ValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x00000000, dtl_sv_to_u32((dtl_sv_t*) dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);

   apx_client_delete(client);
}

static void test_apx_client_writePortData_direct_u32(CuTest* tc)
{
   uint32_t offset = UINT8_SIZE+UINT16_SIZE;
   void *U32ValueHandle;

   uint8_t rawData[UINT32_SIZE] = {0, 0, 0, 0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   U32ValueHandle = apx_client_getPortHandle(client, NULL, "U32Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   CuAssertUIntEquals(tc, 0xFF, rawData[3]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u32(client, U32ValueHandle, 0x00000000));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);
   CuAssertUIntEquals(tc, 0x00, rawData[2]);
   CuAssertUIntEquals(tc, 0x00, rawData[3]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u32(client, U32ValueHandle, 0x12345678));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0x78, rawData[0]);
   CuAssertUIntEquals(tc, 0x56, rawData[1]);
   CuAssertUIntEquals(tc, 0x34, rawData[2]);
   CuAssertUIntEquals(tc, 0x12, rawData[3]);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData_u32(client, U32ValueHandle, 0xffffffff));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   CuAssertUIntEquals(tc, 0xFF, rawData[3]);

   apx_client_delete(client);

}

static void test_apx_client_readPortData_direct_u32(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE+UINT16_SIZE;
   void *U32ValueHandle;
   uint8_t rawData[UINT32_SIZE] = {0, 0, 0, 0};
   uint32_t value;
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   U32ValueHandle = apx_client_getPortHandle(client, NULL, "U32Value");
   CuAssertPtrNotNull(tc, U32ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u32(client, U32ValueHandle, &value));
   CuAssertUIntEquals(tc, 0xffffffff, value);

   packLE(rawData, 0x12345678, UINT32_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT32_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u32(client, U32ValueHandle, &value));
   CuAssertUIntEquals(tc, 0x12345678, value);

   packLE(rawData, 0x00000000, UINT32_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT32_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData_u32(client, U32ValueHandle, &value));
   CuAssertUIntEquals(tc, 0, value);

   apx_client_delete(client);
}


static void test_apx_client_writePortData_dtl_u8_fix_array(CuTest* tc)
{
   const uint32_t offset = 0u;
   apx_nodeInstance_t *nodeInstance;
   void *u8ArrayHandle;
   uint8_t rawData[UINT8_SIZE*UNSIGNED_ARRAY_LEN] = {0, 0, 0};
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition3));
   u8ArrayHandle = apx_client_getProvidePortHandleById(client, NULL, 0u);
   CuAssertPtrNotNull(tc, u8ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*UNSIGNED_ARRAY_LEN));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0x12), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_u32(0xff), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, u8ArrayHandle, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*UNSIGNED_ARRAY_LEN));
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
   uint8_t rawData[UINT8_SIZE*UNSIGNED_ARRAY_LEN] = {0, 0, 0};
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*UNSIGNED_ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, u8ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, UNSIGNED_ARRAY_LEN, dtl_av_length(av));
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
   const uint32_t offset = UINT8_SIZE*UNSIGNED_ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *u16ArrayHandle;
   uint8_t rawData[UINT16_SIZE*UNSIGNED_ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();
   memset(rawData, 0, sizeof(rawData));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition3));
   u16ArrayHandle = apx_client_getProvidePortHandleById(client, NULL, 1u);
   CuAssertPtrNotNull(tc, u16ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*UNSIGNED_ARRAY_LEN));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*UNSIGNED_ARRAY_LEN));
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
   const uint32_t offset = UINT8_SIZE*UNSIGNED_ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *u16ArrayHandle;
   uint8_t rawData[UINT16_SIZE*UNSIGNED_ARRAY_LEN];
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*UNSIGNED_ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, u16ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, UNSIGNED_ARRAY_LEN, dtl_av_length(av));
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
   const uint32_t offset = UINT8_SIZE*UNSIGNED_ARRAY_LEN + UINT16_SIZE*UNSIGNED_ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *u32ArrayHandle;
   uint8_t rawData[UINT32_SIZE*UNSIGNED_ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();
   memset(rawData, 0, sizeof(rawData));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition3));
   u32ArrayHandle = apx_client_getProvidePortHandleById(client, NULL, 2u);
   CuAssertPtrNotNull(tc, u32ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*UNSIGNED_ARRAY_LEN));
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*UNSIGNED_ARRAY_LEN));
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
   const uint32_t offset = UINT8_SIZE*UNSIGNED_ARRAY_LEN+UINT16_SIZE*UNSIGNED_ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *u32ArrayHandle;
   uint8_t rawData[UINT32_SIZE*UNSIGNED_ARRAY_LEN];
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
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*UNSIGNED_ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, u32ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, UNSIGNED_ARRAY_LEN, dtl_av_length(av));
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

static void test_apx_client_writePortData_dtl_s8(CuTest* tc)
{
   const uint32_t offset = 0;
   void *S8ValueHandle;
   uint8_t rawData[UINT8_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   dtl_sv_t *sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition5));
   S8ValueHandle = apx_client_getPortHandle(client, NULL, "S8Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);

   dtl_sv_set_i32(sv, -128);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S8ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x80, rawData[0]);

   dtl_sv_set_i32(sv, -1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S8ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0xff, rawData[0]);

   dtl_sv_set_i32(sv, 1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S8ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x01, rawData[0]);

   dtl_sv_set_i32(sv, 127);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S8ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x7f, rawData[0]);

   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*) sv);
}

static void test_apx_client_readPortData_dtl_S8(CuTest* tc)
{
   const uint32_t offset = 0u;
   void *S8ValueHandle;
   uint8_t rawData[UINT8_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   dtl_sv_t *sv = 0;
   bool ok = false;
   apx_client_t *client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition6));
   S8ValueHandle = apx_client_getPortHandle(client, NULL, "S8Value");
   CuAssertPtrNotNull(tc, S8ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S8ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x80, UINT8_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S8ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -128, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x01, UINT8_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S8ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x7f, UINT8_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S8ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 127, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;


   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_s16(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE;
   void *S16ValueHandle;
   uint8_t rawData[UINT16_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   dtl_sv_t *sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition5));
   S16ValueHandle = apx_client_getPortHandle(client, NULL, "S16Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);

   dtl_sv_set_i32(sv, -32768);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S16ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x80, rawData[1]);

   dtl_sv_set_i32(sv, -1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S16ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xff, rawData[0]);
   CuAssertUIntEquals(tc, 0xff, rawData[1]);

   dtl_sv_set_i32(sv, 1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S16ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x01, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);

   dtl_sv_set_i32(sv, 32767);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S16ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xff, rawData[0]);
   CuAssertUIntEquals(tc, 0x7f, rawData[1]);

   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*) sv);
}

static void test_apx_client_readPortData_dtl_S16(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE;
   void *S16ValueHandle;
   uint8_t rawData[UINT16_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   dtl_sv_t *sv = 0;
   bool ok = false;
   apx_client_t *client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition6));
   S16ValueHandle = apx_client_getPortHandle(client, NULL, "S16Value");
   CuAssertPtrNotNull(tc, S16ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S16ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x8000, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S16ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -32768, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x0001, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S16ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x7fff, UINT16_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S16ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 32767, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;


   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_s32(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE+UINT16_SIZE;
   void *S32ValueHandle;
   uint8_t rawData[UINT32_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   apx_client_t *client = apx_client_new();
   dtl_sv_t *sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition5));
   S32ValueHandle = apx_client_getPortHandle(client, NULL, "S32Value");
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   CuAssertUIntEquals(tc, 0xFF, rawData[3]);

   dtl_sv_set_i32(sv, -2147483648);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S32ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);
   CuAssertUIntEquals(tc, 0x00, rawData[2]);
   CuAssertUIntEquals(tc, 0x80, rawData[3]);

   dtl_sv_set_i32(sv, -1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S32ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0xff, rawData[0]);
   CuAssertUIntEquals(tc, 0xff, rawData[1]);
   CuAssertUIntEquals(tc, 0xff, rawData[2]);
   CuAssertUIntEquals(tc, 0xff, rawData[3]);

   dtl_sv_set_i32(sv, 1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S32ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0x01, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);
   CuAssertUIntEquals(tc, 0x00, rawData[2]);
   CuAssertUIntEquals(tc, 0x00, rawData[3]);

   dtl_sv_set_i32(sv, 2147483647);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S32ValueHandle, (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE));
   CuAssertUIntEquals(tc, 0xff, rawData[0]);
   CuAssertUIntEquals(tc, 0xff, rawData[1]);
   CuAssertUIntEquals(tc, 0xff, rawData[2]);
   CuAssertUIntEquals(tc, 0x7f, rawData[3]);

   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*) sv);
}

static void test_apx_client_readPortData_dtl_S32(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE+UINT16_SIZE;
   void *S32ValueHandle;
   uint8_t rawData[UINT32_SIZE] = {0};
   apx_nodeInstance_t *nodeInstance;
   dtl_sv_t *sv = 0;
   bool ok = false;
   apx_client_t *client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition6));
   S32ValueHandle = apx_client_getPortHandle(client, NULL, "S32Value");
   CuAssertPtrNotNull(tc, S32ValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S32ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x80000000, UINT32_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT32_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S32ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -2147483648, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x00000001, UINT32_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT32_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S32ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 1, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;

   packLE(rawData, 0x7fffffff, UINT32_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, UINT32_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S32ValueHandle, (dtl_dv_t**) &sv));
   CuAssertPtrNotNull(tc, sv);
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 2147483647, dtl_sv_to_i32(sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dec_ref(sv);
   sv = 0;
   ok = false;


   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_S8_fix_array(CuTest* tc)
{
   const uint32_t offset = 0u;
   apx_nodeInstance_t *nodeInstance;
   void *S8ArrayHandle;
   uint8_t rawData[UINT8_SIZE*SIGNED_ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();

   memset(&rawData[0], 0, sizeof(rawData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition7));
   S8ArrayHandle = apx_client_getPortHandle(client, NULL, "S8Array");
   CuAssertPtrNotNull(tc, S8ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*SIGNED_ARRAY_LEN));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   CuAssertUIntEquals(tc, 0xFF, rawData[3]);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-128), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(127), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S8ArrayHandle, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*SIGNED_ARRAY_LEN));
   CuAssertUIntEquals(tc, 0x80, rawData[0]);
   CuAssertUIntEquals(tc, 0xff, rawData[1]);
   CuAssertUIntEquals(tc, 0x00, rawData[2]);
   CuAssertUIntEquals(tc, 0x7f, rawData[3]);

   apx_client_delete(client);
   dtl_dec_ref(av);
}

static void test_apx_client_readPortData_dtl_s8_fix_array(CuTest* tc)
{
   const uint32_t offset = 0u;
   apx_nodeInstance_t *nodeInstance;
   void *S8ArrayHandle;
   uint8_t rawData[UINT8_SIZE*SIGNED_ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_dv_t *dv;
   dtl_av_t *av;
   dtl_sv_t *sv;

   memset(rawData, 0, sizeof(rawData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition8));
   S8ArrayHandle = apx_client_getPortHandle(client, NULL, "S8Array");
   CuAssertPtrNotNull(tc, S8ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   rawData[0] = 0x80;
   rawData[1] = 0xff;
   rawData[2] = 0x00;
   rawData[3] = 0x7f;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT8_SIZE*SIGNED_ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S8ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, SIGNED_ARRAY_LEN, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -128, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 3);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 127, dtl_sv_to_i32(sv, NULL));

   dtl_dv_dec_ref(dv);
   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_S16_fix_array(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE*SIGNED_ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *S16ArrayHandle;
   uint8_t rawData[UINT16_SIZE*SIGNED_ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();

   memset(&rawData[0], 0, sizeof(rawData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition7));
   S16ArrayHandle = apx_client_getPortHandle(client, NULL, "S16Array");
   CuAssertPtrNotNull(tc, S16ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*SIGNED_ARRAY_LEN));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   CuAssertUIntEquals(tc, 0xFF, rawData[3]);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-32768), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(32767), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S16ArrayHandle, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*SIGNED_ARRAY_LEN));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x80, rawData[1]);
   CuAssertUIntEquals(tc, 0xff, rawData[2]);
   CuAssertUIntEquals(tc, 0xff, rawData[3]);
   CuAssertUIntEquals(tc, 0x00, rawData[4]);
   CuAssertUIntEquals(tc, 0x00, rawData[5]);
   CuAssertUIntEquals(tc, 0xff, rawData[6]);
   CuAssertUIntEquals(tc, 0x7f, rawData[7]);

   apx_client_delete(client);
   dtl_dec_ref(av);
}

static void test_apx_client_readPortData_dtl_s16_fix_array(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE*SIGNED_ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *S16ArrayHandle;
   uint8_t rawData[UINT16_SIZE*SIGNED_ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_dv_t *dv;
   dtl_av_t *av;
   dtl_sv_t *sv;

   memset(rawData, 0, sizeof(rawData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition8));
   S16ArrayHandle = apx_client_getPortHandle(client, NULL, "S16Array");
   CuAssertPtrNotNull(tc, S16ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   rawData[0] = 0x00;
   rawData[1] = 0x80;
   rawData[2] = 0xff;
   rawData[3] = 0xff;
   rawData[4] = 0x00;
   rawData[5] = 0x00;
   rawData[6] = 0xff;
   rawData[7] = 0x7f;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT16_SIZE*SIGNED_ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S16ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, SIGNED_ARRAY_LEN, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -32768, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 3);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 32767, dtl_sv_to_i32(sv, NULL));

   dtl_dv_dec_ref(dv);
   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_S32_fix_array(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE*SIGNED_ARRAY_LEN+UINT16_SIZE*SIGNED_ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *S32ArrayHandle;
   uint8_t rawData[UINT32_SIZE*SIGNED_ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_av_t *av = dtl_av_new();

   memset(&rawData[0], 0, sizeof(rawData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition7));
   S32ArrayHandle = apx_client_getPortHandle(client, NULL, "S32Array");
   CuAssertPtrNotNull(tc, S32ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*SIGNED_ARRAY_LEN));
   CuAssertUIntEquals(tc, 0xFF, rawData[0]);
   CuAssertUIntEquals(tc, 0xFF, rawData[1]);
   CuAssertUIntEquals(tc, 0xFF, rawData[2]);
   CuAssertUIntEquals(tc, 0xFF, rawData[3]);
   CuAssertUIntEquals(tc, 0xFF, rawData[4]);
   CuAssertUIntEquals(tc, 0xFF, rawData[5]);
   CuAssertUIntEquals(tc, 0xFF, rawData[7]);
   CuAssertUIntEquals(tc, 0xFF, rawData[8]);
   CuAssertUIntEquals(tc, 0xFF, rawData[9]);
   CuAssertUIntEquals(tc, 0xFF, rawData[10]);
   CuAssertUIntEquals(tc, 0xFF, rawData[11]);
   CuAssertUIntEquals(tc, 0xFF, rawData[12]);
   CuAssertUIntEquals(tc, 0xFF, rawData[13]);
   CuAssertUIntEquals(tc, 0xFF, rawData[14]);
   CuAssertUIntEquals(tc, 0xFF, rawData[15]);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-2147483648), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(-1), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(0), false);
   dtl_av_push(av, (dtl_dv_t*) dtl_sv_make_i32(2147483647), false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, S32ArrayHandle, (dtl_dv_t*) av));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*SIGNED_ARRAY_LEN));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);
   CuAssertUIntEquals(tc, 0x00, rawData[2]);
   CuAssertUIntEquals(tc, 0x80, rawData[3]);
   CuAssertUIntEquals(tc, 0xff, rawData[4]);
   CuAssertUIntEquals(tc, 0xff, rawData[5]);
   CuAssertUIntEquals(tc, 0xff, rawData[6]);
   CuAssertUIntEquals(tc, 0xff, rawData[7]);
   CuAssertUIntEquals(tc, 0x00, rawData[8]);
   CuAssertUIntEquals(tc, 0x00, rawData[9]);
   CuAssertUIntEquals(tc, 0x00, rawData[10]);
   CuAssertUIntEquals(tc, 0x00, rawData[11]);
   CuAssertUIntEquals(tc, 0xff, rawData[12]);
   CuAssertUIntEquals(tc, 0xff, rawData[13]);
   CuAssertUIntEquals(tc, 0xff, rawData[14]);
   CuAssertUIntEquals(tc, 0x7f, rawData[15]);

   apx_client_delete(client);
   dtl_dec_ref(av);
}

static void test_apx_client_readPortData_dtl_s32_fix_array(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE*SIGNED_ARRAY_LEN+UINT16_SIZE*SIGNED_ARRAY_LEN;
   apx_nodeInstance_t *nodeInstance;
   void *S32ArrayHandle;
   uint8_t rawData[UINT32_SIZE*SIGNED_ARRAY_LEN];
   apx_client_t *client = apx_client_new();
   dtl_dv_t *dv;
   dtl_av_t *av;
   dtl_sv_t *sv;

   memset(rawData, 0, sizeof(rawData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition8));
   S32ArrayHandle = apx_client_getPortHandle(client, NULL, "S32Array");
   CuAssertPtrNotNull(tc, S32ArrayHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);
   rawData[0] = 0x00;
   rawData[1] = 0x00;
   rawData[2] = 0x00;
   rawData[3] = 0x80;
   rawData[4] = 0xff;
   rawData[5] = 0xff;
   rawData[6] = 0xff;
   rawData[7] = 0xff;
   rawData[8] = 0x00;
   rawData[9] = 0x00;
   rawData[10] = 0x00;
   rawData[11] = 0x00;
   rawData[12] = 0xff;
   rawData[13] = 0xff;
   rawData[14] = 0xff;
   rawData[15] = 0x7f;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeRequirePortData(nodeInstance, &rawData[0], offset, UINT32_SIZE*SIGNED_ARRAY_LEN));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, S32ArrayHandle, &dv));
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type(dv));
   av = (dtl_av_t*) dv;
   CuAssertIntEquals(tc, SIGNED_ARRAY_LEN, dtl_av_length(av));
   sv = (dtl_sv_t*) dtl_av_value(av, 0);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -2147483648, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 1);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, -1, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 2);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 0, dtl_sv_to_i32(sv, NULL));
   sv = (dtl_sv_t*) dtl_av_value(av, 3);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type( (dtl_dv_t*) sv));
   CuAssertIntEquals(tc, DTL_SV_I32, dtl_sv_type(sv));
   CuAssertIntEquals(tc, 2147483647, dtl_sv_to_i32(sv, NULL));

   dtl_dv_dec_ref(dv);
   apx_client_delete(client);
}

static void test_apx_client_writePortData_dtl_record(CuTest* tc)
{
   const uint32_t offset = 0;
   const apx_size_t dataSize = UINT8_SIZE*3;
   void *recordValueHandle;
   uint8_t rawData[UINT8_SIZE*3];
   dtl_hv_t *hv;
   dtl_sv_t *red;
   dtl_sv_t *green;
   dtl_sv_t *blue;
   apx_nodeInstance_t *nodeInstance;
   memset(&rawData[0], 0, sizeof(rawData));
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition9));
   recordValueHandle = apx_client_getPortHandle(client, NULL, "ColorSetting");
   CuAssertPtrNotNull(tc, recordValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);



   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, dataSize));
   CuAssertUIntEquals(tc, 0x00, rawData[0]);
   CuAssertUIntEquals(tc, 0x00, rawData[1]);
   CuAssertUIntEquals(tc, 0x00, rawData[2]);

   hv = dtl_hv_new();
   CuAssertPtrNotNull(tc, hv);
   red = dtl_sv_make_u32(0x12);
   green = dtl_sv_make_u32(0x34);
   blue = dtl_sv_make_u32(0x56);
   dtl_hv_set_cstr(hv, "Red", (dtl_dv_t*) red, true);
   dtl_hv_set_cstr(hv, "Green", (dtl_dv_t*) green, true);
   dtl_hv_set_cstr(hv, "Blue", (dtl_dv_t*) blue, true);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_writePortData(client, recordValueHandle, (dtl_dv_t*) hv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readProvidePortData(nodeInstance, &rawData[0], offset, dataSize));
   CuAssertUIntEquals(tc, 0x12, rawData[0]);
   CuAssertUIntEquals(tc, 0x34, rawData[1]);
   CuAssertUIntEquals(tc, 0x56, rawData[2]);

   apx_client_delete(client);
   dtl_dec_ref(hv);
   dtl_dec_ref(red);
   dtl_dec_ref(green);
   dtl_dec_ref(blue);
}

static void test_apx_client_readPortData_dtl_record(CuTest* tc)
{
   const uint32_t offset = 0;
   const apx_size_t dataSize = UINT8_SIZE*3;
   void *recordValueHandle;
   uint8_t rawData[UINT8_SIZE*3];
   dtl_dv_t *dv;
   dtl_hv_t *hv;
   dtl_sv_t *red;
   dtl_sv_t *green;
   dtl_sv_t *blue;
   apx_nodeInstance_t *nodeInstance;

   bool ok = false;
   apx_client_t *client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition10));
   recordValueHandle = apx_client_getPortHandle(client, NULL, "ColorSetting");
   CuAssertPtrNotNull(tc, recordValueHandle);
   nodeInstance = apx_client_getLastAttachedNode(client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, recordValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(dv));
   hv =(dtl_hv_t*) dv;
   CuAssertIntEquals(tc, 3, dtl_hv_length(hv));
   red = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Red");
   green = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Green");
   blue = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Blue");
   CuAssertPtrNotNull(tc, red);
   CuAssertPtrNotNull(tc, green);
   CuAssertPtrNotNull(tc, blue);
   CuAssertUIntEquals(tc, 0x00, dtl_sv_to_u32(red, &ok));
   CuAssertTrue(tc, ok);
   CuAssertUIntEquals(tc, 0x00, dtl_sv_to_u32(green, &ok));
   CuAssertTrue(tc, ok);
   CuAssertUIntEquals(tc, 0x00, dtl_sv_to_u32(blue, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;

   packLE(&rawData[0], 0x12, UINT8_SIZE);
   packLE(&rawData[1], 0x34, UINT8_SIZE);
   packLE(&rawData[2], 0x56, UINT8_SIZE);
   apx_nodeInstance_writeRequirePortData(nodeInstance, rawData, offset, dataSize);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_readPortData(client, recordValueHandle, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type(dv));
   hv =(dtl_hv_t*) dv;
   CuAssertIntEquals(tc, 3, dtl_hv_length(hv));
   red = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Red");
   green = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Green");
   blue = (dtl_sv_t*) dtl_hv_get_cstr(hv, "Blue");
   CuAssertPtrNotNull(tc, red);
   CuAssertPtrNotNull(tc, green);
   CuAssertPtrNotNull(tc, blue);
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32(red, &ok));
   CuAssertTrue(tc, ok);
   CuAssertUIntEquals(tc, 0x34, dtl_sv_to_u32(green, &ok));
   CuAssertTrue(tc, ok);
   CuAssertUIntEquals(tc, 0x56, dtl_sv_to_u32(blue, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);

   apx_client_delete(client);
}
