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
static void test_rmf_cmdFileInfo_serialize(CuTest* tc);
static void test_rmf_cmdOpenFile_serialize(CuTest* tc);
static void test_rmf_cmdCloseFile_serialize(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_remotefile(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_rmf_cmdFileInfo_serialize);
   SUITE_ADD_TEST(suite, test_rmf_cmdOpenFile_serialize);
   SUITE_ADD_TEST(suite, test_rmf_cmdCloseFile_serialize);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_rmf_cmdFileInfo_serialize(CuTest* tc)
{
   uint8_t buf[RMF_MAX_CMD_BUF_SIZE];
   uint8_t *p;
   int32_t bufLen = (int32_t) sizeof(buf);
   rmf_fileInfo_t cmdFileInfo;
   rmf_fileInfo_t cmdFileInfo2;
   int32_t result;
   cmdFileInfo.address = 50;
   cmdFileInfo.length = 10;
   cmdFileInfo.fileType = RMF_FILE_TYPE_FIXED;
   cmdFileInfo.digestType = RMF_DIGEST_TYPE_NONE;
   memset(&cmdFileInfo.digestData[0],0,RMF_DIGEST_SIZE);
   strcpy(&cmdFileInfo.name[0],"test.data");
   result = rmf_serialize_cmdFileInfo(buf,bufLen,&cmdFileInfo);
   CuAssertIntEquals(tc,58,result);
   p=buf;
   CuAssertUIntEquals(tc,RMF_CMD_FILE_INFO,unpackLE(p,4)); p+=4;
   CuAssertUIntEquals(tc,cmdFileInfo.address,unpackLE(p,4)); p+=4;
   CuAssertUIntEquals(tc,cmdFileInfo.length,unpackLE(p,4)); p+=4;
   CuAssertUIntEquals(tc,cmdFileInfo.fileType,unpackLE(p,2)); p+=2;
   CuAssertUIntEquals(tc,cmdFileInfo.digestType,unpackLE(p,2)); p+=2;
   p+=RMF_DIGEST_SIZE;
   CuAssertStrEquals(tc,&cmdFileInfo.name[0],(char*) p);

   CuAssertIntEquals(tc, RMF_CMD_FILE_INFO_BASE_SIZE-RMF_CMD_TYPE_LEN+strlen("test.data")+1, rmf_deserialize_cmdFileInfo(buf+RMF_CMD_TYPE_LEN,result-RMF_CMD_TYPE_LEN,&cmdFileInfo2));
   CuAssertUIntEquals(tc,cmdFileInfo.address,cmdFileInfo2.address);
   CuAssertUIntEquals(tc,cmdFileInfo.length,cmdFileInfo2.length);
   CuAssertUIntEquals(tc,cmdFileInfo.fileType,cmdFileInfo2.fileType);
   CuAssertUIntEquals(tc,cmdFileInfo.digestType,cmdFileInfo2.digestType);
   CuAssertStrEquals(tc,cmdFileInfo.name,cmdFileInfo2.name);

   CuAssertIntEquals(tc,58,result);

}


static void test_rmf_cmdOpenFile_serialize(CuTest* tc)
{
   uint8_t buf[RMF_MAX_CMD_BUF_SIZE];
   uint8_t *p;
   int32_t bufLen = (int32_t) sizeof(buf);
   rmf_cmdOpenFile_t cmdOpenFile;
   rmf_cmdOpenFile_t cmdOpenFile2;
   int32_t result;
   cmdOpenFile.address = 12345678;

   result = rmf_serialize_cmdOpenFile(buf, bufLen, &cmdOpenFile);
   CuAssertIntEquals(tc,8,result);
   p=buf;
   CuAssertUIntEquals(tc,RMF_CMD_FILE_OPEN,unpackLE(p,4)); p+=4;
   CuAssertUIntEquals(tc,cmdOpenFile.address,unpackLE(p,4)); p+=4;
   result = rmf_deserialize_cmdOpenFile(buf+RMF_CMD_TYPE_LEN,result-RMF_CMD_TYPE_LEN,&cmdOpenFile2);
   CuAssertIntEquals(tc, RMF_CMD_ADDRESS_LEN, result);
   CuAssertUIntEquals(tc, cmdOpenFile.address, cmdOpenFile2.address);
}

static void test_rmf_cmdCloseFile_serialize(CuTest* tc)
{
   uint8_t buf[RMF_MAX_CMD_BUF_SIZE];
   uint8_t *p;
   int32_t bufLen = (int32_t) sizeof(buf);
   rmf_cmdCloseFile_t cmd;
   rmf_cmdCloseFile_t cmd2;
   int32_t result;
   cmd.address = 12345678;

   result = rmf_serialize_cmdCloseFile(buf, bufLen, &cmd);
   CuAssertIntEquals(tc,8,result);
   p=buf;
   CuAssertUIntEquals(tc,RMF_CMD_FILE_CLOSE,unpackLE(p,4)); p+=4;
   CuAssertUIntEquals(tc,cmd.address,unpackLE(p,4)); p+=4;
   result = rmf_deserialize_cmdCloseFile(buf + RMF_CMD_TYPE_LEN, result - RMF_CMD_TYPE_LEN, &cmd2);
   CuAssertIntEquals(tc, RMF_CMD_ADDRESS_LEN, result);
   CuAssertUIntEquals(tc, cmd.address, cmd2.address);
}
