/*****************************************************************************
 * \file      apx_diag_client_notification.h
 * \author    Conny Gustafsson
 * \date      2026-10-30
 * \brief     Diagnostic notification data and payload ownership
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_DIAG_CLIENT_NOTIFICATION_H
#define APX_DIAG_CLIENT_NOTIFICATION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct apx_diag_client_notification_tag
{
  uint32_t subscription_id;
  uint16_t event_type; // 0x0001 = PORT_VALUE, 0x0002 = SUBSCRIPTION_INVALIDATED, 0x00FE = NOTIFICATIONS_DROPPED
  uint16_t flags;      // bit 0 = INITIAL_VALUE
  uint64_t sequence;
  uint64_t timestamp_ns;
  uint32_t node_id;
  uint32_t port_id;
  uint8_t const *value;
  uint32_t value_size;
  uint32_t reason;
  uint64_t first_sequence;
  uint64_t dropped_count;
  bool is_weak_ref; // true when value is borrowed rather than owned
} apx_diag_client_notification_t;

// Initializes an empty weak reference. self must not be NULL.
void apx_diag_client_notification_create(apx_diag_client_notification_t *self);
// Releases an owned payload and resets to an empty weak reference. NULL is allowed.
void apx_diag_client_notification_destroy(apx_diag_client_notification_t *self);
apx_diag_client_notification_t *apx_diag_client_notification_new(void);
// Additionally frees the outer heap allocation. NULL is allowed.
void apx_diag_client_notification_delete(apx_diag_client_notification_t *self);
void apx_diag_client_notification_vdelete(void *arg);
// Returns an owned deep copy, or NULL on allocation failure. other and any non-empty value must not be NULL.
apx_diag_client_notification_t *apx_diag_client_notification_clone(apx_diag_client_notification_t const *other);

#endif // APX_DIAG_CLIENT_NOTIFICATION_H
