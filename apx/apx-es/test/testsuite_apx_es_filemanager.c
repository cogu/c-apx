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
#include "mockTransmitter.h"
#include "headerutil.h"
#include "ApxNode_ButtonStatus.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_FILE_MANAGER_MAX_NUM_MESSAGES 20
#define APX_FILE_MANAGER_MSG_QUEUE_SIZE (sizeof(apx_msg_t)*APX_FILE_MANAGER_MAX_NUM_MESSAGES)
#define SEND_BUFFER_MAX 256
//#define APPLICATION_DATA_MAX 8
#define LOWER_LAYER_BUFFER_SIZE 256
#define RECEIVE_BUFFER_MAX 256

#define OUTPUT_DATA_SIZE 160

/* COPIED FROM ApxNode_ButtonStatus.c */
#define APX_DEFINITON_LEN 352u
#define APX_IN_PORT_DATA_LEN 1u
#define APX_OUT_PORT_DATA_LEN 7u
/**************************************/

typedef struct apx_file_container_tag
{
   apx_file_t definitionFile;
   apx_file_t outDataFile;
   apx_file_t inDataFile;
}apx_fileContainer_t;

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_es_fileManager_create(CuTest* tc);
static void test_apx_es_fileManager_sendFileInfoWhenConnected(CuTest* tc);
static void test_apx_es_fileManager_sendDefinitionInOneCycle(CuTest* tc);
static void test_apx_es_fileManager_sendDefinitionOverTwoCycles(CuTest* tc);
static void test_apx_es_fileManager_triggerFileUpdate_unaligned(CuTest* tc);
static void test_apx_es_fileManager_triggerFileUpdate_aligned(CuTest* tc);
static void test_apx_es_fileManager_triggerFileUpdate_aligned_large(CuTest* tc);
//static void apx_es_filemanager_serialize_all_commands(CuTest* tc);
//static void apx_es_filemanager_request_files(CuTest* tc);
//static void apx_es_filemanager_enter_pending_mode_when_buffer_is_full(CuTest* tc);

static uint8_t* testStub_getMsgBuffer(void *arg, int32_t *maxMsgLen, int32_t *sendAvail);
static int32_t testStub_sendMsg(void *arg, int32_t offset, int32_t msgLen);
static void testHelper_mockInit(void);
static void testHelper_mockReset(int32_t newDataLen);
static int32_t testHelper_mockNumMessages(void);
static int32_t testHelper_mockGetMessage(void);
static int32_t testHelper_mockGetWriteAvail(void);
static void testHelper_setTransmitHandler(apx_es_fileManager_t* fileManager);
static void testHelper_attachNode(apx_es_fileManager_t *fileManager, apx_nodeData_t *nodeData, apx_fileContainer_t *fileContainer);
static int32_t testHelper_serialize_FileOpen(CuTest* tc, uint32_t fileAddress);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

static uint8_t m_test_send_buffer[SEND_BUFFER_MAX];
static uint8_t m_test_receive_buffer[RECEIVE_BUFFER_MAX];
static uint8_t m_msgBuf[SEND_BUFFER_MAX];
//static uint8_t m_application_data[APPLICATION_DATA_MAX];
static uint8_t m_messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];
static mockTransmitter_t m_mockTransmitter;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_apx_es_filemanager(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_es_fileManager_create);
   SUITE_ADD_TEST(suite, test_apx_es_fileManager_sendFileInfoWhenConnected);
   SUITE_ADD_TEST(suite, test_apx_es_fileManager_sendDefinitionInOneCycle);
   SUITE_ADD_TEST(suite, test_apx_es_fileManager_sendDefinitionOverTwoCycles);
   SUITE_ADD_TEST(suite, test_apx_es_fileManager_triggerFileUpdate_unaligned);
   SUITE_ADD_TEST(suite, test_apx_es_fileManager_triggerFileUpdate_aligned);
   SUITE_ADD_TEST(suite, test_apx_es_fileManager_triggerFileUpdate_aligned_large);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_es_fileManager_create(CuTest* tc)
{
   int8_t rc;
   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];
   rc = apx_es_fileManager_create(&fileManager, messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES, 0, 0);
   CuAssertIntEquals(tc, 0, rc);
   CuAssertTrue(tc, !fileManager.isConnected);
   CuAssertTrue(tc, !fileManager.dropMessage);
   CuAssertTrue(tc, !fileManager.hasPendingWrite);

   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.pendingMsg.msgType);

   CuAssertUIntEquals(tc, 0, fileManager.transmitBuf.avail);
   CuAssertPtrEquals(tc, 0, fileManager.transmitBuf.data);

   CuAssertUIntEquals(tc, 0, fileManager.numRequestedFiles);
   CuAssertUIntEquals(tc, 0, apx_es_fileMap_length(&fileManager.localFileMap));
   CuAssertUIntEquals(tc, 0, apx_es_fileMap_length(&fileManager.remoteFileMap));

   CuAssertUIntEquals(tc, 0, fileManager.receiveBufOffset);
   CuAssertUIntEquals(tc, 0, fileManager.receiveBufLen);
}

static void test_apx_es_fileManager_sendFileInfoWhenConnected(CuTest* tc)
{
   apx_es_fileManager_t fileManager;
   apx_nodeData_t *nodeData;
   apx_fileContainer_t fileContainer;
   rmf_msg_t msg;
   rmf_fileInfo_t fileInfo;
   testHelper_mockInit();
   apx_es_fileManager_create(&fileManager, m_messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES, 0, 0);
   ApxNode_Init_ButtonStatus();
   nodeData = ApxNode_GetNodeData_ButtonStatus();
   testHelper_attachNode(&fileManager, nodeData, &fileContainer);
   testHelper_setTransmitHandler(&fileManager);
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   apx_es_fileManager_onConnected(&fileManager);
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 2, testHelper_mockNumMessages());
   CuAssertIntEquals(tc, 69, testHelper_mockGetMessage());
   CuAssertIntEquals(tc, 69, rmf_unpackMsg(&m_msgBuf[0], 69, &msg));
   CuAssertTrue(tc, !msg.more_bit);
   CuAssertIntEquals(tc, 65, msg.dataLen);
   CuAssertIntEquals(tc, msg.dataLen, rmf_deserialize_cmdFileInfo(msg.data, msg.dataLen, &fileInfo));
   CuAssertStrEquals(tc, "ButtonStatus.out", fileInfo.name);
   CuAssertIntEquals(tc, 69, testHelper_mockGetMessage());
   CuAssertIntEquals(tc, 69, rmf_unpackMsg(&m_msgBuf[0], 69, &msg));
   CuAssertTrue(tc, !msg.more_bit);
   CuAssertIntEquals(tc, 65, msg.dataLen);
   CuAssertIntEquals(tc, msg.dataLen, rmf_deserialize_cmdFileInfo(msg.data, msg.dataLen, &fileInfo));
   CuAssertStrEquals(tc, "ButtonStatus.apx", fileInfo.name);
   CuAssertIntEquals(tc, -1, testHelper_mockGetMessage());
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
}

/*
 * This test checks if its possible for apx_es_fileManager to send the definition
 * using multiple messages while running in the same cycle.
 * In this test we set the underlying buffer to 1024 bytes. The message buffer is 256 bytes.
 * The APX definition in this test is around 300 bytes so we expecte the file manager to create 2 messages,
 * one that is 256 bytes in length and another with the remaining bytes.
 */
static void test_apx_es_fileManager_sendDefinitionInOneCycle(CuTest* tc)
{
   apx_es_fileManager_t fileManager;
   apx_nodeData_t *nodeData;
   apx_fileContainer_t fileContainer;
   rmf_msg_t msg;
   int32_t msgLen;
   uint32_t definitionFileAddress = 0x4000000u;
   int32_t remain = APX_DEFINITON_LEN;
   int32_t offset = 0;
   int32_t blockLen = SEND_BUFFER_MAX-RMF_HIGH_ADDRESS_SIZE;

   testHelper_mockInit();
   apx_es_fileManager_create(&fileManager, m_messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES, 0, 0);
   ApxNode_Init_ButtonStatus();
   nodeData = ApxNode_GetNodeData_ButtonStatus();
   testHelper_attachNode(&fileManager, nodeData, &fileContainer);
   testHelper_setTransmitHandler(&fileManager);
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   apx_es_fileManager_onConnected(&fileManager);
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 2, testHelper_mockNumMessages());
   //assume these are the fileInfo structures (see test_apx_es_fileManager_sendFileInfoWhenConnected)
   testHelper_mockReset(1024);
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   msgLen = testHelper_serialize_FileOpen(tc, definitionFileAddress);
   CuAssertIntEquals(tc, 0, apx_es_fileManager_getNumMessagesInQueue(&fileManager));
   apx_es_fileManager_onMsgReceived(&fileManager, &m_test_receive_buffer[0], msgLen);
   CuAssertIntEquals(tc, 1, apx_es_fileManager_getNumMessagesInQueue(&fileManager));
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 2, testHelper_mockNumMessages());
   //check first message
   CuAssertIntEquals(tc, SEND_BUFFER_MAX, testHelper_mockGetMessage());
   CuAssertIntEquals(tc, SEND_BUFFER_MAX, rmf_unpackMsg(&m_msgBuf[0], SEND_BUFFER_MAX, &msg));
   CuAssertTrue(tc, msg.more_bit);
   CuAssertUIntEquals(tc, definitionFileAddress, msg.address);
   CuAssertUIntEquals(tc, (uint32_t) blockLen, msg.dataLen);
   CuAssertTrue(tc, memcmp(&nodeData->definitionDataBuf[offset], msg.data, msg.dataLen)==0);
   offset+=blockLen;
   remain-=blockLen;
   //check second message
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE+remain, testHelper_mockGetMessage());
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE+remain, rmf_unpackMsg(&m_msgBuf[0], RMF_HIGH_ADDRESS_SIZE+remain, &msg));
   CuAssertTrue(tc, !msg.more_bit);
   CuAssertUIntEquals(tc, definitionFileAddress+offset, msg.address);
   CuAssertUIntEquals(tc, (uint32_t) remain, msg.dataLen);
   CuAssertTrue(tc, memcmp(&nodeData->definitionDataBuf[offset], msg.data, msg.dataLen)==0);
   CuAssertIntEquals(tc, -1, testHelper_mockGetMessage());
   CuAssertTrue(tc, !fileManager.hasPendingWrite);
   CuAssertTrue(tc, !apx_es_fileManager_hasPendingMsg(&fileManager));
}

/**
 * This checks that the definition file can be sent even when underlying buffer gets full during the first cycle.
 * The file manager shall wait until enough buffer is available and then resume transmission
 */
static void test_apx_es_fileManager_sendDefinitionOverTwoCycles(CuTest* tc)
{
   apx_es_fileManager_t fileManager;
   apx_nodeData_t *nodeData;
   apx_fileContainer_t fileContainer;
   rmf_msg_t msg;
   int i;
   int32_t msgLen;
   uint32_t definitionFileAddress = 0x4000000u;
   int32_t remain = APX_DEFINITON_LEN;
   int32_t offset = 0;
   int32_t blockLen;

   testHelper_mockInit();
   apx_es_fileManager_create(&fileManager, m_messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES, 0, 0);
   ApxNode_Init_ButtonStatus();
   nodeData = ApxNode_GetNodeData_ButtonStatus();
   testHelper_attachNode(&fileManager, nodeData, &fileContainer);
   testHelper_setTransmitHandler(&fileManager);
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   apx_es_fileManager_onConnected(&fileManager);
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 2, testHelper_mockNumMessages());
   //assume these are the fileInfo structures (see test_apx_es_fileManager_sendFileInfoWhenConnected)
   testHelper_mockReset(200); //200 bytes available in mock transmitter
   msgLen = testHelper_serialize_FileOpen(tc, definitionFileAddress);
   apx_es_fileManager_onMsgReceived(&fileManager, &m_test_receive_buffer[0], msgLen);
   CuAssertIntEquals(tc, 0, testHelper_mockNumMessages());
   CuAssertTrue(tc, !fileManager.hasPendingWrite);
   CuAssertTrue(tc, !apx_es_fileManager_hasPendingMsg(&fileManager));
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 1, testHelper_mockNumMessages());
   CuAssertTrue(tc, fileManager.hasPendingWrite);
   CuAssertTrue(tc, !apx_es_fileManager_hasPendingMsg(&fileManager));
   CuAssertIntEquals(tc, 0, testHelper_mockGetWriteAvail());

   //check first message
   blockLen = 198-RMF_HIGH_ADDRESS_SIZE;
   CuAssertIntEquals(tc, 198, testHelper_mockGetMessage());
   CuAssertIntEquals(tc, 198, rmf_unpackMsg(&m_msgBuf[0], 198, &msg));
   CuAssertTrue(tc, msg.more_bit);
   CuAssertUIntEquals(tc, definitionFileAddress+offset, msg.address);
   CuAssertUIntEquals(tc, (uint32_t) blockLen, msg.dataLen);
   CuAssertTrue(tc, memcmp(&nodeData->definitionDataBuf[offset], msg.data, msg.dataLen)==0);
   offset+=blockLen;
   remain-=blockLen;

   //while transmit buffer is full manager shall retry to send message without ending up in error mode
   testHelper_mockReset(0);
   for(i=0; i < 100; i++)
   {
      apx_es_fileManager_run(&fileManager);
      CuAssertTrue(tc, fileManager.hasPendingWrite);
      CuAssertTrue(tc, !apx_es_fileManager_hasPendingMsg(&fileManager));
      CuAssertIntEquals(tc, 0, testHelper_mockGetWriteAvail());
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_es_fileManager_getLastError(&fileManager));
   }
   //When buffer becomes available it resumes transfer
   testHelper_mockReset(APX_ES_FILEMANAGER_MIN_BUFFER_TRESHOLD+HEADERUTIL16_SIZE_MAX);
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 1, testHelper_mockNumMessages());
   //Check second message
   blockLen = APX_ES_FILEMANAGER_MIN_BUFFER_TRESHOLD-RMF_HIGH_ADDRESS_SIZE;
   CuAssertIntEquals(tc, APX_ES_FILEMANAGER_MIN_BUFFER_TRESHOLD, testHelper_mockGetMessage());
   CuAssertIntEquals(tc, APX_ES_FILEMANAGER_MIN_BUFFER_TRESHOLD, rmf_unpackMsg(&m_msgBuf[0], APX_ES_FILEMANAGER_MIN_BUFFER_TRESHOLD, &msg));
   CuAssertTrue(tc, msg.more_bit);
   CuAssertUIntEquals(tc, definitionFileAddress+offset, msg.address);
   CuAssertUIntEquals(tc, (uint32_t) blockLen, msg.dataLen);
   CuAssertTrue(tc, memcmp(&nodeData->definitionDataBuf[offset], msg.data, msg.dataLen)==0);
   offset+=blockLen;
   remain-=blockLen;
   testHelper_mockReset(0);
   //while transmit buffer is full manager shall retry to send message without ending up in error mode
   for(i=0; i < 100; i++)
   {
      apx_es_fileManager_run(&fileManager);
      CuAssertTrue(tc, fileManager.hasPendingWrite);
      CuAssertTrue(tc, !apx_es_fileManager_hasPendingMsg(&fileManager));
      CuAssertIntEquals(tc, 0, testHelper_mockGetWriteAvail());
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_es_fileManager_getLastError(&fileManager));
   }
   testHelper_mockReset(200);
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 1, testHelper_mockNumMessages());
   //check last message
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE+remain, testHelper_mockGetMessage());
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE+remain, rmf_unpackMsg(&m_msgBuf[0], RMF_HIGH_ADDRESS_SIZE+remain, &msg));
   CuAssertTrue(tc, !msg.more_bit);
   CuAssertUIntEquals(tc, definitionFileAddress+offset, msg.address);
   CuAssertUIntEquals(tc, (uint32_t) remain, msg.dataLen);
   CuAssertTrue(tc, memcmp(&nodeData->definitionDataBuf[offset], msg.data, msg.dataLen)==0);
   CuAssertIntEquals(tc, -1, testHelper_mockGetMessage());
   CuAssertTrue(tc, !fileManager.hasPendingWrite);
   CuAssertTrue(tc, !apx_es_fileManager_hasPendingMsg(&fileManager));
}


static void test_apx_es_fileManager_triggerFileUpdate_unaligned(CuTest* tc)
{

   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];

   apx_file_t file;
   apx_nodeData_t node;
   uint8_t outPortData[OUTPUT_DATA_SIZE];
   apx_msg_t topOfQueue;
   apx_msg_t prevQueued;
   const uint8_t one_byte_write = 1u;
   uint32_t offset = 0;

   //create file manager
   apx_es_fileManager_create(&fileManager, messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES, 0, 0);

   //create nodeData
   memset(outPortData, 0, OUTPUT_DATA_SIZE);
   apx_nodeData_create(&node,"node", NULL, 0, NULL, 0, 0, &outPortData[0], NULL, OUTPUT_DATA_SIZE);

   //create file
   apx_file_createLocalFile(&file, APX_OUTDATA_FILE, &node);

   //attach file to manager and connect manager
   apx_nodeData_setFileManager(&node, &fileManager);
   apx_nodeData_setOutPortDataFile(&node, &file);
   apx_es_fileManager_attachLocalFile(&fileManager, &file);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   apx_es_fileManager_onConnected(&fileManager);
   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   rbfs_remove(&fileManager.messageQueue, (uint8_t*) &topOfQueue);
   CuAssertUIntEquals(tc, RMF_MSG_FILEINFO, topOfQueue.msgType);
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.queuedWriteNotify.msgType);

   // Before file is marked open it ignore writes
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   offset = 0;
   apx_nodeData_outPortDataNotify(&node, offset, one_byte_write);

   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.queuedWriteNotify.msgType);

   // First write shall be put aside for update if file is open
   apx_file_open(&file);
   CuAssertTrue(tc, file.isOpen);
   apx_nodeData_outPortDataNotify(&node, offset, one_byte_write);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_MSG_WRITE_NOTIFY, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, offset, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);

   // Put the message on queue when writes do not align
   prevQueued = fileManager.queuedWriteNotify;
   offset = 2;
   apx_nodeData_outPortDataNotify(&node, offset, one_byte_write);

   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   rbfs_peek(&fileManager.messageQueue, (uint8_t*) &topOfQueue);
   CuAssertIntEquals(tc, 0, memcmp(&topOfQueue, &prevQueued, sizeof(apx_msg_t)));
   CuAssertUIntEquals(tc, offset, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);

}

static void test_apx_es_fileManager_triggerFileUpdate_aligned(CuTest* tc)
{
   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];

   apx_file_t file;
   apx_nodeData_t node;
   uint8_t outPortData[OUTPUT_DATA_SIZE];
   const uint8_t one_byte_write = 1u;
   uint32_t offset = 0;

   //create file manager
   apx_es_fileManager_create(&fileManager, messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES, 0, 0);

   //create nodeData
   memset(outPortData, 0, OUTPUT_DATA_SIZE);
   apx_nodeData_create(&node,"node", NULL, 0, NULL, 0, 0, &outPortData[0], NULL, OUTPUT_DATA_SIZE);

   //create file
   apx_file_createLocalFile(&file, APX_OUTDATA_FILE, &node);

   //attach file to manager and connect manager
   apx_nodeData_setFileManager(&node, &fileManager);
   apx_nodeData_setOutPortDataFile(&node, &file);
   apx_es_fileManager_attachLocalFile(&fileManager, &file);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   apx_es_fileManager_onConnected(&fileManager);
   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   rbfs_clear(&fileManager.messageQueue);
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.queuedWriteNotify.msgType);
   apx_file_open(&file);
   CuAssertTrue(tc, file.isOpen);

   //First write is placed in queuedWriteNotify
   offset = 2;
   apx_nodeData_outPortDataNotify(&node, offset, one_byte_write);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_MSG_WRITE_NOTIFY, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, offset, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);

   // When next write aligns with previous we keep the message in queuedWriteNotify
   offset = 3;
   apx_nodeData_outPortDataNotify(&node, offset, one_byte_write);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, 2, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, 2*one_byte_write, fileManager.queuedWriteNotify.msgData2);
}

static void test_apx_es_fileManager_triggerFileUpdate_aligned_large(CuTest* tc)
{
   apx_es_fileManager_t fileManager;
   uint8_t messageQueueBuf[APX_FILE_MANAGER_MSG_QUEUE_SIZE];
   apx_msg_t topOfQueue;
   apx_msg_t prevQueued;
   apx_file_t file;
   apx_nodeData_t node;
   uint8_t outPortData[OUTPUT_DATA_SIZE];
   const uint8_t one_byte_write = 1u;
   const uint8_t small_write_size = APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD-RMF_HIGH_ADDRESS_SIZE-one_byte_write;
   const uint8_t large_write_size = APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD-RMF_HIGH_ADDRESS_SIZE;
   uint32_t offset = 0;

   //create file manager
   apx_es_fileManager_create(&fileManager, messageQueueBuf, APX_FILE_MANAGER_MAX_NUM_MESSAGES, 0, 0);

   //create nodeData
   memset(outPortData, 0, OUTPUT_DATA_SIZE);
   apx_nodeData_create(&node,"node", NULL, 0, NULL, 0, 0, &outPortData[0], NULL, OUTPUT_DATA_SIZE);

   //create file
   apx_file_createLocalFile(&file, APX_OUTDATA_FILE, &node);

   //attach file to manager and connect manager
   apx_nodeData_setFileManager(&node, &fileManager);
   apx_nodeData_setOutPortDataFile(&node, &file);
   apx_es_fileManager_attachLocalFile(&fileManager, &file);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   apx_es_fileManager_onConnected(&fileManager);
   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   rbfs_clear(&fileManager.messageQueue);
   CuAssertUIntEquals(tc, RMF_CMD_INVALID_MSG, fileManager.queuedWriteNotify.msgType);
   apx_file_open(&file);
   CuAssertTrue(tc, file.isOpen);

   //First write is placed in queuedWriteNotify
   offset = 0;
   apx_nodeData_outPortDataNotify(&node, offset, one_byte_write);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_MSG_WRITE_NOTIFY, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, offset, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);

   //When next write aligns and is small it is automatically merged
   offset = 1;
   apx_nodeData_outPortDataNotify(&node, offset, small_write_size);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, 0, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write+small_write_size, fileManager.queuedWriteNotify.msgData2);

   //reset
   fileManager.queuedWriteNotify.msgType = RMF_CMD_INVALID_MSG;

   //First write is placed in queuedWriteNotify
   offset = 0;
   apx_nodeData_outPortDataNotify(&node, offset, one_byte_write);
   CuAssertUIntEquals(tc, 0, rbfs_size(&fileManager.messageQueue));
   CuAssertUIntEquals(tc, RMF_MSG_WRITE_NOTIFY, fileManager.queuedWriteNotify.msgType);
   CuAssertUIntEquals(tc, offset, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, one_byte_write, fileManager.queuedWriteNotify.msgData2);

   // When next write aligns but is very large it shall not merge the two writes
   prevQueued = fileManager.queuedWriteNotify;
   offset = 1;
   apx_nodeData_outPortDataNotify(&node, offset, large_write_size);

   CuAssertUIntEquals(tc, 1, rbfs_size(&fileManager.messageQueue));
   rbfs_peek(&fileManager.messageQueue, (uint8_t*) &topOfQueue);
   CuAssertIntEquals(tc, 0, memcmp(&topOfQueue, &prevQueued, sizeof(apx_msg_t)));
   CuAssertUIntEquals(tc, offset, fileManager.queuedWriteNotify.msgData1);
   CuAssertUIntEquals(tc, large_write_size, fileManager.queuedWriteNotify.msgData2);

}



static void testHelper_mockInit(void)
{
   mockTransmitter_create(&m_mockTransmitter);
}

static void testHelper_mockReset(int32_t newDataLen)
{
   mockTransmitter_reset(&m_mockTransmitter, newDataLen);
}
static int32_t testHelper_mockNumMessages(void)
{
   return mockTransmitter_getNumWrites(&m_mockTransmitter);
}

static int32_t testHelper_mockGetMessage(void)
{
   const uint8_t *pBegin = mockTransmitter_getData(&m_mockTransmitter);
   int32_t readAvail = mockTransmitter_readAvail(&m_mockTransmitter);
   if (readAvail > 0)
   {
      uint16_t msgLen;
      const uint8_t *pEnd = pBegin+readAvail;
      const uint8_t *pNext = headerutil_numDecode16(pBegin, pEnd, &msgLen);
      if (pNext > pBegin)
      {
         int32_t headerLen = (int32_t) (pNext-pBegin);
         memcpy(m_msgBuf, pNext, msgLen);
         mockTransmitter_trimLeft(&m_mockTransmitter, headerLen+msgLen);
         return (int32_t) msgLen;
      }
   }
   return -1;
}

static int32_t testHelper_mockGetWriteAvail(void)
{
   return mockTransmitter_writeAvail(&m_mockTransmitter);
}

static void testHelper_setTransmitHandler(apx_es_fileManager_t* fileManager)
{
   apx_transmitHandler_t transmitHandler;
   memset(&transmitHandler, 0, sizeof(transmitHandler));
   transmitHandler.getMsgBuffer = testStub_getMsgBuffer;
   transmitHandler.sendMsg = testStub_sendMsg;
   apx_es_fileManager_setTransmitHandler(fileManager, &transmitHandler);
}


static void testHelper_attachNode(apx_es_fileManager_t *fileManager, apx_nodeData_t *nodeData, apx_fileContainer_t *fileContainer)
{
   memset(fileContainer, 0, sizeof(apx_fileContainer_t));
   apx_file_createLocalFile(&fileContainer->definitionFile, APX_DEFINITION_FILE, nodeData);
   apx_es_fileManager_attachLocalFile(fileManager, &fileContainer->definitionFile);

   if (nodeData->outPortDataLen > 0)
   {
      apx_file_createLocalFile(&fileContainer->outDataFile, APX_OUTDATA_FILE, nodeData);
      apx_es_fileManager_attachLocalFile(fileManager, &fileContainer->outDataFile);
      apx_nodeData_setOutPortDataFile(nodeData, &fileContainer->outDataFile);
   }
   if (nodeData->inPortDataLen > 0 )
   {
      apx_file_createLocalFile(&fileContainer->inDataFile, APX_INDATA_FILE, nodeData);
      apx_es_fileManager_requestRemoteFile(fileManager, &fileContainer->inDataFile);
      apx_nodeData_setInPortDataFile(nodeData, &fileContainer->inDataFile);
   }
   apx_nodeData_setFileManager(nodeData, fileManager);
}

static int32_t testHelper_serialize_FileOpen(CuTest* tc, uint32_t fileAddress)
{
   rmf_cmdOpenFile_t cmd;
   int32_t msgLen = 0;
   uint8_t *pNext = &m_test_receive_buffer[0];
   int32_t bufRemain = (int32_t) sizeof(m_test_receive_buffer);
   int32_t consumed;
   consumed = rmf_packHeader(pNext, bufRemain, RMF_CMD_START_ADDR, false);
   CuAssertIntEquals(tc, RMF_HIGH_ADDRESS_SIZE, consumed);
   bufRemain-=consumed, pNext+=consumed, msgLen+=consumed;
   cmd.address = fileAddress;
   consumed = rmf_serialize_cmdOpenFile(pNext, bufRemain, &cmd);
   CuAssertIntEquals(tc, RMF_CMD_TYPE_LEN+sizeof(uint32_t), consumed);
   msgLen+=consumed;
   return msgLen;
}

static uint8_t* testStub_getMsgBuffer(void *arg, int32_t *maxMsgLen, int32_t *sendAvail)
{
   int32_t writeAvail = mockTransmitter_writeAvail(&m_mockTransmitter);
   *maxMsgLen = SEND_BUFFER_MAX;
   *sendAvail = (writeAvail < HEADERUTIL16_SIZE_MAX)? 0 : (writeAvail-HEADERUTIL16_SIZE_MAX);
   return &m_test_send_buffer[0];
}

static int32_t testStub_sendMsg(void *arg, int32_t offset, int32_t msgLen)
{
   if (offset == 0) //only offset 0 is supported in this stub
   {
      return mockTransmitter_write(&m_mockTransmitter, &m_test_send_buffer[0], msgLen);
   }
   return APX_TRANSMIT_HANDLER_INVALID_ARGUMENT_ERROR;
}





