/*****************************************************************************
* \file      testsuite_apx_nodeDataManager.c
* \author    Conny Gustafsson
* \date      2018-09-03
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
#include "apx_nodeDataManager.h"
#include "ApxNode_TestNode1.h"
#include "ApxNode_TestNode2.h"
#include "apx_test_nodes.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeDataManager_create(CuTest *tc);
static void test_apx_nodeDataManager_fromValidString1(CuTest *tc);
static void test_apx_nodeDataManager_fromValidString2(CuTest *tc);
static void test_apx_nodeDataManager_fromInvalidString1(CuTest *tc);
static void test_apx_nodeDataManager_fromInvalidString2(CuTest *tc);
static void test_apx_nodeDataManager_verifyStaticNode(CuTest *tc);
static void test_apx_nodeDataManager_attachStaticNode(CuTest *tc);
static void test_apx_nodeDataManager_attachStaticNodeProtectFromDuplicates(CuTest *tc);
static void test_apx_nodeDataManager_attachMultipleStaticNodes(CuTest *tc);
static void test_apx_nodeDataManager_verifyInitValueWhenNodeIsCreated(CuTest *tc);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeDataManager(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_create);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_fromValidString1);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_fromValidString2);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_fromInvalidString1);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_fromInvalidString2);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_verifyStaticNode);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_attachStaticNode);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_attachStaticNodeProtectFromDuplicates);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_attachMultipleStaticNodes);
   SUITE_ADD_TEST(suite, test_apx_nodeDataManager_verifyInitValueWhenNodeIsCreated);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeDataManager_create(CuTest* tc)
{
   apx_nodeDataManager_t manager;

   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);

   apx_nodeDataManager_destroy(&manager);
}

static void test_apx_nodeDataManager_fromValidString1(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData;
   uint32_t definitionLen;
   const char *testDefinition=
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "P\"OutPort1\"C(0,1):=0\n";
   definitionLen = (uint32_t) strlen(testDefinition);
   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);
   nodeData = apx_nodeData_new(definitionLen);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertPtrNotNull(tc, nodeData->definitionDataBuf);
   CuAssertUIntEquals(tc, definitionLen, nodeData->definitionDataLen);
   CuAssertPtrEquals(tc, NULL, nodeData->outPortDataBuf);
   CuAssertUIntEquals(tc, 0, nodeData->outPortDataLen);
   CuAssertPtrEquals(tc, NULL, nodeData->inPortDataBuf);
   CuAssertUIntEquals(tc, 0, nodeData->inPortDataLen);
   CuAssertIntEquals(tc, 0, apx_nodeData_writeDefinitionData(nodeData, (const uint8_t*) testDefinition, 0, definitionLen));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeDataManager_parseDefinition(&manager, nodeData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataBuffers(nodeData));
   CuAssertPtrNotNull(tc, nodeData->outPortDataBuf);
   CuAssertUIntEquals(tc, 1, nodeData->outPortDataLen);
   CuAssertPtrEquals(tc, NULL, nodeData->inPortDataBuf);
   CuAssertUIntEquals(tc, 0, nodeData->inPortDataLen);

   apx_nodeData_delete(nodeData);
   apx_nodeDataManager_destroy(&manager);
}


static void test_apx_nodeDataManager_fromValidString2(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData;
   uint32_t definitionLen;
   const char *testDefinition=
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"InPort1\"C(0,1):=0\n";
   definitionLen = (uint32_t) strlen(testDefinition);
   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);

   nodeData = apx_nodeData_new(definitionLen);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertPtrNotNull(tc, nodeData->definitionDataBuf);
   CuAssertUIntEquals(tc, definitionLen, nodeData->definitionDataLen);
   CuAssertPtrEquals(tc, NULL, nodeData->outPortDataBuf);
   CuAssertUIntEquals(tc, 0, nodeData->outPortDataLen);
   CuAssertPtrEquals(tc, NULL, nodeData->inPortDataBuf);
   CuAssertUIntEquals(tc, 0, nodeData->inPortDataLen);
   CuAssertIntEquals(tc, 0, apx_nodeData_writeDefinitionData(nodeData, (const uint8_t*) testDefinition, 0, definitionLen));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeDataManager_parseDefinition(&manager, nodeData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataBuffers(nodeData));
   CuAssertPtrEquals(tc, 0, nodeData->outPortDataBuf);
   CuAssertUIntEquals(tc, 0, nodeData->outPortDataLen);
   CuAssertPtrNotNull(tc, nodeData->inPortDataBuf);
   CuAssertUIntEquals(tc, 1, nodeData->inPortDataLen);

   CuAssertPtrNotNull(tc, nodeData);
   apx_nodeData_delete(nodeData);
   apx_nodeDataManager_destroy(&manager);
}


static void test_apx_nodeDataManager_fromInvalidString1(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData;
   uint32_t definitionLen;
   const char *testDefinition=
         "APX/1.2\n"
         "N\"TestNode\"" //missing the new-line character here
         "P\"OutPort1\"C(0,1):=0\n";
   definitionLen = (uint32_t) strlen(testDefinition);
   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);

   nodeData = apx_nodeData_new(definitionLen);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertIntEquals(tc, 0, apx_nodeData_writeDefinitionData(nodeData, (const uint8_t*) testDefinition, 0, definitionLen));
   CuAssertIntEquals(tc, APX_PARSE_ERROR, apx_nodeDataManager_parseDefinition(&manager, nodeData));
   CuAssertIntEquals(tc, 2, apx_nodeDataManager_getErrorLine(&manager));

   apx_nodeData_delete(nodeData);
   apx_nodeDataManager_destroy(&manager);

}

static void test_apx_nodeDataManager_fromInvalidString2(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData;
   uint32_t definitionLen;
   const char *testDefinition=
         "APX/1.2\n"
         "N\"TestNode\"" //missing the new-line character here
         "R\"OutPort1\"a[10]:\n";
   definitionLen = (uint32_t) strlen(testDefinition);
   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);

   nodeData = apx_nodeData_new(definitionLen);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertIntEquals(tc, 0, apx_nodeData_writeDefinitionData(nodeData, (const uint8_t*) testDefinition, 0, definitionLen));
   CuAssertIntEquals(tc, APX_PARSE_ERROR, apx_nodeDataManager_parseDefinition(&manager, nodeData));
   CuAssertIntEquals(tc, 2, apx_nodeDataManager_getErrorLine(&manager));

   apx_nodeData_delete(nodeData);
   apx_nodeDataManager_destroy(&manager);

}

static void test_apx_nodeDataManager_verifyStaticNode(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData;

   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);

   ApxNode_Init_TestNode1();
   nodeData = ApxNode_GetNodeData_TestNode1();

   CuAssertPtrNotNull(tc, nodeData);
   CuAssertTrue(tc, nodeData->isWeakref);
   CuAssertPtrNotNull(tc, nodeData->definitionDataBuf);
   CuAssertUIntEquals(tc, 126u, nodeData->definitionDataLen);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeDataManager_parseDefinition(&manager, nodeData));

   apx_nodeData_destroy(nodeData);
   apx_nodeDataManager_destroy(&manager);
}

static void test_apx_nodeDataManager_attachStaticNode(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData;
   const char *nodeName;

   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);

   ApxNode_Init_TestNode1();
   nodeData = ApxNode_GetNodeData_TestNode1();
   nodeName = apx_nodeData_getName(nodeData);

   CuAssertPtrEquals(tc, NULL, apx_nodeDataManager_find(&manager, nodeName));
   CuAssertIntEquals(tc, APX_NO_ERROR,apx_nodeDataManager_attach(&manager, nodeData));
   CuAssertPtrEquals(tc, nodeData, apx_nodeDataManager_find(&manager, nodeName));

   apx_nodeDataManager_destroy(&manager);

}

static void test_apx_nodeDataManager_attachStaticNodeProtectFromDuplicates(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData;
   const char *nodeName;

   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);

   ApxNode_Init_TestNode1();
   nodeData = ApxNode_GetNodeData_TestNode1();
   nodeName = apx_nodeData_getName(nodeData);

   CuAssertPtrEquals(tc, NULL, apx_nodeDataManager_find(&manager, nodeName));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeDataManager_attach(&manager, nodeData));
   CuAssertIntEquals(tc, APX_NODE_ALREADY_EXISTS_ERROR, apx_nodeDataManager_attach(&manager, nodeData));
   CuAssertPtrEquals(tc, nodeData, apx_nodeDataManager_find(&manager, nodeName));

   apx_nodeDataManager_destroy(&manager);

}

static void test_apx_nodeDataManager_attachMultipleStaticNodes(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData1;
   apx_nodeData_t *nodeData2;
   const char *nodeName1;
   const char *nodeName2;

   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);

   ApxNode_Init_TestNode1();
   ApxNode_Init_TestNode2();
   nodeData1 = ApxNode_GetNodeData_TestNode1();
   nodeData2 = ApxNode_GetNodeData_TestNode2();
   nodeName1 = apx_nodeData_getName(nodeData1);
   nodeName2 = apx_nodeData_getName(nodeData2);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeDataManager_attach(&manager, nodeData1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeDataManager_attach(&manager, nodeData2));
   CuAssertPtrEquals(tc, nodeData1, apx_nodeDataManager_find(&manager, nodeName1));
   CuAssertPtrEquals(tc, nodeData2, apx_nodeDataManager_find(&manager, nodeName2));

   apx_nodeDataManager_destroy(&manager);
}

static void test_apx_nodeDataManager_verifyInitValueWhenNodeIsCreated(CuTest *tc)
{
   apx_nodeDataManager_t manager;
   apx_nodeData_t *nodeData;
   const char *testDefinition= "APX/1.2\n"
                 "N\"TestNode\"\n"
                 "T\"VehicleSpeed_T\"S\n"
                 "R\"VehicleSpeed\"T[0]:=65535\n";
   apx_nodeDataManager_create(&manager, APX_SERVER_MODE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeDataManager_attachFromString(&manager, testDefinition));
   nodeData = apx_nodeDataManager_getLastAttached(&manager);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertIntEquals(tc, 2, nodeData->inPortDataLen);
   CuAssertPtrNotNull(tc, nodeData->inPortDataBuf);
   CuAssertUIntEquals(tc, 0xFF, nodeData->inPortDataBuf[0]);
   CuAssertUIntEquals(tc, 0xFF, nodeData->inPortDataBuf[1]);

   apx_nodeDataManager_destroy(&manager);

}
