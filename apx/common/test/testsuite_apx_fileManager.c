//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "CuTest.h"
#include "apx_nodeData.h"
#include "apx_file.h"
#include "apx_fileManager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_fileManager_createInServerMode(CuTest* tc);
static void test_apx_fileManager_createInClientMode(CuTest* tc);
static void test_apx_fileManager_openRemoteFile_processRequest_fixedFileNoReadHandler(CuTest* tc);

//helper functions
static void attachApxClientFiles(CuTest* tc, apx_fileManager_t *manager, uint32_t definitionFileAddress);
static void receiveFileOpenRequest(CuTest *tc, apx_fileManager_t *manager, uint32_t fileAddress);

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

	SUITE_ADD_TEST(suite, test_apx_fileManager_createInServerMode);
	SUITE_ADD_TEST(suite, test_apx_fileManager_createInClientMode);
	SUITE_ADD_TEST(suite, test_apx_fileManager_openRemoteFile_processRequest_fixedFileNoReadHandler);

	return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_fileManager_createInServerMode(CuTest* tc)
{
	apx_fileManager_t manager;
	apx_fileManager_create(&manager, APX_FILEMANAGER_SERVER_MODE);
	apx_fileManager_destroy(&manager);
}
static void test_apx_fileManager_openRemoteFile_processRequest_fixedFileNoReadHandler(CuTest* tc)
{

	apx_fileManager_t manager;
	apx_fileManager_create(&manager, APX_FILEMANAGER_CLIENT_MODE);
	const uint32_t fileAddress = 0x10000;

	attachApxClientFiles(tc, &manager, fileAddress);
	receiveFileOpenRequest(tc, &manager, fileAddress);
	apx_fileManager_start(&manager);
	apx_fileManager_stop(&manager);
}

static void test_apx_fileManager_createInClientMode(CuTest* tc)
{
	apx_fileManager_t manager;
	apx_fileManager_create(&manager, APX_FILEMANAGER_CLIENT_MODE);
	apx_fileManager_destroy(&manager);
}
static void attachApxClientFiles(CuTest* tc, apx_fileManager_t *manager, uint32_t definitionFileAddress)
{
	rmf_fileInfo_t info;
	apx_file_t *localFile;
	uint32_t len = (uint32_t)strlen(m_TestNodeDefinition);

	rmf_fileInfo_create(&info, "TestNode.apx", definitionFileAddress, len, RMF_FILE_TYPE_FIXED);
	localFile = apx_file_newLocalFile(info.fileType, NULL);
	apx_fileManager_attachLocalDefinitionFile(manager, localFile);
}

static void receiveFileOpenRequest(CuTest *tc, apx_fileManager_t *manager, uint32_t fileAddress)
{
	uint8_t requestBuffer[100];
	int32_t msgLen;
	rmf_cmdOpenFile_t cmd;

	msgLen = rmf_packHeader(&requestBuffer[0], sizeof(requestBuffer), RMF_CMD_START_ADDR, false);
	cmd.address = fileAddress;
	msgLen += rmf_serialize_cmdOpenFile(&requestBuffer[msgLen], sizeof(requestBuffer) - msgLen, &cmd);
	CuAssertIntEquals(tc, msgLen, apx_fileManager_parseMessage(manager, &requestBuffer[0], msgLen));
}
