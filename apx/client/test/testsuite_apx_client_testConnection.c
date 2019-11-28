/*****************************************************************************
* \file      testsuite_apx_client.c
* \author    Conny Gustafsson
* \date      2018-08-08
* \brief     Description
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#include "apx_clientTestConnection.h"
#include "apx_clientEventListenerSpy.h"
#include "CuTest.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char *m_apx_definition1 = "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"VehicleSpeed\"S:=65535\n"
      "\n";

static const char *m_apx_definition2 = "APX/1.2\n"
      "N\"TestNode2\"\n"
      "P\"EngineSpeed\"S:=65535\n"
      "\n";

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_connectAndDisconnectEvents(CuTest* tc);
static void test_transmitHandlerInitializedOnConnectionAttach(CuTest* tc);
static void test_localNodesShallBeAttachedToConnectionNodeManager(CuTest* tc);
static void test_localNodesShallBeAttachedToConnectionFileManager(CuTest* tc);
static void test_headerSentOnConnect(CuTest* tc);
static void test_headerAcceptedEventTriggered(CuTest* tc);
static void test_localFileInfoShallBeSentAfterHeaderAcknowledge(CuTest* tc);
static void test_definitionFileIsSentWhenServerSendsFileOpenRequest(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_client_testConnection(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_connectAndDisconnectEvents);
   SUITE_ADD_TEST(suite, test_transmitHandlerInitializedOnConnectionAttach);
   SUITE_ADD_TEST(suite, test_localNodesShallBeAttachedToConnectionNodeManager);
   SUITE_ADD_TEST(suite, test_localNodesShallBeAttachedToConnectionFileManager);
   SUITE_ADD_TEST(suite, test_headerSentOnConnect);
   SUITE_ADD_TEST(suite, test_headerAcceptedEventTriggered);
   SUITE_ADD_TEST(suite, test_localFileInfoShallBeSentAfterHeaderAcknowledge);
   //SUITE_ADD_TEST(suite, test_definitionFileIsSentWhenServerSendsFileOpenRequest);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////



static void test_connectAndDisconnectEvents(CuTest* tc)
{
   apx_clientTestConnection_t *connection;
   apx_client_t *client;
   apx_clientEventListenerSpy_t *spy = apx_clientEventListenerSpy_new();
   client = apx_client_new();
   CuAssertPtrNotNull(tc, client);
   apx_clientEventListenerSpy_register(spy, client);
   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_client_attachConnection(client, (apx_clientConnectionBase_t*) connection);

   apx_clientTestConnection_connect(connection);
   CuAssertIntEquals(tc, 0, apx_clientEventListenerSpy_getConnectCount(spy));
   apx_client_run(client);
   CuAssertIntEquals(tc, 1, apx_clientEventListenerSpy_getConnectCount(spy));

   apx_clientTestConnection_disconnect(connection);
   CuAssertIntEquals(tc, 0, apx_clientEventListenerSpy_getDisconnectCount(spy));
   apx_client_run(client);
   CuAssertIntEquals(tc, 1, apx_clientEventListenerSpy_getDisconnectCount(spy));

   apx_client_delete(client);
   apx_clientEventListenerSpy_delete(spy);
}

static void test_transmitHandlerInitializedOnConnectionAttach(CuTest* tc)
{
   apx_clientTestConnection_t *connection;
   apx_client_t *client;
   apx_transmitHandler_t transmitHandler;

   client = apx_client_new();
   CuAssertPtrNotNull(tc, client);

   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);

   apx_client_attachConnection(client, (apx_clientConnectionBase_t*) connection);
   apx_fileManager2_t *fileManager = apx_client_getFileManager(client);
   CuAssertPtrNotNull(tc, fileManager);
   apx_fileManagerWorker_copyTransmitHandler(&fileManager->worker, &transmitHandler);
   CuAssertPtrEquals(tc, connection, transmitHandler.arg);

   apx_client_delete(client);
}

static void test_localNodesShallBeAttachedToConnectionNodeManager(CuTest* tc)
{
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   apx_clientTestConnection_t *connection = apx_clientTestConnection_new();
   apx_client_attachConnection(client, &connection->base);
   apx_nodeManager_t *nodeManager = apx_client_getNodeManager(client);
   CuAssertIntEquals(tc, 2, apx_nodeManager_length(nodeManager));
   apx_client_delete(client);
}

static void test_localNodesShallBeAttachedToConnectionFileManager(CuTest* tc)
{
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   apx_clientTestConnection_t *connection = apx_clientTestConnection_new();
   apx_client_attachConnection(client, &connection->base);
   apx_fileManager2_t *fileManager = apx_client_getFileManager(client);
   CuAssertIntEquals(tc, 4, apx_fileManager2_getNumLocalFiles(fileManager));
   apx_client_delete(client);
}

static void test_headerSentOnConnect(CuTest* tc)
{
   apx_clientTestConnection_t *connection;
   apx_client_t *client;
   const char *expectedGreeting = "RMFP/1.0\nNumHeader-Format:32\n\n";
   adt_bytearray_t *expectedMsg = adt_bytearray_make((const uint8_t*) expectedGreeting, strlen(expectedGreeting), 0u);
   client = apx_client_new();

   CuAssertPtrNotNull(tc, client);
   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_client_attachConnection(client, (apx_clientConnectionBase_t*) connection);
   CuAssertIntEquals(tc, 0, apx_clientTestConnection_getTransmitLogLen(connection));
   apx_clientTestConnection_connect(connection);
   CuAssertIntEquals(tc, 1, apx_clientTestConnection_getTransmitLogLen(connection));
   adt_bytearray_t *transmittedMsg = apx_clientTestConnection_getTransmitLogMsg(connection, 0);
   CuAssertTrue(tc, adt_bytearray_equals(transmittedMsg, expectedMsg));
   adt_bytearray_delete(expectedMsg);

   apx_client_delete(client);
}

static void test_headerAcceptedEventTriggered(CuTest* tc)
{
   apx_clientTestConnection_t *connection;
   apx_client_t *client;
   apx_clientEventListenerSpy_t *spy = apx_clientEventListenerSpy_new();
   client = apx_client_new();
   CuAssertPtrNotNull(tc, client);
   apx_clientEventListenerSpy_register(spy, client);
   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_client_attachConnection(client, (apx_clientConnectionBase_t*) connection);

   apx_clientTestConnection_connect(connection);
   CuAssertIntEquals(tc, 0, apx_clientEventListenerSpy_getHeaderAccepted(spy));
   apx_clientTestConnection_headerAccepted(connection);
   apx_client_run(client);
   CuAssertIntEquals(tc, 1, apx_clientEventListenerSpy_getHeaderAccepted(spy));

   apx_client_delete(client);
   apx_clientEventListenerSpy_delete(spy);
}

static void test_localFileInfoShallBeSentAfterHeaderAcknowledge(CuTest* tc)
{
   apx_clientTestConnection_t *connection;
   apx_client_t *client;
   adt_bytearray_t *msg;
   const uint8_t *data;
   rmf_fileInfo_t fileInfo;
   int32_t dataLen;

   client = apx_client_new();
   CuAssertPtrNotNull(tc, client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition2));
   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_client_attachConnection(client, (apx_clientConnectionBase_t*) connection);

   apx_clientTestConnection_connect(connection);

   CuAssertIntEquals(tc, 1, apx_clientTestConnection_getTransmitLogLen(connection));
   apx_clientTestConnection_clearTransmitLog(connection);
   apx_clientTestConnection_headerAccepted(connection);
   apx_client_run(client);
   CuAssertIntEquals(tc, 4, apx_clientTestConnection_getTransmitLogLen(connection));

   msg = apx_clientTestConnection_getTransmitLogMsg(connection, 0);
   dataLen = RMF_CMD_FILE_INFO_BASE_SIZE+ ((int) strlen("TestNode1.out")) + 1;
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE + dataLen, adt_bytearray_length(msg));
   data = adt_bytearray_data(msg);
   CuAssertIntEquals(tc, dataLen, rmf_deserialize_cmdFileInfo(data + RMF_HIGH_ADDRESS_SIZE, dataLen, &fileInfo));
   CuAssertUIntEquals(tc, 0, fileInfo.address);
   CuAssertStrEquals(tc, "TestNode1.out", &fileInfo.name[0]);

   msg = apx_clientTestConnection_getTransmitLogMsg(connection, 1);
   dataLen = RMF_CMD_FILE_INFO_BASE_SIZE+ ((int) strlen("TestNode2.out")) + 1;
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE + dataLen, adt_bytearray_length(msg));
   data = adt_bytearray_data(msg);
   CuAssertIntEquals(tc, dataLen, rmf_deserialize_cmdFileInfo(data + RMF_HIGH_ADDRESS_SIZE, dataLen, &fileInfo));
   CuAssertUIntEquals(tc, APX_ADDRESS_PORT_DATA_BOUNDARY, fileInfo.address);
   CuAssertStrEquals(tc, "TestNode2.out", &fileInfo.name[0]);

   msg = apx_clientTestConnection_getTransmitLogMsg(connection, 2);
   dataLen = RMF_CMD_FILE_INFO_BASE_SIZE+ ((int) strlen("TestNode1.apx")) + 1;
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE + dataLen, adt_bytearray_length(msg));
   data = adt_bytearray_data(msg);
   CuAssertIntEquals(tc, dataLen, rmf_deserialize_cmdFileInfo(data + RMF_HIGH_ADDRESS_SIZE, dataLen, &fileInfo));
   CuAssertUIntEquals(tc, APX_ADDRESS_DEFINITION_START, fileInfo.address);
   CuAssertStrEquals(tc, "TestNode1.apx", &fileInfo.name[0]);

   msg = apx_clientTestConnection_getTransmitLogMsg(connection, 3);
   dataLen = RMF_CMD_FILE_INFO_BASE_SIZE+ ((int) strlen("TestNode2.apx")) + 1;
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE + dataLen, adt_bytearray_length(msg));
   data = adt_bytearray_data(msg);
   CuAssertIntEquals(tc, dataLen, rmf_deserialize_cmdFileInfo(data + RMF_HIGH_ADDRESS_SIZE, dataLen, &fileInfo));
   CuAssertUIntEquals(tc, APX_ADDRESS_DEFINITION_START + APX_ADDRESS_DEFINITION_BOUNDARY, fileInfo.address);
   CuAssertStrEquals(tc, "TestNode2.apx", &fileInfo.name[0]);

   apx_client_delete(client);
}

static void test_definitionFileIsSentWhenServerSendsFileOpenRequest(CuTest* tc)
{
   apx_clientTestConnection_t *connection;
   apx_client_t *client;
   adt_bytearray_t *msg;
   const uint8_t *data;
   rmf_cmdOpenFile_t fileOpenCmd;
   int32_t dataLen;

   client = apx_client_new();
   CuAssertPtrNotNull(tc, client);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_buildNode_cstr(client, m_apx_definition1));
   connection = apx_clientTestConnection_new();
   CuAssertPtrNotNull(tc, connection);
   apx_client_attachConnection(client, (apx_clientConnectionBase_t*) connection);

   apx_clientTestConnection_connect(connection);
   //We know the definition file is located at address defined by APX_ADDRESS_DEFINITION_START. Let's just open the file from server side.

   fileOpenCmd.address = 0u;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_clientTestConnection_onFileOpenMsgReceived(connection, &fileOpenCmd));

   apx_client_delete(client);
}
