/*****************************************************************************
 * \file      testsuite_diag_client_file.c
 * \author    Conny Gustafsson
 * \date      2026-10-30
 * \brief     Binary diagnostic file reception tests
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "CuTest.h"
#include "apx/node_data.h"
#include "apx_diag_client_file.h"
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

static void test_file_assembly(CuTest *tc)
{
  uint8_t const value[] = {0u, 1u, 0u, 3u, 4u, 5u, 6u, 7u, 8u};
  apx_diag_client_file_t *file = apx_diag_client_file_new(sizeof(value));
  CuAssertPtrNotNull(tc, file);
  CuAssertFalse(tc, apx_diag_client_file_is_complete(file));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_diag_client_file_write(file, 4u, value + 4, 5u));
  CuAssertUIntEquals(tc, 5u, file->received_size);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_diag_client_file_write(file, 4u, value + 4, 5u));
  CuAssertUIntEquals(tc, 5u, file->received_size);
  CuAssertFalse(tc, apx_diag_client_file_is_complete(file));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_diag_client_file_write(file, 0u, value, 5u));
  CuAssertTrue(tc, apx_diag_client_file_is_complete(file));
  CuAssertTrue(tc, memcmp(file->data, value, sizeof(value)) == 0);
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_diag_client_file_write(file, 8u, value, 2u));
  CuAssertIntEquals(tc, APX_BUFFER_BOUNDARY_ERROR, apx_diag_client_file_write(file, UINT32_MAX, value, 2u));
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_diag_client_file_write(file, 0u, NULL, 1u));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_diag_client_file_write(file, sizeof(value), NULL, 0u));
  apx_diag_client_file_delete(file);
}

static void test_empty_and_incomplete_file(CuTest *tc)
{
  apx_diag_client_file_t *empty = apx_diag_client_file_new(0u);
  CuAssertPtrNotNull(tc, empty);
  CuAssertTrue(tc, apx_diag_client_file_is_complete(empty));
  apx_diag_client_file_delete(empty);
  apx_diag_client_file_t *file = apx_diag_client_file_new(3u);
  CuAssertPtrNotNull(tc, file);
  CuAssertIntEquals(tc, APX_DATA_NOT_COMPLETE_ERROR, apx_diag_client_file_save(file, "must-not-be-created.out"));
  apx_diag_client_file_delete(file);
  apx_diag_client_file_delete(NULL);
}

static void test_coherent_node_data_snapshot(CuTest *tc)
{
  apx_node_data_t data;
  uint8_t out_value[] = {100u, 0u, 255u};
  uint8_t in_value[] = {1u, 0u};
  apx_node_data_buffers_t buffers = {0};
  buffers.provide_port_data = out_value;
  buffers.provide_port_data_size = sizeof(out_value);
  buffers.require_port_data = in_value;
  buffers.require_port_data_size = sizeof(in_value);
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create(&data, &buffers));
  uint8_t *out_snapshot = NULL;
  uint8_t *in_snapshot = NULL;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_take_port_data_snapshot(&data, &out_snapshot, &in_snapshot));
  CuAssertPtrNotNull(tc, out_snapshot);
  CuAssertPtrNotNull(tc, in_snapshot);
  CuAssertTrue(tc, memcmp(out_snapshot, out_value, sizeof(out_value)) == 0);
  CuAssertTrue(tc, memcmp(in_snapshot, in_value, sizeof(in_value)) == 0);
  out_value[0] = 0u;
  in_value[0] = 0u;
  CuAssertUIntEquals(tc, 100u, out_snapshot[0]);
  CuAssertUIntEquals(tc, 1u, in_snapshot[0]);
  free(out_snapshot);
  free(in_snapshot);
  apx_node_data_destroy(&data);
}

static void test_absent_and_invalid_snapshot(CuTest *tc)
{
  apx_node_data_t data;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create(&data, NULL));
  uint8_t *provide = NULL;
  uint8_t *require = NULL;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_take_port_data_snapshot(&data, &provide, &require));
  CuAssertTrue(tc, provide == NULL && require == NULL);
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_node_data_take_port_data_snapshot(NULL, &provide, &require));
  CuAssertIntEquals(tc, APX_INVALID_ARGUMENT_ERROR, apx_node_data_take_port_data_snapshot(&data, &provide, &provide));
  uint8_t const value[] = {7u};
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create_provide_port_data(&data, 1u, value, sizeof(value)));
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_take_port_data_snapshot(&data, &provide, &require));
  CuAssertPtrNotNull(tc, provide);
  CuAssertTrue(tc, require == NULL);
  CuAssertUIntEquals(tc, 7u, provide[0]);
  free(provide);
  provide = NULL;
  apx_node_data_destroy(&data);

  apx_node_data_buffers_t buffers = {0};
  buffers.require_port_data_size = 1u;
  CuAssertIntEquals(tc, APX_NO_ERROR, apx_node_data_create(&data, &buffers));
  CuAssertIntEquals(tc, APX_INVALID_STATE_ERROR, apx_node_data_take_port_data_snapshot(&data, &provide, &require));
  CuAssertTrue(tc, provide == NULL && require == NULL);
  apx_node_data_destroy(&data);
}

CuSuite *testsuite_apx_diag_client_file(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_file_assembly);
  SUITE_ADD_TEST(suite, test_empty_and_incomplete_file);
  SUITE_ADD_TEST(suite, test_coherent_node_data_snapshot);
  SUITE_ADD_TEST(suite, test_absent_and_invalid_snapshot);
  return suite;
}
