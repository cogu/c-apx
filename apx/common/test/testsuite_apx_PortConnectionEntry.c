/*****************************************************************************
* \file      testsuite_apx_portConnectionEntry.c
* \author    Conny Gustafsson
* \date      2019-01-28
* \brief     Unit tests for apx_portConnectionEntry.c
*
* Copyright (c) 2019-2020 Conny Gustafsson
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
#include <stdlib.h>
#include "CuTest.h"
#include <stdio.h>
#include "apx_portConnectionEntry.h"
#include "apx_nodeManager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portConnectionEntry_create(CuTest *tc);
static void test_apx_portConnectionEntry_connectOne(CuTest *tc);
static void test_apx_portConnectionEntry_connectThree(CuTest *tc);
static void test_apx_portConnectionEntry_disconnectOne(CuTest *tc);
static void test_apx_portConnectionEntry_disconnectThree(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////
static const char *m_node_text1 =
      "APX/1.2\n"
      "N\"LocalTestNode1\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

static const char *m_node_text2 =
      "APX/1.2\n"
      "N\"LocalTestNode2\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

static const char *m_node_text3 =
      "APX/1.2\n"
      "N\"LocalTestNode3\"\n"
      "R\"VehicleSpeed\"S:=65535\n";

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_portConnectionEntry(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_create);
   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_connectOne);
   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_connectThree);
   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_disconnectOne);
   SUITE_ADD_TEST(suite, test_apx_portConnectionEntry_disconnectThree);
   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_portConnectionEntry_create(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_portConnectionEntry_create(&entry);
   CuAssertIntEquals(tc, 0, entry.count);
   CuAssertPtrEquals(tc, NULL, entry.pAny);
   apx_portConnectionEntry_destroy(&entry);
}

static void test_apx_portConnectionEntry_connectOne(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_portRef_t *portRef1;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(nodeInstance1));
   apx_portConnectionEntry_create(&entry);
   portRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, portRef1);
   apx_portConnectionEntry_addConnection(&entry, portRef1);
   CuAssertIntEquals(tc, 1, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectionEntry_get(&entry, 0));


   apx_portConnectionEntry_destroy(&entry);
   apx_nodeManager_delete(nodeManager);

}

static void test_apx_portConnectionEntry_connectThree(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portRef_t *portRef1;
   apx_portRef_t *portRef2;
   apx_portRef_t *portRef3;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   apx_portConnectionEntry_create(&entry);
   CuAssertPtrNotNull(tc, nodeManager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(nodeInstance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(nodeInstance2));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(nodeInstance3));


   portRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, portRef1);
   portRef2 = apx_nodeInstance_getRequirePortRef(nodeInstance2, 0);
   CuAssertPtrNotNull(tc, portRef2);
   portRef3 = apx_nodeInstance_getRequirePortRef(nodeInstance3, 0);
   CuAssertPtrNotNull(tc, portRef3);
   apx_portConnectionEntry_addConnection(&entry, portRef1);
   apx_portConnectionEntry_addConnection(&entry, portRef2);
   apx_portConnectionEntry_addConnection(&entry, portRef3);
   CuAssertIntEquals(tc, 3, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectionEntry_get(&entry, 0));
   CuAssertPtrEquals(tc, portRef2, apx_portConnectionEntry_get(&entry, 1));
   CuAssertPtrEquals(tc, portRef3, apx_portConnectionEntry_get(&entry, 2));

   apx_portConnectionEntry_destroy(&entry);
   apx_nodeManager_delete(nodeManager);
}


static void test_apx_portConnectionEntry_disconnectOne(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_portRef_t *portRef1;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   CuAssertPtrNotNull(tc, nodeManager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(nodeInstance1));
   apx_portConnectionEntry_create(&entry);
   portRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, portRef1);
   apx_portConnectionEntry_removeConnection(&entry, portRef1);
   CuAssertIntEquals(tc, -1, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectionEntry_get(&entry, 0));


   apx_portConnectionEntry_destroy(&entry);
   apx_nodeManager_delete(nodeManager);
}


static void test_apx_portConnectionEntry_disconnectThree(CuTest *tc)
{
   apx_portConnectionEntry_t entry;
   apx_nodeManager_t *nodeManager;
   apx_nodeInstance_t *nodeInstance1;
   apx_nodeInstance_t *nodeInstance2;
   apx_nodeInstance_t *nodeInstance3;
   apx_portRef_t *portRef1;
   apx_portRef_t *portRef2;
   apx_portRef_t *portRef3;

   nodeManager = apx_nodeManager_new(APX_SERVER_MODE, false);
   apx_portConnectionEntry_create(&entry);
   CuAssertPtrNotNull(tc, nodeManager);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text1));
   nodeInstance1 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance1);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text2));
   nodeInstance2 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance2);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeManager_buildNode_cstr(nodeManager, m_node_text3));
   nodeInstance3 = apx_nodeManager_getLastAttached(nodeManager);
   CuAssertPtrNotNull(tc, nodeInstance3);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(nodeInstance1));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(nodeInstance2));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeInstance_buildPortRefs(nodeInstance3));

   portRef1 = apx_nodeInstance_getRequirePortRef(nodeInstance1, 0);
   CuAssertPtrNotNull(tc, portRef1);
   portRef2 = apx_nodeInstance_getRequirePortRef(nodeInstance2, 0);
   CuAssertPtrNotNull(tc, portRef2);
   portRef3 = apx_nodeInstance_getRequirePortRef(nodeInstance3, 0);
   CuAssertPtrNotNull(tc, portRef3);
   apx_portConnectionEntry_removeConnection(&entry, portRef1);
   apx_portConnectionEntry_removeConnection(&entry, portRef2);
   apx_portConnectionEntry_removeConnection(&entry, portRef3);
   CuAssertIntEquals(tc, -3, entry.count);
   CuAssertPtrEquals(tc, portRef1, apx_portConnectionEntry_get(&entry, 0));
   CuAssertPtrEquals(tc, portRef2, apx_portConnectionEntry_get(&entry, 1));
   CuAssertPtrEquals(tc, portRef3, apx_portConnectionEntry_get(&entry, 2));

   apx_portConnectionEntry_destroy(&entry);
   apx_nodeManager_delete(nodeManager);
}
