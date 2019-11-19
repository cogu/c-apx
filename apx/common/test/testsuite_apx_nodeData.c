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
#include "apx_portDataMap.h"
#include "apx_vm.h"
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

static const char *m_node_text2 =
      "APX/1.2\n"
      "N\"LocalTestNode2\"\n"
      "R\"VehicleSpeed\"S:=65535\n"
      "R\"VehicleMode\"C(0,7):=7\n"
      "R\"SelectedGear\"C(0,15):=15\n";


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeData_newEmpty(CuTest* tc);
static void test_apx_nodeData_providePortConnectionCount(CuTest* tc);
static void test_apx_nodeData_requirePortConnectionCount(CuTest* tc);
static void test_apx_nodeData_createProvidePortInitData(CuTest* tc);
static void test_apx_nodeData_updatePortData(CuTest* tc);
static void test_apx_create_dynamic_nodeData_with_requirePorts(CuTest* tc);
static void test_apx_nodeData_dynamicReadU8Port(CuTest* tc);
static void test_apx_nodeData_dynamicWriteU8Port(CuTest* tc);
static void test_apx_nodeData_dynamicReadU16Port(CuTest* tc);
static void test_apx_nodeData_dynamicWriteU16Port(CuTest* tc);
static void test_apx_nodeData_dynamicReadU32Port(CuTest* tc);
static void test_apx_nodeData_dynamicWriteU32Port(CuTest* tc);

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
   SUITE_ADD_TEST(suite, test_apx_create_dynamic_nodeData_with_requirePorts);
   SUITE_ADD_TEST(suite, test_apx_nodeData_dynamicReadU8Port);
   SUITE_ADD_TEST(suite, test_apx_nodeData_dynamicWriteU8Port);
   SUITE_ADD_TEST(suite, test_apx_nodeData_dynamicReadU16Port);
   SUITE_ADD_TEST(suite, test_apx_nodeData_dynamicWriteU16Port);
   SUITE_ADD_TEST(suite, test_apx_nodeData_dynamicReadU32Port);
   SUITE_ADD_TEST(suite, test_apx_nodeData_dynamicWriteU32Port);

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

static void test_apx_create_dynamic_nodeData_with_requirePorts(CuTest* tc)
{
   apx_portDataMap_t *portDataMap;
   apx_parser_t *parser = apx_parser_new();
   apx_error_t rc = APX_NO_ERROR;
   apx_nodeData_t *nodeData;
   apx_programType_t errProgramType = APX_PACK_PROGRAM;
   apx_uniquePortId_t errPortId = 0;
   int32_t i;

   nodeData = apx_nodeData_makeFromString(parser, m_node_text2, &rc);
   CuAssertPtrNotNull(tc, nodeData);
   CuAssertTrue(tc, nodeData->isDynamic);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData, APX_CLIENT_MODE) );
   portDataMap = apx_nodeData_getPortDataMap(nodeData);
   CuAssertPtrNotNull(tc, portDataMap);
   CuAssertPtrNotNull(tc, portDataMap->requirePortPackPrograms);
   CuAssertPtrNotNull(tc, portDataMap->requirePortUnpackPrograms);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_compilePortPrograms(nodeData, &errProgramType, &errPortId));
   CuAssertIntEquals(tc, 3, apx_portDataMap_getNumRequirePorts(portDataMap));
   for(i=0;i<3;i++)
   {
      const adt_bytes_t *packProgram;
      const adt_bytes_t *unpackProgram;
      packProgram = apx_portDataMap_getRequirePortPackProgram(portDataMap, (apx_portId_t) 0);
      CuAssertPtrNotNull(tc, packProgram);
      CuAssertIntEquals(tc, APX_VM_HEADER_SIZE+1*APX_VM_INSTRUCTION_SIZE, adt_bytes_length(packProgram));
      unpackProgram = apx_portDataMap_getRequirePortUnpackProgram(portDataMap, (apx_portId_t) 0);
      CuAssertPtrNotNull(tc, unpackProgram);
      CuAssertIntEquals(tc, APX_VM_HEADER_SIZE+1*APX_VM_INSTRUCTION_SIZE, adt_bytes_length(unpackProgram));
   }
   apx_parser_delete(parser);
   apx_nodeData_delete(nodeData);
}

static void test_apx_nodeData_dynamicReadU8Port(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"DataPort\"C\n";
   apx_vm_t *vm;
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *nodeData;
   apx_error_t rc = APX_NO_ERROR;
   apx_programType_t errProgramType = APX_PACK_PROGRAM;
   apx_uniquePortId_t errPortId = 0;
   const adt_bytes_t *program;
   vm = apx_vm_new();
   uint8_t expectedValue[3] = {0x00, 0x12, 0xff};
   uint8_t packedData[3] = {0x00, 0x12, 0xff};
   dtl_dv_t *dv = (dtl_dv_t*) 0;
   dtl_sv_t *sv = (dtl_sv_t*) 0;
   int32_t i;
   nodeData = apx_nodeData_makeFromString(parser, apx_text, &rc);
   CuAssertPtrNotNull(tc, nodeData);
   apx_parser_delete(parser);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData, APX_CLIENT_MODE) );
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_compilePortPrograms(nodeData, &errProgramType, &errPortId));
   program = apx_nodeData_getRequirePortUnpackProgram(nodeData, (apx_portId_t) 0);
   CuAssertPtrNotNull(tc, program);
   apx_vm_selectProgram(vm, program);

   for(i=0; i < 3; i++)
   {
      apx_vm_setReadBuffer(vm, &packedData[i], UINT8_SIZE);
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpackValue(vm, &dv));
      CuAssertPtrNotNull(tc, dv);
      CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
      sv = (dtl_sv_t*) dv;
      CuAssertUIntEquals(tc, expectedValue[i], dtl_sv_to_u32(sv, NULL));
      dtl_dec_ref(sv);
   }
   apx_nodeData_delete(nodeData);
   apx_vm_delete(vm);
}

static void test_apx_nodeData_dynamicWriteU8Port(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "P\"DataPort\"C\n";
   apx_vm_t *vm;
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *nodeData;
   apx_error_t rc = APX_NO_ERROR;
   apx_programType_t errProgramType = APX_PACK_PROGRAM;
   apx_uniquePortId_t errPortId = 0;
   const adt_bytes_t *program;
   vm = apx_vm_new();
   uint8_t valueToWrite[3] = {0x00, 0x12, 0xff};
   uint8_t writeBuffer[UINT8_SIZE];
   uint8_t expectedPacked[3] = {0x00, 0x12, 0xff};
   dtl_sv_t *sv = dtl_sv_new();
   int32_t i;
   nodeData = apx_nodeData_makeFromString(parser, apx_text, &rc);
   CuAssertPtrNotNull(tc, nodeData);
   apx_parser_delete(parser);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData, APX_CLIENT_MODE) );
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_compilePortPrograms(nodeData, &errProgramType, &errPortId));
   program = apx_nodeData_getProvidePortPackProgram(nodeData, (apx_portId_t) 0);
   CuAssertPtrNotNull(tc, program);
   apx_vm_selectProgram(vm, program);

   for(i=0; i < 3; i++)
   {
      dtl_sv_set_u32(sv, valueToWrite[i]);
      apx_vm_setWriteBuffer(vm, &writeBuffer[0], UINT8_SIZE);
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_packValue(vm, (const dtl_dv_t*) sv));
      CuAssertUIntEquals(tc, expectedPacked[i], writeBuffer[0]);
   }

   apx_nodeData_delete(nodeData);
   apx_vm_delete(vm);
   dtl_dec_ref(sv);
}

static void test_apx_nodeData_dynamicReadU16Port(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"DataPort\"S\n";
   apx_vm_t *vm;
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *nodeData;
   apx_error_t rc = APX_NO_ERROR;
   apx_programType_t errProgramType = APX_PACK_PROGRAM;
   apx_uniquePortId_t errPortId = 0;
   const adt_bytes_t *program;
   vm = apx_vm_new();
   uint16_t expectedValue[3] = {0x0000,  0x1234, 0xffff};
   uint8_t packedData[3 * UINT16_SIZE] = {0x00, 0x00, 0x34, 0x12, 0xff, 0xff};
   dtl_dv_t *dv = (dtl_dv_t*) 0;
   dtl_sv_t *sv = (dtl_sv_t*) 0;
   int32_t i;
   nodeData = apx_nodeData_makeFromString(parser, apx_text, &rc);
   CuAssertPtrNotNull(tc, nodeData);
   apx_parser_delete(parser);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData, APX_CLIENT_MODE) );
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_compilePortPrograms(nodeData, &errProgramType, &errPortId));
   program = apx_nodeData_getRequirePortUnpackProgram(nodeData, (apx_portId_t) 0);
   CuAssertPtrNotNull(tc, program);
   apx_vm_selectProgram(vm, program);

   for(i=0; i < 3; i++)
   {
      apx_vm_setReadBuffer(vm, &packedData[i*UINT16_SIZE], UINT16_SIZE);
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpackValue(vm, &dv));
      CuAssertPtrNotNull(tc, dv);
      CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
      sv = (dtl_sv_t*) dv;
      CuAssertUIntEquals(tc, expectedValue[i], dtl_sv_to_u32(sv, NULL));
      dtl_dec_ref(sv);
   }

   apx_nodeData_delete(nodeData);
   apx_vm_delete(vm);
}

static void test_apx_nodeData_dynamicWriteU16Port(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "P\"DataPort\"S\n";
   apx_vm_t *vm;
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *nodeData;
   apx_error_t rc = APX_NO_ERROR;
   apx_programType_t errProgramType = APX_PACK_PROGRAM;
   apx_uniquePortId_t errPortId = 0;
   const adt_bytes_t *program;
   vm = apx_vm_new();
   uint16_t valueToWrite[3] = {0x0000, 0x1234, 0xffff};
   uint8_t writeBuffer[UINT16_SIZE];
   uint8_t expectedPacked[3*UINT16_SIZE] = {0x00, 0x00, 0x34, 0x12, 0xff, 0xff};
   dtl_sv_t *sv = dtl_sv_new();
   int32_t i;
   nodeData = apx_nodeData_makeFromString(parser, apx_text, &rc);
   CuAssertPtrNotNull(tc, nodeData);
   apx_parser_delete(parser);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData, APX_CLIENT_MODE) );
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_compilePortPrograms(nodeData, &errProgramType, &errPortId));
   program = apx_nodeData_getProvidePortPackProgram(nodeData, (apx_portId_t) 0);
   CuAssertPtrNotNull(tc, program);
   apx_vm_selectProgram(vm, program);

   for(i=0; i < 3; i++)
   {
      dtl_sv_set_u32(sv, valueToWrite[i]);
      apx_vm_setWriteBuffer(vm, &writeBuffer[0], UINT16_SIZE);
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_packValue(vm, (const dtl_dv_t*) sv));
      CuAssertUIntEquals(tc, expectedPacked[i*UINT16_SIZE+0], writeBuffer[0]);
      CuAssertUIntEquals(tc, expectedPacked[i*UINT16_SIZE+1], writeBuffer[1]);
   }

   apx_nodeData_delete(nodeData);
   apx_vm_delete(vm);
   dtl_dec_ref(sv);
}

static void test_apx_nodeData_dynamicReadU32Port(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "R\"DataPort\"L\n";
   apx_vm_t *vm;
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *nodeData;
   apx_error_t rc = APX_NO_ERROR;
   apx_programType_t errProgramType = APX_PACK_PROGRAM;
   apx_uniquePortId_t errPortId = 0;
   const adt_bytes_t *program;
   vm = apx_vm_new();
   uint32_t expectedValue[3] = {0x00000000,  0x12345678, 0xffffffff};
   uint8_t packedData[3 * UINT32_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff};
   dtl_dv_t *dv = (dtl_dv_t*) 0;
   dtl_sv_t *sv = (dtl_sv_t*) 0;
   int32_t i;
   nodeData = apx_nodeData_makeFromString(parser, apx_text, &rc);
   CuAssertPtrNotNull(tc, nodeData);
   apx_parser_delete(parser);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData, APX_CLIENT_MODE) );
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_compilePortPrograms(nodeData, &errProgramType, &errPortId));
   program = apx_nodeData_getRequirePortUnpackProgram(nodeData, (apx_portId_t) 0);
   CuAssertPtrNotNull(tc, program);
   apx_vm_selectProgram(vm, program);

   for(i=0; i < 3; i++)
   {
      apx_vm_setReadBuffer(vm, &packedData[i*UINT32_SIZE], UINT32_SIZE);
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_unpackValue(vm, &dv));
      CuAssertPtrNotNull(tc, dv);
      CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type(dv));
      sv = (dtl_sv_t*) dv;
      CuAssertUIntEquals(tc, expectedValue[i], dtl_sv_to_u32(sv, NULL));
      dtl_dec_ref(sv);
   }

   apx_nodeData_delete(nodeData);
   apx_vm_delete(vm);
}

static void test_apx_nodeData_dynamicWriteU32Port(CuTest* tc)
{
   const char *apx_text =
         "APX/1.2\n"
         "N\"TestNode\"\n"
         "P\"DataPort\"L\n";
   apx_vm_t *vm;
   apx_parser_t *parser = apx_parser_new();
   apx_nodeData_t *nodeData;
   apx_error_t rc = APX_NO_ERROR;
   apx_programType_t errProgramType = APX_PACK_PROGRAM;
   apx_uniquePortId_t errPortId = 0;
   const adt_bytes_t *program;
   vm = apx_vm_new();
   uint32_t valueToWrite[3] = {0x00000000, 0x12345678, 0xffffffff};
   uint8_t writeBuffer[UINT16_SIZE];
   uint8_t expectedPacked[3*UINT32_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12, 0xff, 0xff, 0xff, 0xff};
   dtl_sv_t *sv = dtl_sv_new();
   int32_t i;
   nodeData = apx_nodeData_makeFromString(parser, apx_text, &rc);
   CuAssertPtrNotNull(tc, nodeData);
   apx_parser_delete(parser);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_createPortDataMap(nodeData, APX_CLIENT_MODE) );
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_compilePortPrograms(nodeData, &errProgramType, &errPortId));
   program = apx_nodeData_getProvidePortPackProgram(nodeData, (apx_portId_t) 0);
   CuAssertPtrNotNull(tc, program);
   apx_vm_selectProgram(vm, program);

   for(i=0; i < 3; i++)
   {
      dtl_sv_set_u32(sv, valueToWrite[i]);
      apx_vm_setWriteBuffer(vm, &writeBuffer[0], UINT32_SIZE);
      CuAssertIntEquals(tc, APX_NO_ERROR, apx_vm_packValue(vm, (const dtl_dv_t*) sv));
      CuAssertUIntEquals(tc, expectedPacked[i*UINT32_SIZE+0], writeBuffer[0]);
      CuAssertUIntEquals(tc, expectedPacked[i*UINT32_SIZE+1], writeBuffer[1]);
      CuAssertUIntEquals(tc, expectedPacked[i*UINT32_SIZE+2], writeBuffer[2]);
      CuAssertUIntEquals(tc, expectedPacked[i*UINT32_SIZE+3], writeBuffer[3]);
   }

   apx_nodeData_delete(nodeData);
   apx_vm_delete(vm);
   dtl_dec_ref(sv);
}
