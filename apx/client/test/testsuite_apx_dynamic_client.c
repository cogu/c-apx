/*****************************************************************************
* \file      testsuite_apx_dynamic_client.c
* \author    Conny Gustafsson
* \date      2019-02-09
* \brief     Unit tests for apx_client_t containing dynamically generated nodes
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
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include "CuTest.h"
#include "pack.h"
#include "apx_client.h"
#include "apx_portDataMap.h"
#include "apx_nodeData.h"
#include "apx_test_nodes.h"
#include "apx_clientTestConnection.h"
#include "rmf.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define ERROR_MSG_SIZE 128

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_create_port_data_map(CuTest* tc);
static void test_create_pack_programs_in_map(CuTest* tc);
static void test_trigger_fileManager_outPortDataWriteEvent(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_dynamic_client(void)
{
   CuSuite* suite = CuSuiteNew();
   SUITE_ADD_TEST(suite, test_create_port_data_map);
   SUITE_ADD_TEST(suite, test_create_pack_programs_in_map);
   SUITE_ADD_TEST(suite, test_trigger_fileManager_outPortDataWriteEvent);

   return suite;

}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_port_data_map(CuTest* tc)
{
   apx_client_t *client;
   apx_nodeData_t *nodeData = NULL;
   client = apx_client_new();
   CuAssertIntEquals(tc, 0, apx_client_getNumAttachedNodes(client));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_createLocalNode_cstr(client, g_apx_test_node1));
   CuAssertIntEquals(tc, 1, apx_client_getNumAttachedNodes(client));
   nodeData = apx_client_getDynamicNode(client, 0);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertTrue(tc, nodeData->isDynamic);
   CuAssertPtrNotNull(tc, apx_nodeData_getPortDataMap(nodeData));
   apx_client_delete(client);
}

static void test_create_pack_programs_in_map(CuTest* tc)
{
   apx_client_t *client;
   apx_nodeData_t *nodeData = NULL;
   apx_portDataMap_t *portDataMap = NULL;
   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_createLocalNode_cstr(client, g_apx_test_node1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_createLocalNode_cstr(client, g_apx_test_node3));
   CuAssertIntEquals(tc, 2, apx_client_getNumAttachedNodes(client));
   nodeData = apx_client_getDynamicNode(client, 0);
   CuAssertPtrNotNull(tc, nodeData);
   portDataMap = apx_nodeData_getPortDataMap(nodeData);
   CuAssertPtrNotNull(tc, portDataMap);
   CuAssertPtrNotNull(tc, portDataMap->requirePortPackPrograms);
   CuAssertPtrNotNull(tc, portDataMap->providePortPackPrograms);

   nodeData = apx_client_getDynamicNode(client, 1);
   CuAssertPtrNotNull(tc, nodeData);
   portDataMap = apx_nodeData_getPortDataMap(nodeData);
   CuAssertPtrNotNull(tc, portDataMap);
   CuAssertPtrNotNull(tc, portDataMap->requirePortPackPrograms);
   CuAssertPtrEquals(tc, NULL, portDataMap->providePortPackPrograms);

   apx_client_delete(client);
}

static void test_trigger_fileManager_outPortDataWriteEvent(CuTest* tc)
{
   apx_client_t *client;
   apx_nodeData_t *nodeData = NULL;
   apx_clientTestConnection_t *connection;
   adt_bytearray_t *byteArray;
   const uint8_t *msg;
   uint8_t tmp[2];
   uint16_t WheelBasedVehicleSpeedValue = 123;
   const uint32_t remoteFileAddress = 0x0;
   const uint32_t WheelBasedVehicleSpeedOffset = 0;
   const uint8_t expectedMsg[] = {0, 0, 123, 0};
   int i;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_createLocalNode_cstr(client, g_apx_test_node1));
   connection = apx_clientTestConnection_new(client);
   nodeData = apx_client_getDynamicNode(client, 0);
   CuAssertPtrEquals(tc, NULL, apx_nodeData_getConnection(nodeData));
   apx_client_attachConnection(client, &connection->base);
   CuAssertPtrNotNull(tc, apx_nodeData_getConnection(nodeData));
   apx_clientTestConnection_openRemoteFile(connection, remoteFileAddress);
   apx_client_run(client);
   CuAssertIntEquals(tc, 1, apx_clientTestConnection_getTransmitLogLen(connection));
   apx_clientTestConnection_clearTransmitLog(connection);
   packLE(&tmp[0], WheelBasedVehicleSpeedValue, sizeof(WheelBasedVehicleSpeedValue));
   apx_nodeData_updateOutPortData(nodeData, &tmp[0], WheelBasedVehicleSpeedOffset, sizeof(WheelBasedVehicleSpeedValue), false);
   CuAssertIntEquals(tc, 0, apx_clientTestConnection_getTransmitLogLen(connection));
   apx_client_run(client);
   CuAssertIntEquals(tc, 1, apx_clientTestConnection_getTransmitLogLen(connection));
   byteArray = apx_clientTestConnection_getTransmitLogMsg(connection, 0);
   CuAssertPtrNotNull(tc, byteArray);
   CuAssertIntEquals(tc, RMF_LOW_ADDRESS_SIZE+sizeof(WheelBasedVehicleSpeedValue), adt_bytearray_length(byteArray));
   msg = adt_bytearray_data(byteArray);
   for(i=0; i < 4;i++)
   {
      char errorMsg[ERROR_MSG_SIZE];
      sprintf(errorMsg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, errorMsg, expectedMsg[i], msg[i]);
   }
   apx_clientTestConnection_clearTransmitLog(connection);
   apx_client_delete(client);
}
