//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_fileManager.h"
#include "apx_eventListener.h"
#include "apx_file.h"
#include "apx_transmitHandlerSpy.h"
#include "apx_connectionEventSpy.h"
#include "apx_nodeData.h"
#include "apx_serverTestConnection.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_fileManager_attachLocalFiles(CuTest* tc);
static void test_apx_fileManager_createRemoteFile(CuTest* tc);
//static void test_apx_fileManager_openRemoteFile_sendMessage(CuTest* tc);
static void test_apx_fileManager_openRemoteFile_setOpenFlag(CuTest* tc);
static void test_apx_fileManager_openRemoteFile_processRequest_fixedFileNoReadHandler(CuTest* tc);
static void test_apx_fileManager_openRemoteFile_processRequest_apxDefinitionFile(CuTest* tc);

//helper functions
static void attachSpyAsTransmitHandler(apx_fileManager_t *manager, apx_transmitHandlerSpy_t *spy);
static void attachApxClientFiles(CuTest* tc, apx_fileManager_t *manager, uint32_t definitionFileAddress);
static void receiveFileOpenRequest(CuTest *tc, apx_fileManager_t *manager, uint32_t fileAddress);
static void verifyInvalidReadHandler(CuTest *tc, apx_fileManager_t *manager, apx_transmitHandlerSpy_t *spy, uint32_t fileAddress);
static apx_error_t readDefinitionData(void* arg, apx_file_t *file, uint32_t offset, uint8_t *dest, uint32_t len);
static void verifyWriteDefinitionFile(CuTest *tc, apx_fileManager_t *manager, apx_transmitHandlerSpy_t *spy, uint32_t fileAddress);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const char *m_TestNodeDefinition = "APX/1.2\n"
      "N\"TestNode\"\n"
      "T\"VehicleSpeed_T\"S\n"
      "R\"VehicleSpeed\"T[0]:=65535\n";

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_fileManager(void)
{
   CuSuite* suite = CuSuiteNew();

//   SUITE_ADD_TEST(suite, test_apx_fileManager_attachLocalFiles);
//   SUITE_ADD_TEST(suite, test_apx_fileManager_createRemoteFile);
//   SUITE_ADD_TEST(suite, test_apx_fileManager_openRemoteFile_sendMessage);
//   SUITE_ADD_TEST(suite, test_apx_fileManager_openRemoteFile_setOpenFlag);
//   SUITE_ADD_TEST(suite, test_apx_fileManager_openRemoteFile_processRequest_fixedFileNoReadHandler);
//   SUITE_ADD_TEST(suite, test_apx_fileManager_openRemoteFile_processRequest_apxDefinitionFile);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/*
static void test_apx_fileManager_attachLocalFiles(CuTest* tc)
{
   apx_nodeData_t *nodeData;
   apx_file2_t *definitionFile;
   apx_file2_t *outDataFile;
   apx_connectionEventSpy_t spy;
   apx_serverTestConnection_t connection;
   apx_fileManager_t *manager;

   apx_serverTestConnection_create(&connection, NULL);
   apx_connectionEventSpy_create(&spy);
   manager = &connection.base.base.fileManager;

   ApxNode_Init_TestNode1();
   nodeData = ApxNode_GetNodeData_TestNode1();
   definitionFile = apx_nodeData_newLocalDefinitionFile(nodeData);
   outDataFile = apx_nodeData_newLocalOutPortDataFile(nodeData);
   CuAssertPtrNotNull(tc, definitionFile);
   CuAssertPtrNotNull(tc, outDataFile);
   //CuAssertIntEquals(tc, 0, spy.numfileCreateCalls);
   CuAssertIntEquals(tc, 0, apx_fileManager_getNumLocalFiles(manager));
   apx_fileManager_attachLocalFile(manager, definitionFile, NULL);
   apx_fileManager_attachLocalFile(manager, outDataFile, NULL);
   apx_serverTestConnection_runEventLoop(&connection);
   //CuAssertIntEquals(tc, 2, spy.numfileCreateCalls);
   //CuAssertIntEquals(tc, 2, apx_fileManager_getNumLocalFiles(manager));
   //CuAssertTrue(tc, !spy.lastFile->isRemoteFile);
   //CuAssertTrue(tc, !spy.lastFile->isOpen);
   apx_serverTestConnection_destroy(&connection);
   apx_connectionEventSpy_destroy(&spy);
}
*/

/*
static void test_apx_fileManager_createRemoteFile(CuTest* tc)
{
   uint8_t buffer[100];
   rmf_fileInfo_t info;
   int32_t msgLen;
   apx_connectionEventSpy_t spy;
   apx_fileManager_t *manager;
   apx_serverTestConnection_t connection;

   apx_serverTestConnection_create(&connection);
   apx_connectionEventSpy_create(&spy);
   apx_connectionEventSpy_register(&spy, &connection.base.base);
   manager = &connection.base.base.fileManager;
   apx_serverTestConnection_runEventLoop(&connection);
   rmf_fileInfo_create(&info, "test.apx", APX_ADDRESS_DEFINITION_START, 100, RMF_FILE_TYPE_FIXED);
   msgLen = rmf_packHeader(&buffer[0], sizeof(buffer), RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdFileInfo(&buffer[msgLen], sizeof(buffer)-msgLen, &info);
   CuAssertIntEquals(tc, msgLen, apx_fileManager_processMessage(manager, &buffer[0], msgLen));
   apx_serverTestConnection_runEventLoop(&connection);
   CuAssertIntEquals(tc, 1, spy.fileCreateCount);
   apx_file2_t *file = apx_fileManager_findFileByAddress(manager, spy.lastFileInfo->address);
   CuAssertPtrNotNull(tc, file);
   CuAssertTrue(tc, apx_file2_isRemoteFile(file));
   CuAssertTrue(tc, apx_file2_isOpen(file));
   apx_file2_unlock(file);
   apx_serverTestConnection_destroy(&connection);
   apx_connectionEventSpy_destroy(&spy);
}
*/
/*

static void test_apx_fileManager_openRemoteFile_sendMessage(CuTest* tc)
{
   apx_fileManager_t *manager;
   apx_serverTestConnection_t connection;

   uint8_t buffer[100];
   rmf_fileInfo_t info;
   apx_file2_t *file;
   int32_t msgLen;
   apx_transmitHandlerSpy_t spy;
   apx_transmitHandler_t handler;
   adt_bytearray_t *array;
   const uint8_t *data;
   rmf_msg_t msg;
   rmf_cmdOpenFile_t cmd;

   memset(&handler, 0, sizeof(handler));
   handler.getSendBuffer = apx_transmitHandlerSpy_getSendBuffer;
   handler.send = apx_transmitHandlerSpy_send;
   handler.arg = &spy;
   apx_transmitHandlerSpy_create(&spy);
   apx_serverTestConnection_create(&connection, NULL);
   manager = &connection.base.base.fileManager;
   apx_fileManager_setTransmitHandler(manager, &handler);
   rmf_fileInfo_create(&info, "test.apx", 0x10000, 100, RMF_FILE_TYPE_FIXED);
   msgLen = rmf_packHeader(&buffer[0], sizeof(buffer), RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdFileInfo(&buffer[msgLen], sizeof(buffer)-msgLen, &info);

   CuAssertIntEquals(tc, msgLen, apx_fileManager_processMessage(manager, &buffer[0], msgLen));

   file = apx_fileMap_findByName(&manager->remote.remoteFileMap, "test.apx");
   CuAssertPtrNotNull(tc, file);
   CuAssertIntEquals(tc, 0, apx_transmitHandlerSpy_length(&spy));
   CuAssertIntEquals(tc, 0, apx_fileManager_openRemoteFile(manager, file->fileInfo.address, (void*) 0));
   CuAssertIntEquals(tc, 1, apx_fileManager_numPendingMessages(manager));
   apx_fileManager_run(manager);
   CuAssertIntEquals(tc, 1, apx_transmitHandlerSpy_length(&spy));
   array = apx_transmitHandlerSpy_next(&spy);
   CuAssertPtrNotNull(tc, array);
   data = adt_bytearray_data(array);
   CuAssertIntEquals(tc, RMF_CMD_TYPE_LEN+RMF_CMD_FILE_OPEN_LEN, adt_bytearray_length(array));
   rmf_unpackMsg(data, adt_bytearray_length(array), &msg);
   CuAssertUIntEquals(tc, RMF_CMD_START_ADDR, msg.address);
   CuAssertIntEquals(tc, 8, rmf_deserialize_cmdOpenFile(msg.data, msg.dataLen, &cmd));
   CuAssertUIntEquals(tc, info.address, cmd.address);

   adt_bytearray_delete(array);
   apx_serverTestConnection_destroy(&connection);
   apx_transmitHandlerSpy_destroy(&spy);
}
*/
/*
static void test_apx_fileManager_openRemoteFile_setOpenFlag(CuTest* tc)
{
   apx_fileManager_t *manager;
   apx_serverTestConnection_t connection;

   uint8_t buffer[100];
   rmf_fileInfo_t info;
   int32_t msgLen;
   apx_file2_t *file;

   apx_serverTestConnection_create(&connection, NULL);
   manager = &connection.base.base.fileManager;

   rmf_fileInfo_create(&info, "test.apx", 0x10000, 100, RMF_FILE_TYPE_FIXED);
   msgLen = rmf_packHeader(&buffer[0], sizeof(buffer), RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdFileInfo(&buffer[msgLen], sizeof(buffer)-msgLen, &info);
   CuAssertIntEquals(tc, msgLen, apx_fileManager_processMessage(manager, &buffer[0], msgLen));

   file = apx_fileManager_findRemoteFileByName(manager, "test.apx");
   CuAssertTrue(tc, file->isFileOpen == false);
   CuAssertIntEquals(tc, 0, apx_fileManager_requestOpenRemoteFile(manager, file->fileInfo.address, (void*) 0));
   CuAssertTrue(tc, file->isFileOpen == true);

   apx_serverTestConnection_destroy(&connection);
}

static void test_apx_fileManager_openRemoteFile_processRequest_fixedFileNoReadHandler(CuTest* tc)
{

   apx_fileManager_t *manager;
   apx_serverTestConnection_t connection;

   apx_transmitHandlerSpy_t spy;
   const uint32_t fileAddress = 0x10000;

   apx_transmitHandlerSpy_create(&spy);
   apx_serverTestConnection_create(&connection, NULL);
   manager = &connection.base.base.fileManager;

   attachSpyAsTransmitHandler(manager, &spy);
   attachApxClientFiles(tc, manager, fileAddress);
   receiveFileOpenRequest(tc, manager, fileAddress);
   CuAssertIntEquals(tc, 1, apx_fileManager_numPendingMessages(manager));
   CuAssertIntEquals(tc, 0, apx_transmitHandlerSpy_length(&spy));
   apx_fileManager_run(manager);
   verifyInvalidReadHandler(tc, manager, &spy, fileAddress);

   apx_serverTestConnection_destroy(&connection);
   apx_transmitHandlerSpy_destroy(&spy);

}

static void test_apx_fileManager_openRemoteFile_processRequest_apxDefinitionFile(CuTest* tc)
{
   apx_fileManager_t *manager;
   apx_serverTestConnection_t connection;

   apx_transmitHandlerSpy_t spy;

   apx_file_handler_t fileHandler;
   const uint32_t fileAddress = 0x10000;
   apx_file2_t *definitionFile;

   apx_transmitHandlerSpy_create(&spy);

   apx_serverTestConnection_create(&connection, NULL);
   manager = &connection.base.base.fileManager;

   attachSpyAsTransmitHandler(manager, &spy);


   attachApxClientFiles(tc, manager, fileAddress);

   definitionFile = apx_fileManager_findLocalFileByName(manager, "TestNode.apx");
   CuAssertPtrNotNull(tc, definitionFile);

   memset(&fileHandler, 0, sizeof(fileHandler));

   fileHandler.read = readDefinitionData;
   apx_file2_setHandler(definitionFile, &fileHandler);

   receiveFileOpenRequest(tc, manager, fileAddress);
   CuAssertIntEquals(tc, 1, apx_fileManager_numPendingMessages(manager));
   CuAssertIntEquals(tc, 0, apx_transmitHandlerSpy_length(&spy));
   apx_fileManager_run(manager);

   verifyWriteDefinitionFile(tc, manager, &spy, fileAddress);

   apx_serverTestConnection_destroy(&connection);
   apx_transmitHandlerSpy_destroy(&spy);

}

//helper functions

static void attachSpyAsTransmitHandler(apx_fileManager_t *manager, apx_transmitHandlerSpy_t *spy)
{
   apx_transmitHandler_t handler;
   memset(&handler, 0, sizeof(handler));
   handler.getSendBuffer = apx_transmitHandlerSpy_getSendBuffer;
   handler.send = apx_transmitHandlerSpy_send;
   handler.arg = spy;
   apx_fileManager_setTransmitHandler(manager, &handler);
}

static void attachApxClientFiles(CuTest* tc, apx_fileManager_t *manager, uint32_t definitionFileAddress)
{
   rmf_fileInfo_t info;
   apx_file2_t *localFile;
   uint32_t len = (uint32_t)strlen(m_TestNodeDefinition);

   rmf_fileInfo_create(&info, "TestNode.apx", definitionFileAddress, len, RMF_FILE_TYPE_FIXED);
   localFile = apx_file2_newLocal(&info, NULL);
   apx_fileManager_attachLocalFile(manager, localFile, NULL);
}

static void receiveFileOpenRequest(CuTest *tc, apx_fileManager_t *manager, uint32_t fileAddress)
{
   uint8_t requestBuffer[100];
   int32_t msgLen;
   rmf_cmdOpenFile_t cmd;

   msgLen = rmf_packHeader(&requestBuffer[0], sizeof(requestBuffer), RMF_CMD_START_ADDR, false);
   cmd.address = fileAddress;
   msgLen += rmf_serialize_cmdOpenFile(&requestBuffer[msgLen], sizeof(requestBuffer)-msgLen, &cmd);
   CuAssertIntEquals(tc, msgLen, apx_fileManager_processMessage(manager, &requestBuffer[0], msgLen));
}

static void verifyInvalidReadHandler(CuTest *tc, apx_fileManager_t *manager, apx_transmitHandlerSpy_t *spy, uint32_t fileAddress)
{
   adt_bytearray_t *array;
   int32_t msgLen;
   const uint8_t *data;
   rmf_msg_t msg;
   uint32_t unpackedAddress;

   CuAssertTrue(tc, apx_transmitHandlerSpy_length(spy) > 0);
   array = apx_transmitHandlerSpy_next(spy);
   CuAssertPtrNotNull(tc, array);
   msgLen = RMF_CMD_HEADER_LEN+RMF_ERROR_INVALID_READ_HANDLER_LEN;
   CuAssertIntEquals(tc, msgLen, adt_bytearray_length(array));
   data = adt_bytearray_data(array);
   rmf_unpackMsg(data, msgLen, &msg);
   CuAssertUIntEquals(tc, RMF_CMD_START_ADDR, msg.address);
   rmf_deserialize_errorInvalidReadHandler(msg.data, msg.dataLen, &unpackedAddress);
   CuAssertUIntEquals(tc, fileAddress, unpackedAddress);
   adt_bytearray_delete(array);
}

static apx_error_t readDefinitionData(void* arg, apx_file2_t *file, uint32_t offset, uint8_t *dest, uint32_t len)
{
   if ( (strcmp(file->fileInfo.name, "TestNode.apx") == 0) && (offset == 0) )
   {
      memcpy(dest, &m_TestNodeDefinition[0], len);
      return 0;
   }
   return -1;
}

static void verifyWriteDefinitionFile(CuTest *tc, apx_fileManager_t *manager, apx_transmitHandlerSpy_t *spy, uint32_t fileAddress)
{
   adt_bytearray_t *array;
   int32_t msgLen;
   const uint8_t *data;
   rmf_msg_t msg;
   int32_t i;
   uint32_t dataLen = (uint32_t)strlen(m_TestNodeDefinition);

   CuAssertTrue(tc, apx_transmitHandlerSpy_length(spy) > 0);
   array = apx_transmitHandlerSpy_next(spy);
   CuAssertPtrNotNull(tc, array);
   msgLen = RMF_HIGH_ADDRESS_SIZE+dataLen;
   CuAssertIntEquals(tc, msgLen, adt_bytearray_length(array));
   data = adt_bytearray_data(array);
   rmf_unpackMsg(data, msgLen, &msg);
   CuAssertUIntEquals(tc, fileAddress, msg.address);
   for(i=0;i< msg.dataLen; i++)
   {
      char errorMsg[14];
      sprintf(errorMsg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, errorMsg, m_TestNodeDefinition[i], msg.data[i]);
   }
   adt_bytearray_delete(array);

}
*/
