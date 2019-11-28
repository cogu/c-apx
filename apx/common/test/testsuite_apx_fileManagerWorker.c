//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string.h>
#include "CuTest.h"
#include "apx_fileManagerWorker.h"
#include "apx_fileManagerSharedSpy.h"
#include "apx_fileMap.h"
#include "rmf.h"
#include "apx_file2.h"
#include "adt_bytearray.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct fileManagerRemoteSpy_tag
{
   int32_t fileCreatedByRemoteCalls;
   void *arg;
   apx_file2_t file;
}fileManagerRemoteSpy_t;
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerWorker_create(CuTest* tc);
//static void test_apx_fileManagerWorker_processFileInfo(CuTest* tc);
//static void test_apx_fileManagerWorker_processFileOpenRequest(CuTest* tc);
//static void test_apx_fileManagerWorker_serializeFileInfo(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_fileManagerWorker(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_fileManagerWorker_create);
   //SUITE_ADD_TEST(suite, test_apx_fileManagerWorker_processFileInfo);
   //SUITE_ADD_TEST(suite, test_apx_fileManagerWorker_processFileOpenRequest);
//   SUITE_ADD_TEST(suite, test_apx_fileManagerWorker_serializeFileInfo);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerWorker_create(CuTest* tc)
{
   apx_fileManagerWorker_t remote;
   apx_fileManagerShared2_t shared;
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerShared2_create(&shared));
   apx_fileManagerWorker_create(&remote, &shared, APX_SERVER_MODE);
   apx_fileManagerWorker_destroy(&remote);
   apx_fileManagerShared2_destroy(&shared);
}

/*
static void test_apx_fileManagerWorker_processFileInfo(CuTest* tc)
{
   apx_fileManagerWorker_t remote;
   apx_fileManagerShared2_t shared;
   apx_fileManagerSharedSpy_t *spy;
   uint8_t buffer[100];
   rmf_fileInfo_t info;
   int32_t msgLen;
   spy = apx_fileManagerSharedSpy_new();
   CuAssertPtrNotNull(tc, spy);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerShared2_create(&shared));
   apx_fileManagerWorker_create(&remote, &shared, APX_SERVER_MODE);

   shared.remoteFileCreated = apx_fileManagerSharedSpy_remoteFileCreated;
   shared.arg = spy;

   rmf_fileInfo_create(&info, "test.apx", 0x10000, 100, RMF_FILE_TYPE_FIXED);
   msgLen = rmf_packHeader(&buffer[0], sizeof(buffer), RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdFileInfo(&buffer[msgLen], sizeof(buffer)-msgLen, &info);

   CuAssertIntEquals(tc, 0, spy->numRemoteFileCreatedCalls);
   CuAssertIntEquals(tc, 0, apx_fileManagerShared2_getNumRemoteFiles(&shared));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerWorker_processMessage(&remote, &buffer[0], msgLen));
   CuAssertIntEquals(tc, 1, spy->numRemoteFileCreatedCalls);
   CuAssertIntEquals(tc, 1, apx_fileManagerShared2_getNumRemoteFiles(&shared));

   apx_fileManagerWorker_destroy(&remote);
   apx_fileManagerShared2_destroy(&shared);
   apx_fileManagerSharedSpy_delete(spy);
}

static void test_apx_fileManagerWorker_processFileOpenRequest(CuTest* tc)
{
   apx_fileManagerWorker_t remote;
   apx_fileManagerShared2_t shared;
   apx_fileManagerSharedSpy_t *spy;
   uint8_t buffer[100];
   rmf_cmdOpenFile_t cmd;
   int32_t msgLen;
   apx_fileInfo_t fileInfo;
   spy = apx_fileManagerSharedSpy_new();
   CuAssertPtrNotNull(tc, spy);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerShared2_create(&shared));
   apx_fileManagerWorker_create(&remote, &shared, APX_SERVER_MODE);
   apx_fileInfo_create(&fileInfo, RMF_INVALID_ADDRESS, 860, "TestNode1.out", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, (const uint8_t*) 0);
   apx_fileManagerShared2_createLocalFile(&shared, &fileInfo);
   apx_fileInfo_destroy(&fileInfo);
   shared.fileOpenRequested = apx_fileManagerSharedSpy_fileOpenRequested;
   shared.arg = spy;
   cmd.address = 0;
   msgLen = rmf_packHeader(&buffer[0], sizeof(buffer), RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdOpenFile(&buffer[msgLen], sizeof(buffer)-msgLen, &cmd);

   CuAssertIntEquals(tc, 1, apx_fileManagerShared2_getNumLocalFiles(&shared));

   CuAssertIntEquals(tc, 0, spy->numfileOpenRequestCalls);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_fileManagerWorker_processMessage(&remote, &buffer[0], msgLen));
   CuAssertIntEquals(tc, 1, spy->numfileOpenRequestCalls);


   apx_fileManagerWorker_destroy(&remote);
   apx_fileManagerShared2_destroy(&shared);
   apx_fileManagerSharedSpy_delete(spy);
}
*/

/*
static void test_apx_fileManagerWorker_serializeFileInfo(CuTest *tc)
{
   apx_nodeData2_t *nodeData;
   apx_file2_t *definitionFile;
   apx_file2_t *outDataFile;
   adt_bytearray_t bytearray;
   int32_t bufLen;
   uint8_t *bufData;
   int32_t i;
   const uint8_t expected1[66] = {
         0xBF, //address+high bit
         0xFF, //address
         0xFC, //address
         0x00, //address
         0x03, //cmdType
         0x00, //cmdType
         0x00, //cmdType
         0x00, //cmdType
         0x00, //address
         0x00, //address
         0x00, //address
         0x00, //address
         0x03, //length
         0x00, //length
         0x00, //length
         0x00, //length
         0x00, //fileType
         0x00, //fileType
         0x00, //digestType
         0x00, //digestType
         0x00, //digestData[0]
         0x00, //digestData[1]
         0x00, //digestData[2]
         0x00, //digestData[3]
         0x00, //digestData[4]
         0x00, //digestData[5]
         0x00, //digestData[6]
         0x00, //digestData[7]
         0x00, //digestData[8]
         0x00, //digestData[9]
         0x00, //digestData[10]
         0x00, //digestData[11]
         0x00, //digestData[12]
         0x00, //digestData[13]
         0x00, //digestData[14]
         0x00, //digestData[15]
         0x00, //digestData[16]
         0x00, //digestData[17]
         0x00, //digestData[18]
         0x00, //digestData[19]
         0x00, //digestData[20]
         0x00, //digestData[21]
         0x00, //digestData[22]
         0x00, //digestData[23]
         0x00, //digestData[24]
         0x00, //digestData[25]
         0x00, //digestData[26]
         0x00, //digestData[27]
         0x00, //digestData[28]
         0x00, //digestData[29]
         0x00, //digestData[30]
         0x00, //digestData[31]
         'T',  //filename
         'e',  //filename
         's',  //filename
         't',  //filename
         'N',  //filename
         'o',  //filename
         'd',  //filename
         'e',  //filename
         '1',  //filename
         '.',  //filename
         'o',  //filename
         'u',  //filename
         't',  //filename
         '\0',  //filename
   };
   const uint8_t expected2[66] = {
         0xBF, //address+high bit
         0xFF, //address
         0xFC, //address
         0x00, //address
         0x03, //cmdType
         0x00, //cmdType
         0x00, //cmdType
         0x00, //cmdType
         0x00, //address
         0x00, //address
         0x00, //address
         0x04, //address
         126,  //length
         0x00, //length
         0x00, //length
         0x00, //length
         0x00, //fileType
         0x00, //fileType
         0x00, //digestType
         0x00, //digestType
         0x00, //digestData[0]
         0x00, //digestData[1]
         0x00, //digestData[2]
         0x00, //digestData[3]
         0x00, //digestData[4]
         0x00, //digestData[5]
         0x00, //digestData[6]
         0x00, //digestData[7]
         0x00, //digestData[8]
         0x00, //digestData[9]
         0x00, //digestData[10]
         0x00, //digestData[11]
         0x00, //digestData[12]
         0x00, //digestData[13]
         0x00, //digestData[14]
         0x00, //digestData[15]
         0x00, //digestData[16]
         0x00, //digestData[17]
         0x00, //digestData[18]
         0x00, //digestData[19]
         0x00, //digestData[20]
         0x00, //digestData[21]
         0x00, //digestData[22]
         0x00, //digestData[23]
         0x00, //digestData[24]
         0x00, //digestData[25]
         0x00, //digestData[26]
         0x00, //digestData[27]
         0x00, //digestData[28]
         0x00, //digestData[29]
         0x00, //digestData[30]
         0x00, //digestData[31]
         'T',  //filename
         'e',  //filename
         's',  //filename
         't',  //filename
         'N',  //filename
         'o',  //filename
         'd',  //filename
         'e',  //filename
         '1',  //filename
         '.',  //filename
         'a',  //filename
         'p',  //filename
         'x',  //filename
         '\0',  //filename
   };
   ApxNode_Init_TestNode1();
   nodeData = ApxNode_GetNodeData_TestNode1();
   definitionFile = apx_nodeData_newLocalDefinitionFile(nodeData);
   outDataFile = apx_nodeData_newLocalOutPortDataFile(nodeData);
   outDataFile->fileInfo.address = 0;
   definitionFile->fileInfo.address = 0x4000000;
   adt_bytearray_create(&bytearray, ADT_BYTE_ARRAY_DEFAULT_GROW_SIZE);
   CuAssertIntEquals(tc, 0, adt_bytearray_resize(&bytearray, RMF_CMD_FILE_INFO_MAX_SIZE*2+8));
   bufLen = adt_bytearray_length(&bytearray);
   bufData = adt_bytearray_data(&bytearray);
   CuAssertIntEquals(tc, 66, apx_fileManagerShared_serializeFileInfo(bufData, bufLen, &outDataFile->fileInfo));
   for(i=0;i<66;i++)
   {
      char msg[14];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected1[i], bufData[i]);
   }
   CuAssertIntEquals(tc, 66, apx_fileManagerShared_serializeFileInfo(bufData, bufLen, &definitionFile->fileInfo));
   for(i=0;i<66;i++)
   {
      char msg[14];
      sprintf(msg, "i=%d",i);
      CuAssertIntEquals_Msg(tc, msg, expected2[i], bufData[i]);
   }
   adt_bytearray_destroy(&bytearray);
   apx_file2_delete(definitionFile);
   apx_file2_delete(outDataFile);
}
*/
