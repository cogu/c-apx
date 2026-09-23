/*****************************************************************************
* \file      testsuite_node_data.c
* \author    Conny Gustafsson
* \date      2019-05-27
* \brief     Unit tests for node data
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "CuTest.h"
#include "apx/parser.h"
#include "apx/node_instance.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_create_empty_node_data(CuTest *tc);
static void test_write_provide_port_data_uint8(CuTest* tc);
static void test_write_provide_port_data_uint16(CuTest* tc);
static void test_write_require_port_data_uint8(CuTest* tc);
static void test_write_require_port_data_uint16(CuTest* tc);
static void test_take_provide_port_data_snapshot(CuTest* tc);
static void test_node_data_provide_port_connection_counts(CuTest* tc);
static void test_node_data_require_port_connection_counts(CuTest* tc);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite* testsuite_apx_node_data(void)
{
   CuSuite* suite = CuSuiteNew();

   SUITE_ADD_TEST(suite, test_create_empty_node_data);
   SUITE_ADD_TEST(suite, test_write_provide_port_data_uint8);
   SUITE_ADD_TEST(suite, test_write_provide_port_data_uint16);
   SUITE_ADD_TEST(suite, test_write_require_port_data_uint8);
   SUITE_ADD_TEST(suite, test_write_require_port_data_uint16);
   SUITE_ADD_TEST(suite, test_take_provide_port_data_snapshot);
   SUITE_ADD_TEST(suite, test_node_data_provide_port_connection_counts);
   SUITE_ADD_TEST(suite, test_node_data_require_port_connection_counts);

   return suite;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void test_create_empty_node_data(CuTest *tc)
{
   apx_node_data_t * node_data;
   node_data =  apx_node_data_new();
   CuAssertPtrNotNull(tc, node_data);
   CuAssertUIntEquals(tc, 0u, apx_node_data_definition_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_node_data_provide_port_data_size(node_data));
   CuAssertUIntEquals(tc, 0u, apx_node_data_require_port_data_size(node_data));
   apx_node_data_delete(node_data);
}

static void test_write_provide_port_data_uint8(CuTest* tc)
{
   apx_node_data_t* node_data;
   uint8_t const init_data[UINT8_SIZE] = { 0x07u };
   uint8_t buf[sizeof(init_data)];
   uint8_t new_value[sizeof(init_data)] = { 0x03 };
   node_data = apx_node_data_new();
   memset(buf, 0, sizeof(buf));
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create_provide_port_data(node_data, 1u, init_data, sizeof(init_data)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_provide_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x07, buf[0]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_write_provide_port_data(node_data, 0u, new_value, sizeof(new_value)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_provide_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x03, buf[0]);
   apx_node_data_delete(node_data);
}

static void test_write_provide_port_data_uint16(CuTest* tc)
{
   apx_node_data_t* node_data;
   uint8_t const init_data[UINT16_SIZE] = { 0xffu, 0xffu };
   uint8_t buf[sizeof(init_data)];
   uint8_t new_value[sizeof(init_data)] = { 0x34u, 0x12u };
   node_data = apx_node_data_new();
   memset(buf, 0, sizeof(buf));
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create_provide_port_data(node_data, 1u, init_data, sizeof(init_data)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_provide_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0xff, buf[0]);
   CuAssertUIntEquals(tc, 0xff, buf[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_write_provide_port_data(node_data, 0u, new_value, sizeof(new_value)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_provide_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x34, buf[0]);
   CuAssertUIntEquals(tc, 0x12, buf[1]);
   apx_node_data_delete(node_data);
}

static void test_write_require_port_data_uint8(CuTest* tc)
{
   apx_node_data_t* node_data;
   uint8_t const init_data[UINT8_SIZE] = { 0x07u };
   uint8_t buf[sizeof(init_data)];
   uint8_t new_value[sizeof(init_data)] = { 0x03 };
   node_data = apx_node_data_new();
   memset(buf, 0, sizeof(buf));
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create_require_port_data(node_data, 1u, init_data, sizeof(init_data)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_require_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x07, buf[0]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_write_require_port_data(node_data, 0u, new_value, sizeof(new_value)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_require_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x03, buf[0]);
   apx_node_data_delete(node_data);
}

static void test_write_require_port_data_uint16(CuTest* tc)
{
   apx_node_data_t* node_data;
   uint8_t const init_data[UINT16_SIZE] = { 0xffu, 0xffu };
   uint8_t buf[sizeof(init_data)];
   uint8_t new_value[sizeof(init_data)] = { 0x34u, 0x12u };
   node_data = apx_node_data_new();
   memset(buf, 0, sizeof(buf));
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create_require_port_data(node_data, 1u, init_data, sizeof(init_data)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_require_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0xff, buf[0]);
   CuAssertUIntEquals(tc, 0xff, buf[1]);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_write_require_port_data(node_data, 0u, new_value, sizeof(new_value)));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_require_port_data(node_data, 0u, buf, sizeof(buf)));
   CuAssertUIntEquals(tc, 0x34, buf[0]);
   CuAssertUIntEquals(tc, 0x12, buf[1]);
   apx_node_data_delete(node_data);
}

static void test_take_provide_port_data_snapshot(CuTest* tc)
{
   apx_node_data_t* node_data;
   uint8_t const init_data[UINT32_SIZE] = { 0x78, 0x56, 0x34, 0x12 };
   uint8_t* snapshot;
   node_data = apx_node_data_new();
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create_provide_port_data(node_data, 1u, init_data, sizeof(init_data)));
   snapshot = apx_node_data_take_provide_port_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, snapshot);
   CuAssertUIntEquals(tc, 0x78, snapshot[0]);
   CuAssertUIntEquals(tc, 0x56, snapshot[1]);
   CuAssertUIntEquals(tc, 0x34, snapshot[2]);
   CuAssertUIntEquals(tc, 0x12, snapshot[3]);
   free(snapshot);
   apx_node_data_delete(node_data);
}

static void test_node_data_provide_port_connection_counts(CuTest* tc)
{
   apx_node_data_t* node_data = apx_node_data_new();
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create_provide_port_connection_count_buffer(node_data, 3u));
   CuAssertUIntEquals(tc, 3u * sizeof(uint16_t), apx_node_data_provide_port_connection_count_data_size(node_data));

   // Verify initial counts are zero
   CuAssertUIntEquals(tc, 0u, apx_node_data_get_provide_port_connection_count(node_data, 0));
   CuAssertUIntEquals(tc, 0u, apx_node_data_get_provide_port_connection_count(node_data, 1));
   CuAssertUIntEquals(tc, 0u, apx_node_data_get_provide_port_connection_count(node_data, 2));

   // Test set and get
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_set_provide_port_connection_count(node_data, 0, 5u));
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_set_provide_port_connection_count(node_data, 2, 42u));
   CuAssertUIntEquals(tc, 5u, apx_node_data_get_provide_port_connection_count(node_data, 0));
   CuAssertUIntEquals(tc, 0u, apx_node_data_get_provide_port_connection_count(node_data, 1));
   CuAssertUIntEquals(tc, 42u, apx_node_data_get_provide_port_connection_count(node_data, 2));

   // Test inc and dec
   apx_node_data_inc_provide_port_connection_count(node_data, 1);
   CuAssertUIntEquals(tc, 1u, apx_node_data_get_provide_port_connection_count(node_data, 1));
   apx_node_data_dec_provide_port_connection_count(node_data, 0);
   CuAssertUIntEquals(tc, 4u, apx_node_data_get_provide_port_connection_count(node_data, 0));

   // Test snapshot
   uint8_t* snapshot = apx_node_data_take_provide_port_count_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, snapshot);
   // Port 0: count 4 (0x0004 LE)
   CuAssertUIntEquals(tc, 0x04u, snapshot[0]);
   CuAssertUIntEquals(tc, 0x00u, snapshot[1]);
   // Port 1: count 1 (0x0001 LE)
   CuAssertUIntEquals(tc, 0x01u, snapshot[2]);
   CuAssertUIntEquals(tc, 0x00u, snapshot[3]);
   // Port 2: count 42 (0x002A LE)
   CuAssertUIntEquals(tc, 0x2Au, snapshot[4]);
   CuAssertUIntEquals(tc, 0x00u, snapshot[5]);
   free(snapshot);

   // Test byte write and read
   uint8_t write_bytes[4] = { 0x10u, 0x00u, 0x20u, 0x00u };
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_write_provide_port_count_data(node_data, 0u, write_bytes, sizeof(write_bytes)));
   CuAssertUIntEquals(tc, 16u, apx_node_data_get_provide_port_connection_count(node_data, 0));
   CuAssertUIntEquals(tc, 32u, apx_node_data_get_provide_port_connection_count(node_data, 1));

   uint8_t read_bytes[6];
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_read_provide_port_count_data(node_data, 0u, read_bytes, sizeof(read_bytes)));
   CuAssertUIntEquals(tc, 0x10u, read_bytes[0]);
   CuAssertUIntEquals(tc, 0x00u, read_bytes[1]);
   CuAssertUIntEquals(tc, 0x20u, read_bytes[2]);
   CuAssertUIntEquals(tc, 0x00u, read_bytes[3]);
   CuAssertUIntEquals(tc, 0x2Au, read_bytes[4]);
   CuAssertUIntEquals(tc, 0x00u, read_bytes[5]);

   apx_node_data_delete(node_data);
}

static void test_node_data_require_port_connection_counts(CuTest* tc)
{
   apx_node_data_t* node_data = apx_node_data_new();
   CuAssertPtrNotNull(tc, node_data);
   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create_require_port_connection_count_buffer(node_data, 2u));
   CuAssertUIntEquals(tc, 2u * sizeof(uint16_t), apx_node_data_require_port_connection_count_data_size(node_data));

   CuAssertUIntEquals(tc, 0u, apx_node_data_get_require_port_connection_count(node_data, 0));
   CuAssertUIntEquals(tc, 0u, apx_node_data_get_require_port_connection_count(node_data, 1));

   CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_set_require_port_connection_count(node_data, 0, 1u));
   CuAssertUIntEquals(tc, 1u, apx_node_data_get_require_port_connection_count(node_data, 0));

   apx_node_data_inc_require_port_connection_count(node_data, 1);
   CuAssertUIntEquals(tc, 1u, apx_node_data_get_require_port_connection_count(node_data, 1));
   apx_node_data_dec_require_port_connection_count(node_data, 0);
   CuAssertUIntEquals(tc, 0u, apx_node_data_get_require_port_connection_count(node_data, 0));

   uint8_t* snapshot = apx_node_data_take_require_port_count_data_snapshot(node_data);
   CuAssertPtrNotNull(tc, snapshot);
   CuAssertUIntEquals(tc, 0x00u, snapshot[0]);
   CuAssertUIntEquals(tc, 0x00u, snapshot[1]);
   CuAssertUIntEquals(tc, 0x01u, snapshot[2]);
   CuAssertUIntEquals(tc, 0x00u, snapshot[3]);
   free(snapshot);

   apx_node_data_delete(node_data);
}

