//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include "CuTest.h"
#include "apx_nodeData.h"
#include "apx_parser.h"
#include "apx_test_nodes.h"
#include "pack.h"
#include "apx_portDataRef.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char *m_node_text1 =
      "APX/1.2\n"
      "N\"LocalTestNode1\"\n"
      "P\"VehicleSpeed\"S:=65535\n"
      "P\"VehicleMode\"C(0,7):=7\n"
      "P\"SelectedGear\"C(0,15):=15\n";


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeData_newEmpty(CuTest* tc);
static void test_apx_nodeData_providePortConnectionCount(CuTest* tc);
static void test_apx_nodeData_requirePortConnectionCount(CuTest* tc);
static void test_apx_nodeData_createProvidePortInitData(CuTest* tc);
static void test_apx_nodeData_updatePortData(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


CuSuite* testSuite_apx_nodeData(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeData_newEmpty);
   SUITE_ADD_TEST(suite, test_apx_nodeData_providePortConnectionCount);
   SUITE_ADD_TEST(suite, test_apx_nodeData_requirePortConnectionCount);
   SUITE_ADD_TEST(suite, test_apx_nodeData_createProvidePortInitData);
   SUITE_ADD_TEST(suite, test_apx_nodeData_updatePortData);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_apx_nodeData_newEmpty(CuTest* tc)
{
   apx_nodeData_t *nodeData;


   nodeData = apx_nodeData_new(100u);
   CuAssertPtrNotNull(tc, nodeData);
   apx_nodeData_delete(nodeData);

}

static void test_apx_nodeData_providePortConnectionCount(CuTest* tc)
{
   apx_parser_t parser;
   apx_parser_create(&parser);
   apx_nodeData_t *nodeData;

   nodeData = apx_nodeData_makeFromString(&parser, g_apx_test_node1, NULL); //This APX node has 3 output ports and 1 input port
   CuAssertPtrNotNull(tc, nodeData);

   CuAssertUIntEquals(tc, 0, apx_nodeData_getProvidePortConnectionCount(nodeData, 0));
   CuAssertUIntEquals(tc, 0, apx_nodeData_getProvidePortConnectionCount(nodeData, 1));
   CuAssertUIntEquals(tc, 0, apx_nodeData_getProvidePortConnectionCount(nodeData, 2));

   apx_nodeData_incProvidePortConnectionCount(nodeData, 0);
   apx_nodeData_incProvidePortConnectionCount(nodeData, 0);
   apx_nodeData_incProvidePortConnectionCount(nodeData, 0);
   apx_nodeData_incProvidePortConnectionCount(nodeData, 1);
   apx_nodeData_incProvidePortConnectionCount(nodeData, 1);
   apx_nodeData_incProvidePortConnectionCount(nodeData, 2);
   CuAssertUIntEquals(tc, 3, apx_nodeData_getProvidePortConnectionCount(nodeData, 0));
   CuAssertUIntEquals(tc, 2, apx_nodeData_getProvidePortConnectionCount(nodeData, 1));
   CuAssertUIntEquals(tc, 1, apx_nodeData_getProvidePortConnectionCount(nodeData, 2));
   CuAssertUIntEquals(tc, 0, apx_nodeData_getProvidePortConnectionCount(nodeData, 3));
   CuAssertUIntEquals(tc, 6, apx_nodeData_getPortConnectionsTotal(nodeData));
   apx_nodeData_decProvidePortConnectionCount(nodeData, 0);
   apx_nodeData_decProvidePortConnectionCount(nodeData, 1);
   apx_nodeData_decProvidePortConnectionCount(nodeData, 2);
   CuAssertUIntEquals(tc, 2, apx_nodeData_getProvidePortConnectionCount(nodeData, 0));
   CuAssertUIntEquals(tc, 1, apx_nodeData_getProvidePortConnectionCount(nodeData, 1));
   CuAssertUIntEquals(tc, 0, apx_nodeData_getProvidePortConnectionCount(nodeData, 2));
   CuAssertUIntEquals(tc, 3, apx_nodeData_getPortConnectionsTotal(nodeData));
   //decreasing a count that is already at zero shall stay at zero
   CuAssertUIntEquals(tc, 0, apx_nodeData_getProvidePortConnectionCount(nodeData, 2));
   apx_nodeData_decProvidePortConnectionCount(nodeData, 2);
   apx_parser_destroy(&parser);
   apx_nodeData_delete(nodeData);
}


static void test_apx_nodeData_requirePortConnectionCount(CuTest* tc)
{
   apx_parser_t parser;
   apx_parser_create(&parser);
   apx_nodeData_t *nodeData;

   nodeData = apx_nodeData_makeFromString(&parser, g_apx_test_node4, NULL); //This APX node has 0 output ports and 3 input ports
   CuAssertPtrNotNull(tc, nodeData);

   CuAssertUIntEquals(tc, 0, apx_nodeData_getRequirePortConnectionCount(nodeData, 0));
   CuAssertUIntEquals(tc, 0, apx_nodeData_getRequirePortConnectionCount(nodeData, 1));
   CuAssertUIntEquals(tc, 0, apx_nodeData_getRequirePortConnectionCount(nodeData, 2));

   apx_nodeData_incRequirePortConnectionCount(nodeData, 0);
   apx_nodeData_incRequirePortConnectionCount(nodeData, 0);
   apx_nodeData_incRequirePortConnectionCount(nodeData, 0);
   apx_nodeData_incRequirePortConnectionCount(nodeData, 1);
   apx_nodeData_incRequirePortConnectionCount(nodeData, 1);
   apx_nodeData_incRequirePortConnectionCount(nodeData, 2);
   CuAssertUIntEquals(tc, 3, apx_nodeData_getRequirePortConnectionCount(nodeData, 0));
   CuAssertUIntEquals(tc, 2, apx_nodeData_getRequirePortConnectionCount(nodeData, 1));
   CuAssertUIntEquals(tc, 1, apx_nodeData_getRequirePortConnectionCount(nodeData, 2));
   CuAssertUIntEquals(tc, 0, apx_nodeData_getRequirePortConnectionCount(nodeData, 3));
   CuAssertUIntEquals(tc, 6, apx_nodeData_getPortConnectionsTotal(nodeData));
   apx_nodeData_decRequirePortConnectionCount(nodeData, 0);
   apx_nodeData_decRequirePortConnectionCount(nodeData, 1);
   apx_nodeData_decRequirePortConnectionCount(nodeData, 2);
   CuAssertUIntEquals(tc, 2, apx_nodeData_getRequirePortConnectionCount(nodeData, 0));
   CuAssertUIntEquals(tc, 1, apx_nodeData_getRequirePortConnectionCount(nodeData, 1));
   CuAssertUIntEquals(tc, 0, apx_nodeData_getRequirePortConnectionCount(nodeData, 2));
   CuAssertUIntEquals(tc, 3, apx_nodeData_getPortConnectionsTotal(nodeData));
   //decreasing a count that is already at zero shall stay at zero
   CuAssertUIntEquals(tc, 0, apx_nodeData_getRequirePortConnectionCount(nodeData, 2));
   apx_nodeData_decRequirePortConnectionCount(nodeData, 2);
   apx_parser_destroy(&parser);
   apx_nodeData_delete(nodeData);
}

static void test_apx_nodeData_createProvidePortInitData(CuTest* tc)
{
   uint8_t dataBuf[4] = {0,0,0,0};
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *nodeData = apx_nodeData_makeFromString(parser, m_node_text1, NULL);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertIntEquals(tc, 4, apx_nodeData_getOutPortDataLen(nodeData));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_readOutPortData(nodeData, dataBuf, 0, 4));
   CuAssertUIntEquals(tc, 255, dataBuf[0]);
   CuAssertUIntEquals(tc, 255, dataBuf[1]);
   CuAssertUIntEquals(tc, 7, dataBuf[2]);
   CuAssertUIntEquals(tc, 15, dataBuf[3]);
   apx_nodeData_delete(nodeData);
   apx_parser_delete(parser);
}

static void test_apx_nodeData_updatePortData(CuTest* tc)
{
   uint8_t input[4] = {0,0};
   uint8_t output[4] = {0,0};
   uint8_t u8Value;
   uint16_t u16Value;
   const char *dest_node_text_dest = "APX/1.2\n"
         "N\"DestNode\"\n"
         "R\"VehicleSpeed\"S:=65535\n" // ID: 0, offset 0
         "R\"VehicleMode\"C(0,7):=7\n" // ID: 1, offset 2
         "R\"SelectedGear\"C(0,15):=15\n"; //ID: 2, offset 3

   const char *src_node_text = "APX/1.2\n"
         "N\"DestNode\"\n"
         "P\"SelectedGear\"C(0,15):=15\n" //ID 0, offset 0
         "P\"VehicleMode\"C(0,7):=7\n"    //ID 1, offset 1
         "P\"VehicleSpeed\"S:=65535\n";   //ID 2, offset 2

   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *destNodeData = apx_nodeData_makeFromString(parser, dest_node_text_dest, NULL);
   apx_nodeData_t *srcNodeData = apx_nodeData_makeFromString(parser, src_node_text, NULL);
   CuAssertPtrNotNull(tc, destNodeData);
   CuAssertPtrNotNull(tc, srcNodeData);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(destNodeData, APX_SERVER_MODE));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(srcNodeData, APX_SERVER_MODE));

   //write vehicle speed port (ID 2 -> ID 0)
   packLE(input, 10000, (uint8_t) sizeof(uint16_t));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_writeOutPortData(srcNodeData, input, 2u, (uint32_t) sizeof(uint16_t)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_updatePortDataDirectById(destNodeData, (apx_portId_t) 0u, srcNodeData, (apx_portId_t)2u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_readInPortData(destNodeData, output, 0u, (uint32_t) sizeof(uint16_t)));
   u16Value = (uint16_t) unpackLE(output, (uint8_t) sizeof(uint16_t));
   CuAssertUIntEquals(tc, 10000, u16Value);

   //write VehicleMode Port (ID 1 -> ID 1)
   packLE(input, 5, (uint32_t) sizeof(uint8_t));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_writeOutPortData(srcNodeData, input, 1u, (uint32_t) sizeof(uint8_t)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_updatePortDataDirectById(destNodeData, (apx_portId_t) 1u, srcNodeData, (apx_portId_t)1u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_readInPortData(destNodeData, output, 2u, (uint32_t) sizeof(uint8_t)));
   u8Value = (uint8_t) unpackLE(output, (uint8_t) sizeof(uint8_t));
   CuAssertUIntEquals(tc, 5, u8Value);

   //write SelectedGear Port (ID 0 -> ID 2)
   packLE(input, 12, (uint32_t) sizeof(uint8_t));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_writeOutPortData(srcNodeData, input, 0u, (uint32_t) sizeof(uint8_t)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_updatePortDataDirectById(destNodeData, (apx_portId_t) 2u, srcNodeData, (apx_portId_t) 0u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_readInPortData(destNodeData, output, 3u, (uint32_t) sizeof(uint8_t)));
   u8Value = (uint8_t) unpackLE(output, (uint8_t) sizeof(uint8_t));
   CuAssertUIntEquals(tc, 12, u8Value);

   apx_nodeData_delete(destNodeData);
   apx_nodeData_delete(srcNodeData);
   apx_parser_delete(parser);
}
