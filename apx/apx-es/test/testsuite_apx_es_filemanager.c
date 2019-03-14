//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "rmf.h"
#include "apx_msg.h"
#include "apx_es_filemanager.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_FILE_MANAGER_MAX_NUM_MESSAGES 1000
#define RECEIVE_BUFFER_LEN 4096
#define APX_FILE_MANAGER_MSG_QUEUE_SIZE (sizeof(apx_msg_t)*APX_FILE_MANAGER_MAX_NUM_MESSAGES)
#define NUMHEADER16_MAX_LEN 2u
#define SEND_BUFFER_MAX 1024
#define APPLICATION_DATA_MAX 8
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_es_filemanager_create(CuTest* tc);
static void apx_es_filemanager_write_notify(CuTest* tc);
static void apx_es_filemanager_serialize_all_commands(CuTest* tc);
static void apx_es_filemanager_request_files(CuTest* tc);
static void apx_es_filemanager_enter_pending_mode_when_buffer_is_full(CuTest* tc);
static int32_t TestStub_getSendAvail(void *arg);
static uint8_t* TestStub_getSendBuffer(void *arg, int32_t msgLen);
static int32_t TestStub_send(void *arg, int32_t offset, int32_t msgLen);
static void TestHelper_resetAndConnectTransmitHandler(apx_es_fileManager_t* fileManager);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
static int32_t m_test_send_avail;
static uint8_t m_test_send_buffer[SEND_BUFFER_MAX];
static uint8_t m_application_data[APPLICATION_DATA_MAX];
static int32_t m_test_data_written;
static int32_t m_test_data_offset;
static apx_transmitHandler_t m_transmitHandler;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_apx_es_filemanager(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, apx_es_filemanager_create);
   SUITE_ADD_TEST(suite, apx_es_filemanager_write_notify);
   SUITE_ADD_TEST(suite, apx_es_filemanager_serialize_all_commands);
   SUITE_ADD_TEST(suite, apx_es_filemanager_request_files);
   SUITE_ADD_TEST(suite, apx_es_filemanager_enter_pending_mode_when_buffer_is_full);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_es_filemanager_create(CuTest* tc)
{
   int8_t rc;
   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];
   uint8_t receiveBuffer[RECEIVE_BUFFER_LEN];
   rc = apx_es_fileManager_create(&fileManager,
                                  messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES,
                                  receiveBuffer, RECEIVE_BUFFER_LEN);
   CuAssertIntEquals(tc, 0, rc);
   CuAssertTrue(tc, !fileManager.isConnected);
   CuAssertTrue(tc, !fileManager.dropMessage);
   CuAssertTrue(tc, !fileManager.pendingWrite);

   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.pendingMsg.msgType);

   CuAssertUIntEquals(tc, 0, fileManager.transmitBufLen);

   CuAssertUIntEquals(tc, 0, fileManager.numRequestedFiles);
   CuAssertUIntEquals(tc, 0, apx_es_fileMap_length(&fileManager.localFileMap));
   CuAssertUIntEquals(tc, 0, apx_es_fileMap_length(&fileManager.remoteFileMap));

   CuAssertUIntEquals(tc, 0, fileManager.receiveBufOffset);
   CuAssertUIntEquals(tc, RECEIVE_BUFFER_LEN, fileManager.receiveBufLen);
}

static void apx_es_filemanager_write_notify(CuTest* tc)
{
#define FILE_WRITE_NOTIFY_SIZE 16
   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];
   uint8_t receiveBuffer[RECEIVE_BUFFER_LEN];
   apx_file_t file;
   apx_nodeData_t node;
   uint8_t data[FILE_WRITE_NOTIFY_SIZE];
   apx_msg_t topOfQueue;
   apx_msg_t prevQueued;
   uint8_t expected_write_notify_size1 = 0;
   uint8_t expected_write_notify_size2 = 0;
   uint8_t expected_write_notify_size3 = 0;
   uint8_t expected_write_notify_size4 = 0;
   const uint8_t one_byte_write = 1u;
   apx_es_fileManager_create(&fileManager,
                             messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES,
                             receiveBuffer, RECEIVE_BUFFER_LEN);

   memset(data, 0, FILE_WRITE_NOTIFY_SIZE);

   TestHelper_resetAndConnectTransmitHandler(&fileManager);

   apx_nodeData_create(&node,"node",NULL,0,NULL,0,0,&data[0],NULL,FILE_WRITE_NOTIFY_SIZE);
   apx_file_createLocalFile(&file, APX_OUTDATA_FILE, &node);
   apx_nodeData_setFileManager(&node, &fileManager);
   apx_nodeData_setOutPortDataFile(&node, &file);
   CuAssertTrue(tc, !file.isOpen);
   CuAssertTrue(tc, !file.isRemoteFile);
   CuAssertTrue(tc, !node.isRemote);
   CuAssertUIntEquals(tc, FILE_WRITE_NOTIFY_SIZE, node.outPortDataLen);

   apx_es_fileManager_attachLocalFile(&fileManager, &file);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   apx_es_fileManager_onConnected(&fileManager);
   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   rbfs_remove(&fileManager.messageQueue, &topOfQueue);
   CuAssertUIntEquals(tc, RMF_MSG_FILEINFO, topOfQueue.msgType);

   CuAssertTrue(tc, fileManager.isConnected);
   CuAssertTrue(tc, !file.isOpen);
   CuAssertTrue(tc, !file.isRemoteFile);

   // Before file is marked open ignore writes
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   apx_nodeData_outPortDataNotify(&node, 0, 1);
   apx_es_fileManager_onFileUpdate(&fileManager, data, 0, 1);
   expected_write_notify_size1 = 1 + RMF_LOW_ADDRESS_SIZE;
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));

   // First write shall be put aside for update if file is open
   file.isOpen = true;
   apx_nodeData_outPortDataNotify(&node, 0, one_byte_write);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_MSG_WRITE_NOTIFY, fileManager.queuedWriteNotify.msgType);

   // Next shall put queuedWiriteNotify on the message queue unless write aligns
   prevQueued = fileManager.queuedWriteNotify;
   apx_nodeData_outPortDataNotify(&node, 2, one_byte_write);

   rbfs_peek(&fileManager.messageQueue, &topOfQueue);
   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   CuAssertIntEquals(tc, 0, memcmp(&topOfQueue, &prevQueued, sizeof(apx_msg_t)));
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);

   // Test write align does not trigger any message to be appended
   apx_nodeData_outPortDataNotify(&node, 3, one_byte_write);

   rbfs_peek(&fileManager.messageQueue, &topOfQueue);
   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   CuAssertIntEquals(tc, 0, memcmp(&topOfQueue, &prevQueued, sizeof(apx_msg_t)));
   CuAssertUIntEquals(tc, one_byte_write + one_byte_write, fileManager.queuedWriteNotify.msgData2);
   expected_write_notify_size2 = 2 + RMF_LOW_ADDRESS_SIZE;

   // With APX_ES_FILEMANAGER_OPTIMIZE_WRITE_NOTIFICATIONS enabled write to the same location twice shall not trigger queued item
   apx_nodeData_outPortDataNotify(&node, 2, one_byte_write);
   apx_nodeData_outPortDataNotify(&node, 3, one_byte_write);
   apx_nodeData_outPortDataNotify(&node, 2, one_byte_write + one_byte_write);
   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));

   // With APX_ES_FILEMANAGER_OPTIMIZE_WRITE_NOTIFICATIONS enabled write to the same location as in the queue shall not trigger adding it to the queue again,
   // It will however move the queuedWriteNotify onto the queue
   apx_nodeData_outPortDataNotify(&node, 0, one_byte_write);
   CuAssertUIntEquals(tc, 2, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_MSG_WRITE_NOTIFY, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, 0, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);
   apx_nodeData_outPortDataNotify(&node, 0, one_byte_write);
   CuAssertUIntEquals(tc, 2, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_MSG_WRITE_NOTIFY, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, 0, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);

   apx_nodeData_outPortDataNotify(&node, 5, one_byte_write);
   CuAssertUIntEquals(tc, 2, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_MSG_WRITE_NOTIFY, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, 5, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);

   /* Aligned writes larger than APX_ES_FILE_WRITE_MSG_FRAGMENTATION_THRESHOLD - RMF_HIGH_ADDRESS_SIZE shall not be aligned */
   // Setup
   apx_nodeData_outPortDataNotify(&node, 1, sizeof(uint64_t));
   expected_write_notify_size3 = 1 + RMF_LOW_ADDRESS_SIZE;
   // Allow previously to be added even it the new write completely covers it...
   CuAssertUIntEquals(tc, 3, rbfs_size(&fileManager.messageQueue));

   // Up to limit is shall be aligned
   apx_nodeData_outPortDataNotify(&node, 1, sizeof(uint64_t));
   CuAssertUIntEquals(tc, 3, rbfs_size(&fileManager.messageQueue));

   // Ensure not building larger than limit
   apx_nodeData_outPortDataNotify(&node, 1 + sizeof(uint64_t), one_byte_write);
   expected_write_notify_size4 = 1 + RMF_LOW_ADDRESS_SIZE;
   CuAssertUIntEquals(tc, 4, rbfs_size(&fileManager.messageQueue));

   // Check how data is transmitted for write notify
   m_test_send_avail = expected_write_notify_size1;
   m_test_data_written = -1;
   apx_es_fileManager_run(&fileManager);
   CuAssertUIntEquals(tc, 3, rbfs_size(&fileManager.messageQueue)); // Without optimal write size expect one send per loop
   CuAssertIntEquals(tc, expected_write_notify_size1, m_test_data_written);
   CuAssertTrue(tc, fileManager.pendingWrite);
   CuAssertUIntEquals(tc, expected_write_notify_size2 - RMF_LOW_ADDRESS_SIZE, fileManager.fileWriteInfo.remain);
   m_test_data_written = -1;
   // Nothing should happen if trying again with if no larger buffer provided
   apx_es_fileManager_run(&fileManager);
   CuAssertUIntEquals(tc, 3, rbfs_size(&fileManager.messageQueue)); // Without optimal write size expect one send per loop
   CuAssertIntEquals(tc, -1, m_test_data_written);
   CuAssertTrue(tc, fileManager.pendingWrite);
   CuAssertUIntEquals(tc, expected_write_notify_size2 - RMF_LOW_ADDRESS_SIZE, fileManager.fileWriteInfo.remain);
   m_test_data_written = -1;
   m_test_send_avail = SEND_BUFFER_MAX; // all of them fit in full buffer
   apx_es_fileManager_run(&fileManager);
   // Ensure last message is sent
   CuAssertIntEquals(tc, expected_write_notify_size4, m_test_data_written);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertTrue(tc, !fileManager.pendingWrite);
}


static void apx_es_filemanager_serialize_all_commands(CuTest* tc)
{
#define FILE_WRITE_NOTIFY_SIZE 16
   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];
   uint8_t receiveBuffer[RECEIVE_BUFFER_LEN];
   apx_file_t file;
   apx_nodeData_t node;
   uint8_t data[FILE_WRITE_NOTIFY_SIZE];
   apx_msg_t aMsg;
   apx_es_fileManager_create(&fileManager,
                             messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES,
                             receiveBuffer, RECEIVE_BUFFER_LEN);

   memset(data, 0, FILE_WRITE_NOTIFY_SIZE);

   TestHelper_resetAndConnectTransmitHandler(&fileManager);

   apx_nodeData_create(&node,"node",NULL,0,NULL,0,0,&data[0],NULL,FILE_WRITE_NOTIFY_SIZE);
   apx_file_createLocalFile(&file, APX_OUTDATA_FILE, &node);
   apx_nodeData_setFileManager(&node, &fileManager);
   apx_nodeData_setOutPortDataFile(&node, &file);
   CuAssertTrue(tc, !file.isOpen);
   CuAssertTrue(tc, !file.isRemoteFile);
   CuAssertTrue(tc, !node.isRemote);
   CuAssertUIntEquals(tc, FILE_WRITE_NOTIFY_SIZE, node.outPortDataLen);

   apx_es_fileManager_attachLocalFile(&fileManager, &file);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   apx_es_fileManager_onConnected(&fileManager);
   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   rbfs_remove(&fileManager.messageQueue, &aMsg);
   CuAssertUIntEquals(tc, RMF_MSG_FILEINFO, aMsg.msgType);

   // Add the message as pending and it should not be sent when buffer has 0 free space
   fileManager.pendingMsg = aMsg;
   m_test_send_avail = 0; // 0 free space in sendbuffer
   m_test_data_written = -1;
   TestHelper_resetAndConnectTransmitHandler(&fileManager);
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, -1, m_test_data_written);
   CuAssertUIntEquals(tc, RMF_MSG_FILEINFO, fileManager.pendingMsg.msgType);

   // All managed msg types should be kept in queue when no buffer
   fileManager.pendingMsg.msgType = RMF_MSG_FILE_OPEN;
   fileManager.pendingMsg.msgData1 = 1;
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, -1, m_test_data_written);

   fileManager.pendingMsg.msgType = RMF_MSG_FILE_SEND;
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, -1, m_test_data_written);

   // Provide sendbuffer
   m_test_send_avail = SEND_BUFFER_MAX;

   // Now sent
   fileManager.pendingMsg.msgType = RMF_MSG_FILEINFO;
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 61, m_test_data_written);
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.pendingMsg.msgType);

   fileManager.pendingMsg = aMsg;
   fileManager.pendingMsg.msgType = RMF_MSG_FILE_OPEN;
   fileManager.pendingMsg.msgData1 = 1;
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 12, m_test_data_written);
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.pendingMsg.msgType);

   fileManager.pendingMsg = aMsg;
   fileManager.pendingMsg.msgType = RMF_MSG_FILE_SEND;
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 18, m_test_data_written);
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.pendingMsg.msgType);
}

static void apx_es_filemanager_request_files(CuTest* tc)
{
#define FILE1_SIZE 8
#define FILE2_SIZE 16
#define FILE3_SIZE 32
   apx_file_t file1;
   apx_nodeData_t node1;
   uint8_t data1[FILE1_SIZE];
   uint8_t flags1[FILE1_SIZE];
   apx_file_t file2;
   apx_nodeData_t node2;
   uint8_t data2[FILE2_SIZE];
   uint8_t flags2[FILE2_SIZE];
   apx_file_t file3;
   apx_nodeData_t node3;
   uint8_t data3[FILE3_SIZE];
   uint8_t flags3[FILE3_SIZE];

   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];
   uint8_t receiveBuffer[RECEIVE_BUFFER_LEN];

   int8_t rc;
   uint32_t file1_addr = 123;
   apx_nodeData_create(&node1,"node1",0,0,&data1[0],&flags1[0],FILE1_SIZE,0,0,0);
   apx_nodeData_create(&node2,"node2",0,0,&data2[0],&flags2[0],FILE2_SIZE,0,0,0);
   apx_nodeData_create(&node3,"node3",0,0,&data3[0],&flags3[0],FILE3_SIZE,0,0,0);
   apx_file_createLocalFile(&file1, APX_INDATA_FILE, &node1);
   apx_file_createLocalFile(&file2, APX_INDATA_FILE, &node2);
   apx_file_createLocalFile(&file3, APX_INDATA_FILE, &node3);
   CuAssertStrEquals(tc, "node1.in",file1.fileInfo.name);
   CuAssertStrEquals(tc, "node2.in",file2.fileInfo.name);
   CuAssertStrEquals(tc, "node3.in",file3.fileInfo.name);
   CuAssertIntEquals(tc, FILE1_SIZE,file1.fileInfo.length);
   CuAssertIntEquals(tc, FILE2_SIZE,file2.fileInfo.length);
   CuAssertIntEquals(tc, FILE3_SIZE,file3.fileInfo.length);
   file1.fileInfo.address = file1_addr;
   CuAssertIntEquals(tc, file1_addr,file1.fileInfo.address);
   CuAssertIntEquals(tc, RMF_INVALID_ADDRESS,file2.fileInfo.address);
   CuAssertIntEquals(tc, RMF_INVALID_ADDRESS,file3.fileInfo.address);
   rc = apx_es_fileManager_create(&fileManager,messageQueueBuf,APX_FILE_MANAGER_MAX_NUM_MESSAGES,receiveBuffer,RECEIVE_BUFFER_LEN);
   CuAssertIntEquals(tc, 0, (int) rc);
   CuAssertIntEquals(tc, fileManager.numRequestedFiles, 0);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file1);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file2);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file3);
   CuAssertIntEquals(tc, fileManager.numRequestedFiles, 3);
   //test 1. Check that no duplicates appear
   apx_es_fileManager_requestRemoteFile(&fileManager, &file1);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file2);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file3);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file3);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file2);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file1);
   CuAssertIntEquals(tc, fileManager.numRequestedFiles, 3);
   //test 2. remove from beginning of list
   rc = apx_es_fileManager_removeRequestedAt(&fileManager,0); //removes file1, moves file2 and file3 left 1 step
   CuAssertIntEquals(tc, 0, (int) rc);
   CuAssertIntEquals(tc, 2, (int) fileManager.numRequestedFiles);
   CuAssertPtrEquals(tc,&file2, fileManager.requestedFileList[0]);
   CuAssertPtrEquals(tc,&file3, fileManager.requestedFileList[1]);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file1); //appends file1 to end of list
   CuAssertIntEquals(tc, fileManager.numRequestedFiles, 3);
   //test 3. remove from middle of list
   rc = apx_es_fileManager_removeRequestedAt(&fileManager,1); //removes file3, moves file3 and file1 left 1 step
   CuAssertIntEquals(tc, 0, (int) rc);
   CuAssertIntEquals(tc, 2, (int) fileManager.numRequestedFiles);
   CuAssertPtrEquals(tc,&file2, fileManager.requestedFileList[0]);
   CuAssertPtrEquals(tc,&file1, fileManager.requestedFileList[1]);
   apx_es_fileManager_requestRemoteFile(&fileManager, &file3); //appends file3 to end of list
   CuAssertIntEquals(tc, fileManager.numRequestedFiles, 3);
   //test 4. remove from end of list
   rc = apx_es_fileManager_removeRequestedAt(&fileManager,2); //removes file3
   CuAssertIntEquals(tc, 0, (int) rc);
   CuAssertIntEquals(tc, 2, (int) fileManager.numRequestedFiles);
   CuAssertPtrEquals(tc, &file2, fileManager.requestedFileList[0]);
   CuAssertPtrEquals(tc, &file1, fileManager.requestedFileList[1]);

   //test 5. test with invalid arguments
   CuAssertIntEquals(tc,-1, (int) apx_es_fileManager_removeRequestedAt(&fileManager,-1));
   CuAssertIntEquals(tc,-1, (int) apx_es_fileManager_removeRequestedAt(&fileManager,2));

   //test 6. remove last 2 by triggering input parse string to request them
   m_test_data_written=-1;
   CuAssertIntEquals(tc, file1_addr,file1.fileInfo.address);
   CuAssertIntEquals(tc, RMF_INVALID_ADDRESS,file2.fileInfo.address);
   apx_es_fileManager_onConnected(&fileManager);

   CuAssertIntEquals(tc, file1_addr,file1.fileInfo.address);
   CuAssertIntEquals(tc, RMF_INVALID_ADDRESS,file2.fileInfo.address);
   apx_es_fileManager_attachLocalFile(&fileManager, &file1);
   apx_es_fileManager_attachLocalFile(&fileManager, &file2);
   CuAssertIntEquals(tc, 0, file1.fileInfo.address);
   CuAssertIntEquals(tc, 1024, file2.fileInfo.address);
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, -1, m_test_data_written);
   CuAssertIntEquals(tc, 2, fileManager.numRequestedFiles);

   CuAssertPtrEquals(tc, &file2, fileManager.requestedFileList[0]);
   CuAssertPtrEquals(tc, &file1, fileManager.requestedFileList[1]);
   apx_es_fileManager_processRemoteFileInfo(&fileManager, &file1.fileInfo); // file1 in remote file map
   CuAssertPtrEquals(tc, &file2, fileManager.requestedFileList[0]);
   apx_es_fileManager_run(&fileManager);
   // No sendbuffer
   CuAssertIntEquals(tc, -1, m_test_data_written);
   TestHelper_resetAndConnectTransmitHandler(&fileManager);
   m_test_send_avail = SEND_BUFFER_MAX;
   apx_es_fileManager_run(&fileManager);
   int file2_info_len = 57;
   int file1_open_len = 12;
   int file2_open_len = 12;
   CuAssertIntEquals(tc, file1_open_len, m_test_data_written);
   CuAssertIntEquals(tc, ((RMF_CMD_START_ADDR >> 24) & 0xffu) | 0x80u, m_test_send_buffer[0]);
   CuAssertIntEquals(tc, (RMF_CMD_START_ADDR >> 16) & 0xffu, m_test_send_buffer[1]);
   CuAssertIntEquals(tc, (RMF_CMD_START_ADDR >> 8) & 0xffu, m_test_send_buffer[2]);
   CuAssertIntEquals(tc, (RMF_CMD_START_ADDR >> 0) & 0xffu, m_test_send_buffer[3]);
   CuAssertIntEquals(tc, RMF_CMD_FILE_OPEN, m_test_send_buffer[4]);
   CuAssertIntEquals(tc, file1.fileInfo.address, m_test_send_buffer[8]);
   CuAssertIntEquals(tc, 1, fileManager.numRequestedFiles);

   m_test_data_written=-1;
   CuAssertIntEquals(tc, file2_info_len, rmf_serialize_cmdFileInfo(&m_test_send_buffer[RMF_HIGH_ADDRESS_SIZE], SEND_BUFFER_MAX-RMF_LOW_ADDRESS_SIZE, &file2.fileInfo));
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, rmf_packHeader(m_test_send_buffer, RMF_HIGH_ADDRESS_SIZE, RMF_CMD_START_ADDR, false));
   apx_es_fileManager_onMsgReceived(&fileManager, m_test_send_buffer, file2_info_len+RMF_HIGH_ADDRESS_SIZE);
   CuAssertIntEquals(tc, 0, fileManager.numRequestedFiles);
   CuAssertIntEquals(tc, -1, m_test_data_written);
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, file2_open_len, m_test_data_written);
}


static void apx_es_filemanager_enter_pending_mode_when_buffer_is_full(CuTest* tc)
{
   apx_file_t file1;
   apx_nodeData_t node1;
   uint8_t data1[FILE1_SIZE];
   uint8_t flags1[FILE1_SIZE];
   uint32_t i;

   int8_t rc;
   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];
   uint8_t receiveBuffer[RECEIVE_BUFFER_LEN];

   apx_nodeData_create(&node1,"node1",0,0,0,0,0,&data1[0],&flags1[0],FILE1_SIZE);
   rc = apx_file_createLocalFile(&file1, APX_OUTDATA_FILE, &node1);
   CuAssertIntEquals(tc, 0, (int) rc);
   CuAssertStrEquals(tc, "node1.out",file1.fileInfo.name);
   CuAssertIntEquals(tc, FILE1_SIZE,file1.fileInfo.length);

   rc = apx_es_fileManager_create(&fileManager, messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES, receiveBuffer,RECEIVE_BUFFER_LEN);
   CuAssertIntEquals(tc, 0, (int) rc);

   TestHelper_resetAndConnectTransmitHandler(&fileManager);

   m_test_send_avail = 2;
   apx_es_fileManager_attachLocalFile(&fileManager, &file1);
   apx_es_fileManager_onConnected(&fileManager);
   file1.fileInfo.address = 0;
   m_application_data[0]=0x12;
   m_application_data[1]=0x34;
   m_application_data[2]=0x56;
   m_application_data[3]=0x78;
   apx_nodeData_writeOutPortData(&node1, &m_application_data[0], 2, 4);
   node1.outPortDirtyFlags[2] = 1;
   apx_es_fileManager_onFileUpdate(&fileManager, &file1, 2, 4);
   //fake long delay in data transfer
   for(i=0;i<1000;i++)
   {
      m_test_data_offset = -1;
      m_test_data_written=-1;
      apx_es_fileManager_run(&fileManager);
      CuAssertIntEquals(tc, -1, m_test_data_written);
   }
   //Make some buffer available (below threshold)
   m_test_send_avail  = 3;
   m_test_data_offset = 2;
   m_test_data_written=-1;
   apx_es_fileManager_run(&fileManager);
   // Expect no write when only one byte can be flushed
   CuAssertIntEquals(tc, -1, m_test_data_written);
   m_test_send_avail = SEND_BUFFER_MAX;
   apx_es_fileManager_run(&fileManager);
   // Expect file info
   CuAssertIntEquals(tc, 62, m_test_data_written);
   m_test_data_written=-1;
   // Expect no file write before file is open
   CuAssertIntEquals(tc, -1, m_test_data_written);
   apx_es_fileManager_onFileUpdate(&fileManager, &file1, 2, 4);
   // Expect no file write before file is open
   CuAssertIntEquals(tc, -1, m_test_data_written);
}

static void TestHelper_resetAndConnectTransmitHandler(apx_es_fileManager_t* fileManager)
{
   memset(&m_transmitHandler, 0, sizeof(m_transmitHandler));
   m_transmitHandler.getSendAvail = TestStub_getSendAvail;
   m_transmitHandler.getSendBuffer = TestStub_getSendBuffer;
   m_transmitHandler.send = TestStub_send;
   apx_es_fileManager_setTransmitHandler(fileManager, &m_transmitHandler);
}

static int32_t TestStub_getSendAvail(void *arg)
{
   return m_test_send_avail;
}

static uint8_t* TestStub_getSendBuffer(void *arg, int32_t msgLen)
{
   if (msgLen > m_test_send_avail)
   {
      return 0;
   }
   else
   {
      return &m_test_send_buffer[0];
   }
}

static int32_t TestStub_send(void *arg, int32_t offset, int32_t msgLen)
{
   if (msgLen > m_test_send_avail)
   {
      m_test_data_written=0;
   }
   else
   {
      m_test_data_written = msgLen;
      m_test_data_offset = offset;
   }
   return 0;
}



