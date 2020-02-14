/*****************************************************************************
* \file      testsuite_apx_file2.c
* \author    Conny Gustafsson
* \date      2018-08-31
* \brief     Description
*
* Copyright (c) 2018 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "apx_file2.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_file_create_local(CuTest* tc);
static void test_apx_file_create_remote(CuTest* tc);
static void test_apx_file_newLocal(CuTest* tc);
static void test_apx_file_newRemote(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_file2(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_file_create_local);
   SUITE_ADD_TEST(suite, test_apx_file_create_remote);
   SUITE_ADD_TEST(suite, test_apx_file_newLocal);
   SUITE_ADD_TEST(suite, test_apx_file_newRemote);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_file_create_local(CuTest* tc)
{
   apx_file_t file;
   apx_fileInfo_t info;
   apx_fileInfo_create(&info, APX_ADDRESS_DEFINITION_START, 100, "test.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);

   apx_file_create(&file, &info);

   CuAssertTrue(tc, apx_file_isOpen(&file)==false);
   CuAssertTrue(tc, apx_file_isRemoteFile(&file)==false);
   CuAssertStrEquals(tc, "test.apx", file.fileInfo.name);
   CuAssertIntEquals(tc, 100, file.fileInfo.length);
   CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, file.fileInfo.fileType);
   CuAssertIntEquals(tc, APX_DEFINITION_FILE_TYPE, apx_file_getApxFileType(&file));
   CuAssertPtrEquals(tc, 0, file.notificationHandler.arg);
   CuAssertPtrEquals(tc, 0, file.notificationHandler.openNotify);
   CuAssertPtrEquals(tc, 0, file.notificationHandler.writeNotify);
   apx_file_destroy(&file);
   apx_fileInfo_destroy(&info);
}

static void test_apx_file_create_remote(CuTest* tc)
{
   apx_file_t file;
   apx_fileInfo_t info;
   apx_fileInfo_create(&info, APX_ADDRESS_DEFINITION_START | RMF_REMOTE_ADDRESS_BIT, 100, "test.apx", RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);

   apx_file_create(&file, &info);

   CuAssertTrue(tc, apx_file_isOpen(&file)==false);
   CuAssertTrue(tc, apx_file_isRemoteFile(&file)==true);
   CuAssertStrEquals(tc, "test.apx", file.fileInfo.name);
   CuAssertIntEquals(tc, 100, file.fileInfo.length);
   CuAssertUIntEquals(tc, RMF_FILE_TYPE_FIXED, file.fileInfo.fileType);
   CuAssertIntEquals(tc, APX_DEFINITION_FILE_TYPE, apx_file_getApxFileType(&file));
   CuAssertPtrEquals(tc, 0, file.notificationHandler.arg);
   CuAssertPtrEquals(tc, 0, file.notificationHandler.openNotify);
   CuAssertPtrEquals(tc, 0, file.notificationHandler.writeNotify);
   apx_file_destroy(&file);
   apx_fileInfo_destroy(&info);
}

static void test_apx_file_newLocal(CuTest* tc)
{
   apx_file_t *file1;
   rmf_fileInfo_t info1;
   rmf_fileInfo_create(&info1, "test.apx", 0, 100, RMF_FILE_TYPE_FIXED);

   file1 = apx_file_newLocal(&info1);

   CuAssertTrue(tc, apx_file_isOpen(file1)==false);
   CuAssertTrue(tc, apx_file_isRemoteFile(file1)==false);


   apx_file_delete(file1);
}

static void test_apx_file_newRemote(CuTest* tc)
{
   apx_file_t *file1;
   rmf_fileInfo_t info1;
   rmf_fileInfo_create(&info1, "test.apx", 0, 100, RMF_FILE_TYPE_FIXED);

   file1 = apx_file_newRemote(&info1);

   CuAssertTrue(tc, apx_file_isOpen(file1)==false);
   CuAssertTrue(tc, apx_file_isRemoteFile(file1)==true);

   apx_file_delete(file1);
}
