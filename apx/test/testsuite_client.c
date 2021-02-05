//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "apx/client.h"
#include "client_event_listener_spy.h"
#include "CuTest.h"
#include "pack.h"
#include "apx/util.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

static const char *m_apx_definition1 = "APX/1.2\n"
      "N\"TestNode1\"\n"
      "P\"U8Value\"C:=0xff\n"
      "P\"U16Value\"S:=0xffff\n"
      "P\"U32Value\"L:=0xffffffff\n"
      "\n";

static const char *m_apx_definition2 = "APX/1.2\n"
      "N\"TestNode2\"\n"
      "R\"U8Value\"C:=0xff\n"
      "R\"U16Value\"S:=0xffff\n"
      "R\"U32Value\"L:=0xffffffff\n"
      "\n";
/*
static const char *m_apx_definition3 = "APX/1.2\n"
      "N\"TestNode3\"\n"
      "P\"U8Array\"C[3]:={0xFF, 0xFF, 0xFF}\n"
      "P\"U16Array\"S[3]:={0xFFFF, 0xFFFF, 0xFFFF}\n"
      "P\"U32Array\"L[3]:={0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}\n"
      "\n";

static const char *m_apx_definition4 = "APX/1.2\n"
      "N\"TestNode4\"\n"
      "R\"U8Array\"C[3]:={0xFF, 0xFF, 0xFF}\n"
      "R\"U16Array\"S[3]:={0xFFFF, 0xFFFF, 0xFFFF}\n"
      "R\"U32Array\"L[3]:={0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}\n"
      "\n";

static const char *m_apx_definition5 = "APX/1.2\n"
      "N\"TestNode5\"\n"
      "P\"S8Value\"c:=-1\n"
      "P\"S16Value\"s:=-1\n"
      "P\"S32Value\"l:=-1\n"
      "\n";

static const char *m_apx_definition6 = "APX/1.2\n"
      "N\"TestNode6\"\n"
      "R\"S8Value\"c:=-1\n"
      "R\"S16Value\"s:=-1\n"
      "R\"S32Value\"l:=-1\n"
      "\n";

static const char *m_apx_definition7 = "APX/1.2\n"
      "N\"TestNode7\"\n"
      "P\"S8Array\"c[4]:={-1, -1, -1, -1}\n"
      "P\"S16Array\"s[4]:={-1, -1, -1, -1}\n"
      "P\"S32Array\"l[4]:={-1, -1, -1, -1}\n"
      "\n";

static const char *m_apx_definition8 = "APX/1.2\n"
      "N\"TestNode8\"\n"
      "R\"S8Array\"c[4]:={-1, -1, -1, -1}\n"
      "R\"S16Array\"s[4]:={-1, -1, -1, -1}\n"
      "R\"S32Array\"l[4]:={-1, -1, -1, -1}\n"
      "\n";

static const char *m_apx_definition9 = "APX/1.2\n"
      "N\"TestNode9\"\n"
      "P\"ColorSetting\"{\"Red\"C\"Green\"C\"Blue\"C}:={0,0,0}\n"
      "P\"StringInRecord\"{\"UserName\"a[8]\"UserId\"L}:={\"Guest\", 0xffffffff}\n"
      "\n";

static const char *m_apx_definition10 = "APX/1.2\n"
      "N\"TestNode10\"\n"
      "R\"ColorSetting\"{\"Red\"C\"Green\"C\"Blue\"C}:={0,0,0}\n"
      "R\"StringInRecord\"{\"UserName\"a[8]\"UserId\"L}:={\"Guest\", 0xffffffff}\n"
      "\n";

static const char *m_apx_definition11 = "APX/1.2\n"
      "N\"TestNode11\"\n"
      "P\"String16\"a[16]:=\"\"\n"
      "P\"String8\"a[8]:=\"\342\204\203\"\n" //degrees Centigrade symbol U+2103
      "\n";

static const char *m_apx_definition12 = "APX/1.2\n"
      "N\"TestNode12\"\n"
      "R\"String16\"a[16]:=\"\"\n"
      "R\"String8\"a[8]:=\"\342\204\203\"\n" //degrees Centigrade symbol U+2103
      "\n";
*/

#define UNSIGNED_ARRAY_LEN 3
#define SIGNED_ARRAY_LEN   4

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_apx_client_create(CuTest* tc);
static void test_apx_client_buildNodeFromString1(CuTest* tc);
static void test_apx_client_buildNodeFromString2(CuTest* tc);




//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_client(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_apx_client_create);
   SUITE_ADD_TEST(suite, test_apx_client_buildNodeFromString1);
   SUITE_ADD_TEST(suite, test_apx_client_buildNodeFromString2);


   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_apx_client_create(CuTest* tc)
{
   apx_client_t *client = apx_client_new();
   CuAssertPtrNotNull(tc, client);
   apx_client_delete(client);
}

static void test_apx_client_buildNodeFromString1(CuTest* tc)
{
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition1));
   apx_nodeInstance_t *node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, node_instance);
   CuAssertIntEquals(tc, 3, apx_nodeInstance_get_num_provide_ports(node_instance));
   CuAssertIntEquals(tc, 0, apx_nodeInstance_get_num_require_ports(node_instance));
   apx_client_delete(client);
}

static void test_apx_client_buildNodeFromString2(CuTest* tc)
{
   apx_client_t* client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition2));
   apx_nodeInstance_t* node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, node_instance);
   CuAssertIntEquals(tc, 0, apx_nodeInstance_get_num_provide_ports(node_instance));
   CuAssertIntEquals(tc, 3, apx_nodeInstance_get_num_require_ports(node_instance));
   apx_client_delete(client);
}
