/*****************************************************************************
* \file      testsuite_apx_dataRouting.c
* \author    Conny Gustafsson
* \date      2020-02-20
* \brief     Test cases for server port connections and data routing
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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "numheader.h"
#include "CuTest.h"
#include "apx_server.h"
#include "apx_serverTestConnection.h"
#include "apx_connectionEventSpy.h"
#include "apx_transmitHandlerSpy.h"
#include "apx_nodeManager.h"
#include "apx_util.h"
#include "pack.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_connectors_connectNodeWithOnlyRequirePorts(CuTest* tc);
static void test_connectors_disconnectNodeWithOnlyRequirePorts(CuTest* tc);
static void test_connectors_connectNodeWithOnlyProvidePorts(CuTest* tc);
static void test_connectors_disconnectNodeWithOnlyProvidePorts(CuTest* tc);
static void test_connectors_nodeWithRequirePortIsConnectedAfterNodeWithProvidePort(CuTest* tc);
static void test_connectors_nodeWithProvidePortIsConnectedAfterMultipleRequireNodesAreWaiting(CuTest* tc);
static void test_connectors_nodeWithProvidePortIsDisconnectedFromMultipleRequireNodes(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const char *m_apx_definition1 = "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"VehicleSpeed\"S:=65535\n"
      "\n";

static const char *m_apx_definition2 = "APX/1.2\n"
      "N\"TestNode2\"\n"
      "R\"VehicleSpeed\"S:=65535\n"
      "\n";

static const char *m_apx_definition3 = "APX/1.2\n"
      "N\"TestNode3\"\n"
      "R\"EngineSpeed\"S:=65535\n"
      "R\"VehicleSpeed\"S:=65535\n"
      "\n";

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_dataRouting(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_connectors_connectNodeWithOnlyRequirePorts);
   SUITE_ADD_TEST(suite, test_connectors_disconnectNodeWithOnlyRequirePorts);
   SUITE_ADD_TEST(suite, test_connectors_connectNodeWithOnlyProvidePorts);
   SUITE_ADD_TEST(suite, test_connectors_disconnectNodeWithOnlyProvidePorts);
   SUITE_ADD_TEST(suite, test_connectors_nodeWithRequirePortIsConnectedAfterNodeWithProvidePort);
   SUITE_ADD_TEST(suite, test_connectors_nodeWithProvidePortIsConnectedAfterMultipleRequireNodesAreWaiting);
   SUITE_ADD_TEST(suite, test_connectors_nodeWithProvidePortIsDisconnectedFromMultipleRequireNodes);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_connectors_connectNodeWithOnlyRequirePorts(CuTest* tc)
{
   apx_serverTestConnection_t *connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *buffer;
   apx_server_t *server;
   apx_portSignatureMap_t *portSignatureMap;
   apx_size_t definitionLen;
   adt_bytearray_t *receivedMsg;
   adt_bytearray_t *expectedMsg;
   uint8_t *expectedData;
   int32_t msgSize;
   apx_nodeInstance_t *nodeInstance;

   //Init
   server = apx_server_new();
   connection = apx_serverTestConnection_new();
   portSignatureMap = apx_server_getPortSignatureMap(server);
   CuAssertPtrNotNull(tc, portSignatureMap);
   apx_server_acceptConnection(server, (apx_serverConnectionBase_t*) connection);
   CuAssertPtrEquals(tc, server, apx_serverConnectionBase_getServer((apx_serverConnectionBase_t*) connection));
   apx_serverTestConnection_onProtocolHeaderReceived(connection);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected headerAck
   apx_serverTestConnection_clearTransmitLogMsg(connection);

   //Client sends info about TestNode2 to server
   CuAssertIntEquals(tc, 0, apx_serverTestConnection_getTransmitLogLen(connection));
   definitionLen = strlen(m_apx_definition2);
   rmf_fileInfo_create(&fileInfo, "TestNode2.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected openFile("TestNode2.apx")

   //Client sends contents of TestNode2.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition2[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 2, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected openFile("TestNode2.apx")+fileInfo("TestNode2.in")

   //Verify that server sent fileInfo("TestNode2.in")
   receivedMsg = apx_serverTestConnection_getTransmitLogMsg(connection, 1);
   expectedMsg = adt_bytearray_new(ADT_BYTEARRAY_DEFAULT_GROW_SIZE);
   msgSize = RMF_HIGH_ADDRESS_SIZE+RMF_CMD_FILE_INFO_BASE_SIZE+strlen("TestNode2.in")+1;
   adt_bytearray_resize(expectedMsg, msgSize);
   expectedData = adt_bytearray_data(expectedMsg);
   rmf_fileInfo_create(&fileInfo, "TestNode2.in", 0u, UINT16_SIZE, RMF_FILE_TYPE_FIXED);
   rmf_packHeader(expectedData, RMF_HIGH_ADDRESS_SIZE, RMF_CMD_START_ADDR, false);
   rmf_serialize_cmdFileInfo(expectedData+RMF_HIGH_ADDRESS_SIZE, msgSize-RMF_HIGH_ADDRESS_SIZE, &fileInfo);
   CuAssertTrue(tc, adt_bytearray_equals(receivedMsg, expectedMsg));

   nodeInstance = apx_serverTestConnection_findNodeInstance(connection, "TestNode2");
   CuAssertPtrNotNull(tc, nodeInstance);

   //Client sends fileOpen("TestNode2.in")
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_nodeInstance_getRequirePortDataState(nodeInstance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onFileOpenMsgReceived(connection, 0u));
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance));

   free(buffer);
   adt_bytearray_delete(expectedMsg);

   apx_server_delete(server);

}

static void test_connectors_disconnectNodeWithOnlyRequirePorts(CuTest* tc)
{
   apx_serverTestConnection_t *connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *buffer;
   apx_server_t *server;
   apx_portSignatureMap_t *portSignatureMap;
   apx_size_t definitionLen;
   adt_bytearray_t *receivedMsg;
   adt_bytearray_t *expectedMsg;
   uint8_t *expectedData;
   int32_t msgSize;
   apx_nodeInstance_t *nodeInstance;

   //Init
   server = apx_server_new();
   connection = apx_serverTestConnection_new();
   portSignatureMap = apx_server_getPortSignatureMap(server);
   CuAssertPtrNotNull(tc, portSignatureMap);
   apx_server_acceptConnection(server, (apx_serverConnectionBase_t*) connection);
   CuAssertPtrEquals(tc, server, apx_serverConnectionBase_getServer((apx_serverConnectionBase_t*) connection));
   apx_serverTestConnection_onProtocolHeaderReceived(connection);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected headerAck
   apx_serverTestConnection_clearTransmitLogMsg(connection);

   //Client sends info about TestNode2 to server
   CuAssertIntEquals(tc, 0, apx_serverTestConnection_getTransmitLogLen(connection));
   definitionLen = strlen(m_apx_definition2);
   rmf_fileInfo_create(&fileInfo, "TestNode2.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected openFile("TestNode2.apx")

   //Client sends contents of TestNode2.apx

   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition2[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 2, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected openFile("TestNode2.apx")+fileInfo("TestNode2.in")

   //Verify that server sent fileInfo("TestNode2.in")
   receivedMsg = apx_serverTestConnection_getTransmitLogMsg(connection, 1);
   expectedMsg = adt_bytearray_new(ADT_BYTEARRAY_DEFAULT_GROW_SIZE);
   msgSize = RMF_HIGH_ADDRESS_SIZE+RMF_CMD_FILE_INFO_BASE_SIZE+strlen("TestNode2.in")+1;
   adt_bytearray_resize(expectedMsg, msgSize);
   expectedData = adt_bytearray_data(expectedMsg);
   rmf_fileInfo_create(&fileInfo, "TestNode2.in", 0u, UINT16_SIZE, RMF_FILE_TYPE_FIXED);
   rmf_packHeader(expectedData, RMF_HIGH_ADDRESS_SIZE, RMF_CMD_START_ADDR, false);
   rmf_serialize_cmdFileInfo(expectedData+RMF_HIGH_ADDRESS_SIZE, msgSize-RMF_HIGH_ADDRESS_SIZE, &fileInfo);
   CuAssertTrue(tc, adt_bytearray_equals(receivedMsg, expectedMsg));

   nodeInstance = apx_serverTestConnection_findNodeInstance(connection, "TestNode2");
   CuAssertPtrNotNull(tc, nodeInstance);

   //Client sends fileOpen("TestNode2.in")
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_nodeInstance_getRequirePortDataState(nodeInstance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onFileOpenMsgReceived(connection, 0u));
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance));

   //Detach connection from server
   //apx_server_detachConnection(server, (apx_serverConnectionBase_t*) connection);
   apx_serverTestConnection_onDisconnect(connection);
   nodeInstance = apx_serverTestConnection_findNodeInstance(connection, "TestNode2");
   CuAssertPtrNotNull(tc, nodeInstance);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance));

   free(buffer);
   adt_bytearray_delete(expectedMsg);

   apx_server_delete(server);
}

static void test_connectors_connectNodeWithOnlyProvidePorts(CuTest* tc)
{
   apx_serverTestConnection_t *connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *buffer;
   apx_server_t *server;
   apx_portSignatureMap_t *portSignatureMap;
   apx_size_t definitionLen;
   apx_nodeInstance_t *nodeInstance;
   uint8_t providePortData[UINT16_SIZE];

   //Init
   server = apx_server_new();
   connection = apx_serverTestConnection_new();
   portSignatureMap = apx_server_getPortSignatureMap(server);
   CuAssertPtrNotNull(tc, portSignatureMap);
   apx_server_acceptConnection(server, (apx_serverConnectionBase_t*) connection);
   CuAssertPtrEquals(tc, server, apx_serverConnectionBase_getServer((apx_serverConnectionBase_t*) connection));
   apx_serverTestConnection_onProtocolHeaderReceived(connection);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected headerAck
   apx_serverTestConnection_clearTransmitLogMsg(connection);

   //Client sends file info about TestNode1 to server
   CuAssertIntEquals(tc, 0, apx_serverTestConnection_getTransmitLogLen(connection));
   definitionLen = strlen(m_apx_definition1);
   rmf_fileInfo_create(&fileInfo, "TestNode1.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   rmf_fileInfo_create(&fileInfo, "TestNode1.out", 0u, UINT16_SIZE, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected openFile("TestNode2.apx")

   //Client sends contents of TestNode1.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition1[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 2, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected openFile("TestNode1.apx")+fileOpen("TestNode1.out")

   nodeInstance = apx_serverTestConnection_findNodeInstance(connection, "TestNode1");
   CuAssertPtrNotNull(tc, nodeInstance);

   //Verify port connection state before sending data
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance));
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_getProvidePortDataState(nodeInstance));

   //Send contents of TestNode1.out
   packLE(&providePortData[0], 0xFFFF, UINT16_SIZE); //VehicleSpeed value
   CuAssertIntEquals(tc, RMF_LOW_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_LOW_ADDRESS_SIZE, 0u, false));
   memcpy(&buffer[RMF_LOW_ADDRESS_SIZE], &providePortData[0], UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_LOW_ADDRESS_SIZE+UINT16_SIZE));


   //Verify port connection state after sending data
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance));
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getProvidePortDataState(nodeInstance));

   free(buffer);
   apx_server_delete(server);

}

static void test_connectors_disconnectNodeWithOnlyProvidePorts(CuTest* tc)
{
   apx_serverTestConnection_t *connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *buffer;
   apx_server_t *server;
   apx_portSignatureMap_t *portSignatureMap;
   apx_size_t definitionLen;
   apx_nodeInstance_t *nodeInstance;
   uint8_t providePortData[UINT16_SIZE];

   //Init
   server = apx_server_new();
   connection = apx_serverTestConnection_new();
   portSignatureMap = apx_server_getPortSignatureMap(server);
   CuAssertPtrNotNull(tc, portSignatureMap);
   apx_server_acceptConnection(server, (apx_serverConnectionBase_t*) connection);
   CuAssertPtrEquals(tc, server, apx_serverConnectionBase_getServer((apx_serverConnectionBase_t*) connection));
   apx_serverTestConnection_onProtocolHeaderReceived(connection);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected headerAck
   apx_serverTestConnection_clearTransmitLogMsg(connection);

   //Client sends file info about TestNode1 to server
   CuAssertIntEquals(tc, 0, apx_serverTestConnection_getTransmitLogLen(connection));
   definitionLen = strlen(m_apx_definition1);
   rmf_fileInfo_create(&fileInfo, "TestNode1.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   rmf_fileInfo_create(&fileInfo, "TestNode1.out", 0u, UINT16_SIZE, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected openFile("TestNode2.apx")

   //Client sends contents of TestNode1.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition1[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 2, apx_serverTestConnection_getTransmitLogLen(connection)); //Expected openFile("TestNode1.apx")+fileOpen("TestNode1.out")

   nodeInstance = apx_serverTestConnection_findNodeInstance(connection, "TestNode1");
   CuAssertPtrNotNull(tc, nodeInstance);

   //Verify port connection state before sending data
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance));
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_WAITING_FOR_FILE_DATA, apx_nodeInstance_getProvidePortDataState(nodeInstance));

   //Send contents of TestNode1.out
   packLE(&providePortData[0], 0xFFFF, UINT16_SIZE); //VehicleSpeed value
   CuAssertIntEquals(tc, RMF_LOW_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_LOW_ADDRESS_SIZE, 0u, false));
   memcpy(&buffer[RMF_LOW_ADDRESS_SIZE], &providePortData[0], UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_LOW_ADDRESS_SIZE+UINT16_SIZE));

   //Verify port connection state after sending data
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance));
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getProvidePortDataState(nodeInstance));

   //Detach connection from server
   apx_serverTestConnection_onDisconnect(connection);
   nodeInstance = apx_serverTestConnection_findNodeInstance(connection, "TestNode1");
   CuAssertPtrNotNull(tc, nodeInstance);
   //Verify port connection state after sending data
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance));
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getProvidePortDataState(nodeInstance));

   free(buffer);
   apx_server_delete(server);

}


static void test_connectors_nodeWithRequirePortIsConnectedAfterNodeWithProvidePort(CuTest* tc)
{
   apx_serverTestConnection_t *connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *buffer;
   apx_server_t *server;
   apx_portSignatureMap_t *portSignatureMap;
   apx_size_t definitionLen;
   apx_nodeInstance_t *nodeInstance1; //Associated with TestNode1 (the one with provide ports)
   apx_nodeInstance_t *nodeInstance2; //Associated with TestNode2 (the one with require ports)
   uint8_t rawProvidePortData[UINT16_SIZE];
   uint8_t rawRequirePortData[UINT16_SIZE];
   adt_bytearray_t *transmittedMsg;
   const uint8_t *transmittedBytes;

   //Init
   server = apx_server_new();
   connection = apx_serverTestConnection_new();
   portSignatureMap = apx_server_getPortSignatureMap(server);
   CuAssertPtrNotNull(tc, portSignatureMap);
   apx_server_acceptConnection(server, (apx_serverConnectionBase_t*) connection);
   CuAssertPtrEquals(tc, server, apx_serverConnectionBase_getServer((apx_serverConnectionBase_t*) connection));
   apx_serverTestConnection_onProtocolHeaderReceived(connection);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends file info about TestNode1 to server
   definitionLen = strlen(m_apx_definition1);
   rmf_fileInfo_create(&fileInfo, "TestNode1.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   rmf_fileInfo_create(&fileInfo, "TestNode1.out", 0u, UINT16_SIZE, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends contents of TestNode1.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition1[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);

   nodeInstance1 = apx_serverTestConnection_findNodeInstance(connection, "TestNode1");
   CuAssertPtrNotNull(tc, nodeInstance1);

   //Send contents of TestNode1.out
   packLE(&rawProvidePortData[0], 0x1234, UINT16_SIZE); //VehicleSpeed value
   CuAssertIntEquals(tc, RMF_LOW_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_LOW_ADDRESS_SIZE, 0u, false));
   memcpy(&buffer[RMF_LOW_ADDRESS_SIZE], &rawProvidePortData[0], UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_LOW_ADDRESS_SIZE+UINT16_SIZE));
   free(buffer);

   //Verify port connection state after sending data
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getProvidePortDataState(nodeInstance1));

   //Client sends info about TestNode2 to server
   definitionLen = strlen(m_apx_definition2);
   rmf_fileInfo_create(&fileInfo, "TestNode2.apx", APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends contents of TestNode2.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition2[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   free(buffer);

   nodeInstance2 = apx_serverTestConnection_findNodeInstance(connection, "TestNode2");
   CuAssertPtrNotNull(tc, nodeInstance2);

   //Verify port connection state before sending openFile request
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST, apx_nodeInstance_getRequirePortDataState(nodeInstance2));

   //Client sends fileOpen("TestNode2.in")
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onFileOpenMsgReceived(connection, 0u));

   //Verify port connection state after openFIle
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance2));

   //Verify port connector TestNode1.VehicleSpeed -> TestNode2.VehicleSpeed
   apx_portConnectorList_t *connectors = apx_nodeInstance_getProvidePortConnectors(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, connectors);
   CuAssertIntEquals(tc, 1, apx_portConnectorList_length(connectors));

   //Verify that TestNode2.VehicleSpeed has the value 0x1234 (which is the value sent from TestNode1)
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readRequirePortData(nodeInstance2, &rawRequirePortData[0], 0u, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0x1234, unpackLE(&rawRequirePortData[0], UINT16_SIZE));

   //Verify that TestNode2.in has been sent from server
   apx_serverTestConnection_clearTransmitLogMsg(connection);
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection));
   transmittedMsg = apx_serverTestConnection_getTransmitLogMsg(connection, 0);
   CuAssertPtrNotNull(tc, transmittedMsg);
   transmittedBytes = adt_bytearray_data(transmittedMsg);
   CuAssertUIntEquals(tc, 0, rmf_unpackAddress(transmittedBytes, RMF_LOW_ADDRESS_SIZE));
   CuAssertUIntEquals(tc, 0x1234, unpackLE(&transmittedBytes[RMF_LOW_ADDRESS_SIZE], UINT16_SIZE));

   //Cleanup
   apx_serverTestConnection_runEventLoop(connection);
   apx_server_delete(server);

}

static void test_connectors_nodeWithProvidePortIsConnectedAfterMultipleRequireNodesAreWaiting(CuTest* tc)
{
   apx_serverTestConnection_t *connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *buffer;
   apx_server_t *server;
   apx_portSignatureMap_t *portSignatureMap;
   apx_size_t definitionLen;
   apx_nodeInstance_t *nodeInstance1; //Associated with TestNode1
   apx_nodeInstance_t *nodeInstance2; //Associated with TestNode2
   apx_nodeInstance_t *nodeInstance3; //Associated with TestNode3
   apx_portRef_t *requirePortRef;
   uint8_t rawProvidePortData[UINT16_SIZE];
   uint8_t rawRequirePortData[UINT16_SIZE];
   adt_bytearray_t *transmittedMsg;
   const uint8_t *transmittedBytes;


   //Init
   server = apx_server_new();
   connection = apx_serverTestConnection_new();
   portSignatureMap = apx_server_getPortSignatureMap(server);
   CuAssertPtrNotNull(tc, portSignatureMap);
   apx_server_acceptConnection(server, (apx_serverConnectionBase_t*) connection);
   CuAssertPtrEquals(tc, server, apx_serverConnectionBase_getServer((apx_serverConnectionBase_t*) connection));
   apx_serverTestConnection_onProtocolHeaderReceived(connection);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends info about TestNode2 to server
   definitionLen = strlen(m_apx_definition2);
   rmf_fileInfo_create(&fileInfo, "TestNode2.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends contents of TestNode2.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition2[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   free(buffer);

   //Client sends fileOpen("TestNode2.in")
   apx_serverTestConnection_clearTransmitLogMsg(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onFileOpenMsgReceived(connection, 0u));
   apx_serverTestConnection_runEventLoop(connection);

   //Verify port connection state on TestNode2
   nodeInstance2 = apx_serverTestConnection_findNodeInstance(connection, "TestNode2");
   CuAssertPtrNotNull(tc, nodeInstance2);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance2));

   //Verify that TestNode2.VehicleSpeed has init value 0xFFFF
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readRequirePortData(nodeInstance2, &rawRequirePortData[0], 0u, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFFFF, unpackLE(&rawRequirePortData[0], UINT16_SIZE));

   //Verify that TestNode2.in has been sent from server
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection));
   transmittedMsg = apx_serverTestConnection_getTransmitLogMsg(connection, 0);
   CuAssertPtrNotNull(tc, transmittedMsg);
   transmittedBytes = adt_bytearray_data(transmittedMsg);
   CuAssertUIntEquals(tc, APX_ADDRESS_PORT_DATA_START, rmf_unpackAddress(transmittedBytes, RMF_LOW_ADDRESS_SIZE));
   CuAssertUIntEquals(tc, 0xFFFF, unpackLE(&transmittedBytes[RMF_LOW_ADDRESS_SIZE], UINT16_SIZE));

   //Client sends info about TestNode3 to server
   definitionLen = strlen(m_apx_definition3);
   rmf_fileInfo_create(&fileInfo, "TestNode3.apx", APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends contents of TestNode3.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition3[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   free(buffer);

   //Client sends fileOpen("TestNode3.in")
   apx_serverTestConnection_clearTransmitLogMsg(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onFileOpenMsgReceived(connection, APX_ADDRESS_PORT_DATA_BOUNDARY));
   apx_serverTestConnection_runEventLoop(connection);

   //Verify port connection state on TestNode2
   nodeInstance3 = apx_serverTestConnection_findNodeInstance(connection, "TestNode3");
   CuAssertPtrNotNull(tc, nodeInstance3);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance3));

   //Verify that TestNode3.VehicleSpeed has init value 0xFFFF
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readRequirePortData(nodeInstance3, &rawRequirePortData[0], UINT16_SIZE, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFFFF, unpackLE(&rawRequirePortData[0], UINT16_SIZE));

   //Verify that TestNode2.in has been sent from server
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection));
   transmittedMsg = apx_serverTestConnection_getTransmitLogMsg(connection, 0);
   CuAssertPtrNotNull(tc, transmittedMsg);
   transmittedBytes = adt_bytearray_data(transmittedMsg);
   CuAssertUIntEquals(tc, APX_ADDRESS_PORT_DATA_START+APX_ADDRESS_PORT_DATA_BOUNDARY, rmf_unpackAddress(transmittedBytes, RMF_LOW_ADDRESS_SIZE));
   CuAssertUIntEquals(tc, 0xFFFF, unpackLE(&transmittedBytes[RMF_LOW_ADDRESS_SIZE], UINT16_SIZE));

   //Client sends file info about TestNode1 to server
   definitionLen = strlen(m_apx_definition1);
   rmf_fileInfo_create(&fileInfo, "TestNode1.apx", APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY*2, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   rmf_fileInfo_create(&fileInfo, "TestNode1.out", APX_ADDRESS_PORT_DATA_START, UINT16_SIZE, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends contents of TestNode1.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY*2, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition1[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);

   //Send contents of TestNode1.out
   apx_serverTestConnection_clearTransmitLogMsg(connection);
   packLE(&rawProvidePortData[0], 0x1234, UINT16_SIZE); //VehicleSpeed value
   CuAssertIntEquals(tc, RMF_LOW_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_LOW_ADDRESS_SIZE, APX_ADDRESS_PORT_DATA_START, false));
   memcpy(&buffer[RMF_LOW_ADDRESS_SIZE], &rawProvidePortData[0], UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_LOW_ADDRESS_SIZE+UINT16_SIZE));
   free(buffer);

   //Verify port connection state on TestNode1
   nodeInstance1 = apx_serverTestConnection_findNodeInstance(connection, "TestNode1");
   CuAssertPtrNotNull(tc, nodeInstance1);
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getProvidePortDataState(nodeInstance1));

      //Verify that all port connector change tables are cleared
   CuAssertPtrEquals(tc, 0, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrEquals(tc, 0, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false));
   CuAssertPtrEquals(tc, 0, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance3, false));


   //Verify connectors
   apx_portConnectorList_t *connectors = apx_nodeInstance_getProvidePortConnectors(nodeInstance1, 0); //VehicleSpeed
   CuAssertPtrNotNull(tc, connectors);
   CuAssertIntEquals(tc, 2, apx_portConnectorList_length(connectors));
   requirePortRef = apx_portConnectorList_get(connectors, 0);
   CuAssertPtrNotNull(tc, requirePortRef);
   CuAssertPtrEquals(tc, nodeInstance2, requirePortRef->nodeInstance);
   CuAssertIntEquals(tc, 0, requirePortRef->portId);

   requirePortRef = apx_portConnectorList_get(connectors, 1);
   CuAssertPtrEquals(tc, nodeInstance3, requirePortRef->nodeInstance);
   CuAssertIntEquals(tc, 1, requirePortRef->portId);

   //Verify that 2 messages have been sent from the server
   apx_serverTestConnection_runEventLoop(connection);
   CuAssertIntEquals(tc, 2, apx_serverTestConnection_getTransmitLogLen(connection));

   //Verify that TestNode2.in has been written to at offset 0
   transmittedMsg = apx_serverTestConnection_getTransmitLogMsg(connection, 0);
   CuAssertPtrNotNull(tc, transmittedMsg);
   transmittedBytes = adt_bytearray_data(transmittedMsg);
   CuAssertUIntEquals(tc, APX_ADDRESS_PORT_DATA_START, rmf_unpackAddress(transmittedBytes, RMF_LOW_ADDRESS_SIZE));
   CuAssertUIntEquals(tc, 0x1234, unpackLE(&transmittedBytes[RMF_LOW_ADDRESS_SIZE], UINT16_SIZE));

   //Verify that TestNode2.in has been written to at offset UINT16_SIZE
   transmittedMsg = apx_serverTestConnection_getTransmitLogMsg(connection, 1);
   CuAssertPtrNotNull(tc, transmittedMsg);
   transmittedBytes = adt_bytearray_data(transmittedMsg);
   CuAssertUIntEquals(tc, APX_ADDRESS_PORT_DATA_START+APX_ADDRESS_PORT_DATA_BOUNDARY+UINT16_SIZE, rmf_unpackAddress(transmittedBytes, RMF_LOW_ADDRESS_SIZE));
   CuAssertUIntEquals(tc, 0x1234, unpackLE(&transmittedBytes[RMF_LOW_ADDRESS_SIZE], UINT16_SIZE));


   apx_serverTestConnection_runEventLoop(connection);
   apx_server_delete(server);
}

static void test_connectors_nodeWithProvidePortIsDisconnectedFromMultipleRequireNodes(CuTest* tc)
{
   apx_serverTestConnection_t *connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *buffer;
   apx_server_t *server;
   apx_portSignatureMap_t *portSignatureMap;
   apx_size_t definitionLen;
   apx_nodeInstance_t *nodeInstance1; //Associated with TestNode1
   apx_nodeInstance_t *nodeInstance2; //Associated with TestNode2
   apx_nodeInstance_t *nodeInstance3; //Associated with TestNode3

   uint8_t rawProvidePortData[UINT16_SIZE];
   uint8_t rawRequirePortData[UINT16_SIZE];
   adt_bytearray_t *transmittedMsg;
   const uint8_t *transmittedBytes;


   //Init
   server = apx_server_new();
   connection = apx_serverTestConnection_new();
   portSignatureMap = apx_server_getPortSignatureMap(server);
   CuAssertPtrNotNull(tc, portSignatureMap);
   apx_server_acceptConnection(server, (apx_serverConnectionBase_t*) connection);
   CuAssertPtrEquals(tc, server, apx_serverConnectionBase_getServer((apx_serverConnectionBase_t*) connection));
   apx_serverTestConnection_onProtocolHeaderReceived(connection);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends info about TestNode2 to server
   definitionLen = strlen(m_apx_definition2);
   rmf_fileInfo_create(&fileInfo, "TestNode2.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends contents of TestNode2.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition2[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   free(buffer);

   //Client sends fileOpen("TestNode2.in")
   apx_serverTestConnection_clearTransmitLogMsg(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onFileOpenMsgReceived(connection, 0u));
   apx_serverTestConnection_runEventLoop(connection);

   //Verify port connection state on TestNode2
   nodeInstance2 = apx_serverTestConnection_findNodeInstance(connection, "TestNode2");
   CuAssertPtrNotNull(tc, nodeInstance2);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance2));

   //Verify that TestNode2.VehicleSpeed has init value 0xFFFF
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_readRequirePortData(nodeInstance2, &rawRequirePortData[0], 0u, UINT16_SIZE));
   CuAssertUIntEquals(tc, 0xFFFF, unpackLE(&rawRequirePortData[0], UINT16_SIZE));

   //Verify that TestNode2.in has been sent from server
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(connection));
   transmittedMsg = apx_serverTestConnection_getTransmitLogMsg(connection, 0);
   CuAssertPtrNotNull(tc, transmittedMsg);
   transmittedBytes = adt_bytearray_data(transmittedMsg);
   CuAssertUIntEquals(tc, APX_ADDRESS_PORT_DATA_START, rmf_unpackAddress(transmittedBytes, RMF_LOW_ADDRESS_SIZE));
   CuAssertUIntEquals(tc, 0xFFFF, unpackLE(&transmittedBytes[RMF_LOW_ADDRESS_SIZE], UINT16_SIZE));

   //Client sends info about TestNode3 to server
   definitionLen = strlen(m_apx_definition3);
   rmf_fileInfo_create(&fileInfo, "TestNode3.apx", APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends contents of TestNode3.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition3[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);
   free(buffer);

   //Client sends fileOpen("TestNode3.in")
   apx_serverTestConnection_clearTransmitLogMsg(connection);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onFileOpenMsgReceived(connection, APX_ADDRESS_PORT_DATA_BOUNDARY));
   apx_serverTestConnection_runEventLoop(connection);

   //Verify port connection state on TestNode3
   nodeInstance3 = apx_serverTestConnection_findNodeInstance(connection, "TestNode3");
   CuAssertPtrNotNull(tc, nodeInstance3);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance3));

   //Verify connector cleanup
   CuAssertPtrEquals(tc, 0, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false));
   CuAssertPtrEquals(tc, 0, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance3, false));


   //Client sends file info about TestNode1 to server
   definitionLen = strlen(m_apx_definition1);
   rmf_fileInfo_create(&fileInfo, "TestNode1.apx", APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY*2, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   rmf_fileInfo_create(&fileInfo, "TestNode1.out", APX_ADDRESS_PORT_DATA_START, UINT16_SIZE, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(connection);

   //Client sends contents of TestNode1.apx
   buffer = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(buffer != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START+APX_ADDRESS_DEFINITION_BOUNDARY*2, false));
   memcpy(&buffer[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition1[0], definitionLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   apx_serverTestConnection_runEventLoop(connection);

   //Send contents of TestNode1.out
   apx_serverTestConnection_clearTransmitLogMsg(connection);
   packLE(&rawProvidePortData[0], 0x1234, UINT16_SIZE); //VehicleSpeed value
   CuAssertIntEquals(tc, RMF_LOW_ADDRESS_SIZE, rmf_packHeader(&buffer[0], RMF_LOW_ADDRESS_SIZE, APX_ADDRESS_PORT_DATA_START, false));
   memcpy(&buffer[RMF_LOW_ADDRESS_SIZE], &rawProvidePortData[0], UINT16_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(connection, buffer, RMF_LOW_ADDRESS_SIZE+UINT16_SIZE));
   free(buffer);

   //Verify port connection state on TestNode1
   nodeInstance1 = apx_serverTestConnection_findNodeInstance(connection, "TestNode1");
   CuAssertPtrNotNull(tc, nodeInstance1);
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_CONNECTED, apx_nodeInstance_getProvidePortDataState(nodeInstance1));

   apx_serverTestConnection_runEventLoop(connection);

   //Before disconnect, verify that all port connector tables are freed
   CuAssertPtrEquals(tc, 0, apx_nodeInstance_getProvidePortConnectorChanges(nodeInstance1, false));
   CuAssertPtrEquals(tc, 0, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance2, false));
   CuAssertPtrEquals(tc, 0, apx_nodeInstance_getRequirePortConnectorChanges(nodeInstance3, false));
   //Disconnect TestNode1
   apx_serverTestConnection_onDisconnect(connection);

   //Verify all connected nodes have their port states disconnected
   CuAssertIntEquals(tc, APX_PROVIDE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getProvidePortDataState(nodeInstance1));
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance2));
   CuAssertIntEquals(tc, APX_REQUIRE_PORT_DATA_STATE_DISCONNECTED, apx_nodeInstance_getRequirePortDataState(nodeInstance3));

   apx_serverTestConnection_runEventLoop(connection);
   apx_server_delete(server);
}
