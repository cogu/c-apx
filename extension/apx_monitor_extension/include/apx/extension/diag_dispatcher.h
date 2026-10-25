/*****************************************************************************
 * \file      diag_dispatcher.h
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Interactive diagnostics query dispatcher (apxmon)
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_DIAG_DISPATCHER_H
#define APX_DIAG_DISPATCHER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "apx/error.h"
#include "apx/types.h"
#include "osmacro.h"
#include <stdbool.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_DIAG_HEADER_SIZE ((uint32_t)8u)
#define APX_DIAG_RESPONSE_HEADER_SIZE ((uint32_t)12u)
#define APX_DIAG_NOTIFY_HEADER_SIZE ((uint32_t)24u)

#define APX_DIAG_CMD_GET_SERVER_INFO ((uint16_t)0x0001u)
#define APX_DIAG_CMD_LIST_CONNECTIONS ((uint16_t)0x0002u)
#define APX_DIAG_CMD_LIST_NODES ((uint16_t)0x0003u)
#define APX_DIAG_CMD_LIST_ROUTES ((uint16_t)0x0004u)
#define APX_DIAG_CMD_LIST_UNCONNECTED_PORTS ((uint16_t)0x0005u)
#define APX_DIAG_CMD_GET_NODE_DEFINITION ((uint16_t)0x0006u)
#define APX_DIAG_CMD_SUBSCRIBE_PORTS ((uint16_t)0x0010u)
#define APX_DIAG_CMD_UNSUBSCRIBE ((uint16_t)0x0011u)

#define APX_DIAG_NOTIF_PORT_VALUE ((uint16_t)0x0001u)
#define APX_DIAG_NOTIF_SUBSCRIPTION_INVALIDATED ((uint16_t)0x0002u)
#define APX_DIAG_NOTIF_NOTIFICATIONS_DROPPED ((uint16_t)0x00FEu)

#define APX_DIAG_FLAG_SEND_INITIAL ((uint16_t)0x0001u)
#define APX_DIAG_FLAG_INITIAL_VALUE ((uint16_t)0x0001u)

#define APX_DIAG_MAX_LIST_ITEMS 1024u
#define APX_DIAG_MAX_SELECTORS 256u
#define APX_DIAG_CURSOR_END 0xFFFFFFFFu

struct apx_file_tag;
struct apx_monitor_session_tag;

typedef struct apx_diag_port_watch_tag
{
  uint32_t node_id;
  uint32_t port_id;
  uint8_t port_type;
  uint32_t data_offset;
  uint32_t value_size;
  char *node_name;
  char *port_name;
} apx_diag_port_watch_t;

typedef struct apx_diag_subscription_tag
{
  uint32_t subscription_id;
  adt_ary_t watches; // strong references to apx_diag_port_watch_t*
} apx_diag_subscription_t;

typedef struct apx_diag_snapshot_tag
{
  uint64_t snapshot_id;
  uint16_t opcode;
  adt_ary_t items; // array of allocated item pointers
  uint64_t created_at_ns;
} apx_diag_snapshot_t;

typedef struct apx_diag_temp_file_tag
{
  uint32_t request_id;
  uint32_t address;
  char *file_name;
  uint8_t *data;
  uint32_t size;
} apx_diag_temp_file_t;

typedef struct apx_diag_dispatcher_tag
{
  struct apx_monitor_session_tag *session;
  uint64_t next_snapshot_id;
  uint32_t next_subscription_id;
  uint64_t notify_sequence;
  adt_ary_t snapshots;     // strong references to apx_diag_snapshot_t*
  adt_ary_t subscriptions; // strong references to apx_diag_subscription_t*
  adt_ary_t temp_files;    // strong references to apx_diag_temp_file_t*
  MUTEX_T lock;
} apx_diag_dispatcher_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_diag_dispatcher_create(apx_diag_dispatcher_t *self, struct apx_monitor_session_tag *session);
void apx_diag_dispatcher_destroy(apx_diag_dispatcher_t *self);
apx_diag_dispatcher_t *apx_diag_dispatcher_new(struct apx_monitor_session_tag *session);
void apx_diag_dispatcher_delete(apx_diag_dispatcher_t *self);
void apx_diag_dispatcher_vdelete(void *arg);

void apx_diag_dispatcher_on_request_written(
  apx_diag_dispatcher_t *self, uint32_t offset, const uint8_t *data, apx_size_t size);
void apx_diag_dispatcher_on_file_opened(apx_diag_dispatcher_t *self, struct apx_file_tag *file);
void apx_diag_dispatcher_on_file_closed(apx_diag_dispatcher_t *self, struct apx_file_tag *file);

void apx_diag_dispatcher_notify_port_change(
  apx_diag_dispatcher_t *self, const char *node_name, uint32_t offset, const uint8_t *data, uint32_t size);
void apx_diag_dispatcher_notify_connection_closed(apx_diag_dispatcher_t *self, uint32_t connection_id);

int32_t apx_diag_dispatcher_num_subscriptions(apx_diag_dispatcher_t *self);
int32_t apx_diag_dispatcher_num_snapshots(apx_diag_dispatcher_t *self);

#endif // APX_DIAG_DISPATCHER_H
