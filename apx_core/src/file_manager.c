/*****************************************************************************
 * \file      file_manager.c
 * \author    Conny Gustafsson
 * \date      2020-01-27
 * \brief     New APX file manager
 *
 * Copyright (c) 2020-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/connection_base.h"
#include "apx/node_data.h"
#include "pack.h"
#include <assert.h>
#include <stdio.h> //DEBUG only
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
static void publish_local_files(apx_file_manager_t *self);
static apx_error_t process_message(apx_file_manager_t *self, uint32_t address, uint8_t const *data, apx_size_t size);
static apx_error_t process_command_message(apx_file_manager_t *self, uint8_t const *data, apx_size_t size);
static apx_error_t process_file_write_message(
  apx_file_manager_t *self, uint32_t address, uint8_t const *data, apx_size_t size);
static apx_error_t process_open_file_request(apx_file_manager_t *self, uint32_t start_address);
static apx_error_t process_close_file_request(apx_file_manager_t *self, uint32_t start_address);
static apx_error_t process_revoke_file_request(apx_file_manager_t *self, uint32_t start_address);
static apx_error_t process_remote_file_published(apx_file_manager_t *self, rmf_file_info_t const *file_info);
static apx_error_t process_nack_message(apx_file_manager_t *self, apx_error_t error_code, char const *name);
static apx_error_t process_stream_open_request(apx_file_manager_t *self, uint8_t const *data, apx_size_t size);
static apx_error_t process_stream_open_response(apx_file_manager_t *self, uint8_t const *data, apx_size_t size);
static apx_error_t process_stream_window_update(apx_file_manager_t *self, uint8_t const *data, apx_size_t size);
static apx_error_t process_stream_abort(apx_file_manager_t *self, uint8_t const *data, apx_size_t size);
static void abort_stream_with_error(apx_file_manager_t *self, apx_file_t *file, apx_error_t status);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_file_manager_create(apx_file_manager_t *self, uint8_t mode,
  apx_connection_interface_t const *parent_connection, apx_allocator_t *allocator)
{
  if (self != NULL)
  {
    apx_error_t result;
    self->mode = mode;
    result = apx_file_manager_receiver_create(&self->receiver);
    if (result == APX_NO_ERROR)
    {
      result = apx_file_manager_worker_create(&self->worker, &self->shared, mode);
      if (result == APX_NO_ERROR)
      {
        apx_file_manager_shared_create(&self->shared, parent_connection, allocator);
      }
      else
      {
        apx_file_manager_receiver_destroy(&self->receiver);
      }
    }
    return result;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_manager_destroy(apx_file_manager_t *self)
{
  if (self != NULL)
  {
    apx_file_manager_worker_destroy(&self->worker);
    apx_file_manager_receiver_destroy(&self->receiver);
    apx_file_manager_shared_destroy(&self->shared);
  }
}

apx_error_t apx_file_manager_start(apx_file_manager_t *self)
{
  if (self != NULL)
  {
    apx_file_manager_shared_start(&self->shared);
    return apx_file_manager_worker_start(&self->worker);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_manager_stop(apx_file_manager_t *self)
{
  if (self != NULL)
  {
    apx_file_manager_worker_stop(&self->worker);
  }
}

void apx_file_manager_connected(apx_file_manager_t *self)
{
  if (self != NULL)
  {
    if (self->mode == APX_CLIENT_MODE)
    {
      publish_local_files(self);
    }
    else
    {
      apx_file_manager_shared_connected(&self->shared);
      rmf_version_id_t const version_id = apx_file_manager_shared_get_remotefile_version_id(&self->shared);
      if (version_id == RMF_PROTOCOL_VERSION_ID_1_1)
      {
        uint32_t connection_id = apx_file_manager_shared_get_connection_id(&self->shared);
        apx_file_manager_worker_prepare_header_accepted(&self->worker, connection_id);
      }
      else
      {
        apx_file_manager_worker_prepare_acknowledge(&self->worker);
      }
    }
  }
}

void apx_file_manager_disconnected(apx_file_manager_t *self)
{
  if (self != NULL)
  {
    apx_file_manager_receiver_reset(&self->receiver);
    apx_file_manager_shared_disconnected(&self->shared);
  }
}

apx_file_t *apx_file_manager_create_local_file(apx_file_manager_t *self, rmf_file_info_t const *file_info)
{
  if (self != NULL)
  {
    apx_file_t *file = apx_file_manager_shared_create_local_file(&self->shared, file_info);
    if (file != NULL)
    {
      apx_file_set_file_manager(file, self);
    }
    return file;
  }
  return NULL;
}

apx_error_t apx_file_manager_publish_local_file(apx_file_manager_t *self, rmf_file_info_t const *file_info)
{
  if ((self != NULL) && (file_info != NULL))
  {
    apx_error_t retval = APX_MEM_ERROR;
    rmf_file_info_t *cloned_info = rmf_file_info_clone(file_info);
    if (cloned_info != NULL)
    {
      retval = apx_file_manager_worker_prepare_publish_local_file(&self->worker, cloned_info);
      if (retval != APX_NO_ERROR)
      {
        rmf_file_info_delete(cloned_info);
      }
    }
    return retval;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_revoke_local_file(apx_file_manager_t *self, uint32_t address)
{
  if (self != NULL)
  {
    uint32_t const clean_addr = address & RMF_ADDRESS_MASK_INTERNAL;
    apx_file_t *local_file = apx_file_manager_shared_find_file_by_address(&self->shared, clean_addr);
    if ((local_file == NULL) || (apx_file_get_address_without_flags(local_file) != clean_addr))
    {
      return APX_FILE_NOT_FOUND_ERROR;
    }
    apx_file_close(local_file);
    if (apx_file_is_stream(local_file))
    {
      apx_file_set_stream_state(local_file, APX_STREAM_STATE_REVOKED);
    }
    (void)apx_file_close_notify(local_file);
    return apx_file_manager_worker_prepare_revoke_local_file(&self->worker, clean_addr);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_file_t *apx_file_manager_find_file_by_address(apx_file_manager_t *self, uint32_t address)
{
  if (self != NULL)
  {
    return apx_file_manager_shared_find_file_by_address(&self->shared, address);
  }
  return NULL;
}

apx_file_t *apx_file_manager_find_local_file_by_name(apx_file_manager_t *self, char const *name)
{
  if (self != NULL)
  {
    return apx_file_manager_shared_find_local_file_by_name(&self->shared, name);
  }
  return NULL;
}

apx_file_t *apx_file_manager_find_remote_file_by_name(apx_file_manager_t *self, char const *name)
{
  if (self != NULL)
  {
    return apx_file_manager_shared_find_remote_file_by_name(&self->shared, name);
  }
  return NULL;
}

apx_error_t apx_file_manager_message_received(apx_file_manager_t *self, uint8_t const *msg_data, apx_size_t msg_len)
{
  if ((self != NULL) && (msg_data != NULL))
  {
    uint32_t address = RMF_INVALID_ADDRESS;
    bool more_bit = false;
    apx_size_t const header_size = rmf_address_decode(msg_data, msg_data + msg_len, &address, &more_bit);
    if (header_size > 0)
    {
      apx_error_t error_code;
      apx_file_manager_reception_result_t result;
      assert(msg_len >= header_size);
      apx_size_t const payload_len = msg_len - header_size;
      uint32_t const active_address = apx_file_manager_receiver_active_address(&self->receiver);
      apx_file_t *active_stream_file = NULL;

      if (active_address == RMF_INVALID_ADDRESS)
      {
        if (address < RMF_CMD_AREA_START_ADDRESS)
        {
          apx_file_t *remote_file =
            apx_file_manager_shared_find_file_by_address(&self->shared, address | RMF_REMOTE_ADDRESS_BIT);
          if ((remote_file != NULL) && apx_file_is_stream(remote_file))
          {
            active_stream_file = remote_file;
            if (apx_file_get_stream_state(remote_file) != APX_STREAM_STATE_OPEN)
            {
              abort_stream_with_error(self, remote_file, APX_FILE_NOT_OPEN_ERROR);
              return APX_FILE_NOT_OPEN_ERROR;
            }
            if (address != apx_file_get_address_without_flags(remote_file))
            {
              abort_stream_with_error(self, remote_file, APX_INVALID_ADDRESS_ERROR);
              return APX_INVALID_ADDRESS_ERROR;
            }
            if (payload_len == 0u)
            {
              abort_stream_with_error(self, remote_file, APX_VALUE_LENGTH_ERROR);
              return APX_VALUE_LENGTH_ERROR;
            }
            apx_file_manager_receiver_set_max_write_size(&self->receiver, apx_file_get_stream_block_size(remote_file));
          }
          else
          {
            apx_file_manager_receiver_set_max_write_size(&self->receiver, APX_MAX_FILE_SIZE);
          }
        }
        else
        {
          apx_file_manager_receiver_set_max_write_size(&self->receiver, APX_MAX_FILE_SIZE);
        }
      }
      else if (active_address < RMF_CMD_AREA_START_ADDRESS)
      {
        apx_file_t *remote_file =
          apx_file_manager_shared_find_file_by_address(&self->shared, active_address | RMF_REMOTE_ADDRESS_BIT);
        if ((remote_file != NULL) && apx_file_is_stream(remote_file))
        {
          active_stream_file = remote_file;
        }
      }

      error_code = apx_file_manager_receiver_write(
        &self->receiver, &result, address, msg_data + header_size, payload_len, more_bit);
      if (error_code != APX_NO_ERROR)
      {
        if (active_stream_file != NULL)
        {
          abort_stream_with_error(self, active_stream_file, error_code);
        }
        return error_code;
      }
      else if (result.is_complete)
      {
        return process_message(self, result.address, result.data, result.size);
      }
      else
      {
            // Wait for more data to arrive
      }
    }
    else
    {
      return APX_INVALID_MSG_ERROR;
    }
    return APX_NO_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_local_const_data(
  apx_file_manager_t *self, uint32_t address, uint8_t const *data, apx_size_t size)
{
  if (self != NULL && data != NULL)
  {
    apx_file_t *file = apx_file_manager_shared_find_file_by_address(&self->shared, address);
    if (file == NULL)
    {
      return APX_FILE_NOT_FOUND_ERROR;
    }
    if (!apx_file_is_open(file))
    {
      return APX_FILE_NOT_OPEN_ERROR;
    }
    if (apx_file_is_stream(file))
    {
      if (address != apx_file_get_address_without_flags(file))
      {
        return APX_INVALID_ADDRESS_ERROR;
      }
      if (size == 0u)
      {
        return APX_VALUE_LENGTH_ERROR;
      }
      if (size > apx_file_get_stream_block_size(file))
      {
        return APX_FILE_TOO_LARGE_ERROR;
      }
    }
    return apx_file_manager_worker_prepare_send_local_const_data(&self->worker, address, data, (uint32_t)size);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_local_data(apx_file_manager_t *self, uint32_t address, uint8_t *data, apx_size_t size)
{
  if (self != NULL && data != NULL)
  {
    apx_file_t *file = apx_file_manager_shared_find_file_by_address(&self->shared, address);
    if (file == NULL)
    {
      free(data);
      return APX_FILE_NOT_FOUND_ERROR;
    }
    if (!apx_file_is_open(file))
    {
      free(data);
      return APX_FILE_NOT_OPEN_ERROR;
    }
    if (apx_file_is_stream(file))
    {
      if (address != apx_file_get_address_without_flags(file))
      {
        free(data);
        return APX_INVALID_ADDRESS_ERROR;
      }
      if (size == 0u)
      {
        free(data);
        return APX_VALUE_LENGTH_ERROR;
      }
      if (size > apx_file_get_stream_block_size(file))
      {
        free(data);
        return APX_FILE_TOO_LARGE_ERROR;
      }
    }
    apx_error_t retval = apx_file_manager_worker_prepare_send_local_data(&self->worker, address, data, (uint32_t)size);
    if (retval != APX_NO_ERROR)
    {
      free(data);
    }
    return retval;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_open_file_request(apx_file_manager_t *self, uint32_t address)
{
  if (self != NULL)
  {
    apx_file_t *remote_file =
      apx_file_manager_shared_find_file_by_address(&self->shared, address | RMF_REMOTE_ADDRESS_BIT);
    if ((remote_file != NULL) && apx_file_is_stream(remote_file))
    {
      return APX_UNSUPPORTED_ERROR;
    }
    return apx_file_manager_worker_prepare_send_open_file_request(&self->worker, address & RMF_ADDRESS_MASK_INTERNAL);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_close_file_request(apx_file_manager_t *self, uint32_t address)
{
  if (self != NULL)
  {
    uint32_t const clean_addr = address & RMF_ADDRESS_MASK_INTERNAL;
    if (apx_file_manager_receiver_active_address(&self->receiver) == clean_addr)
    {
      apx_file_manager_receiver_reset(&self->receiver);
    }
    apx_file_t *remote_file =
      apx_file_manager_shared_find_file_by_address(&self->shared, clean_addr | RMF_REMOTE_ADDRESS_BIT);
    if (remote_file != NULL)
    {
      apx_file_close(remote_file);
    }
    return apx_file_manager_worker_prepare_send_close_file_request(&self->worker, clean_addr);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_open_stream(
  apx_file_manager_t *self, uint32_t address, uint32_t max_block_size, uint16_t window_blocks)
{
  if ((self == NULL) || (max_block_size == 0u) || (window_blocks == 0u))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  uint32_t const clean_addr = address & RMF_ADDRESS_MASK_INTERNAL;
  apx_file_t *remote_file =
    apx_file_manager_shared_find_file_by_address(&self->shared, clean_addr | RMF_REMOTE_ADDRESS_BIT);
  if ((remote_file == NULL) || (apx_file_get_address_without_flags(remote_file) != clean_addr))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  if (!apx_file_is_stream(remote_file))
  {
    return APX_INVALID_FILE_ERROR;
  }
  if (apx_file_get_stream_state(remote_file) != APX_STREAM_STATE_CLOSED)
  {
    return APX_BUSY_ERROR;
  }
  apx_file_set_stream_open_pending(remote_file, max_block_size, window_blocks, 0u);
  rmf_stream_open_cmd_t cmd = {
    .start_address = clean_addr,
    .max_block_size = max_block_size,
    .window_blocks = window_blocks,
    .flags = 0u,
  };
  return apx_file_manager_worker_prepare_send_stream_open(&self->worker, &cmd);
}

apx_error_t apx_file_manager_abort_stream(apx_file_manager_t *self, uint32_t address, apx_error_t status)
{
  if ((self == NULL) || (status == APX_NO_ERROR))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  uint32_t const clean_addr = address & RMF_ADDRESS_MASK_INTERNAL;
  apx_file_t *file = apx_file_manager_shared_find_file_by_address(&self->shared, address);
  if ((file == NULL) || (apx_file_get_address_without_flags(file) != clean_addr))
  {
    file = apx_file_manager_shared_find_file_by_address(&self->shared, clean_addr | RMF_REMOTE_ADDRESS_BIT);
  }
  if ((file == NULL) || (apx_file_get_address_without_flags(file) != clean_addr))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  if (!apx_file_is_stream(file))
  {
    return APX_INVALID_FILE_ERROR;
  }
  abort_stream_with_error(self, file, status);
  return APX_NO_ERROR;
}

apx_error_t apx_file_manager_consume_stream_blocks(apx_file_manager_t *self, uint32_t address, uint32_t count)
{
  if ((self == NULL) || (count == 0u))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  uint32_t const clean_addr = address & RMF_ADDRESS_MASK_INTERNAL;
  apx_file_t *remote_file =
    apx_file_manager_shared_find_file_by_address(&self->shared, clean_addr | RMF_REMOTE_ADDRESS_BIT);
  if ((remote_file == NULL) || (apx_file_get_address_without_flags(remote_file) != clean_addr))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  if (!apx_file_is_stream(remote_file))
  {
    return APX_INVALID_FILE_ERROR;
  }
  bool should_send_update = false;
  uint64_t new_consumed_count = 0u;
  apx_error_t err = apx_file_stream_consume_blocks(remote_file, count, &should_send_update, &new_consumed_count);
  if (err != APX_NO_ERROR)
  {
    return err;
  }
  if (should_send_update)
  {
    rmf_stream_window_update_cmd_t cmd = {
      .start_address = clean_addr,
      .consumed_block_count = new_consumed_count,
    };
    return apx_file_manager_worker_prepare_send_stream_window_update(&self->worker, &cmd);
  }
  return APX_NO_ERROR;
}

apx_error_t apx_file_manager_send_error_code(apx_file_manager_t *self, apx_error_t error_code, char const *name)
{
  if (self != NULL)
  {
    return apx_file_manager_worker_prepare_error_code(&self->worker, error_code, name);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_connection_create(
  apx_file_manager_t *self, apx_connection_id_t connection_id, apx_connection_state_t connection_state, char const *tag)
{
  if (self != NULL)
  {
    return apx_file_manager_worker_prepare_send_connection_create(&self->worker, connection_id, connection_state, tag);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

uint16_t apx_file_manager_get_num_pending_worker_commands(apx_file_manager_t *self)
{
  if (self != NULL)
  {
    return apx_file_manager_worker_num_pending_commands(&self->worker);
  }
  return 0u;
}


#ifdef UNIT_TEST
bool apx_file_manager_run(apx_file_manager_t *self)
{
  if (self != NULL)
  {
    return apx_file_manager_worker_run(&self->worker);
  }
  return false;
}

#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void publish_local_files(apx_file_manager_t *self)
{
  adt_ary_t local_file_list;
  int32_t i;
  int32_t num_files;
  adt_ary_create(&local_file_list, NULL);
  num_files = apx_file_manager_shared_copy_local_file_info(&self->shared, &local_file_list);
  if (num_files < 0)
  {
    int32_t len = adt_ary_length(&local_file_list);
    for (i = 0; i < len; i++)
    {
      rmf_file_info_delete((rmf_file_info_t *)adt_ary_value(&local_file_list, i));
    }
  }
  else
  {
    for (i = 0; i < num_files; i++)
    {
      rmf_file_info_t *file_info = (rmf_file_info_t *)adt_ary_value(&local_file_list, i);
      if (file_info != NULL)
      {
            // Worker takes memory ownership of file_info on success
        apx_error_t result = apx_file_manager_worker_prepare_publish_local_file(&self->worker, file_info);
        if (result != APX_NO_ERROR)
        {
          rmf_file_info_delete(file_info);
        }
      }
    }
  }
  adt_ary_destroy(&local_file_list);
}

static apx_error_t process_message(apx_file_manager_t *self, uint32_t address, uint8_t const *data, apx_size_t size)
{
  if (address == RMF_CMD_AREA_START_ADDRESS)
  {
    return process_command_message(self, data, size);
  }
  else if (address < RMF_CMD_AREA_START_ADDRESS)
  {
    return process_file_write_message(self, address, data, size);
  }
  return APX_INVALID_ADDRESS_ERROR;
}

static apx_error_t process_command_message(apx_file_manager_t *self, uint8_t const *data, apx_size_t size)
{
  if (size < RMF_CMD_TYPE_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  apx_error_t retval = APX_NO_ERROR;
  uint32_t const cmd_type = unpackLE(data, RMF_CMD_TYPE_SIZE);
  apx_size_t const cmd_size = size - RMF_CMD_TYPE_SIZE;
  uint8_t const *next = data + RMF_CMD_TYPE_SIZE;
  rmf_file_info_t *file_info;
  apx_size_t decoded_size;
  switch (cmd_type)
  {
  case RMF_CMD_PUBLISH_FILE_MSG:
    file_info = rmf_file_info_make_empty();
    if (file_info == NULL)
    {
      retval = APX_MEM_ERROR;
    }
    else
    {
      decoded_size = rmf_decode_publish_file_cmd(data, size, file_info);
      if (decoded_size > 0u)
      {
        retval = process_remote_file_published(self, file_info);
      }
      else
      {
        retval = APX_INVALID_MSG_ERROR;
      }
      rmf_file_info_delete(file_info);
    }
    break;
  case RMF_CMD_PUBLISH_SIGNED_FILE_MSG:
    if (apx_file_manager_shared_get_remotefile_version_id(&self->shared) < RMF_PROTOCOL_VERSION_ID_1_1)
    {
      retval = APX_UNSUPPORTED_ERROR;
      break;
    }
    file_info = rmf_file_info_make_empty();
    if (file_info == NULL)
    {
      retval = APX_MEM_ERROR;
    }
    else
    {
      decoded_size = rmf_decode_publish_signed_file_cmd(data, size, file_info);
      if (decoded_size > 0u)
      {
        retval = process_remote_file_published(self, file_info);
      }
      else
      {
        retval = APX_INVALID_MSG_ERROR;
      }
      rmf_file_info_delete(file_info);
    }
    break;
  case RMF_CMD_REVOKE_FILE_MSG:
    if (cmd_size == RMF_FILE_OPEN_CMD_SIZE)
    {
      uint32_t const address = unpackLE(next, UINT32_SIZE);
      retval = process_revoke_file_request(self, address);
    }
    else
    {
      retval = APX_INVALID_MSG_ERROR;
    }
    break;
  case RMF_CMD_NACK_MSG:
    if (cmd_size >= RMF_CMD_NACK_DATA_SIZE)
    {
      uint32_t nack_error_code = 0;
      char const *nack_name = NULL;
      decoded_size = rmf_decode_nack_cmd(data, data + size, &nack_error_code, &nack_name);
      if (decoded_size > 0u)
      {
        retval = process_nack_message(self, (apx_error_t)nack_error_code, nack_name);
      }
      else
      {
        retval = APX_INVALID_MSG_ERROR;
      }
    }
    else
    {
      retval = APX_INVALID_MSG_ERROR;
    }
    break;
  case RMF_CMD_OPEN_FILE_MSG:
    if (cmd_size == RMF_FILE_OPEN_CMD_SIZE)
    {
      uint32_t const address = unpackLE(next, UINT32_SIZE);
      retval = process_open_file_request(self, address);
    }
    else
    {
      retval = APX_INVALID_MSG_ERROR;
    }
    break;
  case RMF_CMD_CLOSE_FILE_MSG:
    if (cmd_size == RMF_FILE_CLOSE_CMD_SIZE)
    {
      uint32_t const address = unpackLE(next, UINT32_SIZE);
      retval = process_close_file_request(self, address);
    }
    else
    {
      retval = APX_INVALID_MSG_ERROR;
    }
    break;
  case RMF_CMD_STREAM_OPEN_MSG:
    retval = process_stream_open_request(self, data, size);
    break;
  case RMF_CMD_STREAM_OPEN_RSP_MSG:
    retval = process_stream_open_response(self, data, size);
    break;
  case RMF_CMD_STREAM_WINDOW_UPDATE_MSG:
    retval = process_stream_window_update(self, data, size);
    break;
  case RMF_CMD_STREAM_ABORT_MSG:
    retval = process_stream_abort(self, data, size);
    break;
  default:
    retval = APX_UNSUPPORTED_ERROR;
  }
  return retval;
}

static apx_error_t process_file_write_message(
  apx_file_manager_t *self, uint32_t address, uint8_t const *data, apx_size_t size)
{
  apx_file_t *file = apx_file_manager_shared_find_file_by_address(&self->shared, address | RMF_HIGH_ADDR_BIT);
  if (file == NULL)
  {
    return APX_INVALID_WRITE_ERROR;
  }
  if (apx_file_is_stream(file))
  {
    if (apx_file_get_stream_state(file) != APX_STREAM_STATE_OPEN)
    {
      abort_stream_with_error(self, file, APX_FILE_NOT_OPEN_ERROR);
      return APX_FILE_NOT_OPEN_ERROR;
    }
    uint32_t const start_address = apx_file_get_address_without_flags(file);
    if (address != start_address)
    {
      abort_stream_with_error(self, file, APX_INVALID_ADDRESS_ERROR);
      return APX_INVALID_ADDRESS_ERROR;
    }
    if (size == 0u)
    {
      abort_stream_with_error(self, file, APX_VALUE_LENGTH_ERROR);
      return APX_VALUE_LENGTH_ERROR;
    }
    if (size > apx_file_get_stream_block_size(file))
    {
      abort_stream_with_error(self, file, APX_FILE_TOO_LARGE_ERROR);
      return APX_FILE_TOO_LARGE_ERROR;
    }
    apx_error_t const win_err = apx_file_stream_on_block_received(file);
    if (win_err != APX_NO_ERROR)
    {
      abort_stream_with_error(self, file, win_err);
      return win_err;
    }
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(&self->shared);
    apx_error_t notify_err = APX_NO_ERROR;
    if ((connection != NULL) && (connection->remote_file_write_notification != NULL))
    {
      notify_err = connection->remote_file_write_notification(connection->arg, file, 0u, data, size);
      if (notify_err == APX_NOT_IMPLEMENTED_ERROR)
      {
        notify_err = apx_file_write_notify(file, 0u, data, (uint32_t)size);
      }
    }
    else
    {
      notify_err = apx_file_write_notify(file, 0u, data, (uint32_t)size);
    }
    if (notify_err == APX_INVALID_WRITE_HANDLER_ERROR)
    {
      notify_err = APX_NO_ERROR;
    }
    if ((notify_err == APX_NO_ERROR) && apx_file_get_stream_auto_consume(file))
    {
      bool should_send_update = false;
      uint64_t new_consumed_count = 0u;
      if (apx_file_stream_consume_blocks(file, 1u, &should_send_update, &new_consumed_count) == APX_NO_ERROR)
      {
        if (should_send_update)
        {
          rmf_stream_window_update_cmd_t update_cmd = {
            .start_address = start_address,
            .consumed_block_count = new_consumed_count,
          };
          (void)apx_file_manager_worker_prepare_send_stream_window_update(&self->worker, &update_cmd);
        }
      }
    }
    return notify_err;
  }

  if (!apx_file_is_open(file))
  {
      // Ignore writes on closed files
    return APX_NO_ERROR;
  }
  apx_connection_interface_t const *connection = apx_file_manager_shared_connection(&self->shared);
  if (connection == NULL)
  {
    return APX_NULL_PTR_ERROR;
  }
  uint32_t const start_address = apx_file_get_address_without_flags(file);
  assert(start_address <= address);
  uint32_t const offset = address - start_address;
  if (connection->remote_file_write_notification != NULL)
  {
    return connection->remote_file_write_notification(connection->arg, file, offset, data, size);
  }
  return apx_file_write_notify(file, offset, data, (uint32_t)size);
}

static apx_error_t process_open_file_request(apx_file_manager_t *self, uint32_t start_address)
{
  apx_file_t *file = apx_file_manager_shared_find_file_by_address(&self->shared, start_address);
  if (file == NULL)
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  if (apx_file_is_stream(file))
  {
    (void)apx_file_manager_worker_prepare_error_code(&self->worker, APX_UNSUPPORTED_ERROR, apx_file_get_name(file));
    return APX_UNSUPPORTED_ERROR;
  }
  apx_file_open(file);
  apx_file_open_notify(file);
  apx_connection_interface_t const *connection = apx_file_manager_shared_connection(&self->shared);
  if (connection != NULL && connection->remote_file_open_notification != NULL)
  {
    return connection->remote_file_open_notification(connection->arg, file);
  }
  return APX_NO_ERROR;
}

static apx_error_t process_close_file_request(apx_file_manager_t *self, uint32_t start_address)
{
  apx_file_t *file = apx_file_manager_shared_find_file_by_address(&self->shared, start_address);
  if ((file == NULL) || (apx_file_get_address_without_flags(file) != start_address))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  apx_file_close(file);
  (void)apx_file_close_notify(file);
  apx_connection_interface_t const *connection = apx_file_manager_shared_connection(&self->shared);
  if ((connection != NULL) && (connection->remote_file_close_notification != NULL))
  {
    return connection->remote_file_close_notification(connection->arg, file);
  }
  return APX_NO_ERROR;
}

static apx_error_t process_revoke_file_request(apx_file_manager_t *self, uint32_t start_address)
{
  if (apx_file_manager_receiver_active_address(&self->receiver) == start_address)
  {
    apx_file_manager_receiver_reset(&self->receiver);
  }
  apx_file_t *file =
    apx_file_manager_shared_find_file_by_address(&self->shared, start_address | RMF_REMOTE_ADDRESS_BIT);
  if ((file == NULL) || (apx_file_get_address_without_flags(file) != start_address))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  apx_file_close(file);
  if (apx_file_is_stream(file))
  {
    apx_file_set_stream_state(file, APX_STREAM_STATE_REVOKED);
  }
  (void)apx_file_close_notify(file);
  return APX_NO_ERROR;
}

static apx_error_t process_remote_file_published(apx_file_manager_t *self, rmf_file_info_t const *file_info)
{
  apx_file_t *file = apx_file_manager_shared_create_remote_file(&self->shared, file_info);
  if (file == NULL)
  {
    return APX_FILE_CREATE_ERROR;
  }
  apx_file_set_file_manager(file, self);
  apx_connection_interface_t const *connection = apx_file_manager_shared_connection(&self->shared);
  if (connection == NULL)
  {
    return APX_NULL_PTR_ERROR;
  }
  if (connection->remote_file_published_notification != NULL)
  {
    return connection->remote_file_published_notification(connection->arg, file);
  }
  return APX_NO_ERROR;
}

static apx_error_t process_nack_message(apx_file_manager_t *self, apx_error_t error_code, char const *name)
{
  apx_connection_interface_t const *connection = apx_file_manager_shared_connection(&self->shared);
  if (connection != NULL && connection->nack_notification != NULL)
  {
    return connection->nack_notification(connection->arg, error_code, name);
  }
  return APX_NO_ERROR;
}

static apx_error_t process_stream_open_request(apx_file_manager_t *self, uint8_t const *data, apx_size_t size)
{
  if (size != RMF_STREAM_OPEN_CMD_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  rmf_stream_open_cmd_t cmd;
  if (rmf_decode_stream_open_cmd(data, data + size, &cmd) != RMF_STREAM_OPEN_CMD_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  rmf_stream_open_rsp_cmd_t rsp = {
    .start_address = cmd.start_address,
    .status = (uint32_t)APX_NO_ERROR,
    .block_size = 0u,
    .window_blocks = 0u,
    .flags = 0u,
  };
  apx_file_t *local_file = apx_file_manager_shared_find_file_by_address(&self->shared, cmd.start_address);
  if ((local_file == NULL) || (apx_file_get_address_without_flags(local_file) != cmd.start_address))
  {
    rsp.status = (uint32_t)APX_FILE_NOT_FOUND_ERROR;
  }
  else if (!apx_file_is_stream(local_file))
  {
    rsp.status = (uint32_t)APX_INVALID_FILE_ERROR;
  }
  else if ((cmd.max_block_size == 0u) || (cmd.window_blocks == 0u) || (cmd.flags != 0u))
  {
    rsp.status = (uint32_t)APX_INVALID_ARGUMENT_ERROR;
  }
  else if (apx_file_get_stream_state(local_file) != APX_STREAM_STATE_CLOSED)
  {
    rsp.status = (uint32_t)APX_BUSY_ERROR;
  }
  else
  {
    uint32_t const published_size = apx_file_get_size(local_file);
    uint32_t const negotiated_block_size = (cmd.max_block_size < published_size) ? cmd.max_block_size : published_size;
    uint16_t const max_local_win = apx_file_get_stream_local_max_window_blocks(local_file);
    uint16_t const negotiated_window = (cmd.window_blocks < max_local_win) ? cmd.window_blocks : max_local_win;
    if ((negotiated_block_size == 0u) || (negotiated_window == 0u))
    {
      rsp.status = (uint32_t)APX_BUSY_ERROR;
    }
    else
    {
      rsp.status = (uint32_t)APX_NO_ERROR;
      rsp.block_size = negotiated_block_size;
      rsp.window_blocks = negotiated_window;
      rsp.flags = 0u;
      apx_file_open_stream(local_file, negotiated_block_size, negotiated_window, 0u);
    }
  }
  apx_error_t const send_rc = apx_file_manager_worker_prepare_send_stream_open_rsp(&self->worker, &rsp);
  if ((rsp.status == (uint32_t)APX_NO_ERROR) && (local_file != NULL))
  {
    (void)apx_file_open_notify(local_file);
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(&self->shared);
    if ((connection != NULL) && (connection->remote_file_open_notification != NULL))
    {
      (void)connection->remote_file_open_notification(connection->arg, local_file);
    }
  }
  return send_rc;
}

static apx_error_t process_stream_open_response(apx_file_manager_t *self, uint8_t const *data, apx_size_t size)
{
  if (size != RMF_STREAM_OPEN_RSP_CMD_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  rmf_stream_open_rsp_cmd_t rsp;
  if (rmf_decode_stream_open_rsp_cmd(data, data + size, &rsp) != RMF_STREAM_OPEN_RSP_CMD_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  apx_file_t *remote_file =
    apx_file_manager_shared_find_file_by_address(&self->shared, rsp.start_address | RMF_REMOTE_ADDRESS_BIT);
  if ((remote_file == NULL) || (apx_file_get_address_without_flags(remote_file) != rsp.start_address) ||
    (!apx_file_is_stream(remote_file)))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  if (apx_file_get_stream_state(remote_file) != APX_STREAM_STATE_OPEN_PENDING)
  {
    return APX_INVALID_STATE_ERROR;
  }
  if (rsp.status != (uint32_t)APX_NO_ERROR)
  {
    apx_file_abort_stream(remote_file, rsp.status);
    return APX_NO_ERROR;
  }
  uint32_t const requested_max_block = apx_file_get_stream_block_size(remote_file);
  uint16_t const requested_window = apx_file_get_stream_window_blocks(remote_file);
  uint32_t const published_size = apx_file_get_size(remote_file);
  if ((rsp.block_size == 0u) || (rsp.block_size > published_size) || (rsp.block_size > requested_max_block) ||
    (rsp.window_blocks == 0u) || (rsp.window_blocks > requested_window) || (rsp.flags != 0u))
  {
    abort_stream_with_error(self, remote_file, APX_INVALID_MSG_ERROR);
    return APX_INVALID_MSG_ERROR;
  }
  apx_file_open_stream(remote_file, rsp.block_size, rsp.window_blocks, rsp.flags);
  (void)apx_file_open_notify(remote_file);
  return APX_NO_ERROR;
}

static apx_error_t process_stream_window_update(apx_file_manager_t *self, uint8_t const *data, apx_size_t size)
{
  if (size != RMF_STREAM_WINDOW_UPDATE_CMD_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  rmf_stream_window_update_cmd_t cmd;
  if (rmf_decode_stream_window_update_cmd(data, data + size, &cmd) != RMF_STREAM_WINDOW_UPDATE_CMD_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  apx_file_t *local_file = apx_file_manager_shared_find_file_by_address(&self->shared, cmd.start_address);
  if ((local_file == NULL) || (apx_file_get_address_without_flags(local_file) != cmd.start_address) ||
    (!apx_file_is_stream(local_file)))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  apx_error_t const update_err = apx_file_stream_update_consumed_from_peer(local_file, cmd.consumed_block_count);
  if (update_err != APX_NO_ERROR)
  {
    abort_stream_with_error(self, local_file, update_err);
    return update_err;
  }
  if ((apx_file_stream_pending_block_count(local_file) > 0) && apx_file_stream_can_transmit(local_file))
  {
    return apx_file_manager_worker_prepare_drain_stream_data(&self->worker, cmd.start_address);
  }
  return APX_NO_ERROR;
}

static apx_error_t process_stream_abort(apx_file_manager_t *self, uint8_t const *data, apx_size_t size)
{
  if (size != RMF_STREAM_ABORT_CMD_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  rmf_stream_abort_cmd_t cmd;
  if (rmf_decode_stream_abort_cmd(data, data + size, &cmd) != RMF_STREAM_ABORT_CMD_SIZE)
  {
    return APX_INVALID_MSG_ERROR;
  }
  if (cmd.status == (uint32_t)APX_NO_ERROR)
  {
    return APX_INVALID_MSG_ERROR;
  }
  if (apx_file_manager_receiver_active_address(&self->receiver) == cmd.start_address)
  {
    apx_file_manager_receiver_reset(&self->receiver);
  }
  apx_file_t *local_file = apx_file_manager_shared_find_file_by_address(&self->shared, cmd.start_address);
  if ((local_file != NULL) && (apx_file_get_address_without_flags(local_file) == cmd.start_address) &&
    apx_file_is_stream(local_file))
  {
    apx_file_abort_stream(local_file, cmd.status);
    (void)apx_file_close_notify(local_file);
  }
  apx_file_t *remote_file =
    apx_file_manager_shared_find_file_by_address(&self->shared, cmd.start_address | RMF_REMOTE_ADDRESS_BIT);
  if ((remote_file != NULL) && (apx_file_get_address_without_flags(remote_file) == cmd.start_address) &&
    apx_file_is_stream(remote_file))
  {
    apx_file_abort_stream(remote_file, cmd.status);
    (void)apx_file_close_notify(remote_file);
  }
  return APX_NO_ERROR;
}

static void abort_stream_with_error(apx_file_manager_t *self, apx_file_t *file, apx_error_t status)
{
  if ((self != NULL) && (file != NULL))
  {
    uint32_t const start_address = apx_file_get_address_without_flags(file);
    if (apx_file_manager_receiver_active_address(&self->receiver) == start_address)
    {
      apx_file_manager_receiver_reset(&self->receiver);
    }
    apx_file_abort_stream(file, (uint32_t)status);
    rmf_stream_abort_cmd_t abort_cmd = {
      .start_address = start_address,
      .status = (uint32_t)status,
    };
    (void)apx_file_manager_worker_prepare_send_stream_abort(&self->worker, &abort_cmd);
  }
}
