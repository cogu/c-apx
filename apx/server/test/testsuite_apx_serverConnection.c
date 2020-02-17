/*****************************************************************************
* \file      testsuite_apx_serverConnection.c
* \author    Conny Gustafsson
* \date      2020-01-02
* \brief     Unit Tests for server connection
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

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char *m_apx_definition1 = "APX/1.2\n"
      "N\"TestNode\"\n"
      "P\"VehicleSpeed\"S:=65535\n"
      "\n";
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_headerAcceptedEventIsTriggered(CuTest* tc);
static void test_createRemoteFileShallTriggerFileCreatedEvent(CuTest* tc);
static void test_nodeInstanceIsCreatedWhenDefinitionFileIsSeen(CuTest* tc);
static void test_serverInitializesFileHandlerWhenDefinitionFileIsSeen(CuTest* tc);
static void test_serverInitializesNodeDataBufferWhenDefinitionFileIsSeen(CuTest* tc);
static void test_fileOpenRequestIsSentWhenDefinitionFileIsSeen(CuTest* tc);
static void test_serverProcessesDefinitionAfterClientWritesDefinitionData(CuTest* tc);
static void test_serverCreatesOutPortDataBuffersAfterProcessingNodeDefinition(CuTest* tc);
static void test_serverDetectsOutPortDataFileAfterProcessingNodeDefinition(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_serverConnection(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_headerAcceptedEventIsTriggered);
   SUITE_ADD_TEST(suite, test_createRemoteFileShallTriggerFileCreatedEvent);
   SUITE_ADD_TEST(suite, test_nodeInstanceIsCreatedWhenDefinitionFileIsSeen);
   SUITE_ADD_TEST(suite, test_serverInitializesFileHandlerWhenDefinitionFileIsSeen);
   SUITE_ADD_TEST(suite, test_serverInitializesNodeDataBufferWhenDefinitionFileIsSeen);
   SUITE_ADD_TEST(suite, test_fileOpenRequestIsSentWhenDefinitionFileIsSeen);
   SUITE_ADD_TEST(suite, test_serverProcessesDefinitionAfterClientWritesDefinitionData);
   SUITE_ADD_TEST(suite, test_serverCreatesOutPortDataBuffersAfterProcessingNodeDefinition);
   SUITE_ADD_TEST(suite, test_serverDetectsOutPortDataFileAfterProcessingNodeDefinition);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_headerAcceptedEventIsTriggered(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   apx_connectionEventSpy_t spy;
   apx_serverTestConnection_create(&connection);
   apx_connectionEventSpy_create(&spy);

   apx_connectionEventSpy_register(&spy, &connection.base.base);
   CuAssertTrue(tc, !connection.base.isGreetingParsed);
   CuAssertIntEquals(tc, 0, spy.headerAcceptedCount);
   apx_serverConnectionBase_onRemoteFileHeaderReceived(&connection.base);
   CuAssertTrue(tc, connection.base.isGreetingParsed);
   apx_serverTestConnection_runEventLoop(&connection);
   CuAssertIntEquals(tc, 1, spy.headerAcceptedCount);

   apx_connectionEventSpy_destroy(&spy);
   apx_serverTestConnection_destroy(&connection);
}

static void test_createRemoteFileShallTriggerFileCreatedEvent(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   apx_connectionEventSpy_t spy;
   rmf_fileInfo_t fileInfo;
   apx_serverTestConnection_create(&connection);
   apx_connectionEventSpy_create(&spy);

   rmf_fileInfo_create(&fileInfo, "TestNode.apx", APX_ADDRESS_DEFINITION_START, 123, RMF_FILE_TYPE_FIXED);
   apx_connectionEventSpy_register(&spy, &connection.base.base);
   CuAssertIntEquals(tc, 0, spy.fileCreateCount);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo));
   apx_serverTestConnection_runEventLoop(&connection);
   CuAssertIntEquals(tc, 1, spy.fileCreateCount);
   CuAssertStrEquals(tc, "TestNode.apx", spy.lastFileInfo->name);
   CuAssertUIntEquals(tc, 123, spy.lastFileInfo->length);
   CuAssertUIntEquals(tc, APX_ADDRESS_DEFINITION_START | RMF_REMOTE_ADDRESS_BIT, spy.lastFileInfo->address);

   apx_serverTestConnection_destroy(&connection);
   apx_connectionEventSpy_destroy(&spy);
}

static void test_serverInitializesFileHandlerWhenDefinitionFileIsSeen(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   apx_connectionEventSpy_t spy;
   rmf_fileInfo_t fileInfo;
   apx_nodeInstance_t *nodeInstance = NULL;
   apx_file_t *definitionFile;
   apx_size_t definitionLen = strlen(m_apx_definition1);
   apx_serverTestConnection_create(&connection);
   apx_connectionEventSpy_create(&spy);

   rmf_fileInfo_create(&fileInfo, "TestNode.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_connectionEventSpy_register(&spy, &connection.base.base);
   apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(&connection);
   nodeInstance = apx_nodeManager_getLastAttached(&connection.base.base.nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance);
   definitionFile = apx_fileManager_findFileByAddress(&connection.base.base.fileManager, fileInfo.address | RMF_REMOTE_ADDRESS_BIT);
   CuAssertPtrNotNull(tc, definitionFile);
   CuAssertPtrEquals(tc, nodeInstance, definitionFile->notificationHandler.arg);

   apx_serverTestConnection_destroy(&connection);
   apx_connectionEventSpy_destroy(&spy);
}

static void test_nodeInstanceIsCreatedWhenDefinitionFileIsSeen(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   apx_connectionEventSpy_t spy;
   rmf_fileInfo_t fileInfo;
   apx_nodeInstance_t *nodeInstance = NULL;
   apx_nodeData_t *nodeData;
   apx_size_t definitionLen = strlen(m_apx_definition1);
   apx_serverTestConnection_create(&connection);
   apx_connectionEventSpy_create(&spy);

   rmf_fileInfo_create(&fileInfo, "TestNode.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_connectionEventSpy_register(&spy, &connection.base.base);
   apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(&connection);
   nodeInstance = apx_nodeManager_getLastAttached(&connection.base.base.nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance);
   nodeData = apx_nodeInstance_getNodeData(nodeInstance);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertUIntEquals(tc, definitionLen, apx_nodeData_getDefinitionDataLen(nodeData));
   apx_serverTestConnection_destroy(&connection);
   apx_connectionEventSpy_destroy(&spy);
}

static void test_serverInitializesNodeDataBufferWhenDefinitionFileIsSeen(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   apx_connectionEventSpy_t spy;
   rmf_fileInfo_t fileInfo;
   apx_nodeInstance_t *nodeInstance = NULL;
   apx_nodeData_t *nodeData;
   apx_size_t definitionLen = strlen(m_apx_definition1);
   apx_serverTestConnection_create(&connection);
   apx_connectionEventSpy_create(&spy);

   rmf_fileInfo_create(&fileInfo, "TestNode.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_connectionEventSpy_register(&spy, &connection.base.base);
   apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(&connection);
   nodeInstance = apx_nodeManager_getLastAttached(&connection.base.base.nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance);
   nodeData = apx_nodeInstance_getNodeData(nodeInstance);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertPtrNotNull(tc, apx_nodeData_getDefinitionDataBuf(nodeData));
   CuAssertUIntEquals(tc, strlen(m_apx_definition1), apx_nodeData_getDefinitionDataLen(nodeData));

   apx_serverTestConnection_destroy(&connection);
   apx_connectionEventSpy_destroy(&spy);
}

static void test_fileOpenRequestIsSentWhenDefinitionFileIsSeen(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   rmf_fileInfo_t fileInfo;
   rmf_cmdOpenFile_t fileOpenCmd;
   adt_bytearray_t *transmittedMsg;
   int32_t msgLen;
   adt_bytearray_t *expectedMsg;
   uint8_t *expectedData;

   apx_size_t definitionLen = strlen(m_apx_definition1);
   apx_serverTestConnection_create(&connection);

   rmf_fileInfo_create(&fileInfo, "TestNode.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);

   apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo);
   CuAssertIntEquals(tc, 0, apx_serverTestConnection_getTransmitLogLen(&connection));
   apx_serverTestConnection_runEventLoop(&connection);
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(&connection));
   transmittedMsg = apx_serverTestConnection_getTransmitLogMsg(&connection, 0);
   msgLen = RMF_HIGH_ADDRESS_SIZE+RMF_CMD_TYPE_LEN+UINT32_SIZE;
   CuAssertIntEquals(tc, msgLen, adt_bytearray_length(transmittedMsg));
   expectedMsg = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(expectedMsg, msgLen);
   expectedData = adt_bytearray_data(expectedMsg);
   rmf_packHeader(expectedData, RMF_HIGH_ADDRESS_SIZE, RMF_CMD_START_ADDR, false);
   fileOpenCmd.address = APX_ADDRESS_DEFINITION_START;
   rmf_serialize_cmdOpenFile(expectedData+RMF_HIGH_ADDRESS_SIZE, msgLen-RMF_HIGH_ADDRESS_SIZE, &fileOpenCmd);
   CuAssertTrue(tc, adt_bytearray_equals(transmittedMsg, expectedMsg));
   adt_bytearray_delete(expectedMsg);
   apx_serverTestConnection_destroy(&connection);
}

static void test_serverProcessesDefinitionAfterClientWritesDefinitionData(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *data;

   apx_size_t definitionLen = strlen(m_apx_definition1);
   apx_serverTestConnection_create(&connection);


   rmf_fileInfo_create(&fileInfo, "TestNode.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);

   apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(&connection);
   //File is now open (as verified by previous test case)

   //Client writes data
   data = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(data != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&data[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&data[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition1[0], definitionLen);
   apx_nodeInstance_t *nodeInstance = apx_nodeManager_find(&connection.base.base.nodeManager, "TestNode");
   CuAssertPtrNotNull(tc, nodeInstance);
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getNodeInfo(nodeInstance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(&connection, data, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getNodeInfo(nodeInstance));

   apx_serverTestConnection_destroy(&connection);
   free(data);
}

static void test_serverCreatesOutPortDataBuffersAfterProcessingNodeDefinition(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *data;

   apx_size_t definitionLen = strlen(m_apx_definition1);
   apx_serverTestConnection_create(&connection);
   rmf_fileInfo_create(&fileInfo, "TestNode.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(&connection);
   data = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(data != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&data[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&data[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition1[0], definitionLen);
   apx_nodeInstance_t *nodeInstance = apx_nodeManager_find(&connection.base.base.nodeManager, "TestNode");
   CuAssertPtrNotNull(tc, nodeInstance);
   apx_nodeData_t *nodeData = apx_nodeInstance_getNodeData(nodeInstance);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertUIntEquals(tc, 0u, apx_nodeData_getProvidePortDataLen(nodeData));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getNodeInfo(nodeInstance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(&connection, data, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   CuAssertUIntEquals(tc, UINT16_SIZE, apx_nodeData_getProvidePortDataLen(nodeData));
   apx_nodeInfo_t *nodeInfo = apx_nodeInstance_getNodeInfo(nodeInstance);
   CuAssertPtrNotNull(tc, nodeInfo);
   apx_bytePortMap_t *bytePortMap = apx_nodeInfo_getServerBytePortMap(nodeInfo);
   CuAssertPtrNotNull(tc, bytePortMap);
   CuAssertUIntEquals(tc, UINT16_SIZE, apx_bytePortMap_length(bytePortMap));

   apx_serverTestConnection_destroy(&connection);
   free(data);
}

static void test_serverDetectsOutPortDataFileAfterProcessingNodeDefinition(CuTest* tc)
{
   apx_serverTestConnection_t connection;
   rmf_fileInfo_t fileInfo;
   uint8_t *data;
   adt_bytearray_t *transmittedMsg;
   int32_t msgLen;
   rmf_cmdOpenFile_t fileOpenCmd;
   adt_bytearray_t *expectedMsg;
   uint8_t *expectedData;


   apx_size_t definitionLen = strlen(m_apx_definition1);
   apx_serverTestConnection_create(&connection);
   rmf_fileInfo_create(&fileInfo, "TestNode.apx", APX_ADDRESS_DEFINITION_START, definitionLen, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo);
   rmf_fileInfo_create(&fileInfo, "TestNode.out", 0u, UINT16_SIZE, RMF_FILE_TYPE_FIXED);
   apx_serverTestConnection_onFileInfoMsgReceived(&connection, &fileInfo);
   apx_serverTestConnection_runEventLoop(&connection);
   data = (uint8_t*) malloc(RMF_HIGH_ADDRESS_SIZE+definitionLen);
   assert(data != 0);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(&data[0], RMF_HIGH_ADDRESS_SIZE, APX_ADDRESS_DEFINITION_START, false));
   memcpy(&data[RMF_HIGH_ADDRESS_SIZE], &m_apx_definition1[0], definitionLen);
   apx_nodeInstance_t *nodeInstance = apx_nodeManager_find(&connection.base.base.nodeManager, "TestNode");
   CuAssertPtrNotNull(tc, nodeInstance);
   apx_nodeData_t *nodeData = apx_nodeInstance_getNodeData(nodeInstance);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertUIntEquals(tc, 0u, apx_nodeData_getProvidePortDataLen(nodeData));
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getNodeInfo(nodeInstance));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_serverTestConnection_onSerializedMsgReceived(&connection, data, RMF_HIGH_ADDRESS_SIZE+definitionLen));
   CuAssertIntEquals(tc, 1, apx_serverTestConnection_getTransmitLogLen(&connection));
   apx_serverTestConnection_runEventLoop(&connection);
   CuAssertIntEquals(tc, 2, apx_serverTestConnection_getTransmitLogLen(&connection));
   transmittedMsg = apx_serverTestConnection_getTransmitLogMsg(&connection, 1);
   msgLen = RMF_HIGH_ADDRESS_SIZE+RMF_CMD_TYPE_LEN+UINT32_SIZE;
   CuAssertIntEquals(tc, msgLen, adt_bytearray_length(transmittedMsg));
   expectedMsg = adt_bytearray_new(ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   adt_bytearray_resize(expectedMsg, msgLen);
   expectedData = adt_bytearray_data(expectedMsg);
   rmf_packHeader(expectedData, RMF_HIGH_ADDRESS_SIZE, RMF_CMD_START_ADDR, false);
   fileOpenCmd.address = 0u;
   rmf_serialize_cmdOpenFile(expectedData+RMF_HIGH_ADDRESS_SIZE, msgLen-RMF_HIGH_ADDRESS_SIZE, &fileOpenCmd);
   CuAssertTrue(tc, adt_bytearray_equals(transmittedMsg, expectedMsg));

   adt_bytearray_delete(expectedMsg);
   apx_serverTestConnection_destroy(&connection);
   free(data);
}
