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
#include "apx_fileManagerRemote.h"
#include "apx_fileManagerSharedSpy.h"
#include "apx_fileMap.h"
#include "rmf.h"
#include "apx_file2.h"
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

static void test_apx_fileManagerRemote_create(CuTest* tc);
static void test_apx_fileManagerRemote_processFileInfo(CuTest* tc);
static void test_apx_fileManagerRemote_processFileOpenRequest(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_fileManagerRemote(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_fileManagerRemote_create);
   SUITE_ADD_TEST(suite, test_apx_fileManagerRemote_processFileInfo);
   SUITE_ADD_TEST(suite, test_apx_fileManagerRemote_processFileOpenRequest);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManagerRemote_create(CuTest* tc)
{
   apx_fileManagerRemote_t remote;
   apx_fileManagerShared_t shared;
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_create(&shared));
   apx_fileManagerRemote_create(&remote, &shared);
   CuAssertIntEquals(tc, 0, apx_fileMap_length(&remote.remoteFileMap));
   apx_fileManagerRemote_destroy(&remote);
   apx_fileManagerShared_destroy(&shared);
}

static void test_apx_fileManagerRemote_processFileInfo(CuTest* tc)
{
   apx_fileManagerRemote_t remote;
   apx_fileManagerShared_t shared;
   apx_fileManagerSharedSpy_t *spy;
   uint8_t buffer[100];
   rmf_fileInfo_t info;
   int32_t msgLen;
   spy = apx_fileManagerSharedSpy_new();
   CuAssertPtrNotNull(tc, spy);
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_create(&shared));
   apx_fileManagerRemote_create(&remote, &shared);
   shared.fileCreated = apx_fileManagerSharedSpy_fileCreated;
   shared.arg = spy;

   rmf_fileInfo_create(&info, "test.apx", 0x10000, 100, RMF_FILE_TYPE_FIXED);
   msgLen = rmf_packHeader(&buffer[0], sizeof(buffer), RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdFileInfo(&buffer[msgLen], sizeof(buffer)-msgLen, &info);

   CuAssertIntEquals(tc, 0, spy->numFileCreatedCalls);
   CuAssertIntEquals(tc, 0, apx_fileMap_length(&remote.remoteFileMap));
   CuAssertIntEquals(tc, msgLen, apx_fileManagerRemote_processMessage(&remote, &buffer[0], msgLen));
   CuAssertIntEquals(tc, 1, spy->numFileCreatedCalls);
   CuAssertIntEquals(tc, 1, apx_fileMap_length(&remote.remoteFileMap));

   apx_fileManagerRemote_destroy(&remote);
   apx_fileManagerShared_destroy(&shared);
   apx_fileManagerSharedSpy_delete(spy);
}

static void test_apx_fileManagerRemote_processFileOpenRequest(CuTest* tc)
{
   apx_fileManagerRemote_t remote;
   apx_fileManagerShared_t shared;
   apx_fileManagerSharedSpy_t *spy;
   uint8_t buffer[100];
   rmf_cmdOpenFile_t cmd;
   int32_t msgLen;
   spy = apx_fileManagerSharedSpy_new();
   CuAssertPtrNotNull(tc, spy);
   CuAssertIntEquals(tc, 0, apx_fileManagerShared_create(&shared));
   apx_fileManagerRemote_create(&remote, &shared);
   shared.openFileRequest = apx_fileManagerSharedSpy_openFileRequest;
   shared.arg = spy;
   cmd.address = 0x4000;
   msgLen = rmf_packHeader(&buffer[0], sizeof(buffer), RMF_CMD_START_ADDR, false);
   msgLen += rmf_serialize_cmdOpenFile(&buffer[msgLen], sizeof(buffer)-msgLen, &cmd);

   CuAssertIntEquals(tc, 0, spy->numOpenFileRequestCalls);

   CuAssertIntEquals(tc, msgLen, apx_fileManagerRemote_processMessage(&remote, &buffer[0], msgLen));
   CuAssertIntEquals(tc, 1, spy->numOpenFileRequestCalls);

   apx_fileManagerRemote_destroy(&remote);
   apx_fileManagerShared_destroy(&shared);
   apx_fileManagerSharedSpy_delete(spy);
}
