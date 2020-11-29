/*****************************************************************************
* \file      testsuite_apx_nodeInfo.c
* \author    Conny Gustafsson
* \date      2019-11-25
* \brief     Unit tests for apx_nodeInfo
*
* Copyright (c) 2019 Conny Gustafsson
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "apx_nodeInfo.h"
#include "apx_test_nodes.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeInfo_buildU8RequirePortNoInitValue(CuTest *tc);
static void test_apx_nodeInfo_buildU8RequirePortInitValue(CuTest *tc);
static void test_apx_nodeInfo_calcPortDataLen1(CuTest *tc);
static void test_apx_nodeInfo_calcPortDataLen2(CuTest *tc);
static void test_apx_nodeInfo_calcPortDataLen3(CuTest *tc);
static void test_apx_nodeInfo_calcPortDataLen4(CuTest *tc);
static void test_apx_nodeInfo_calcPortDataLen5(CuTest *tc);
static void test_apx_nodeInfo_buildDerivedPortSignatures1(CuTest *tc);
static void test_apx_nodeInfo_getClientPortNamesFromSignatures(CuTest *tc);
static void test_apx_nodeInfo_getRequirePortName(CuTest *tc);
static void test_apx_nodeInfo_getProvidePortName(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeInfo(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeInfo_buildU8RequirePortNoInitValue);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_buildU8RequirePortInitValue);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_calcPortDataLen1);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_calcPortDataLen2);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_calcPortDataLen3);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_calcPortDataLen4);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_calcPortDataLen5);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_buildDerivedPortSignatures1);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_getClientPortNamesFromSignatures);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_getRequirePortName);
   SUITE_ADD_TEST(suite, test_apx_nodeInfo_getProvidePortName);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_nodeInfo_buildU8RequirePortNoInitValue(CuTest *tc)
{
   const char *apx_node1 = "APX/1.2\n"
   "N\"Node\"\n"
   "R\"GearSelectionMode\"C(0,7)\n";

   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(apx_node1, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertIntEquals(tc, 1, apx_nodeInfo_getNumRequirePorts(nodeInfo));
   CuAssertIntEquals(tc, 0, apx_nodeInfo_getNumProvidePorts(nodeInfo));
   CuAssertUIntEquals(tc, 1, apx_nodeInfo_calcRequirePortDataLen(nodeInfo));
   CuAssertUIntEquals(tc, 0, apx_nodeInfo_calcProvidePortDataLen(nodeInfo));
   apx_portDataProps_t *props = apx_nodeInfo_getRequirePortDataProps(nodeInfo, 0);
   CuAssertPtrNotNull(tc, props);
   CuAssertIntEquals(tc, 0, props->portId);
   CuAssertUIntEquals(tc, 1, props->dataSize);
   CuAssertUIntEquals(tc, 0, props->offset);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT, props->portType);
   CuAssertIntEquals(tc, APX_QUE_LEN_NONE, props->queLenType);
   CuAssertTrue(tc, !props->isDynamicArray);
   CuAssertUIntEquals(tc, 1, apx_nodeInfo_getRequirePortInitDataSize(nodeInfo));
   CuAssertUIntEquals(tc, 0, apx_nodeInfo_getProvidePortInitDataSize(nodeInfo));
   const uint8_t *requirePortInitData = apx_nodeInfo_getRequirePortInitDataPtr(nodeInfo);
   CuAssertUIntEquals(tc, 0, requirePortInitData[0]);
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_buildU8RequirePortInitValue(CuTest *tc)
{
   const char *apx_node1 = "APX/1.2\n"
   "N\"Node\"\n"
   "R\"GearSelectionMode\"C(0,7):=7\n";

   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(apx_node1, APX_CLIENT_MODE);
   CuAssertUIntEquals(tc, 1, apx_nodeInfo_getRequirePortInitDataSize(nodeInfo));
   CuAssertUIntEquals(tc, 0, apx_nodeInfo_getProvidePortInitDataSize(nodeInfo));
   const uint8_t *requirePortInitData = apx_nodeInfo_getRequirePortInitDataPtr(nodeInfo);
   CuAssertUIntEquals(tc, 7, requirePortInitData[0]);
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_calcPortDataLen1(CuTest *tc)
{
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node1, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertIntEquals(tc, 1, apx_nodeInfo_getNumRequirePorts(nodeInfo));
   CuAssertIntEquals(tc, 3, apx_nodeInfo_getNumProvidePorts(nodeInfo));
   CuAssertUIntEquals(tc, 1, apx_nodeInfo_calcRequirePortDataLen(nodeInfo));
   CuAssertUIntEquals(tc, 4, apx_nodeInfo_calcProvidePortDataLen(nodeInfo));
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_calcPortDataLen2(CuTest *tc)
{
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node2, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertIntEquals(tc, 1, apx_nodeInfo_getNumRequirePorts(nodeInfo));
   CuAssertIntEquals(tc, 2, apx_nodeInfo_getNumProvidePorts(nodeInfo));
   CuAssertUIntEquals(tc, 1, apx_nodeInfo_calcRequirePortDataLen(nodeInfo));
   CuAssertUIntEquals(tc, 3, apx_nodeInfo_calcProvidePortDataLen(nodeInfo));
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_calcPortDataLen3(CuTest *tc)
{
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node3, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertIntEquals(tc, 1, apx_nodeInfo_getNumRequirePorts(nodeInfo));
   CuAssertIntEquals(tc, 0, apx_nodeInfo_getNumProvidePorts(nodeInfo));
   CuAssertUIntEquals(tc, 2, apx_nodeInfo_calcRequirePortDataLen(nodeInfo));
   CuAssertUIntEquals(tc, 0, apx_nodeInfo_calcProvidePortDataLen(nodeInfo));
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_calcPortDataLen4(CuTest *tc)
{
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node4, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertIntEquals(tc, 3, apx_nodeInfo_getNumRequirePorts(nodeInfo));
   CuAssertIntEquals(tc, 0, apx_nodeInfo_getNumProvidePorts(nodeInfo));
   CuAssertUIntEquals(tc, 4, apx_nodeInfo_calcRequirePortDataLen(nodeInfo));
   CuAssertUIntEquals(tc, 0, apx_nodeInfo_calcProvidePortDataLen(nodeInfo));
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_calcPortDataLen5(CuTest *tc)
{
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node5, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertIntEquals(tc, 2, apx_nodeInfo_getNumRequirePorts(nodeInfo));
   CuAssertIntEquals(tc, 1, apx_nodeInfo_getNumProvidePorts(nodeInfo));
   CuAssertUIntEquals(tc, 3, apx_nodeInfo_calcRequirePortDataLen(nodeInfo));
   CuAssertUIntEquals(tc, 1, apx_nodeInfo_calcProvidePortDataLen(nodeInfo));
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_buildDerivedPortSignatures1(CuTest *tc)
{
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node1, APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertStrEquals(tc, "\"WheelBasedVehicleSpeed\"S", apx_nodeInfo_getProvidePortSignature(nodeInfo, 0));
   CuAssertStrEquals(tc, "\"CabTiltLockWarning\"C(0,7)", apx_nodeInfo_getProvidePortSignature(nodeInfo, 1));
   CuAssertStrEquals(tc, "\"VehicleMode\"C(0,15)", apx_nodeInfo_getProvidePortSignature(nodeInfo, 2));
   CuAssertConstPtrEquals(tc, NULL, apx_nodeInfo_getProvidePortSignature(nodeInfo, 3));
   CuAssertStrEquals(tc, "\"GearSelectionMode\"C(0,7)", apx_nodeInfo_getRequirePortSignature(nodeInfo, 0));
   CuAssertConstPtrEquals(tc, NULL, apx_nodeInfo_getRequirePortSignature(nodeInfo, 1));
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_getClientPortNamesFromSignatures(CuTest *tc)
{
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node1, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);
   CuAssertUIntEquals(tc, APX_PORT_ID_PROVIDE_PORT | 0u, apx_nodeInfo_findPortIdByName(nodeInfo, "WheelBasedVehicleSpeed"));
   CuAssertUIntEquals(tc, APX_PORT_ID_PROVIDE_PORT | 1u, apx_nodeInfo_findPortIdByName(nodeInfo, "CabTiltLockWarning"));
   CuAssertUIntEquals(tc, APX_PORT_ID_PROVIDE_PORT | 2u, apx_nodeInfo_findPortIdByName(nodeInfo, "VehicleMode"));
   CuAssertUIntEquals(tc, 0u, apx_nodeInfo_findPortIdByName(nodeInfo, "GearSelectionMode"));
   CuAssertUIntEquals(tc, APX_INVALID_PORT_ID, apx_nodeInfo_findPortIdByName(nodeInfo, "DoesNotExist"));
   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_getRequirePortName(CuTest *tc)
{
   adt_str_t *portName;
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node4, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);

   portName = apx_nodeInfo_getRequirePortName(nodeInfo, 0);
   CuAssertPtrNotNull(tc, portName);
   CuAssertStrEquals(tc, "WheelBasedVehicleSpeed", adt_str_cstr(portName));
   adt_str_delete(portName);

   portName = apx_nodeInfo_getRequirePortName(nodeInfo, 1);
   CuAssertPtrNotNull(tc, portName);
   CuAssertStrEquals(tc, "ParkBrakeAlert", adt_str_cstr(portName));
   adt_str_delete(portName);

   portName = apx_nodeInfo_getRequirePortName(nodeInfo, 2);
   CuAssertPtrNotNull(tc, portName);
   CuAssertStrEquals(tc, "VehicleMode", adt_str_cstr(portName));
   adt_str_delete(portName);

   CuAssertPtrEquals(tc, 0, apx_nodeInfo_getRequirePortName(nodeInfo, 3));

   apx_nodeInfo_delete(nodeInfo);
}

static void test_apx_nodeInfo_getProvidePortName(CuTest *tc)
{
   adt_str_t *portName;
   apx_nodeInfo_t *nodeInfo = apx_nodeInfo_make_from_cstr(g_apx_test_node1, APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, nodeInfo);

   portName = apx_nodeInfo_getProvidePortName(nodeInfo, 0);
   CuAssertPtrNotNull(tc, portName);
   CuAssertStrEquals(tc, "WheelBasedVehicleSpeed", adt_str_cstr(portName));
   adt_str_delete(portName);

   portName = apx_nodeInfo_getProvidePortName(nodeInfo, 1);
   CuAssertPtrNotNull(tc, portName);
   CuAssertStrEquals(tc, "CabTiltLockWarning", adt_str_cstr(portName));
   adt_str_delete(portName);

   portName = apx_nodeInfo_getProvidePortName(nodeInfo, 2);
   CuAssertPtrNotNull(tc, portName);
   CuAssertStrEquals(tc, "VehicleMode", adt_str_cstr(portName));
   adt_str_delete(portName);

   CuAssertPtrEquals(tc, 0, apx_nodeInfo_getProvidePortName(nodeInfo, 3));

   apx_nodeInfo_delete(nodeInfo);
}
