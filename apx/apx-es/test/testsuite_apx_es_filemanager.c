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
#define APX_FILE_MANAGER_MAX_PENDING_MESSAGE_DATA 4096
#define APX_FILE_MANAGER_MSG_QUEUE_SIZE (sizeof(apx_msg_t)*APX_FILE_MANAGER_MAX_NUM_MESSAGES)
#define TRANSMIT_BUF_SIZE 4096
#define SEND_MIN_TRESHOLD 100 //at least this many bytes must be available in IPC channel before sending any new data
#define NUMHEADER16_MAX_LEN 2u
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_es_filemanager_request_files(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testsuite_apx_es_filemanager(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, apx_es_filemanager_request_files);

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
   uint8_t pendingMessageDataBuf[APX_FILE_MANAGER_MAX_PENDING_MESSAGE_DATA];
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
   rc = apx_es_fileManager_create(&fileManager,messageQueueBuf,APX_FILE_MANAGER_MAX_NUM_MESSAGES,pendingMessageDataBuf,APX_FILE_MANAGER_MAX_PENDING_MESSAGE_DATA);
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


