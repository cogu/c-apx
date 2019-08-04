//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "ApxNode_TestNode1.h"
#include "apx_fileManagerShared.h"
#include "apx_eventListener.h"
#include "adt_bytearray.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerShared_create(CuTest* tc);
static void test_apx_fileManagerShared_alloc(CuTest* tc);
static void test_apx_fileManagerShared_serializeFileInfo(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_fileManagerShared(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_fileManagerShared_create);
   SUITE_ADD_TEST(suite, test_apx_fileManagerShared_alloc);
   SUITE_ADD_TEST(suite, test_apx_fileManagerShared_serializeFileInfo);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerShared_create(CuTest* tc)
{
   apx_fileManagerShared_t data;
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_create(&data));
   CuAssertUIntEquals(tc, 0, data.fmid);

   CuAssertPtrEquals(tc, 0, data.fileCreated);
   CuAssertPtrEquals(tc, 0, data.sendFileInfo);
   CuAssertPtrEquals(tc, 0, data.sendFileOpen);
   CuAssertPtrEquals(tc, 0, data.openFileRequest);
   CuAssertTrue(tc, apx_allocator_isRunning(&data.allocator));
   apx_fileManagerShared_destroy(&data);
}

static void test_apx_fileManagerShared_alloc(CuTest* tc)
{
   apx_fileManagerShared_t data;
   uint8_t *ptr;
   size_t size;
   int i;
   apx_fileManagerShared_create(&data);
   //allocate small objects
   for(i=1;i<SOA_SMALL_OBJECT_MAX_SIZE;i++)
   {
      char msg[20];
      size = i;
      ptr = apx_fileManagerShared_alloc(&data, size);
      sprintf(msg, "size=%d", i);
      CuAssertPtrNotNullMsg(tc, msg, ptr);
      apx_fileManagerShared_free(&data, ptr, size);
   }
   //allocate some large objects
   size = 100;
   ptr = apx_fileManagerShared_alloc(&data, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_fileManagerShared_free(&data, ptr, size);
   size = 1000;
   ptr = apx_fileManagerShared_alloc(&data, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_fileManagerShared_free(&data, ptr, size);
   size = 10000;
   ptr = apx_fileManagerShared_alloc(&data, size);
   CuAssertPtrNotNull(tc, ptr);
   apx_fileManagerShared_free(&data, ptr, size);

   apx_fileManagerShared_destroy(&data);
}

static void test_apx_fileManagerShared_serializeFileInfo(CuTest *tc)
{
   apx_nodeData_t *nodeData;
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
