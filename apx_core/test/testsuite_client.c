/*****************************************************************************
* \file      testsuite_client.c
* \author    Conny Gustafsson
* \date      2019-08-04
* \brief     Unit tests for apx_client
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
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

static const char *m_apx_definition3 = "APX/1.2\n"
      "N\"TestNode3\"\n"
      "T\"RecordType\"{\"First\"C(0,7)\"Second\"C[4]}\n"
      "R\"RecordPort\"T[0]:={7,{0,0,0,0}}\n"
      "\n";

static const char *m_apx_definition4 = "APX/1.3\n"
      "N\"TestNode4\"\n"
      "T\"ArrayType\"{\"First\"C(0,15)\"Second\"C(0,15)}[3]\n"
      "R\"ArrayPort\"T[0]:={{0xf,0xf},{0xf,0xf},{0xf,0xf}}\n"
      "\n";
/*
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
static void test_apx_client_read_struct_with_array(CuTest* tc);
static void test_apx_client_read_array_of_structs(CuTest* tc);


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
   SUITE_ADD_TEST(suite, test_apx_client_read_struct_with_array);
   SUITE_ADD_TEST(suite, test_apx_client_read_array_of_structs);


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

static void test_apx_client_read_struct_with_array(CuTest* tc)
{
   const uint32_t offset = 0;
   void* port_instance = NULL;
   uint8_t raw_data[UINT8_SIZE*5] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
   apx_nodeInstance_t* node_instance;
   apx_nodeData_t* node_data;
   dtl_hv_t* hv = NULL;
   dtl_sv_t* child_sv = NULL;
   dtl_av_t* child_av = NULL;
   bool ok = false;
   apx_client_t* client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition3));
   port_instance = apx_client_get_port_instance_by_name(client, NULL, "RecordPort");
   CuAssertPtrNotNull(tc, port_instance);
   node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, node_instance);
   node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   apx_nodeData_write_require_port_data(node_data, offset, raw_data, (apx_size_t)sizeof(raw_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, (dtl_dv_t**)&hv));
   CuAssertPtrNotNull(tc, hv);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x01, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_av = (dtl_av_t*)dtl_hv_get_cstr(hv, "Second");
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type((dtl_dv_t*)child_av));
   CuAssertPtrNotNull(tc, child_av);
   CuAssertIntEquals(tc, 4, dtl_av_length(child_av));
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 0);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertIntEquals(tc, DTL_DV_SCALAR, dtl_dv_type((dtl_dv_t*)child_sv));
   CuAssertUIntEquals(tc, 0x02, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 1);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x03, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 2);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x04, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_av_value(child_av, 3);
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x05, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   dtl_dv_dec_ref((dtl_dv_t*)hv);
   apx_client_delete(client);
}

static void test_apx_client_read_array_of_structs(CuTest* tc)
{
   const uint32_t offset = 0;
   void* port_instance = NULL;
   uint8_t raw_data[UINT8_SIZE*6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
   apx_nodeInstance_t* node_instance;
   apx_nodeData_t* node_data;
   dtl_av_t* av = NULL;
   dtl_hv_t* child_hv = NULL;
   dtl_sv_t* child_sv = NULL;
   bool ok = false;
   apx_client_t* client;

   client = apx_client_new();
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_build_node(client, m_apx_definition4));
   port_instance = apx_client_get_port_instance_by_name(client, NULL, "ArrayPort");
   CuAssertPtrNotNull(tc, port_instance);
   node_instance = apx_client_get_last_attached_node(client);
   CuAssertPtrNotNull(tc, node_instance);
   node_data = apx_nodeInstance_get_node_data(node_instance);
   CuAssertPtrNotNull(tc, node_data);
   apx_nodeData_write_require_port_data(node_data, offset, raw_data, (apx_size_t)sizeof(raw_data));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_client_read_port_data(client, port_instance, (dtl_dv_t**)&av));
   CuAssertPtrNotNull(tc, av);
   CuAssertIntEquals(tc, DTL_DV_ARRAY, dtl_dv_type((dtl_dv_t*)av));
   CuAssertIntEquals(tc, 3, dtl_av_length(av));
   child_hv = (dtl_hv_t*)dtl_av_value(av, 0);
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type((dtl_dv_t*)child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x01, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x02, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_hv = (dtl_hv_t*)dtl_av_value(av, 1);
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type((dtl_dv_t*)child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x03, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x04, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_hv = (dtl_hv_t*)dtl_av_value(av, 2);
   CuAssertPtrNotNull(tc, child_hv);
   CuAssertIntEquals(tc, DTL_DV_HASH, dtl_dv_type((dtl_dv_t*)child_hv));
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "First");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x05, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);
   child_sv = (dtl_sv_t*)dtl_hv_get_cstr(child_hv, "Second");
   CuAssertPtrNotNull(tc, child_sv);
   CuAssertUIntEquals(tc, 0x06, dtl_sv_to_u32(child_sv, &ok));
   CuAssertTrue(tc, ok);

   dtl_dv_dec_ref((dtl_dv_t*)av);
   apx_client_delete(client);
}