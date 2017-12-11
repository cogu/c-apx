//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_msg.h"
#include "apx_es_filemanager.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_FILE_MANAGER_MAX_NUM_MESSAGES 1000
#define RECEIVE_BUFFER_LEN 4096
#define APX_FILE_MANAGER_MSG_QUEUE_SIZE (sizeof(apx_msg_t)*APX_FILE_MANAGER_MAX_NUM_MESSAGES)
//#define TRANSMIT_BUF_SIZE 4096
#define SEND_MIN_TRESHOLD 100 //at least this many bytes must be available in IPC channel before sending any new data
#define NUMHEADER16_MAX_LEN 2u
#define SEND_BUFFER_MAX 1024
#define APPLICATION_DATA_MAX 8
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_es_filemanager_request_files(CuTest* tc);
static void apx_es_filemanager_enter_pending_mode_when_buffer_is_full(CuTest* tc);
static int32_t TestStub_getSendAvail(void *arg);
static uint8_t* TestStub_getSendBuffer(void *arg, int32_t msgLen);
static int32_t TestStub_send(void *arg, int32_t offset, int32_t msgLen);

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


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_apx_es_filemanager(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, apx_es_filemanager_request_files);
   SUITE_ADD_TEST(suite, apx_es_filemanager_enter_pending_mode_when_buffer_is_full);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

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
   CuAssertIntEquals(tc, RMF_INVALID_ADDRESS,file1.fileInfo.address);
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

   //test 6. remove until list is empty
   CuAssertIntEquals(tc, fileManager.numRequestedFiles, 2);
   CuAssertIntEquals(tc, 0, (int) apx_es_fileManager_removeRequestedAt(&fileManager,0));
   CuAssertIntEquals(tc, fileManager.numRequestedFiles, 1);
   CuAssertIntEquals(tc, 0, (int) apx_es_fileManager_removeRequestedAt(&fileManager,0));
   CuAssertIntEquals(tc, fileManager.numRequestedFiles, 0);
}


static void apx_es_filemanager_enter_pending_mode_when_buffer_is_full(CuTest* tc)
{
   apx_file_t file1;
   apx_nodeData_t node1;
   uint8_t data1[FILE1_SIZE];
   uint8_t flags1[FILE1_SIZE];
   uint32_t i;

   apx_transmitHandler_t transmitHandler;
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
   memset(&transmitHandler, 0, sizeof(transmitHandler));
   transmitHandler.getSendAvail = TestStub_getSendAvail;
   transmitHandler.getSendBuffer = TestStub_getSendBuffer;
   transmitHandler.send = TestStub_send;
   apx_es_fileManager_setTransmitHandler(&fileManager, &transmitHandler);
   m_test_send_avail = 2;
   apx_es_fileManager_attachLocalFile(&fileManager, &file1);
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
   //Make some buffer available
   m_test_send_avail  = 100;
   m_test_data_offset = 2;
   m_test_data_written=-1;
   apx_es_fileManager_run(&fileManager);
   CuAssertIntEquals(tc, 6, m_test_data_written);
   CuAssertUIntEquals(tc, 0, m_test_send_buffer[0]);
   CuAssertUIntEquals(tc, 2, m_test_send_buffer[1]);
   CuAssertUIntEquals(tc, 0x12, m_test_send_buffer[2]);
   CuAssertUIntEquals(tc, 0x34, m_test_send_buffer[3]);
   CuAssertUIntEquals(tc, 0x56, m_test_send_buffer[4]);
   CuAssertUIntEquals(tc, 0x78, m_test_send_buffer[5]);

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



