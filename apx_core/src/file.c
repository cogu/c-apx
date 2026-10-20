/*****************************************************************************
 * \file      file.c
 * \author    Conny Gustafsson
 * \date      2018-08-30
 * \brief     Improved version of apx_file
 *
 * Copyright (c) 2018-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/file.h"
#include "apx/error.h"
#include "apx/event_listener.h"
#include "apx/file_manager.h"
#include "apx/node_data.h"
#include "bstr.h"
#include <malloc.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void apx_file_calc_file_type(apx_file_t *self);
static void apx_file_lock(apx_file_t *self);
static void apx_file_unlock(apx_file_t *self);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_file_create(apx_file_t *self, const rmf_file_info_t *file_info)
{
  if ((self != NULL) && (file_info != NULL))
  {
    apx_error_t retval = APX_NO_ERROR;
    self->is_file_open = false;
    self->has_first_write = false;
    self->file_manager = (apx_file_manager_t *)NULL;
    self->apx_file_type = APX_UNKNOWN_FILE_TYPE;
    memset(&self->notification_handler, 0, sizeof(apx_file_notification_handler_t));
    self->stream_state.state = APX_STREAM_STATE_CLOSED;
    self->stream_state.block_size = 0u;
    self->stream_state.window_blocks = 0u;
    self->stream_state.local_max_window_blocks = APX_STREAM_DEFAULT_MAX_WINDOW_BLOCKS;
    self->stream_state.flags = 0u;
    self->stream_state.last_status = (uint32_t)APX_NO_ERROR;
    self->stream_state.transmitted_blocks = 0u;
    self->stream_state.received_blocks = 0u;
    self->stream_state.consumed_blocks = 0u;
    self->stream_state.last_acked_consumed_blocks = 0u;
    self->stream_state.auto_consume = true;
    adt_list_create(&self->stream_state.pending_blocks, apx_stream_block_item_vdelete);
    retval = rmf_file_info_create_copy(&self->file_info, file_info);
    if (retval == APX_NO_ERROR)
    {
      apx_file_calc_file_type(self);
      MUTEX_INIT(self->lock);
    }
    else
    {
      adt_list_destroy(&self->stream_state.pending_blocks);
    }
    return retval;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_destroy(apx_file_t *self)
{
  if (self != NULL)
  {
    adt_list_destroy(&self->stream_state.pending_blocks);
    rmf_file_info_destroy(&self->file_info);
    MUTEX_DESTROY(self->lock);
  }
}

apx_file_t *apx_file_new(const rmf_file_info_t *file_info)
{
  apx_file_t *self = (apx_file_t *)malloc(sizeof(apx_file_t));
  if (self != NULL)
  {
    apx_error_t result = apx_file_create(self, file_info);
    if (result != APX_NO_ERROR)
    {
      free(self);
      self = NULL;
    }
  }
  return self;
}

void apx_file_delete(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_file_destroy(self);
    free(self);
  }
}

void apx_file_vdelete(void *arg) { apx_file_delete((apx_file_t *)arg); }

void apx_file_open(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->is_file_open = true;
    apx_file_unlock(self);
  }
}

void apx_file_close(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->is_file_open = false;
    self->stream_state.state = APX_STREAM_STATE_CLOSED;
    self->stream_state.block_size = 0u;
    self->stream_state.window_blocks = 0u;
    self->stream_state.flags = 0u;
    self->stream_state.transmitted_blocks = 0u;
    self->stream_state.received_blocks = 0u;
    self->stream_state.consumed_blocks = 0u;
    self->stream_state.last_acked_consumed_blocks = 0u;
    adt_list_clear(&self->stream_state.pending_blocks);
    apx_file_unlock(self);
  }
}

void apx_file_set_notification_handler(apx_file_t *self, const apx_file_notification_handler_t *handler)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    if (handler == NULL)
    {
      memset(&self->notification_handler, 0, sizeof(apx_file_notification_handler_t));
    }
    else
    {
      memcpy(&self->notification_handler, handler, sizeof(apx_file_notification_handler_t));
    }
    apx_file_unlock(self);
  }
}

bool apx_file_has_first_write(apx_file_t *self)
{
  if (self != NULL)
  {
    return self->has_first_write;
  }
  return false;
}

void apx_file_set_first_write(apx_file_t *self)
{
  if (self != NULL)
  {
    self->has_first_write = true;
  }
}


bool apx_file_is_open(apx_file_t *self)
{
  if (self != NULL)
  {
    bool retval;
    apx_file_lock(self);
    retval = self->is_file_open;
    apx_file_unlock(self);
    return retval;
  }
  return false;
}

bool apx_file_is_local(apx_file_t *self)
{
  if (self != NULL)
  {
    uint32_t const address = rmf_file_info_address(&self->file_info);
    uint32_t const address_without_flags = address & APX_ADDRESS_MASK_INTERNAL;
    return (address_without_flags != RMF_INVALID_ADDRESS) && ((address & RMF_REMOTE_ADDRESS_BIT) == 0u);
  }
  return false;
}

bool apx_file_is_remote(apx_file_t *self)
{
  if (self != NULL)
  {
    uint32_t const address = rmf_file_info_address(&self->file_info);
    uint32_t const address_without_flags = address & APX_ADDRESS_MASK_INTERNAL;
    return (address_without_flags != RMF_INVALID_ADDRESS) && ((address & RMF_REMOTE_ADDRESS_BIT) != 0u);
  }
  return false;
}

bool apx_file_has_valid_address(apx_file_t *self)
{
  if (self != NULL)
  {
    uint32_t const address_without_flags = rmf_file_info_address_without_flags(&self->file_info);
    return (address_without_flags != RMF_INVALID_ADDRESS);
  }
  return false;
}

struct apx_file_manager_tag *apx_file_get_file_manager(apx_file_t *self)
{
  if (self != NULL)
  {
    return self->file_manager;
  }
  return NULL;
}

void apx_file_set_file_manager(apx_file_t *self, struct apx_file_manager_tag *file_manager)
{
  if (self != NULL)
  {
    self->file_manager = file_manager;
  }
}

apx_file_type_t apx_file_get_apx_file_type(const apx_file_t *self)
{
  if (self != NULL)
  {
    return self->apx_file_type;
  }
  return APX_UNKNOWN_FILE_TYPE;
}

rmf_file_type_t apx_file_get_rmf_file_type(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_rmf_file_type(&self->file_info);
  }
  return RMF_FILE_TYPE_FIXED;
}

bool apx_file_is_stream(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_rmf_file_type(&self->file_info) == RMF_FILE_TYPE_STREAM;
  }
  return false;
}

uint32_t apx_file_get_address(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_address(&self->file_info);
  }
  return RMF_INVALID_ADDRESS;
}

uint32_t apx_file_get_address_without_flags(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_address_without_flags(&self->file_info);
  }
  return RMF_INVALID_ADDRESS;
}

void apx_file_set_address(apx_file_t *self, uint32_t address)
{
  if (self != NULL)
  {
    rmf_file_info_set_address(&self->file_info, address);
  }
}

uint32_t apx_file_get_size(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_size(&self->file_info);
  }
  return 0;
}

const char *apx_file_get_name(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_name(&self->file_info);
  }
  return NULL;
}

uint32_t apx_file_get_end_address(const apx_file_t *self)
{
  if (self != NULL)
  {
    return self->file_info.address + self->file_info.size;
  }
  return RMF_INVALID_ADDRESS;
}

uint32_t apx_file_get_end_address_without_flags(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_address_without_flags(&self->file_info) + self->file_info.size;
  }
  return RMF_INVALID_ADDRESS;
}

bool apx_file_address_in_range(const apx_file_t *self, uint32_t address)
{
  if (self != NULL)
  {
    return rmf_file_info_address_in_range(&self->file_info, address);
  }
  return false;
}

rmf_file_info_t const *apx_file_get_file_info(const apx_file_t *self)
{
  if (self != NULL)
  {
    return &self->file_info;
  }
  return NULL;
}

rmf_digest_type_t apx_file_get_digest_type(apx_file_t const *self)
{
  if (self != NULL)
  {
    return rmf_file_info_digest_type(&self->file_info);
  }
  return RMF_DIGEST_TYPE_NONE;
}

uint8_t const *apx_file_get_digest_data(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_digest_data(&self->file_info);
  }
  return NULL;
}

rmf_signature_type_t apx_file_get_signature_type(apx_file_t const *self)
{
  if (self != NULL)
  {
    return rmf_file_info_signature_type(&self->file_info);
  }
  return RMF_SIGNATURE_TYPE_NONE;
}

uint8_t const *apx_file_get_signature_data(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_signature_data(&self->file_info);
  }
  return NULL;
}

bool apx_file_is_signed(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_is_signed(&self->file_info);
  }
  return false;
}

rmf_file_info_t *apx_file_clone_file_info(const apx_file_t *self)
{
  if (self != NULL)
  {
    return rmf_file_info_clone(&self->file_info);
  }
  return NULL;
}

apx_error_t apx_file_open_notify(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_file_open_close_notify_func *open_notify = NULL;
    void *arg = NULL;
    apx_file_lock(self);
    open_notify = self->notification_handler.open_notify;
    arg = self->notification_handler.arg;
    apx_file_unlock(self);
    if (open_notify != NULL)
    {
      return open_notify(arg, self);
    }
    return APX_INVALID_OPEN_HANDLER_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_close_notify(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_file_open_close_notify_func *close_notify = NULL;
    void *arg = NULL;
    apx_file_lock(self);
    close_notify = self->notification_handler.close_notify;
    arg = self->notification_handler.arg;
    apx_file_unlock(self);
    if (close_notify != NULL)
    {
      return close_notify(arg, self);
    }
    return APX_NO_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_write_notify(apx_file_t *self, uint32_t offset, const uint8_t *src, uint32_t len)
{
  if (self != NULL)
  {
    apx_file_write_notify_func *write_notify = NULL;
    void *arg = NULL;
    apx_file_lock(self);
    if (!self->has_first_write)
    {
      self->has_first_write = true;
    }
    write_notify = self->notification_handler.write_notify;
    arg = self->notification_handler.arg;
    apx_file_unlock(self);
    if (write_notify != NULL)
    {
      return write_notify(arg, self, offset, src, len);
    }
    return APX_INVALID_WRITE_HANDLER_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_stream_state_t apx_file_get_stream_state(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_stream_state_t retval;
    apx_file_lock(self);
    retval = self->stream_state.state;
    apx_file_unlock(self);
    return retval;
  }
  return APX_STREAM_STATE_CLOSED;
}

void apx_file_set_stream_state(apx_file_t *self, apx_stream_state_t state)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->stream_state.state = state;
    apx_file_unlock(self);
  }
}

uint32_t apx_file_get_stream_block_size(apx_file_t *self)
{
  if (self != NULL)
  {
    uint32_t retval;
    apx_file_lock(self);
    retval = self->stream_state.block_size;
    apx_file_unlock(self);
    return retval;
  }
  return 0u;
}

uint16_t apx_file_get_stream_window_blocks(apx_file_t *self)
{
  if (self != NULL)
  {
    uint16_t retval;
    apx_file_lock(self);
    retval = self->stream_state.window_blocks;
    apx_file_unlock(self);
    return retval;
  }
  return 0u;
}

uint16_t apx_file_get_stream_local_max_window_blocks(apx_file_t *self)
{
  if (self != NULL)
  {
    uint16_t retval;
    apx_file_lock(self);
    retval = self->stream_state.local_max_window_blocks;
    apx_file_unlock(self);
    return retval;
  }
  return 0u;
}

void apx_file_set_stream_local_max_window_blocks(apx_file_t *self, uint16_t max_window_blocks)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->stream_state.local_max_window_blocks = max_window_blocks;
    apx_file_unlock(self);
  }
}

uint32_t apx_file_get_stream_last_status(apx_file_t *self)
{
  if (self != NULL)
  {
    uint32_t retval;
    apx_file_lock(self);
    retval = self->stream_state.last_status;
    apx_file_unlock(self);
    return retval;
  }
  return (uint32_t)APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_set_stream_last_status(apx_file_t *self, uint32_t status)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->stream_state.last_status = status;
    apx_file_unlock(self);
  }
}

bool apx_file_get_stream_auto_consume(apx_file_t *self)
{
  if (self != NULL)
  {
    bool retval;
    apx_file_lock(self);
    retval = self->stream_state.auto_consume;
    apx_file_unlock(self);
    return retval;
  }
  return false;
}

void apx_file_set_stream_auto_consume(apx_file_t *self, bool auto_consume)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->stream_state.auto_consume = auto_consume;
    apx_file_unlock(self);
  }
}

uint64_t apx_file_get_stream_transmitted_blocks(apx_file_t *self)
{
  if (self != NULL)
  {
    uint64_t retval;
    apx_file_lock(self);
    retval = self->stream_state.transmitted_blocks;
    apx_file_unlock(self);
    return retval;
  }
  return 0u;
}

uint64_t apx_file_get_stream_received_blocks(apx_file_t *self)
{
  if (self != NULL)
  {
    uint64_t retval;
    apx_file_lock(self);
    retval = self->stream_state.received_blocks;
    apx_file_unlock(self);
    return retval;
  }
  return 0u;
}

uint64_t apx_file_get_stream_consumed_blocks(apx_file_t *self)
{
  if (self != NULL)
  {
    uint64_t retval;
    apx_file_lock(self);
    retval = self->stream_state.consumed_blocks;
    apx_file_unlock(self);
    return retval;
  }
  return 0u;
}

uint64_t apx_file_get_stream_last_acked_consumed_blocks(apx_file_t *self)
{
  if (self != NULL)
  {
    uint64_t retval;
    apx_file_lock(self);
    retval = self->stream_state.last_acked_consumed_blocks;
    apx_file_unlock(self);
    return retval;
  }
  return 0u;
}

void apx_file_set_stream_open_pending(apx_file_t *self, uint32_t max_block_size, uint16_t window_blocks, uint16_t flags)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->is_file_open = false;
    self->stream_state.state = APX_STREAM_STATE_OPEN_PENDING;
    self->stream_state.block_size = max_block_size;
    self->stream_state.window_blocks = window_blocks;
    self->stream_state.flags = flags;
    self->stream_state.last_status = (uint32_t)APX_NO_ERROR;
    self->stream_state.transmitted_blocks = 0u;
    self->stream_state.received_blocks = 0u;
    self->stream_state.consumed_blocks = 0u;
    self->stream_state.last_acked_consumed_blocks = 0u;
    adt_list_clear(&self->stream_state.pending_blocks);
    apx_file_unlock(self);
  }
}

void apx_file_open_stream(apx_file_t *self, uint32_t block_size, uint16_t window_blocks, uint16_t flags)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->is_file_open = true;
    self->stream_state.state = APX_STREAM_STATE_OPEN;
    self->stream_state.block_size = block_size;
    self->stream_state.window_blocks = window_blocks;
    self->stream_state.flags = flags;
    self->stream_state.last_status = (uint32_t)APX_NO_ERROR;
    self->stream_state.transmitted_blocks = 0u;
    self->stream_state.received_blocks = 0u;
    self->stream_state.consumed_blocks = 0u;
    self->stream_state.last_acked_consumed_blocks = 0u;
    adt_list_clear(&self->stream_state.pending_blocks);
    apx_file_unlock(self);
  }
}

void apx_file_abort_stream(apx_file_t *self, uint32_t status)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    self->is_file_open = false;
    self->stream_state.state = APX_STREAM_STATE_CLOSED;
    self->stream_state.block_size = 0u;
    self->stream_state.window_blocks = 0u;
    self->stream_state.flags = 0u;
    self->stream_state.last_status = status;
    self->stream_state.transmitted_blocks = 0u;
    self->stream_state.received_blocks = 0u;
    self->stream_state.consumed_blocks = 0u;
    self->stream_state.last_acked_consumed_blocks = 0u;
    adt_list_clear(&self->stream_state.pending_blocks);
    apx_file_unlock(self);
  }
}

void apx_file_reset_stream(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_file_close(self);
  }
}

bool apx_file_stream_can_transmit(apx_file_t *self)
{
  if (self != NULL)
  {
    bool retval = false;
    apx_file_lock(self);
    if ((self->stream_state.state == APX_STREAM_STATE_OPEN) && (self->stream_state.transmitted_blocks < UINT64_MAX) &&
      (self->stream_state.transmitted_blocks >= self->stream_state.consumed_blocks))
    {
      uint64_t const outstanding = self->stream_state.transmitted_blocks - self->stream_state.consumed_blocks;
      retval = (outstanding < (uint64_t)self->stream_state.window_blocks);
    }
    apx_file_unlock(self);
    return retval;
  }
  return false;
}

void apx_file_stream_increment_transmitted(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_file_lock(self);
    if (self->stream_state.transmitted_blocks < UINT64_MAX)
    {
      self->stream_state.transmitted_blocks++;
    }
    apx_file_unlock(self);
  }
}

apx_error_t apx_file_stream_update_consumed_from_peer(apx_file_t *self, uint64_t consumed_block_count)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  apx_error_t retval = APX_NO_ERROR;
  apx_file_lock(self);
  if (self->stream_state.state != APX_STREAM_STATE_OPEN)
  {
    retval = APX_FILE_NOT_OPEN_ERROR;
  }
  else if ((consumed_block_count < self->stream_state.consumed_blocks) ||
    (consumed_block_count > self->stream_state.transmitted_blocks))
  {
    retval = APX_VALUE_RANGE_ERROR;
  }
  else
  {
    self->stream_state.consumed_blocks = consumed_block_count;
  }
  apx_file_unlock(self);
  return retval;
}

apx_error_t apx_file_stream_on_block_received(apx_file_t *self)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  apx_error_t retval = APX_NO_ERROR;
  apx_file_lock(self);
  if (self->stream_state.state != APX_STREAM_STATE_OPEN)
  {
    retval = APX_FILE_NOT_OPEN_ERROR;
  }
  else if (self->stream_state.received_blocks == UINT64_MAX)
  {
    retval = APX_VALUE_RANGE_ERROR;
  }
  else
  {
    uint64_t const unconsumed = self->stream_state.received_blocks - self->stream_state.consumed_blocks;
    uint64_t const unacked_in_flight =
      self->stream_state.received_blocks - self->stream_state.last_acked_consumed_blocks;
    if ((unconsumed >= (uint64_t)self->stream_state.window_blocks) ||
      (unacked_in_flight >= (uint64_t)self->stream_state.window_blocks))
    {
      retval = APX_BUFFER_FULL_ERROR;
    }
    else
    {
      self->stream_state.received_blocks++;
    }
  }
  apx_file_unlock(self);
  return retval;
}

apx_error_t apx_file_stream_consume_blocks(
  apx_file_t *self, uint32_t count, bool *should_send_update, uint64_t *new_consumed_count)
{
  if ((self == NULL) || (count == 0u) || (should_send_update == NULL) || (new_consumed_count == NULL))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  apx_error_t retval = APX_NO_ERROR;
  *should_send_update = false;
  *new_consumed_count = 0u;
  apx_file_lock(self);
  if (self->stream_state.state != APX_STREAM_STATE_OPEN)
  {
    retval = APX_FILE_NOT_OPEN_ERROR;
  }
  else if (((uint64_t)count > (self->stream_state.received_blocks - self->stream_state.consumed_blocks)))
  {
    retval = APX_VALUE_RANGE_ERROR;
  }
  else
  {
    self->stream_state.consumed_blocks += (uint64_t)count;
    *new_consumed_count = self->stream_state.consumed_blocks;
    uint64_t const unacked = self->stream_state.consumed_blocks - self->stream_state.last_acked_consumed_blocks;
    uint64_t const threshold = ((uint64_t)self->stream_state.window_blocks + 1u) / 2u;
    if ((threshold > 0u) && (unacked >= threshold))
    {
      self->stream_state.last_acked_consumed_blocks = self->stream_state.consumed_blocks;
      *should_send_update = true;
    }
  }
  apx_file_unlock(self);
  return retval;
}

apx_error_t apx_file_stream_enqueue_block(apx_file_t *self, uint8_t const *data, uint32_t size)
{
  if ((self == NULL) || (data == NULL) || (size == 0u))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  apx_stream_block_item_t *item = apx_stream_block_item_new(data, size);
  if (item == NULL)
  {
    return APX_MEM_ERROR;
  }
  apx_file_lock(self);
  adt_list_insert(&self->stream_state.pending_blocks, item);
  apx_file_unlock(self);
  return APX_NO_ERROR;
}

apx_stream_block_item_t *apx_file_stream_dequeue_block(apx_file_t *self)
{
  if (self != NULL)
  {
    apx_stream_block_item_t *item = NULL;
    apx_file_lock(self);
    adt_list_elem_t *first = adt_list_iter_first(&self->stream_state.pending_blocks);
    if (first != NULL)
    {
      item = (apx_stream_block_item_t *)first->pItem;
      adt_list_destructor_enable(&self->stream_state.pending_blocks, false);
      adt_list_erase(&self->stream_state.pending_blocks, first);
      adt_list_destructor_enable(&self->stream_state.pending_blocks, true);
    }
    apx_file_unlock(self);
    return item;
  }
  return NULL;
}

int32_t apx_file_stream_pending_block_count(apx_file_t *self)
{
  if (self != NULL)
  {
    int32_t count = 0;
    apx_file_lock(self);
    count = adt_list_length(&self->stream_state.pending_blocks);
    apx_file_unlock(self);
    return count;
  }
  return 0;
}

apx_stream_block_item_t *apx_stream_block_item_new(uint8_t const *data, uint32_t size)
{
  if ((data == NULL) || (size == 0u))
  {
    return NULL;
  }
  apx_stream_block_item_t *self = (apx_stream_block_item_t *)malloc(sizeof(apx_stream_block_item_t));
  if (self != NULL)
  {
    self->data = (uint8_t *)malloc(size);
    if (self->data != NULL)
    {
      memcpy(self->data, data, size);
      self->size = size;
    }
    else
    {
      free(self);
      self = NULL;
    }
  }
  return self;
}

void apx_stream_block_item_delete(apx_stream_block_item_t *self)
{
  if (self != NULL)
  {
    if (self->data != NULL)
    {
      free(self->data);
    }
    free(self);
  }
}

void apx_stream_block_item_vdelete(void *arg) { apx_stream_block_item_delete((apx_stream_block_item_t *)arg); }

// global functions
char const *apx_file_type_to_extension(apx_file_type_t file_type)
{
  switch (file_type)
  {
  case APX_DEFINITION_FILE_TYPE:
    return APX_DEFINITION_FILE_EXT;
  case APX_PROVIDE_PORT_DATA_FILE_TYPE:
    return APX_PROVIDE_PORT_DATA_EXT;
  case APX_REQUIRE_PORT_DATA_FILE_TYPE:
    return APX_REQUIRE_PORT_DATA_EXT;
  case APX_PROVIDE_PORT_COUNT_FILE_TYPE:
    return APX_PROVIDE_PORT_COUNT_EXT;
  case APX_REQUIRE_PORT_COUNT_FILE_TYPE:
    return APX_REQUIRE_PORT_COUNT_EXT;
  }
  return "";
}

bool apx_file_less_than(const apx_file_t *a, const apx_file_t *b)
{
  if ((a != NULL) && (b != NULL))
  {
    return apx_file_get_address_without_flags(a) < apx_file_get_address_without_flags(b);
  }
  return false;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_file_calc_file_type(apx_file_t *self)
{
  if (rmf_file_info_name_ends_with(&self->file_info, APX_DEFINITION_FILE_EXT))
  {
    self->apx_file_type = APX_DEFINITION_FILE_TYPE;
  }
  else if (rmf_file_info_name_ends_with(&self->file_info, APX_PROVIDE_PORT_DATA_EXT))
  {
    self->apx_file_type = APX_PROVIDE_PORT_DATA_FILE_TYPE;
  }
  else if (rmf_file_info_name_ends_with(&self->file_info, APX_REQUIRE_PORT_DATA_EXT))
  {
    self->apx_file_type = APX_REQUIRE_PORT_DATA_FILE_TYPE;
  }
  else if (rmf_file_info_name_ends_with(&self->file_info, APX_PROVIDE_PORT_COUNT_EXT))
  {
    self->apx_file_type = APX_PROVIDE_PORT_COUNT_FILE_TYPE;
  }
  else if (rmf_file_info_name_ends_with(&self->file_info, APX_REQUIRE_PORT_COUNT_EXT))
  {
    self->apx_file_type = APX_REQUIRE_PORT_COUNT_FILE_TYPE;
  }
  else
  {
    self->apx_file_type = APX_UNKNOWN_FILE_TYPE;
  }
}

static void apx_file_lock(apx_file_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
  }
}

static void apx_file_unlock(apx_file_t *self)
{
  if (self != NULL)
  {
    MUTEX_UNLOCK(self->lock);
  }
}
