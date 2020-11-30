//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "test_nodes.h"
#include "apx/node_manager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static const char *m_apx_definition1 = "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"VehicleSpeed\"S:=65535\n"
      "\n";

static const char *m_apx_definition2 = "APX/1.2\n"
      "N\"TestNode2\"\n"
      "P\"EngineSpeed\"S:=65535\n"
      "\n";

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_nodeManager_createNode(CuTest *tc);
static void test_apx_nodeManager_buildNode(CuTest *tc);
static void test_apx_nodeManager_copyNodeReference(CuTest *tc);
static void test_apx_nodeManager_copyMultipleNodeReference(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_nodeManager(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_nodeManager_createNode);
   SUITE_ADD_TEST(suite, test_apx_nodeManager_buildNode);
   SUITE_ADD_TEST(suite, test_apx_nodeManager_copyNodeReference);
   SUITE_ADD_TEST(suite, test_apx_nodeManager_copyMultipleNodeReference);

   return suite;
}
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//Server Mode
static void test_apx_nodeManager_createNode(CuTest *tc)
{
   apx_nodeManager_t *manager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, manager);
   apx_nodeInstance_t *nodeInstance = apx_nodeManager_createNode(manager, "Node1");
   CuAssertPtrNotNull(tc, nodeInstance);
   apx_nodeManager_delete(manager);
}

//Client mode
static void test_apx_nodeManager_buildNode(CuTest *tc)
{
   apx_nodeManager_t *manager = apx_nodeManager_new(APX_CLIENT_MODE, false);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(manager, m_apx_definition1));
   apx_nodeManager_delete(manager);
}

/**
 * Demonstrates that one nodeManager (manager1) can be owner of a node while it can also share a weak reference with
 * another nodeManager (manager2)
 */
static void test_apx_nodeManager_copyNodeReference(CuTest *tc)
{
   apx_nodeManager_t *manager1 = apx_nodeManager_new(APX_CLIENT_MODE, false);
   apx_nodeManager_t *manager2 = apx_nodeManager_new(APX_CLIENT_MODE, true);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(manager1, m_apx_definition1));
   apx_nodeInstance_t *node1 = apx_nodeManager_getLastAttached(manager1);
   CuAssertPtrNotNull(tc, node1);
   apx_nodeManager_attachNode(manager2, node1);
   apx_nodeInstance_t *node2 = apx_nodeManager_getLastAttached(manager2);
   CuAssertPtrEquals(tc, node1, node2);

   apx_nodeManager_delete(manager1);
   apx_nodeManager_delete(manager2);
}

static void test_apx_nodeManager_copyMultipleNodeReference(CuTest *tc)
{
   apx_nodeManager_t *manager1 = apx_nodeManager_new(APX_CLIENT_MODE, false);
   apx_nodeManager_t *manager2 = apx_nodeManager_new(APX_CLIENT_MODE, true);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(manager1, m_apx_definition1));
   apx_nodeInstance_t *node1 = apx_nodeManager_getLastAttached(manager1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(manager1, m_apx_definition2));
   apx_nodeInstance_t *node2 = apx_nodeManager_getLastAttached(manager1);
   CuAssertPtrNotNull(tc, node1);
   CuAssertPtrNotNull(tc, node2);
   CuAssertTrue(tc, node1 != node2);
   apx_nodeManager_attachNode(manager2, node1);
   apx_nodeManager_attachNode(manager2, node2);
   apx_nodeInstance_t *node = apx_nodeManager_find(manager2, "TestNode1");
   CuAssertPtrEquals(tc, node1, node);
   node = apx_nodeManager_find(manager2, "TestNode2");
   CuAssertPtrEquals(tc, node2, node);

   apx_nodeManager_delete(manager1);
   apx_nodeManager_delete(manager2);
}

