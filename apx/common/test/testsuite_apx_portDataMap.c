/*****************************************************************************
* \file      testsuite_apx_portDataMap.c
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Unit tests for apx_portDataMap
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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"
#include "apx_portDataMap.h"
#include "apx_parser.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define ERROR_SIZE 15

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portDataMap_createFromNodeWithOnlyRequirePorts(CuTest* tc);
static void test_apx_portDataMap_createFromNodeWithOnlyProvidePorts(CuTest* tc);
static void test_create_portDataMap_fromNodeData(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_portDataMap(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_portDataMap_createFromNodeWithOnlyRequirePorts);
   SUITE_ADD_TEST(suite, test_apx_portDataMap_createFromNodeWithOnlyProvidePorts);
   SUITE_ADD_TEST(suite, test_create_portDataMap_fromNodeData);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portDataMap_createFromNodeWithOnlyRequirePorts(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"ComplexPort\"{\"Left\"L\"Right\"L}\n"
         "R\"U8Port\"C\n"
         "R\"U16Port\"S\n"
         "R\"StrPort\"a[21]\n";
   apx_parser_t parser;
   apx_node_t *node;
   apx_nodeData_t *nodeData;
   apx_portDataElement_t *portDataElement;
   apx_portDataMap_t portDataMap;

   apx_parser_create(&parser);
   node = apx_parser_parseString(&parser, apx_text);
   CuAssertPtrNotNull(tc, node);
   apx_parser_clearNodes(&parser);
   nodeData = apx_nodeData_new((uint32_t) strlen(apx_text));
   apx_nodeData_setNode(nodeData, node);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataBuffers(nodeData));

   apx_portDataMap_create(&portDataMap, nodeData);
   CuAssertIntEquals(tc, 4, portDataMap.numRequirePorts);
   CuAssertIntEquals(tc, 0, portDataMap.numProvidePorts);
   CuAssertPtrNotNull(tc, portDataMap.requirePortData);
   CuAssertPtrNotNull(tc, portDataMap.requirePortDataAttributes);
   CuAssertPtrEquals(tc, 0, portDataMap.providePortData);
   CuAssertPtrEquals(tc, 0, portDataMap.providePortDataAttributes);

   portDataElement = apx_portDataMap_getRequirePortDataElement(&portDataMap, 0);
   CuAssertPtrNotNull(tc, portDataElement);
   CuAssertIntEquals(tc, 0, portDataElement->portId);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT, portDataElement->portType);
   CuAssertUIntEquals(tc, 0, portDataElement->offset);
   CuAssertUIntEquals(tc, 8, portDataElement->dataSize);
   CuAssertUIntEquals(tc, 8, portDataElement->totalSize);

   portDataElement = apx_portDataMap_getRequirePortDataElement(&portDataMap, 1);
   CuAssertPtrNotNull(tc, portDataElement);
   CuAssertIntEquals(tc, 1, portDataElement->portId);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT, portDataElement->portType);
   CuAssertUIntEquals(tc, 8, portDataElement->offset);
   CuAssertUIntEquals(tc, 1, portDataElement->dataSize);
   CuAssertUIntEquals(tc, 1, portDataElement->totalSize);

   portDataElement = apx_portDataMap_getRequirePortDataElement(&portDataMap, 2);
   CuAssertPtrNotNull(tc, portDataElement);
   CuAssertIntEquals(tc, 2, portDataElement->portId);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT, portDataElement->portType);
   CuAssertUIntEquals(tc, 9, portDataElement->offset);
   CuAssertUIntEquals(tc, 2, portDataElement->dataSize);
   CuAssertUIntEquals(tc, 2, portDataElement->totalSize);

   portDataElement = apx_portDataMap_getRequirePortDataElement(&portDataMap, 3);
   CuAssertPtrNotNull(tc, portDataElement);
   CuAssertIntEquals(tc, 3, portDataElement->portId);
   CuAssertIntEquals(tc, APX_REQUIRE_PORT, portDataElement->portType);
   CuAssertUIntEquals(tc, 11, portDataElement->offset);
   CuAssertUIntEquals(tc, 21, portDataElement->dataSize);
   CuAssertUIntEquals(tc, 21, portDataElement->totalSize);

   CuAssertUIntEquals(tc, 11+21, nodeData->inPortDataLen);

   apx_parser_destroy(&parser);
   apx_portDataMap_destroy(&portDataMap);
   apx_nodeData_delete(nodeData);
}

static void test_apx_portDataMap_createFromNodeWithOnlyProvidePorts(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "P\"ComplexPort\"{\"Left\"L\"Right\"L}\n"
         "P\"U8Port\"C\n"
         "P\"U16Port\"S\n"
         "P\"U32ArrayPort\"L[10]\n";
   apx_parser_t parser;
   apx_node_t *node;
   apx_nodeData_t *nodeData;
   apx_portDataElement_t *portDataElement;

   apx_portDataMap_t portDataMap;
   apx_parser_create(&parser);
   node = apx_parser_parseString(&parser, apx_text);
   CuAssertPtrNotNull(tc, node);
   apx_parser_clearNodes(&parser);
   nodeData = apx_nodeData_new((uint32_t) strlen(apx_text));
   apx_nodeData_setNode(nodeData, node);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataBuffers(nodeData));

   apx_portDataMap_create(&portDataMap, nodeData);
   CuAssertIntEquals(tc, 0, portDataMap.numRequirePorts);
   CuAssertIntEquals(tc, 4, portDataMap.numProvidePorts);
   CuAssertPtrEquals(tc, 0, portDataMap.requirePortData);
   CuAssertPtrEquals(tc, 0, portDataMap.requirePortDataAttributes);
   CuAssertPtrNotNull(tc, portDataMap.providePortData);
   CuAssertPtrNotNull(tc, portDataMap.providePortDataAttributes);

   portDataElement = apx_portDataMap_getProvidePortDataElement(&portDataMap, 0);
   CuAssertPtrNotNull(tc, portDataElement);
   CuAssertIntEquals(tc, 0, portDataElement->portId);
   CuAssertIntEquals(tc, APX_PROVIDE_PORT, portDataElement->portType);
   CuAssertUIntEquals(tc, 0, portDataElement->offset);
   CuAssertUIntEquals(tc, 8, portDataElement->dataSize);
   CuAssertUIntEquals(tc, 8, portDataElement->totalSize);

   portDataElement = apx_portDataMap_getProvidePortDataElement(&portDataMap, 1);
   CuAssertPtrNotNull(tc, portDataElement);
   CuAssertIntEquals(tc, 1, portDataElement->portId);
   CuAssertIntEquals(tc, APX_PROVIDE_PORT, portDataElement->portType);
   CuAssertUIntEquals(tc, 8, portDataElement->offset);
   CuAssertUIntEquals(tc, 1, portDataElement->dataSize);
   CuAssertUIntEquals(tc, 1, portDataElement->totalSize);

   portDataElement = apx_portDataMap_getProvidePortDataElement(&portDataMap, 2);
   CuAssertPtrNotNull(tc, portDataElement);
   CuAssertIntEquals(tc, 2, portDataElement->portId);
   CuAssertIntEquals(tc, APX_PROVIDE_PORT, portDataElement->portType);
   CuAssertUIntEquals(tc, 9, portDataElement->offset);
   CuAssertUIntEquals(tc, 2, portDataElement->dataSize);
   CuAssertUIntEquals(tc, 2, portDataElement->totalSize);

   portDataElement = apx_portDataMap_getProvidePortDataElement(&portDataMap, 3);
   CuAssertPtrNotNull(tc, portDataElement);
   CuAssertIntEquals(tc, 3, portDataElement->portId);
   CuAssertIntEquals(tc, APX_PROVIDE_PORT, portDataElement->portType);
   CuAssertUIntEquals(tc, 11, portDataElement->offset);
   CuAssertUIntEquals(tc, 4*10, portDataElement->dataSize);
   CuAssertUIntEquals(tc, 4*10, portDataElement->totalSize);

   CuAssertUIntEquals(tc, 11+4*10, nodeData->outPortDataLen);

   apx_parser_destroy(&parser);
   apx_portDataMap_destroy(&portDataMap);
   apx_nodeData_delete(nodeData);
}

static void test_create_portDataMap_fromNodeData(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "P\"ComplexPort\"{\"Left\"L\"Right\"L}\n"
         "P\"U8Port\"C\n"
         "P\"U16Port\"S\n"
         "P\"U32ArrayPort\"L[10]\n";
   apx_parser_t parser;
   apx_node_t *node;
   apx_nodeData_t *nodeData;
   apx_parser_create(&parser);
   node = apx_parser_parseString(&parser, apx_text);
   CuAssertPtrNotNull(tc, node);
   apx_parser_clearNodes(&parser);
   nodeData = apx_nodeData_new((uint32_t) strlen(apx_text));
   apx_nodeData_setNode(nodeData, node);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataBuffers(nodeData));
   CuAssertPtrEquals(tc, 0, nodeData->portDataMap);
   apx_nodeData_createPortDataMap(nodeData, APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, nodeData->portDataMap);

   apx_parser_destroy(&parser);
   apx_nodeData_delete(nodeData);

}
