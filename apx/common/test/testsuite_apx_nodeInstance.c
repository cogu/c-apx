/*****************************************************************************
* \file      testsuite_apx_nodeInstance.c
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Unit tests for apx_nodeInstance
*
* Copyright (c) 2019 Conny Gustafsson
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
#include "apx_parser.h"
#include "apx_nodeInstance.h"
#include "rmf.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define RMF_DEFINITION_START_ADDRESS 0x10000

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeInstance_manuallyCreateClientNodeUsingAPI(CuTest *tc);
static void test_apx_nodeInstance_manuallyCreateServerNodeUsingAPI(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeInstance(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeInstance_manuallyCreateClientNodeUsingAPI);
   SUITE_ADD_TEST(suite, test_apx_nodeInstance_manuallyCreateServerNodeUsingAPI);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeInstance_manuallyCreateClientNodeUsingAPI(CuTest *tc)
{
   const char *apx_text = "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"TestPort\"C:=255\n";

   apx_nodeData_t *nodeData;
   apx_nodeInstance_t *inst;
   apx_node_t *node;
   apx_programType_t errProgramType;
   apx_uniquePortId_t errPortId;
   apx_nodeInfo_t *nodeInfo;
   apx_parser_t *parser = apx_parser_new();
   apx_size_t apx_len = (apx_size_t) strlen(apx_text);

   inst = apx_nodeInstance_new(APX_CLIENT_MODE);
   CuAssertPtrNotNull(tc, inst);
   nodeData = apx_nodeInstance_getNodeData(inst);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createDefinitionBuffer(nodeData, apx_len ));
   CuAssertUIntEquals(tc, (apx_size_t) strlen(apx_text), nodeData->definitionDataLen);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_writeDefinitionData(nodeData, (const uint8_t*) apx_text, 0u, apx_len));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_parseDefinition(inst, parser));
   node = apx_nodeInstance_getParseTree(inst);
   CuAssertPtrNotNull(tc, node);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildNodeInfo(inst, &errProgramType, &errPortId));
   nodeInfo = apx_nodeInstane_getNodeInfo(inst);
   CuAssertPtrNotNull(tc, nodeInfo);
   apx_nodeInstance_delete(inst);
   apx_parser_delete(parser);
}

static void test_apx_nodeInstance_manuallyCreateServerNodeUsingAPI(CuTest *tc)
{
   const char *apx_text = "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"TestPort\"C:=255\n";

   apx_nodeInstance_t *inst;
   apx_programType_t errProgramType;
   apx_uniquePortId_t errPortId;
   apx_nodeInfo_t *nodeInfo;
   apx_parser_t *parser = apx_parser_new();
   apx_size_t apx_len = (apx_size_t) strlen(apx_text);

   inst = apx_nodeInstance_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, inst);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_createDefinitionBuffer(inst, apx_len));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeDefinitionData(inst, (const uint8_t*) apx_text, 0u, apx_len));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_parseDefinition(inst, parser));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getParseTree(inst));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildNodeInfo(inst, &errProgramType, &errPortId));
   nodeInfo = apx_nodeInstane_getNodeInfo(inst);
   CuAssertPtrNotNull(tc, nodeInfo);


   apx_parser_delete(parser);
   apx_nodeInstance_delete(inst);

}

