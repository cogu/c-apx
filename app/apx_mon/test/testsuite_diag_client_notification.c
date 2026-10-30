/*****************************************************************************
 * \file      testsuite_diag_client_notification.c
 * \author    Conny Gustafsson
 * \date      2026-10-30
 * \brief     Unit tests for diagnostic notification ownership
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "CuTest.h"
#include "adt_ary.h"
#include "apx_diag_client_notification.h"
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

static void test_create_defaults(CuTest *tc);
static void test_destroy_weak_reference(CuTest *tc);
static void test_new_delete_weak_reference(CuTest *tc);
static void test_clone_weak_reference(CuTest *tc);
static void test_clone_owned_reference(CuTest *tc);
static void test_clone_empty_notifications(CuTest *tc);
static void test_destroy_owned_value(CuTest *tc);
static void test_container_destructor(CuTest *tc);
static void assert_empty(CuTest *tc, apx_diag_client_notification_t const *notification);
static void create_test_notification(apx_diag_client_notification_t *self, uint8_t const *value, uint32_t value_size);
static void assert_metadata_equal(
  CuTest *tc, apx_diag_client_notification_t const *expected, apx_diag_client_notification_t const *actual);

CuSuite *testsuite_apx_diag_client_notification(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_create_defaults);
  SUITE_ADD_TEST(suite, test_destroy_weak_reference);
  SUITE_ADD_TEST(suite, test_new_delete_weak_reference);
  SUITE_ADD_TEST(suite, test_clone_weak_reference);
  SUITE_ADD_TEST(suite, test_clone_owned_reference);
  SUITE_ADD_TEST(suite, test_clone_empty_notifications);
  SUITE_ADD_TEST(suite, test_destroy_owned_value);
  SUITE_ADD_TEST(suite, test_container_destructor);
  return suite;
}

static void test_create_defaults(CuTest *tc)
{
  apx_diag_client_notification_t notification;
  apx_diag_client_notification_create(&notification);
  assert_empty(tc, &notification);
  apx_diag_client_notification_destroy(&notification);
  assert_empty(tc, &notification);
}

static void test_destroy_weak_reference(CuTest *tc)
{
  uint8_t value[] = {0x64u, 0x00u};
  apx_diag_client_notification_t notification;
  create_test_notification(&notification, value, sizeof(value));
  CuAssertTrue(tc, notification.is_weak_ref);
  apx_diag_client_notification_destroy(&notification);
  assert_empty(tc, &notification);
  CuAssertUIntEquals(tc, 0x64u, value[0]);
  CuAssertUIntEquals(tc, 0x00u, value[1]);
  apx_diag_client_notification_destroy(&notification);
}

static void test_new_delete_weak_reference(CuTest *tc)
{
  uint8_t value[] = {0x64u, 0x00u};
  apx_diag_client_notification_t *notification = apx_diag_client_notification_new();
  CuAssertPtrNotNull(tc, notification);
  assert_empty(tc, notification);
  notification->value = value;
  notification->value_size = sizeof(value);
  apx_diag_client_notification_delete(notification);
  CuAssertUIntEquals(tc, 0x64u, value[0]);
  CuAssertUIntEquals(tc, 0x00u, value[1]);
  apx_diag_client_notification_destroy(NULL);
  apx_diag_client_notification_delete(NULL);
  apx_diag_client_notification_vdelete(NULL);
}

static void test_clone_weak_reference(CuTest *tc)
{
  uint8_t value[] = {0x64u, 0x00u, 0xA5u};
  uint8_t const expected[] = {0x64u, 0x00u, 0xA5u};
  apx_diag_client_notification_t source;
  create_test_notification(&source, value, sizeof(value));
  apx_diag_client_notification_t *copy = apx_diag_client_notification_clone(&source);
  CuAssertPtrNotNull(tc, copy);
  CuAssertFalse(tc, copy->is_weak_ref);
  CuAssertTrue(tc, source.is_weak_ref);
  CuAssertTrue(tc, copy->value != source.value);
  assert_metadata_equal(tc, &source, copy);
  CuAssertTrue(tc, memcmp(copy->value, expected, sizeof(expected)) == 0);

  value[0] = 0xFFu;
  apx_diag_client_notification_destroy(&source);
  CuAssertTrue(tc, memcmp(copy->value, expected, sizeof(expected)) == 0);
  apx_diag_client_notification_delete(copy);
}

static void test_clone_owned_reference(CuTest *tc)
{
  uint8_t const value[] = {0x0Fu};
  apx_diag_client_notification_t source;
  create_test_notification(&source, value, sizeof(value));
  apx_diag_client_notification_t *first = apx_diag_client_notification_clone(&source);
  CuAssertPtrNotNull(tc, first);
  apx_diag_client_notification_t *second = apx_diag_client_notification_clone(first);
  CuAssertPtrNotNull(tc, second);
  CuAssertFalse(tc, first->is_weak_ref);
  CuAssertFalse(tc, second->is_weak_ref);
  CuAssertTrue(tc, first->value != second->value);
  assert_metadata_equal(tc, first, second);
  apx_diag_client_notification_delete(first);
  CuAssertUIntEquals(tc, 0x0Fu, second->value[0]);
  apx_diag_client_notification_delete(second);
}

static void test_clone_empty_notifications(CuTest *tc)
{
  uint16_t const event_types[] = {0x0001u, 0x0002u, 0x00FEu};
  for (size_t i = 0; i < sizeof(event_types) / sizeof(event_types[0]); ++i)
  {
    apx_diag_client_notification_t source;
    create_test_notification(&source, NULL, 0u);
    source.event_type = event_types[i];
    apx_diag_client_notification_t *copy = apx_diag_client_notification_clone(&source);
    CuAssertPtrNotNull(tc, copy);
    CuAssertFalse(tc, copy->is_weak_ref);
    CuAssertTrue(tc, copy->value == NULL);
    assert_metadata_equal(tc, &source, copy);
    apx_diag_client_notification_delete(copy);
  }
}

static void test_destroy_owned_value(CuTest *tc)
{
  uint8_t const value[] = {0x64u, 0x00u};
  apx_diag_client_notification_t source;
  create_test_notification(&source, value, sizeof(value));
  apx_diag_client_notification_t *copy = apx_diag_client_notification_clone(&source);
  CuAssertPtrNotNull(tc, copy);
  CuAssertFalse(tc, copy->is_weak_ref);
  apx_diag_client_notification_destroy(copy);
  assert_empty(tc, copy);
  apx_diag_client_notification_destroy(copy);
  assert_empty(tc, copy);
  apx_diag_client_notification_delete(copy);
}

static void test_container_destructor(CuTest *tc)
{
  uint8_t const value[] = {0x64u, 0x00u};
  apx_diag_client_notification_t source;
  create_test_notification(&source, value, sizeof(value));
  adt_ary_t notifications;
  adt_ary_create(&notifications, apx_diag_client_notification_vdelete);
  for (int i = 0; i < 3; ++i)
  {
    apx_diag_client_notification_t *copy = apx_diag_client_notification_clone(&source);
    CuAssertPtrNotNull(tc, copy);
    CuAssertIntEquals(tc, ADT_NO_ERROR, adt_ary_push(&notifications, copy));
  }
  adt_ary_clear(&notifications);
  CuAssertIntEquals(tc, 0, adt_ary_length(&notifications));
  apx_diag_client_notification_t *copy = apx_diag_client_notification_clone(&source);
  CuAssertPtrNotNull(tc, copy);
  CuAssertIntEquals(tc, ADT_NO_ERROR, adt_ary_push(&notifications, copy));
  adt_ary_destroy(&notifications);
}

static void assert_empty(CuTest *tc, apx_diag_client_notification_t const *notification)
{
  CuAssertTrue(tc, notification->is_weak_ref);
  CuAssertTrue(tc, notification->value == NULL);
  CuAssertUIntEquals(tc, 0u, notification->value_size);
  CuAssertUIntEquals(tc, 0u, notification->subscription_id);
  CuAssertUIntEquals(tc, 0u, notification->event_type);
  CuAssertUIntEquals(tc, 0u, notification->flags);
  CuAssertULIntEquals(tc, 0u, notification->sequence);
  CuAssertULIntEquals(tc, 0u, notification->timestamp_ns);
  CuAssertUIntEquals(tc, 0u, notification->node_id);
  CuAssertUIntEquals(tc, 0u, notification->port_id);
  CuAssertUIntEquals(tc, 0u, notification->reason);
  CuAssertULIntEquals(tc, 0u, notification->first_sequence);
  CuAssertULIntEquals(tc, 0u, notification->dropped_count);
}

static void create_test_notification(apx_diag_client_notification_t *self, uint8_t const *value, uint32_t value_size)
{
  apx_diag_client_notification_create(self);
  self->subscription_id = 7u;
  self->event_type = 0x0001u;
  self->flags = 1u;
  self->sequence = UINT64_C(0x100000002);
  self->timestamp_ns = UINT64_C(510396415835);
  self->node_id = 3u;
  self->port_id = 5u;
  self->value = value;
  self->value_size = value_size;
  self->reason = 12u;
  self->first_sequence = UINT64_C(0x200000003);
  self->dropped_count = UINT64_C(0x300000004);
}

static void assert_metadata_equal(
  CuTest *tc, apx_diag_client_notification_t const *expected, apx_diag_client_notification_t const *actual)
{
  CuAssertUIntEquals(tc, expected->subscription_id, actual->subscription_id);
  CuAssertUIntEquals(tc, expected->event_type, actual->event_type);
  CuAssertUIntEquals(tc, expected->flags, actual->flags);
  CuAssertULIntEquals(tc, expected->sequence, actual->sequence);
  CuAssertULIntEquals(tc, expected->timestamp_ns, actual->timestamp_ns);
  CuAssertUIntEquals(tc, expected->node_id, actual->node_id);
  CuAssertUIntEquals(tc, expected->port_id, actual->port_id);
  CuAssertUIntEquals(tc, expected->value_size, actual->value_size);
  CuAssertUIntEquals(tc, expected->reason, actual->reason);
  CuAssertULIntEquals(tc, expected->first_sequence, actual->first_sequence);
  CuAssertULIntEquals(tc, expected->dropped_count, actual->dropped_count);
}
