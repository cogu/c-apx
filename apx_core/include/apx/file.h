/*****************************************************************************
 * \file      file.h
 * \author    Conny Gustafsson
 * \date      2018-08-30
 * \brief     APX file class
 *
 * Copyright (c) 2018-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_FILE_H
#define APX_FILE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_list.h"
#include "apx/error.h"
#include "apx/event_listener.h"
#include "apx/file_info.h"
#include "apx/types.h"

#ifndef APX_EMBEDDED
# ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>
# else
#  include <pthread.h>
# endif
# include "osmacro.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
// forward declarations
struct apx_file_tag;
struct apx_file_manager_tag;

typedef apx_error_t(apx_file_open_close_notify_func)(void *arg, struct apx_file_tag *file);
typedef apx_error_t(apx_file_write_notify_func)(
  void *arg, struct apx_file_tag *file, uint32_t offset, const uint8_t *src, uint32_t len);
typedef apx_error_t(apx_file_read_const_data_func)(
  void *arg, struct apx_file_tag *file, uint32_t offset, uint8_t *dest, uint32_t len);

typedef struct apx_file_notification_handler_tag
{
  void *arg;
  apx_file_open_close_notify_func
    *open_notify; // Notifies file owner that the file was openened on remote end (used for local files)
  apx_file_open_close_notify_func
    *close_notify; // Notifies file owner that the file was closed on remote end (used for local files)
  apx_file_write_notify_func
    *write_notify; // Notifies file owner that the file has just been written to (used for remote files)
} apx_file_notification_handler_t;

#define APX_STREAM_DEFAULT_MAX_WINDOW_BLOCKS ((uint16_t)64u)

typedef enum apx_stream_state_tag
{
  APX_STREAM_STATE_CLOSED = 0,
  APX_STREAM_STATE_OPEN_PENDING,
  APX_STREAM_STATE_OPEN,
  APX_STREAM_STATE_ABORTED,
  APX_STREAM_STATE_REVOKED
} apx_stream_state_t;

typedef struct apx_stream_block_item_tag
{
  uint8_t *data;
  uint32_t size;
} apx_stream_block_item_t;

typedef struct apx_stream_file_state_tag
{
  apx_stream_state_t state;
  uint32_t block_size;
  uint16_t window_blocks;
  uint16_t local_max_window_blocks;
  uint16_t flags;
  uint32_t last_status;
  uint64_t transmitted_blocks;
  uint64_t received_blocks;
  uint64_t consumed_blocks;
  uint64_t last_acked_consumed_blocks;
  bool auto_consume;
  adt_list_t pending_blocks; // list of apx_stream_block_item_t*
} apx_stream_file_state_t;

typedef struct apx_file_tag
{
  bool is_file_open;
  bool has_first_write;

  apx_file_type_t apx_file_type;
  rmf_file_info_t file_info;
  apx_file_notification_handler_t notification_handler;
  apx_stream_file_state_t stream_state;
  struct apx_file_manager_tag *file_manager;
  MUTEX_T lock;
} apx_file_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_file_create(apx_file_t *self, const rmf_file_info_t *file_info);
void apx_file_destroy(apx_file_t *self);
apx_file_t *apx_file_new(const rmf_file_info_t *file_info);
// apx_file_t* apx_file_clone(const apx_file_t* file);
void apx_file_delete(apx_file_t *self);
void apx_file_vdelete(void *arg);
void apx_file_open(apx_file_t *self);
void apx_file_close(apx_file_t *self);
void apx_file_set_notification_handler(apx_file_t *self, const apx_file_notification_handler_t *handler);
bool apx_file_has_first_write(apx_file_t *self);
void apx_file_set_first_write(apx_file_t *self);
bool apx_file_is_open(apx_file_t *self);
bool apx_file_is_local(apx_file_t *self);
bool apx_file_is_remote(apx_file_t *self);
bool apx_file_has_valid_address(apx_file_t *self);
struct apx_file_manager_tag *apx_file_get_file_manager(apx_file_t *self);
void apx_file_set_file_manager(apx_file_t *self, struct apx_file_manager_tag *file_manager);
apx_file_type_t apx_file_get_apx_file_type(const apx_file_t *self);
rmf_file_type_t apx_file_get_rmf_file_type(const apx_file_t *self);
bool apx_file_is_stream(const apx_file_t *self);
uint32_t apx_file_get_address(const apx_file_t *self);
uint32_t apx_file_get_address_without_flags(const apx_file_t *self);
void apx_file_set_address(apx_file_t *self, uint32_t address);
uint32_t apx_file_get_size(const apx_file_t *self);
const char *apx_file_get_name(const apx_file_t *self);
uint32_t apx_file_get_end_address(const apx_file_t *self);
uint32_t apx_file_get_end_address_without_flags(const apx_file_t *self);
bool apx_file_address_in_range(const apx_file_t *self, uint32_t address);
rmf_file_info_t const *apx_file_get_file_info(const apx_file_t *self);
rmf_file_info_t *apx_file_clone_file_info(const apx_file_t *self);
rmf_digest_type_t apx_file_get_digest_type(apx_file_t const *self);
uint8_t const *apx_file_get_digest_data(const apx_file_t *self);
rmf_signature_type_t apx_file_get_signature_type(apx_file_t const *self);
uint8_t const *apx_file_get_signature_data(const apx_file_t *self);
bool apx_file_is_signed(const apx_file_t *self);
apx_error_t apx_file_open_notify(apx_file_t *self);
apx_error_t apx_file_close_notify(apx_file_t *self);
apx_error_t apx_file_write_notify(apx_file_t *self, uint32_t offset, const uint8_t *src, uint32_t len);

// stream file functions
apx_stream_state_t apx_file_get_stream_state(apx_file_t *self);
void apx_file_set_stream_state(apx_file_t *self, apx_stream_state_t state);
uint32_t apx_file_get_stream_block_size(apx_file_t *self);
uint16_t apx_file_get_stream_window_blocks(apx_file_t *self);
uint16_t apx_file_get_stream_local_max_window_blocks(apx_file_t *self);
void apx_file_set_stream_local_max_window_blocks(apx_file_t *self, uint16_t max_window_blocks);
uint32_t apx_file_get_stream_last_status(apx_file_t *self);
void apx_file_set_stream_last_status(apx_file_t *self, uint32_t status);
bool apx_file_get_stream_auto_consume(apx_file_t *self);
void apx_file_set_stream_auto_consume(apx_file_t *self, bool auto_consume);
uint64_t apx_file_get_stream_transmitted_blocks(apx_file_t *self);
uint64_t apx_file_get_stream_received_blocks(apx_file_t *self);
uint64_t apx_file_get_stream_consumed_blocks(apx_file_t *self);
uint64_t apx_file_get_stream_last_acked_consumed_blocks(apx_file_t *self);
void apx_file_set_stream_open_pending(
  apx_file_t *self, uint32_t max_block_size, uint16_t window_blocks, uint16_t flags);
void apx_file_open_stream(apx_file_t *self, uint32_t block_size, uint16_t window_blocks, uint16_t flags);
void apx_file_abort_stream(apx_file_t *self, uint32_t status);
void apx_file_reset_stream(apx_file_t *self);
bool apx_file_stream_can_transmit(apx_file_t *self);
void apx_file_stream_increment_transmitted(apx_file_t *self);
apx_error_t apx_file_stream_update_consumed_from_peer(apx_file_t *self, uint64_t consumed_block_count);
apx_error_t apx_file_stream_on_block_received(apx_file_t *self);
apx_error_t apx_file_stream_consume_blocks(
  apx_file_t *self, uint32_t count, bool *should_send_update, uint64_t *new_consumed_count);
apx_error_t apx_file_stream_enqueue_block(apx_file_t *self, uint8_t const *data, uint32_t size);
apx_stream_block_item_t *apx_file_stream_dequeue_block(apx_file_t *self);
int32_t apx_file_stream_pending_block_count(apx_file_t *self);
apx_stream_block_item_t *apx_stream_block_item_new(uint8_t const *data, uint32_t size);
void apx_stream_block_item_delete(apx_stream_block_item_t *self);
void apx_stream_block_item_vdelete(void *arg);

// global functions
char const *apx_file_type_to_extension(apx_file_type_t file_type);
bool apx_file_less_than(const apx_file_t *a, const apx_file_t *b);


#endif // APX_FILE_H
