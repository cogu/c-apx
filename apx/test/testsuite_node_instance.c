//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "test_nodes.h"
#include "apx/parser.h"
#include "apx/node_instance.h"
#include "apx/rmf.h"
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
static void test_apx_nodeInstance_buildPortReferences(CuTest *tc);
static void test_apx_nodeInstance_buildConnectorTable(CuTest *tc);

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
   SUITE_ADD_TEST(suite, test_apx_nodeInstance_buildPortReferences);
   SUITE_ADD_TEST(suite, test_apx_nodeInstance_buildConnectorTable);

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

static void test_apx_nodeInstance_buildPortReferences(CuTest *tc)
{
   apx_nodeInstance_t *inst;
   apx_programType_t errProgramType;
   apx_uniquePortId_t errPortId;
   apx_portRef_t *portref;
   apx_parser_t *parser = apx_parser_new();
   apx_size_t apx_len = (apx_size_t) strlen(g_apx_test_node1);

   inst = apx_nodeInstance_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, inst);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_createDefinitionBuffer(inst, apx_len));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeDefinitionData(inst, (const uint8_t*) g_apx_test_node1, 0u, apx_len));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_parseDefinition(inst, parser));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getParseTree(inst));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildNodeInfo(inst, &errProgramType, &errPortId));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(inst));
   portref = apx_nodeInstance_getRequirePortRef(inst, 0);
   CuAssertPtrNotNull(tc, portref);
   CuAssertPtrEquals(tc, inst, portref->nodeInstance);
   CuAssertUIntEquals(tc, 0, portref->portId);
   CuAssertUIntEquals(tc, UINT8_SIZE, portref->portDataProps->dataSize);
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getRequirePortRef(inst, 1));
   portref = apx_nodeInstance_getProvidePortRef(inst, 0);
   CuAssertPtrNotNull(tc, portref);
   CuAssertPtrEquals(tc, inst, portref->nodeInstance);
   CuAssertUIntEquals(tc, 0 | APX_PORT_ID_PROVIDE_PORT, portref->portId);
   CuAssertUIntEquals(tc, UINT16_SIZE, portref->portDataProps->dataSize);
   portref = apx_nodeInstance_getProvidePortRef(inst, 1);
   CuAssertPtrNotNull(tc, portref);
   CuAssertPtrEquals(tc, inst, portref->nodeInstance);
   CuAssertUIntEquals(tc, 1 | APX_PORT_ID_PROVIDE_PORT, portref->portId);
   CuAssertUIntEquals(tc, UINT8_SIZE, portref->portDataProps->dataSize);
   portref = apx_nodeInstance_getProvidePortRef(inst, 2);
   CuAssertPtrNotNull(tc, portref);
   CuAssertPtrEquals(tc, inst, portref->nodeInstance);
   CuAssertUIntEquals(tc, 2 | APX_PORT_ID_PROVIDE_PORT, portref->portId);
   CuAssertUIntEquals(tc, UINT8_SIZE, portref->portDataProps->dataSize);
   CuAssertPtrEquals(tc, NULL, apx_nodeInstance_getProvidePortRef(inst, 3));


   apx_parser_delete(parser);
   apx_nodeInstance_delete(inst);
}

static void test_apx_nodeInstance_buildConnectorTable(CuTest *tc)
{
   apx_nodeInstance_t *inst;
   apx_programType_t errProgramType;
   apx_uniquePortId_t errPortId;
   apx_parser_t *parser = apx_parser_new();
   apx_size_t apx_len = (apx_size_t) strlen(g_apx_test_node1);

   inst = apx_nodeInstance_new(APX_SERVER_MODE);
   CuAssertPtrNotNull(tc, inst);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_createDefinitionBuffer(inst, apx_len));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_writeDefinitionData(inst, (const uint8_t*) g_apx_test_node1, 0u, apx_len));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_parseDefinition(inst, parser));
   CuAssertPtrNotNull(tc, apx_nodeInstance_getParseTree(inst));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildNodeInfo(inst, &errProgramType, &errPortId));
   CuAssertPtrEquals(tc, NULL, inst->connectorTable);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildConnectorTable(inst));
   CuAssertPtrNotNull(tc, inst->connectorTable );

   apx_parser_delete(parser);
   apx_nodeInstance_delete(inst);

}
