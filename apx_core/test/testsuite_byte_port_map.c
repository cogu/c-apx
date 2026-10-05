/*****************************************************************************
 * \file      testsuite_byte_port_map.c
 * \author    Conny Gustafsson
 * \date      2026-10-03
 * \brief     Unit tests for byte_port_map using partition indexing
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "CuTest.h"
#include "apx/byte_port_map.h"
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void test_byte_port_map_u16_small(CuTest *tc);
static void test_byte_port_map_u16_large_node(CuTest *tc);
static void test_byte_port_map_u32_exceeds_16bit(CuTest *tc);
static void test_byte_port_map_invalid_args(CuTest *tc);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
CuSuite *testsuite_byte_port_map(void)
{
  CuSuite *suite = CuSuiteNew();

  SUITE_ADD_TEST(suite, test_byte_port_map_u16_small);
  SUITE_ADD_TEST(suite, test_byte_port_map_u16_large_node);
  SUITE_ADD_TEST(suite, test_byte_port_map_u32_exceeds_16bit);
  SUITE_ADD_TEST(suite, test_byte_port_map_invalid_args);

  return suite;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void test_byte_port_map_u16_small(CuTest *tc)
{
  apx_port_instance_t ports[3];
  memset(ports, 0, sizeof(ports));
  ports[0].data_size = 4u;
  ports[1].data_size = 2u;
  ports[2].data_size = 1u;

  apx_error_t err = APX_NO_ERROR;
  apx_byte_port_map_t *map = apx_byte_port_map_new(7u, ports, 3u, &err);
  CuAssertPtrNotNull(tc, map);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  CuAssertUIntEquals(tc, APX_PARTITION_U16, apx_byte_port_map_type(map));
  CuAssertUIntEquals(tc, 7u, apx_byte_port_map_length(map));

   /* Port 0: offsets 0..3 */
  CuAssertIntEquals(tc, 0, apx_byte_port_map_lookup(map, 0u));
  CuAssertIntEquals(tc, 0, apx_byte_port_map_lookup(map, 1u));
  CuAssertIntEquals(tc, 0, apx_byte_port_map_lookup(map, 2u));
  CuAssertIntEquals(tc, 0, apx_byte_port_map_lookup(map, 3u));

   /* Port 1: offsets 4..5 */
  CuAssertIntEquals(tc, 1, apx_byte_port_map_lookup(map, 4u));
  CuAssertIntEquals(tc, 1, apx_byte_port_map_lookup(map, 5u));

   /* Port 2: offset 6 */
  CuAssertIntEquals(tc, 2, apx_byte_port_map_lookup(map, 6u));

   /* Out of bounds */
  CuAssertIntEquals(tc, APX_INVALID_PORT_ID, apx_byte_port_map_lookup(map, 7u));
  CuAssertIntEquals(tc, APX_INVALID_PORT_ID, apx_byte_port_map_lookup(map, 100u));

  apx_byte_port_map_delete(map);
}

static void test_byte_port_map_u16_large_node(CuTest *tc)
{
   /* Simulate MainDisplay provide ports: 1223 ports, total size 3638 bytes */
  const uint32_t num_ports = 1223u;
  apx_port_instance_t *ports = (apx_port_instance_t *)calloc(num_ports, sizeof(apx_port_instance_t));
  CuAssertPtrNotNull(tc, ports);

   /* 5-element repeating pattern summing to 16 bytes across 5 ports (average 3.2 bytes/port) */
  const uint32_t sizes[5] = {1u, 2u, 4u, 1u, 8u};
  uint32_t total_size = 0u;
  uint32_t i;
  for (i = 0u; i < num_ports; i++)
  {
    ports[i].data_size = sizes[i % 5u];
    total_size += ports[i].data_size;
  }

  apx_error_t err = APX_NO_ERROR;
  apx_byte_port_map_t *map = apx_byte_port_map_new(total_size, ports, num_ports, &err);
  CuAssertPtrNotNull(tc, map);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  CuAssertUIntEquals(tc, APX_PARTITION_U16, apx_byte_port_map_type(map));
  CuAssertUIntEquals(tc, total_size, apx_byte_port_map_length(map));

   /* Verify every single byte offset maps to the correct port */
  uint32_t current_offset = 0u;
  for (i = 0u; i < num_ports; i++)
  {
    uint32_t sz = ports[i].data_size;
    uint32_t byte_idx;
    for (byte_idx = 0u; byte_idx < sz; byte_idx++)
    {
      apx_port_id_t port_id = apx_byte_port_map_lookup(map, current_offset + byte_idx);
      CuAssertIntEquals(tc, (apx_port_id_t)i, port_id);
    }
    current_offset += sz;
  }
  CuAssertIntEquals(tc, APX_INVALID_PORT_ID, apx_byte_port_map_lookup(map, total_size));

  apx_byte_port_map_delete(map);
  free(ports);
}

static void test_byte_port_map_u32_exceeds_16bit(CuTest *tc)
{
   /* Total size > 65535 bytes triggers APX_PARTITION_U32 */
  apx_port_instance_t ports[2];
  memset(ports, 0, sizeof(ports));
  ports[0].data_size = 40000u;
  ports[1].data_size = 30000u;
  const uint32_t total_size = 70000u;

  apx_error_t err = APX_NO_ERROR;
  apx_byte_port_map_t *map = apx_byte_port_map_new(total_size, ports, 2u, &err);
  CuAssertPtrNotNull(tc, map);
  CuAssertIntEquals(tc, APX_NO_ERROR, err);

  CuAssertUIntEquals(tc, APX_PARTITION_U32, apx_byte_port_map_type(map));
  CuAssertUIntEquals(tc, total_size, apx_byte_port_map_length(map));

   /* Port 0: 0..39999 */
  CuAssertIntEquals(tc, 0, apx_byte_port_map_lookup(map, 0u));
  CuAssertIntEquals(tc, 0, apx_byte_port_map_lookup(map, 39999u));

   /* Port 1: 40000..69999 */
  CuAssertIntEquals(tc, 1, apx_byte_port_map_lookup(map, 40000u));
  CuAssertIntEquals(tc, 1, apx_byte_port_map_lookup(map, 69999u));

   /* Out of bounds */
  CuAssertIntEquals(tc, APX_INVALID_PORT_ID, apx_byte_port_map_lookup(map, 70000u));

  apx_byte_port_map_delete(map);
}

static void test_byte_port_map_invalid_args(CuTest *tc)
{
  apx_port_instance_t ports[1];
  memset(ports, 0, sizeof(ports));
  ports[0].data_size = 10u;

  apx_error_t err = APX_NO_ERROR;

   /* NULL ports pointer */
  apx_byte_port_map_t *map = apx_byte_port_map_new(10u, NULL, 1u, &err);
  CuAssertPtrEquals(tc, NULL, map);
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, err);

   /* Zero num_ports */
  map = apx_byte_port_map_new(10u, ports, 0u, &err);
  CuAssertPtrEquals(tc, NULL, map);
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, err);

   /* Zero total_size */
  map = apx_byte_port_map_new(0u, ports, 1u, &err);
  CuAssertPtrEquals(tc, NULL, map);
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, err);

   /* Mismatched total_size (sum of ports = 10, total_size specified = 12) */
  map = apx_byte_port_map_new(12u, ports, 1u, &err);
  CuAssertPtrEquals(tc, NULL, map);
  CuAssertIntEquals(tc, APX_LENGTH_ERROR, err);
}
