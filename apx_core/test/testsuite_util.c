//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include "CuTest.h"
#include "apx/util.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_parse_resource_name_containing_slash_is_file(CuTest* tc);
static void test_parse_resource_name_containing_invalid_ipv4_address(CuTest* tc);
static void test_parse_resource_name_containing_ipv4_address_without_port(CuTest* tc);
static void test_parse_resource_name_containing_ipv4_address_with_port(CuTest* tc);
static void test_parse_resource_name_localhost_without_port(CuTest* tc);
static void test_parse_resource_name_localhost_with_port(CuTest* tc);
static void test_parse_resource_name_port_only(CuTest* tc);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testSuite_apx_util(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_parse_resource_name_containing_slash_is_file);
   SUITE_ADD_TEST(suite, test_parse_resource_name_containing_invalid_ipv4_address);
   SUITE_ADD_TEST(suite, test_parse_resource_name_containing_ipv4_address_without_port);
   SUITE_ADD_TEST(suite, test_parse_resource_name_containing_ipv4_address_with_port);
   SUITE_ADD_TEST(suite, test_parse_resource_name_localhost_without_port);
   SUITE_ADD_TEST(suite, test_parse_resource_name_localhost_with_port);
   SUITE_ADD_TEST(suite, test_parse_resource_name_port_only);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_parse_resource_name_containing_slash_is_file(CuTest* tc)
{
   const char *socket_in_current_directory = "./test.socket";
   const char *socket_in_sub_directory = "sockets/test";
   const char *socket_in_tmp_directory = "/tmp/apx_server.socket";
   adt_str_t *address = 0;
   uint16_t port = 0u;

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_FILE, apx_parse_resource_name(socket_in_current_directory, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, socket_in_current_directory, adt_str_cstr(address));
   adt_str_delete(address);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_FILE, apx_parse_resource_name(socket_in_sub_directory, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, socket_in_sub_directory, adt_str_cstr(address));
   adt_str_delete(address);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_FILE, apx_parse_resource_name(socket_in_tmp_directory, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, socket_in_tmp_directory, adt_str_cstr(address));
   adt_str_delete(address);
}

static void test_parse_resource_name_containing_invalid_ipv4_address(CuTest* tc)
{
   const char *invalid_address_1 = "381.1.1.0";
   const char *invalid_address_2 = "127.0.01";
   const char *invalid_address_3 = "192.168..1.1";
   adt_str_t *address = 0;
   uint16_t port = 0u;

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_ERROR, apx_parse_resource_name(invalid_address_1, &address, &port));
   CuAssertPtrEquals(tc, 0, address);
   CuAssertUIntEquals(tc, 0u, port);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_ERROR, apx_parse_resource_name(invalid_address_2, &address, &port));
   CuAssertPtrEquals(tc, 0, address);
   CuAssertUIntEquals(tc, 0u, port);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_ERROR, apx_parse_resource_name(invalid_address_3, &address, &port));
   CuAssertPtrEquals(tc, 0, address);
   CuAssertUIntEquals(tc, 0u, port);


}

static void test_parse_resource_name_containing_ipv4_address_without_port(CuTest* tc)
{
   const char *address1 = "127.0.0.1";
   const char *address2 = "192.168.1.1";
   const char *address3 = "192.168.1.12";
   adt_str_t *address = 0;
   uint16_t port = 0u;

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_IPV4, apx_parse_resource_name(address1, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, address1, adt_str_cstr(address));
   CuAssertUIntEquals(tc, 0u, port);
   adt_str_delete(address);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_IPV4, apx_parse_resource_name(address2, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, address2, adt_str_cstr(address));
   CuAssertUIntEquals(tc, 0u, port);
   adt_str_delete(address);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_IPV4, apx_parse_resource_name(address3, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, address3, adt_str_cstr(address));
   CuAssertUIntEquals(tc, 0u, port);
   adt_str_delete(address);

}

static void test_parse_resource_name_containing_ipv4_address_with_port(CuTest* tc)
{
   const char *address1 = "127.0.0.1:8080";
   const char *address2 = "192.168.1.1:5000";
   const char *address3 = "192.168.1.123:5001";
   adt_str_t *address = 0;
   uint16_t port = 0u;

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_IPV4, apx_parse_resource_name(address1, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, "127.0.0.1", adt_str_cstr(address));
   CuAssertUIntEquals(tc, 8080u, port);
   adt_str_delete(address);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_IPV4, apx_parse_resource_name(address2, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, "192.168.1.1", adt_str_cstr(address));
   CuAssertUIntEquals(tc, 5000u, port);
   adt_str_delete(address);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_IPV4, apx_parse_resource_name(address3, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, "192.168.1.123", adt_str_cstr(address));
   CuAssertUIntEquals(tc, 5001u, port);
   adt_str_delete(address);
}

static void test_parse_resource_name_localhost_without_port(CuTest* tc)
{
   const char *text = "localhost";
   adt_str_t *address = 0;
   uint16_t port = 0u;

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_NAME, apx_parse_resource_name(text, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, "localhost", adt_str_cstr(address));
   CuAssertUIntEquals(tc, 0u, port);
   adt_str_delete(address);
}

static void test_parse_resource_name_localhost_with_port(CuTest* tc)
{
   const char *text = "localhost:5000";
   adt_str_t *address = 0;
   uint16_t port = 0u;

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_NAME, apx_parse_resource_name(text, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, "localhost", adt_str_cstr(address));
   CuAssertUIntEquals(tc, 5000u, port);
   adt_str_delete(address);
}

static void test_parse_resource_name_port_only(CuTest* tc)
{
   const char *text1 = ":5000";
   const char *text2 = ":8080";
   adt_str_t *address = 0;
   uint16_t port = 0u;

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_NAME, apx_parse_resource_name(text1, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, "", adt_str_cstr(address));
   CuAssertUIntEquals(tc, 5000u, port);
   adt_str_delete(address);

   CuAssertUIntEquals(tc, APX_RESOURCE_TYPE_NAME, apx_parse_resource_name(text2, &address, &port));
   CuAssertPtrNotNull(tc, address);
   CuAssertStrEquals(tc, "", adt_str_cstr(address));
   CuAssertUIntEquals(tc, 8080u, port);
   adt_str_delete(address);

}
