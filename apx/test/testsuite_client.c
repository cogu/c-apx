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
static void test_apx_client_build_node_from_string1(CuTest* tc);
static void test_apx_client_build_node_from_string2(CuTest* tc);
static void test_apx_client_port_instance_without_defining_node_name1(CuTest* tc);
static void test_apx_client_port_instance_without_defining_node_name2(CuTest* tc);
static void test_apx_client_write_port_dtl_u8(CuTest* tc);
static void test_apx_client_read_port_dtl_u8(CuTest* tc);
static void test_apx_client_write_port_dtl_u16(CuTest* tc);
static void test_apx_client_read_port_dtl_u16(CuTest* tc);
static void test_apx_client_write_port_dtl_u32(CuTest* tc);
static void test_apx_client_read_port_dtl_u32(CuTest* tc);






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
   SUITE_ADD_TEST(suite, test_apx_client_build_node_from_string1);
   SUITE_ADD_TEST(suite, test_apx_client_build_node_from_string2);
   SUITE_ADD_TEST(suite, test_apx_client_port_instance_without_defining_node_name1);
   SUITE_ADD_TEST(suite, test_apx_client_port_instance_without_defining_node_name2);
   SUITE_ADD_TEST(suite, test_apx_client_write_port_dtl_u8);
   SUITE_ADD_TEST(suite, test_apx_client_read_port_dtl_u8);
   SUITE_ADD_TEST(suite, test_apx_client_write_port_dtl_u16);
   SUITE_ADD_TEST(suite, test_apx_client_read_port_dtl_u16);
   SUITE_ADD_TEST(suite, test_apx_client_write_port_dtl_u32);
   SUITE_ADD_TEST(suite, test_apx_client_read_port_dtl_u32);


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

static void test_apx_client_build_node_from_string1(CuTest* tc)
{
   apx_client_t *client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition1));
   apx_nodeInstance_t *node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, node_instance);
   CuAssertIntEquals(tc, 3, apx_nodeInstance_get_num_provide_ports(node_instance));
   CuAssertIntEquals(tc, 0, apx_nodeInstance_get_num_require_ports(node_instance));
   apx_client_delete(client);
}

static void test_apx_client_build_node_from_string2(CuTest* tc)
{
   apx_client_t* client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition2));
   apx_nodeInstance_t* node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, node_instance);
   CuAssertIntEquals(tc, 0, apx_nodeInstance_get_num_provide_ports(node_instance));
   CuAssertIntEquals(tc, 3, apx_nodeInstance_get_num_require_ports(node_instance));
   apx_client_delete(client);
}

static void test_apx_client_port_instance_without_defining_node_name1(CuTest* tc)
{
   apx_portInstance_t* uint8_port_instance;
   apx_portInstance_t* uint16_port_instance;
   apx_portInstance_t* uint32_port_instance;
   apx_nodeInstance_t* node;
   apx_client_t* client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition1));

   node = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, node);

   uint8_port_instance = apx_client_get_port_instance_by_name(client, NULL, "U8Value");
   uint16_port_instance = apx_client_get_port_instance_by_name(client, NULL, "U16Value");
   uint32_port_instance = apx_client_get_port_instance_by_name(client, NULL, "U32Value");
   CuAssertPtrEquals(tc, apx_nodeInstance_get_provide_port(node, 0), uint8_port_instance);
   CuAssertPtrEquals(tc, apx_nodeInstance_get_provide_port(node, 1), uint16_port_instance);
   CuAssertPtrEquals(tc, apx_nodeInstance_get_provide_port(node, 2), uint32_port_instance);

   apx_client_delete(client);
}

static void test_apx_client_port_instance_without_defining_node_name2(CuTest* tc)
{
   apx_portInstance_t* uint8_port_instance;
   apx_portInstance_t* uint16_port_instance;
   apx_portInstance_t* uint32_port_instance;
   apx_nodeInstance_t* node;
   apx_client_t* client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition2));

   node = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, node);

   uint8_port_instance = apx_client_get_port_instance_by_name(client, NULL, "U8Value");
   uint16_port_instance = apx_client_get_port_instance_by_name(client, NULL, "U16Value");
   uint32_port_instance = apx_client_get_port_instance_by_name(client, NULL, "U32Value");
   CuAssertPtrEquals(tc, apx_nodeInstance_get_require_port(node, 0), uint8_port_instance);
   CuAssertPtrEquals(tc, apx_nodeInstance_get_require_port(node, 1), uint16_port_instance);
   CuAssertPtrEquals(tc, apx_nodeInstance_get_require_port(node, 2), uint32_port_instance);
   apx_client_delete(client);
}

static void test_apx_client_write_port_dtl_u8(CuTest* tc)
{
   uint32_t const offset = 0;
   apx_portInstance_t* port_instance;
   uint8_t raw_data[UINT8_SIZE] = { 0 };
   apx_nodeInstance_t* node_instance;
   apx_nodeData_t* node_data;
   apx_client_t* client = apx_client_new();
   dtl_sv_t* sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition1));
   port_instance = apx_client_get_port_instance_by_name(client, NULL, "U8Value");
   node_instance = apx_client_get_last_attached_node(client);
   node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], UINT8_SIZE));
   CuAssertUIntEquals(tc, 0xffu, raw_data[0]);

   dtl_sv_set_u32(sv, 0x00);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x00, raw_data[0]);

   dtl_sv_set_u32(sv, 0x12);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], UINT8_SIZE));
   CuAssertUIntEquals(tc, 0x12u, raw_data[0]);

   dtl_sv_set_u32(sv, 0xff);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], UINT8_SIZE));
   CuAssertUIntEquals(tc, 0xffu, raw_data[0]);

   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*)sv);
}

static void test_apx_client_read_port_dtl_u8(CuTest* tc)
{
   const uint32_t offset = 0u;
   apx_portInstance_t* port_instance = NULL;
   uint8_t raw_data[UINT8_SIZE] = { 0xff };
   apx_nodeInstance_t* node_instance;
   apx_nodeData_t* node_data;
   dtl_dv_t* dv = 0;
   bool ok = false;
   apx_client_t* client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition2));
   port_instance = apx_client_get_port_instance_by_name(client, NULL, "U8Value");
   node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, port_instance);
   node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 255, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(raw_data, 0x12, UINT8_SIZE);
   apx_nodeData_write_require_port_data(node_data, offset, raw_data, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x12, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(raw_data, 0x00, UINT8_SIZE);
   apx_nodeData_write_require_port_data(node_data, offset, raw_data, UINT8_SIZE);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x00, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);

   apx_client_delete(client);
}

static void test_apx_client_write_port_dtl_u16(CuTest* tc)
{
   uint32_t const offset = UINT8_SIZE;
   apx_portInstance_t* port_instance;
   uint8_t raw_data[UINT16_SIZE] = { 0, 0 };
   apx_nodeInstance_t* node_instance;
   apx_nodeData_t* node_data;
   apx_client_t* client = apx_client_new();
   dtl_sv_t* sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition1));
   port_instance = apx_client_get_port_instance_by_name(client, NULL, "U16Value");
   node_instance = apx_client_get_last_attached_node(client);
   node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], (apx_size_t)sizeof(raw_data)));
   CuAssertUIntEquals(tc, 0xffu, raw_data[0]);
   CuAssertUIntEquals(tc, 0xffu, raw_data[1]);

   dtl_sv_set_u32(sv, 0x0000);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], (apx_size_t)sizeof(raw_data)));
   CuAssertUIntEquals(tc, 0x00, raw_data[0]);
   CuAssertUIntEquals(tc, 0x00, raw_data[1]);

   dtl_sv_set_u32(sv, 0x1234);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], (apx_size_t)sizeof(raw_data)));
   CuAssertUIntEquals(tc, 0x34u, raw_data[0]);
   CuAssertUIntEquals(tc, 0x12u, raw_data[1]);

   dtl_sv_set_u32(sv, 0xffff);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], (apx_size_t)sizeof(raw_data)));
   CuAssertUIntEquals(tc, 0xffu, raw_data[0]);
   CuAssertUIntEquals(tc, 0xffu, raw_data[1]);

   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*)sv);
}

static void test_apx_client_read_port_dtl_u16(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE;
   void* port_instance = NULL;
   uint8_t raw_data[UINT16_SIZE] = { 0xff, 0xff };
   apx_nodeInstance_t* node_instance;
   apx_nodeData_t* node_data;
   dtl_dv_t* dv = 0;
   bool ok = false;
   apx_client_t* client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition2));
   port_instance = apx_client_get_port_instance_by_name(client, NULL, "U16Value");
   node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, port_instance);
   node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, UINT16_MAX, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(raw_data, 0x1234, UINT16_SIZE);
   apx_nodeData_write_require_port_data(node_data, offset, raw_data, (apx_size_t)sizeof(raw_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x1234, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(raw_data, 0x0, UINT16_SIZE);
   apx_nodeData_write_require_port_data(node_data, offset, raw_data, (apx_size_t)sizeof(raw_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x0, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);

   apx_client_delete(client);
}

static void test_apx_client_write_port_dtl_u32(CuTest* tc)
{
   uint32_t const offset = UINT8_SIZE + UINT16_SIZE;
   void* port_instance;
   uint8_t raw_data[UINT32_SIZE] = { 0, 0, 0, 0 };
   apx_nodeInstance_t* node_instance;
   apx_nodeData_t* node_data;
   apx_client_t* client = apx_client_new();
   dtl_sv_t* sv = dtl_sv_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition1));
   port_instance = apx_client_get_port_instance_by_name(client, NULL, "U32Value");
   node_instance = apx_client_get_last_attached_node(client);
   node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], (apx_size_t)sizeof(raw_data)));
   CuAssertUIntEquals(tc, 0xffu, raw_data[0]);
   CuAssertUIntEquals(tc, 0xffu, raw_data[1]);
   CuAssertUIntEquals(tc, 0xffu, raw_data[2]);
   CuAssertUIntEquals(tc, 0xffu, raw_data[3]);

   dtl_sv_set_u32(sv, 0x0000);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], (apx_size_t)sizeof(raw_data)));
   CuAssertUIntEquals(tc, 0x00, raw_data[0]);
   CuAssertUIntEquals(tc, 0x00, raw_data[1]);
   CuAssertUIntEquals(tc, 0x00, raw_data[2]);
   CuAssertUIntEquals(tc, 0x00, raw_data[3]);

   dtl_sv_set_u32(sv, 0x12345678);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], (apx_size_t)sizeof(raw_data)));
   CuAssertUIntEquals(tc, 0x78u, raw_data[0]);
   CuAssertUIntEquals(tc, 0x56u, raw_data[1]);
   CuAssertUIntEquals(tc, 0x34u, raw_data[2]);
   CuAssertUIntEquals(tc, 0x12u, raw_data[3]);

   dtl_sv_set_u32(sv, 0xffffffff);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_write_port_data(client, port_instance, (dtl_dv_t*)sv));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_nodeData_read_provide_port_data(node_data, offset, &raw_data[0], (apx_size_t)sizeof(raw_data)));
   CuAssertUIntEquals(tc, 0xffu, raw_data[0]);
   CuAssertUIntEquals(tc, 0xffu, raw_data[1]);
   CuAssertUIntEquals(tc, 0xffu, raw_data[2]);
   CuAssertUIntEquals(tc, 0xffu, raw_data[3]);

   apx_client_delete(client);
   dtl_dec_ref((dtl_dv_t*)sv);
}

static void test_apx_client_read_port_dtl_u32(CuTest* tc)
{
   const uint32_t offset = UINT8_SIZE + UINT16_SIZE;
   void* port_instance = NULL;
   uint8_t raw_data[UINT32_SIZE] = { 0xff, 0xff,  0xff, 0xff };
   apx_nodeInstance_t* node_instance;
   apx_nodeData_t* node_data;
   dtl_dv_t* dv = 0;
   bool ok = false;
   apx_client_t* client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition2));
   port_instance = apx_client_get_port_instance_by_name(client, NULL, "U32Value");
   node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, port_instance);
   node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, UINT32_MAX, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(raw_data, 0x12345678, UINT32_SIZE);
   apx_nodeData_write_require_port_data(node_data, offset, raw_data, (apx_size_t)sizeof(raw_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x12345678, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);
   dv = 0;
   ok = false;

   packLE(raw_data, 0x0, UINT32_SIZE);
   apx_nodeData_write_require_port_data(node_data, offset, raw_data, (apx_size_t)sizeof(raw_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, &dv));
   CuAssertPtrNotNull(tc, dv);
   CuAssertUIntEquals(tc, 0x0, dtl_sv_to_u32((dtl_sv_t*)dv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref(dv);

   apx_client_delete(client);
}
