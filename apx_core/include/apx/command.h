/*****************************************************************************
 * \file      command.h
 * \author    Conny Gustafsson
 * \date      2021-01-21
 * \brief     Command data structure
 *
 * Copyright (c) 2021-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_COMMAND_H
#define APX_COMMAND_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef uint8_t apx_cmd_type_t;
#define APX_CMD_EXIT ((apx_cmd_type_t)0u)
#define APX_CMD_SEND_ACKNOWLEDGE ((apx_cmd_type_t)1u)
#define APX_CMD_SEND_ERROR_CODE ((apx_cmd_type_t)2u)
#define APX_CMD_PUBLISH_LOCAL_FILE ((apx_cmd_type_t)3u)
#define APX_CMD_REVOKE_LOCAL_FILE ((apx_cmd_type_t)4u)
#define APX_CMD_OPEN_REMOTE_FILE ((apx_cmd_type_t)5u)
#define APX_CMD_CLOSE_REMOTE_FILE ((apx_cmd_type_t)6u)
#define APX_CMD_SEND_LOCAL_CONST_DATA ((apx_cmd_type_t)7u)
#define APX_CMD_SEND_LOCAL_SMALL_DATA ((apx_cmd_type_t)8u)
#define APX_CMD_SEND_LOCAL_SOA_DATA ((apx_cmd_type_t)9u)
#define APX_CMD_SEND_LOCAL_DATA ((apx_cmd_type_t)10u)
#define APX_CMD_SEND_HEADER_ACCEPTED ((apx_cmd_type_t)11u)
#define APX_CMD_CREATE_CONNECTION ((apx_cmd_type_t)12u)
#define APX_CMD_SEND_STREAM_OPEN ((apx_cmd_type_t)13u)
#define APX_CMD_SEND_STREAM_OPEN_RSP ((apx_cmd_type_t)14u)
#define APX_CMD_SEND_STREAM_WINDOW_UPDATE ((apx_cmd_type_t)15u)
#define APX_CMD_SEND_STREAM_ABORT ((apx_cmd_type_t)16u)
#define APX_CMD_DRAIN_STREAM_DATA ((apx_cmd_type_t)17u)

/**
 * Command field usage and ownership:
 *
 * APX_CMD_EXIT:
 *   - Signals the worker thread to exit. No data fields used.
 *
 * APX_CMD_SEND_ACKNOWLEDGE:
 *   - Sends RMF_CMD_ACK_MSG. No data fields used.
 *
 * APX_CMD_SEND_ERROR_CODE:
 *   - data1: Error code (apx_error_t)
 *   - data3.ptr: Optional file/context name (char *, strong reference: allocated via STRDUP, freed with free)
 *
 * APX_CMD_PUBLISH_LOCAL_FILE:
 *   - data3.ptr: File info object (rmf_file_info_t *, strong reference: freed with rmf_file_info_delete)
 *
 * APX_CMD_REVOKE_LOCAL_FILE:
 *   - data1: Local file start address (uint32_t)
 *
 * APX_CMD_OPEN_REMOTE_FILE / APX_CMD_CLOSE_REMOTE_FILE:
 *   - data1: Remote file start address (uint32_t)
 *
 * APX_CMD_SEND_LOCAL_CONST_DATA:
 *   - data1: Write address (uint32_t)
 *   - data2: Data size in bytes (uint32_t)
 *   - data3.ptr: Read-only data pointer (uint8_t const *, weak reference: not freed by worker)
 *
 * APX_CMD_SEND_LOCAL_SMALL_DATA:
 *   - data1: Write address (uint32_t)
 *   - data2: Data size in bytes (uint32_t, 1 <= data2 <= APX_SMALL_DATA_SIZE)
 *   - data3.data: Inline payload bytes (uint8_t[APX_SMALL_DATA_SIZE], embedded in command struct)
 *
 * APX_CMD_SEND_LOCAL_SOA_DATA:
 *   - data1: Write address (uint32_t)
 *   - data2: Data size in bytes (uint32_t, APX_SMALL_DATA_SIZE < data2 <= SOA_SMALL_OBJECT_MAX_SIZE)
 *   - data3.ptr: Payload buffer (uint8_t *, strong reference: allocated via worker->soa, freed with soa_free)
 *
 * APX_CMD_SEND_LOCAL_DATA:
 *   - data1: Write address (uint32_t)
 *   - data2: Data size in bytes (uint32_t)
 *   - data3.ptr: Payload buffer (uint8_t *, strong reference: allocated on heap, freed with free)
 *
 * APX_CMD_SEND_HEADER_ACCEPTED:
 *   - data1: Connection ID (uint32_t)
 *
 * APX_CMD_CREATE_CONNECTION:
 *   - data1: Connection ID (apx_connection_id_t)
 *   - data2: Connection state (apx_connection_state_t)
 *   - data3.ptr: Optional connection tag (char *, strong reference: allocated via STRDUP, freed with free)
 *
 * APX_CMD_SEND_STREAM_OPEN:
 *   - data1: Stream start address (uint32_t)
 *   - data2: Max block size (uint32_t)
 *   - data3.data[0..1]: Window blocks (uint16_t LE)
 *   - data3.data[2..3]: Flags (uint16_t LE)
 *
 * APX_CMD_SEND_STREAM_OPEN_RSP:
 *   - data1: Stream start address (uint32_t)
 *   - data2: Status code (uint32_t)
 *   - data3.data[0..3]: Block size (uint32_t LE)
 *   - data3.data[4..5]: Window blocks (uint16_t LE)
 *   - data3.data[6..7]: Flags (uint16_t LE)
 *
 * APX_CMD_SEND_STREAM_WINDOW_UPDATE:
 *   - data1: Stream start address (uint32_t)
 *   - data3.data[0..7]: Consumed block count (uint64_t LE)
 *
 * APX_CMD_SEND_STREAM_ABORT:
 *   - data1: Stream start address (uint32_t)
 *   - data2: Abort status code (uint32_t)
 *
 * APX_CMD_DRAIN_STREAM_DATA:
 *   - data1: Stream start address (uint32_t)
 */


typedef struct apx_command_tag
{
  apx_cmd_type_t cmd_type;
  uint32_t data1; // generic uint32 value
  uint32_t data2; // generic uint32 value
  union
  {
    void *ptr;                         // generic pointer value
    uint8_t data[APX_SMALL_DATA_SIZE]; // port data (when port data length is small)
  } data3;
  void *data4; // generic pointer value
} apx_command_t;


#define APX_COMMAND_SIZE sizeof(apx_command_t)


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_build_command_with_ptr(
  apx_command_t *self, apx_cmd_type_t cmd_type, uint32_t d1, uint32_t d2, void *d3, void *d4);
void apx_build_command_with_data(
  apx_command_t *self, apx_cmd_type_t cmd_type, uint32_t d1, uint32_t d2, uint8_t const *d3, void *d4);

#endif // APX_COMMAND_H
