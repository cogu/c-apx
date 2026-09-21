/*****************************************************************************
* \file      file_manager.h
* \author    Conny Gustafsson
* \date      2017-03-12
* \brief     APX embedded file manager
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
/**
 * description: embedded version of the APX fileManager. The intention is to:
 * 1. apply to MISRA rules
 * 2. attempt not to use dynamic memory (malloc/free) as far as possible.
 */
#ifndef APX_ES_FILE_MANAGER_H
#define APX_ES_FILE_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx_node_data.h"
#include "apx_file.h"
#include "apx_msg.h"
#include "apx_transmit_handler.h"
#include "apx_es_file_manager_cfg.h"
#include "apx_es_file_map.h"
#include "ringbuf.h"
#include "apx_error.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_es_file_write_tag
{
   uint32_t writeAddress;
   uint32_t readOffset;
   apx_file_t *local_file;
   int32_t remain;
   int32_t headerLen;
   int32_t dataLen;
} apx_es_file_write_t;

typedef struct apx_es_transmit_buf_tag
{
   uint8_t *data;
   int32_t avail;
   int32_t maxMsgLen;
}apx_es_transmit_buf_t;

typedef struct apx_es_file_manager_tag
{
   rbfs_t messageQueue; //internal message queue (contains apx_msg_t object)

   uint8_t *receive_buf; //receive buffer for large writes
   uint32_t receive_buf_len; //length of receive buffer
   uint32_t receiveBufOffset; //current write position (and length) of receive buffer
   uint32_t receiveStartAddress;

   apx_es_file_map_t localFileMap;
   apx_es_file_map_t remoteFileMap;
   apx_file_t *requestedFileList[APX_ES_FILEMANAGER_MAX_NUM_REQUEST_FILES];
   uint16_t numRequestedFiles;
   apx_error_t lastErrorCode;

   apx_transmit_handler_t transmitHandler;
   apx_es_transmit_buf_t transmitBuf;

   bool hasPendingWrite; // Used when a sent message is using more_bit. Write info found in pendingMsg struct
   bool dropMessage; // Used when received messages are larger than receive buffer
   bool isConnected; // When fileManager is connected to an underlying communication device (like a TCP socket or SPI stream)
   apx_file_t *curFile; // Weak pointer to last accessed file

   apx_msg_t queuedWriteNotify; // Last write notification waiting for more data (until apx_es_file_manager_run() is called)
   apx_msg_t pendingMsg; // Message taken out of the queue and not yet serialized
   apx_es_file_write_t fileWriteInfo;

}apx_es_file_manager_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_es_file_manager_create(apx_es_file_manager_t *self, uint8_t *message_queue_buf, uint16_t message_queue_len, uint8_t *receive_buf, uint16_t receive_buf_len);

// Files added will be kept track of through connect/disconnect events
// The fileManager will open/close these files as requested by the server
void apx_es_file_manager_attach_local_file(apx_es_file_manager_t *self, apx_file_t *local_file);
void apx_es_file_manager_request_remote_file(apx_es_file_manager_t *self, apx_file_t *requested_file);

void apx_es_file_manager_set_transmit_handler(apx_es_file_manager_t *self, apx_transmit_handler_t *handler);

//these messages can be sent to the fileManager to be processed by its internal worker thread
void apx_es_file_manager_on_connected(apx_es_file_manager_t *self);
void apx_es_file_manager_on_disconnected(apx_es_file_manager_t *self);
int8_t apx_es_file_manager_trigger_file_update(apx_es_file_manager_t *self, apx_file_t *file, uint32_t offset, uint32_t length);
int8_t apx_es_file_manager_trigger_direct_write(apx_es_file_manager_t *self, uint8_t *data, uint32_t address, uint32_t length);
void apx_es_file_manager_on_msg_received(apx_es_file_manager_t *self, const uint8_t *msg_buf, int32_t msg_len);

void apx_es_file_manager_run(apx_es_file_manager_t *self);
bool apx_es_file_manager_has_pending_msg(apx_es_file_manager_t *self);
apx_error_t apx_es_file_manager_get_last_error(apx_es_file_manager_t *self);
#ifdef UNIT_TEST
#define DYN_STATIC

int32_t apx_es_file_manager_get_num_messages_in_queue(apx_es_file_manager_t *self);
DYN_STATIC int8_t apx_es_file_manager_remove_requested_at(apx_es_file_manager_t *self, int32_t removeIndex);
DYN_STATIC void apx_es_file_manager_process_remote_file_info(apx_es_file_manager_t *self, const rmf_file_info_t *fileInfo);

#else
#define DYN_STATIC static
#endif


#endif //APX_ES_FILE_MANAGER_H

