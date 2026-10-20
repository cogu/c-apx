/*****************************************************************************
 * \file      file_manager_worker.c
 * \author    Conny Gustafsson
 * \date      2020-01-23
 * \brief     APX Filemanager worker
 *
 * Copyright (c) 2020-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <malloc.h>
#include <string.h>
#ifdef _WIN32
# include <process.h>
#endif
#include "apx/file_manager_worker.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define RMF_FRAME_OVERHEAD_MARGIN 16u

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void cleanup_cmd_queue(adt_rbfh_t *queue);
static bool process_single_command(apx_file_manager_worker_t *self, apx_command_t const *cmd);
static apx_error_t run_send_acknowledge(apx_file_manager_worker_t *self);
static apx_error_t run_send_error_code(apx_file_manager_worker_t *self, uint32_t error_code, char const *name);
static apx_error_t run_publish_local_file(apx_file_manager_worker_t *self, rmf_file_info_t *file);
static apx_error_t run_send_local_const_data(
  apx_file_manager_worker_t *self, uint32_t address, uint8_t const *data, uint32_t size);
static apx_error_t run_send_local_data(apx_file_manager_worker_t *self, uint32_t address, uint8_t *data, uint32_t size);
static apx_error_t run_open_remote_file(apx_file_manager_worker_t *self, uint32_t address);
static apx_error_t run_close_remote_file(apx_file_manager_worker_t *self, uint32_t address);
static apx_error_t run_send_header_accepted(apx_file_manager_worker_t *self, uint32_t connection_id);
static apx_error_t run_send_create_connection(
  apx_file_manager_worker_t *self, uint32_t connection_id, apx_connection_state_t connection_state, char const *tag);
static apx_error_t run_send_stream_open(apx_file_manager_worker_t *self, rmf_stream_open_cmd_t const *cmd);
static apx_error_t run_send_stream_open_rsp(apx_file_manager_worker_t *self, rmf_stream_open_rsp_cmd_t const *cmd);
static apx_error_t run_send_stream_window_update(
  apx_file_manager_worker_t *self, rmf_stream_window_update_cmd_t const *cmd);
static apx_error_t run_send_stream_abort(apx_file_manager_worker_t *self, rmf_stream_abort_cmd_t const *cmd);
static apx_error_t run_drain_stream_data(apx_file_manager_worker_t *self, uint32_t address);
static apx_error_t transmit_data_with_fragmentation(
  apx_connection_interface_t const *connection, uint32_t address, uint8_t const *data, uint32_t size);
static apx_error_t apx_file_manager_worker_process_ringbuffer_error(adt_buf_err_t error_code);
#ifndef UNIT_TEST
static apx_error_t start_worker_thread(apx_file_manager_worker_t *self);
static apx_error_t stop_worker_thread(apx_file_manager_worker_t *self);
static THREAD_PROTO(worker_main, arg);
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_file_manager_worker_create(
  apx_file_manager_worker_t *self, apx_file_manager_shared_t *shared, apx_mode_t mode)
{
  if (self != NULL)
  {
    adt_buf_err_t buf_result = adt_rbfh_create(&self->queue, (uint8_t)APX_COMMAND_SIZE);
    if (buf_result != BUF_E_OK)
    {
      return APX_MEM_ERROR;
    }
    self->mode = mode;
    self->shared = shared;
    self->worker_thread_valid = false;
    MUTEX_INIT(self->mutex);
    (void)SPINLOCK_INIT(self->queue_lock);
    SEMAPHORE_CREATE(self->semaphore);
#ifdef _WIN32
    self->worker_thread = INVALID_HANDLE_VALUE;
    self->worker_thread_id = 0u;
#else
    self->worker_thread = 0;
#endif
    return APX_NO_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_manager_worker_destroy(apx_file_manager_worker_t *self)
{
  if (self != NULL)
  {
    if (self->worker_thread_valid == true)
    {
#ifndef UNIT_TEST
      stop_worker_thread(self);
#endif
    }
    MUTEX_DESTROY(self->mutex);
    SPINLOCK_DESTROY(self->queue_lock);
    SEMAPHORE_DESTROY(self->semaphore);
    cleanup_cmd_queue(&self->queue);
    adt_rbfh_destroy(&self->queue);
  }
}

uint16_t apx_file_manager_worker_num_pending_commands(apx_file_manager_worker_t *self)
{
  if (self != NULL)
  {
    uint16_t retval;
    SPINLOCK_ENTER(self->queue_lock);
    retval = adt_rbfh_length(&self->queue);
    SPINLOCK_LEAVE(self->queue_lock);
    return retval;
  }
  return 0u;
}

#ifdef UNIT_TEST
bool apx_file_manager_worker_run(apx_file_manager_worker_t *self)
{
  if (self != NULL)
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      assert(connection->transmit_begin != NULL);
      connection->transmit_begin(connection->arg);
    }
    while (adt_rbfh_length(&self->queue) > 0)
    {
      apx_command_t cmd;
      bool result;
      adt_rbfh_remove(&self->queue, (uint8_t *)&cmd);
      result = process_single_command(self, &cmd);
      if (!result)
      {
        if (connection != NULL)
        {
          assert(connection->transmit_end != NULL);
          connection->transmit_end(connection->arg);
        }
        return result;
      }
    }
    if (connection != NULL)
    {
      assert(connection->transmit_end != NULL);
      connection->transmit_end(connection->arg);
    }
    return true;
  }
  return false;
}
#endif

apx_error_t apx_file_manager_worker_start(apx_file_manager_worker_t *self)
{
  if (self != NULL)
  {
#ifdef UNIT_TEST
    return APX_NO_ERROR;
#else
    return start_worker_thread(self);
#endif
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_manager_worker_stop(apx_file_manager_worker_t *self)
{
  if (self != NULL)
  {
#ifndef UNIT_TEST
    stop_worker_thread(self);
#endif
  }
}

// Command API

apx_error_t apx_file_manager_worker_prepare_acknowledge(apx_file_manager_worker_t *self)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_SEND_ACKNOWLEDGE, 0, 0, {0}, 0};
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_error_code(
  apx_file_manager_worker_t *self, apx_error_t error_code, char const *name)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_SEND_ERROR_CODE, 0, 0, {0}, 0};
    cmd.data1 = (uint32_t)error_code;
    if (name != NULL)
    {
      cmd.data3.ptr = STRDUP(name);
      if (cmd.data3.ptr == NULL)
      {
        return APX_MEM_ERROR;
      }
    }
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_header_accepted(apx_file_manager_worker_t *self, uint32_t connection_id)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_SEND_HEADER_ACCEPTED, 0, 0, {0}, 0};
    cmd.data1 = connection_id;
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_publish_local_file(
  apx_file_manager_worker_t *self, rmf_file_info_t *file_info)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_PUBLISH_LOCAL_FILE, 0, 0, {0}, 0};
    cmd.data3.ptr = (void *)file_info;
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_local_const_data(
  apx_file_manager_worker_t *self, uint32_t address, uint8_t const *data, uint32_t size)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd;
    apx_build_command_with_ptr(&cmd, APX_CMD_SEND_LOCAL_CONST_DATA, address, size, (void *)data, NULL);
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_local_data(
  apx_file_manager_worker_t *self, uint32_t address, uint8_t *data, uint32_t size)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd;
    apx_build_command_with_ptr(&cmd, APX_CMD_SEND_LOCAL_DATA, address, size, data, NULL);
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_open_file_request(apx_file_manager_worker_t *self, uint32_t address)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_OPEN_REMOTE_FILE, 0, 0, {0}, 0};
    cmd.data1 = address;
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_close_file_request(apx_file_manager_worker_t *self, uint32_t address)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_CLOSE_REMOTE_FILE, 0, 0, {0}, 0};
    cmd.data1 = address;
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_connection_create(apx_file_manager_worker_t *self,
  apx_connection_id_t connection_id, apx_connection_state_t connection_state, char const *tag)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_CREATE_CONNECTION, 0, 0, {0}, 0};
    cmd.data1 = connection_id;
    cmd.data2 = connection_state;
    if (tag != NULL)
    {
      cmd.data3.ptr = STRDUP(tag);
      if (cmd.data3.ptr == NULL)
      {
        return APX_MEM_ERROR;
      }
    }
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_stream_open(
  apx_file_manager_worker_t *self, rmf_stream_open_cmd_t const *cmd_in)
{
  if ((self != NULL) && (cmd_in != NULL))
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_SEND_STREAM_OPEN, 0, 0, {0}, 0};
    cmd.data1 = cmd_in->start_address;
    cmd.data2 = cmd_in->max_block_size;
    packLE(&cmd.data3.data[0], (uint32_t)cmd_in->window_blocks, (uint8_t)UINT16_SIZE);
    packLE(&cmd.data3.data[2], (uint32_t)cmd_in->flags, (uint8_t)UINT16_SIZE);
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_stream_open_rsp(
  apx_file_manager_worker_t *self, rmf_stream_open_rsp_cmd_t const *cmd_in)
{
  if ((self != NULL) && (cmd_in != NULL))
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_SEND_STREAM_OPEN_RSP, 0, 0, {0}, 0};
    cmd.data1 = cmd_in->start_address;
    cmd.data2 = cmd_in->status;
    packLE(&cmd.data3.data[0], cmd_in->block_size, (uint8_t)UINT32_SIZE);
    packLE(&cmd.data3.data[4], (uint32_t)cmd_in->window_blocks, (uint8_t)UINT16_SIZE);
    packLE(&cmd.data3.data[6], (uint32_t)cmd_in->flags, (uint8_t)UINT16_SIZE);
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_stream_window_update(
  apx_file_manager_worker_t *self, rmf_stream_window_update_cmd_t const *cmd_in)
{
  if ((self != NULL) && (cmd_in != NULL))
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_SEND_STREAM_WINDOW_UPDATE, 0, 0, {0}, 0};
    cmd.data1 = cmd_in->start_address;
    packLE64(&cmd.data3.data[0], cmd_in->consumed_block_count, (uint8_t)UINT64_SIZE);
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_send_stream_abort(
  apx_file_manager_worker_t *self, rmf_stream_abort_cmd_t const *cmd_in)
{
  if ((self != NULL) && (cmd_in != NULL))
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_SEND_STREAM_ABORT, 0, 0, {0}, 0};
    cmd.data1 = cmd_in->start_address;
    cmd.data2 = cmd_in->status;
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_worker_prepare_drain_stream_data(apx_file_manager_worker_t *self, uint32_t address)
{
  if (self != NULL)
  {
    adt_buf_err_t rc;
    apx_command_t cmd = {APX_CMD_DRAIN_STREAM_DATA, 0, 0, {0}, 0};
    cmd.data1 = address;
    SPINLOCK_ENTER(self->queue_lock);
    rc = adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
    SEMAPHORE_POST(self->semaphore);
#endif
    return apx_file_manager_worker_process_ringbuffer_error(rc);
  }
  return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void cleanup_cmd_queue(adt_rbfh_t *queue)
{
  assert(queue != NULL);
  while (adt_rbfh_length(queue) > 0)
  {
    apx_command_t cmd;
    adt_rbfh_remove(queue, (uint8_t *)&cmd);
    if (cmd.cmd_type == APX_CMD_PUBLISH_LOCAL_FILE)
    {
      rmf_file_info_delete((rmf_file_info_t *)cmd.data3.ptr);
    }
    else if (cmd.cmd_type == APX_CMD_SEND_LOCAL_DATA || cmd.cmd_type == APX_CMD_CREATE_CONNECTION ||
      cmd.cmd_type == APX_CMD_SEND_ERROR_CODE)
    {
      free(cmd.data3.ptr);
    }
  }
}

static bool process_single_command(apx_file_manager_worker_t *self, apx_command_t const *cmd)
{
  apx_error_t result = APX_NO_ERROR;
  assert((self != NULL) && (cmd != NULL));
  switch (cmd->cmd_type)
  {
  case APX_CMD_EXIT:
    return false;
  case APX_CMD_SEND_ACKNOWLEDGE:
    result = run_send_acknowledge(self);
    break;
  case APX_CMD_SEND_ERROR_CODE:
    result = run_send_error_code(self, cmd->data1, (char const *)cmd->data3.ptr);
    if (cmd->data3.ptr != NULL)
    {
      free(cmd->data3.ptr);
    }
    break;
  case APX_CMD_PUBLISH_LOCAL_FILE:
    result = run_publish_local_file(self, (rmf_file_info_t *)cmd->data3.ptr);
    break;
  case APX_CMD_REVOKE_LOCAL_FILE:
      // Deferred to roadmap Item 7
    break;
  case APX_CMD_OPEN_REMOTE_FILE:
    result = run_open_remote_file(self, cmd->data1);
    break;
  case APX_CMD_CLOSE_REMOTE_FILE:
    result = run_close_remote_file(self, cmd->data1);
    break;
  case APX_CMD_SEND_LOCAL_CONST_DATA:
    result = run_send_local_const_data(self, cmd->data1, (uint8_t const *)cmd->data3.ptr, cmd->data2);
    break;
  case APX_CMD_SEND_LOCAL_DATA:
    result = run_send_local_data(self, cmd->data1, (uint8_t *)cmd->data3.ptr, cmd->data2);
    break;
  case APX_CMD_SEND_HEADER_ACCEPTED:
    result = run_send_header_accepted(self, cmd->data1);
    break;
  case APX_CMD_CREATE_CONNECTION:
    result =
      run_send_create_connection(self, cmd->data1, (apx_connection_state_t)cmd->data2, (char const *)cmd->data3.ptr);
    if (cmd->data3.ptr != NULL)
    {
      free(cmd->data3.ptr);
    }
    break;
  case APX_CMD_SEND_STREAM_OPEN: {
    rmf_stream_open_cmd_t stream_open_cmd;
    stream_open_cmd.start_address = cmd->data1;
    stream_open_cmd.max_block_size = cmd->data2;
    stream_open_cmd.window_blocks = (uint16_t)unpackLE(&cmd->data3.data[0], (uint8_t)UINT16_SIZE);
    stream_open_cmd.flags = (uint16_t)unpackLE(&cmd->data3.data[2], (uint8_t)UINT16_SIZE);
    result = run_send_stream_open(self, &stream_open_cmd);
    break;
  }
  case APX_CMD_SEND_STREAM_OPEN_RSP: {
    rmf_stream_open_rsp_cmd_t stream_open_rsp_cmd;
    stream_open_rsp_cmd.start_address = cmd->data1;
    stream_open_rsp_cmd.status = cmd->data2;
    stream_open_rsp_cmd.block_size = unpackLE(&cmd->data3.data[0], (uint8_t)UINT32_SIZE);
    stream_open_rsp_cmd.window_blocks = (uint16_t)unpackLE(&cmd->data3.data[4], (uint8_t)UINT16_SIZE);
    stream_open_rsp_cmd.flags = (uint16_t)unpackLE(&cmd->data3.data[6], (uint8_t)UINT16_SIZE);
    result = run_send_stream_open_rsp(self, &stream_open_rsp_cmd);
    break;
  }
  case APX_CMD_SEND_STREAM_WINDOW_UPDATE: {
    rmf_stream_window_update_cmd_t window_update_cmd;
    window_update_cmd.start_address = cmd->data1;
    window_update_cmd.consumed_block_count = unpackLE64(&cmd->data3.data[0], (uint8_t)UINT64_SIZE);
    result = run_send_stream_window_update(self, &window_update_cmd);
    break;
  }
  case APX_CMD_SEND_STREAM_ABORT: {
    rmf_stream_abort_cmd_t abort_cmd;
    abort_cmd.start_address = cmd->data1;
    abort_cmd.status = cmd->data2;
    result = run_send_stream_abort(self, &abort_cmd);
    break;
  }
  case APX_CMD_DRAIN_STREAM_DATA:
    result = run_drain_stream_data(self, cmd->data1);
    break;
  default:
    return false;
  }
  (void)result;
  return true;
}

static apx_error_t run_send_acknowledge(apx_file_manager_worker_t *self)
{
  uint8_t buffer[RMF_CMD_TYPE_SIZE];
  apx_size_t const encoded_size = rmf_encode_acknowledge_cmd(buffer, (apx_size_t)sizeof(buffer));
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_send_error_code(apx_file_manager_worker_t *self, uint32_t error_code, char const *name)
{
  uint8_t buffer[RMF_CMD_NACK_SIZE + RMF_MAX_FILE_NAME_SIZE + 1u];
  apx_size_t const encoded_size = rmf_encode_nack_cmd(buffer, (apx_size_t)sizeof(buffer), error_code, name);
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_publish_local_file(apx_file_manager_worker_t *self, rmf_file_info_t *file_info)
{
  uint8_t buffer[RMF_SIGNED_FILE_INFO_HEADER_SIZE + RMF_FILE_NAME_MAX_SIZE + 1]; // add 1 byte for null-terminator
  apx_size_t encoded_size;
  if (rmf_file_info_is_signed(file_info))
  {
    encoded_size = rmf_encode_publish_signed_file_cmd(buffer, (apx_size_t)sizeof(buffer), file_info);
  }
  else
  {
    encoded_size = rmf_encode_publish_file_cmd(buffer, (apx_size_t)sizeof(buffer), file_info);
  }
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  rmf_file_info_delete(file_info);
  return retval;
}

static apx_error_t run_send_local_const_data(
  apx_file_manager_worker_t *self, uint32_t address, uint8_t const *data, uint32_t size)
{
  apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
  apx_error_t retval = APX_NO_ERROR;
  if (connection != NULL)
  {
    apx_file_t *file = apx_file_manager_shared_find_file_by_address(self->shared, address);
    if ((file != NULL) && apx_file_is_stream(file))
    {
      if (apx_file_get_stream_state(file) != APX_STREAM_STATE_OPEN)
      {
        return APX_FILE_NOT_OPEN_ERROR;
      }
      if ((apx_file_stream_pending_block_count(file) > 0) || (!apx_file_stream_can_transmit(file)))
      {
        return apx_file_stream_enqueue_block(file, data, size);
      }
      retval = transmit_data_with_fragmentation(connection, address, data, size);
      if (retval == APX_NO_ERROR)
      {
        apx_file_stream_increment_transmitted(file);
      }
    }
    else
    {
      retval = transmit_data_with_fragmentation(connection, address, data, size);
    }
  }
  else
  {
    retval = APX_NOT_CONNECTED_ERROR;
  }
  return retval;
}

static apx_error_t run_send_local_data(apx_file_manager_worker_t *self, uint32_t address, uint8_t *data, uint32_t size)
{
  apx_error_t retval = run_send_local_const_data(self, address, data, size);
  free(data);
  return retval;
}

static apx_error_t run_open_remote_file(apx_file_manager_worker_t *self, uint32_t address)
{
  uint8_t buffer[RMF_CMD_TYPE_SIZE + RMF_FILE_OPEN_CMD_SIZE];
  apx_size_t const encoded_size = rmf_encode_open_file_cmd(buffer, (apx_size_t)sizeof(buffer), address);
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_close_remote_file(apx_file_manager_worker_t *self, uint32_t address)
{
  uint8_t buffer[RMF_CMD_TYPE_SIZE + RMF_FILE_CLOSE_CMD_SIZE];
  apx_size_t const encoded_size = rmf_encode_close_file_cmd(buffer, (apx_size_t)sizeof(buffer), address);
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_send_header_accepted(apx_file_manager_worker_t *self, uint32_t connection_id)
{
  uint8_t buffer[RMF_CMD_TYPE_SIZE + UINT32_SIZE];
  apx_size_t const encoded_size = rmf_encode_header_accepted(buffer, (apx_size_t)sizeof(buffer), connection_id);
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_send_create_connection(
  apx_file_manager_worker_t *self, uint32_t connection_id, apx_connection_state_t connection_state, char const *tag)
{
  apx_error_t retval = APX_NO_ERROR;
  uint8_t buffer[RMF_CMD_TYPE_SIZE + UINT32_SIZE + UINT8_SIZE + APX_MAX_TAG_STR_SIZE + 1];
  apx_size_t const encoded_size =
    rmf_encode_connection_create(buffer, (apx_size_t)sizeof(buffer), connection_id, (uint8_t)connection_state, tag);
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_send_stream_open(apx_file_manager_worker_t *self, rmf_stream_open_cmd_t const *cmd)
{
  uint8_t buffer[RMF_STREAM_OPEN_CMD_SIZE];
  apx_size_t const encoded_size = rmf_encode_stream_open_cmd(buffer, (apx_size_t)sizeof(buffer), cmd);
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_send_stream_open_rsp(apx_file_manager_worker_t *self, rmf_stream_open_rsp_cmd_t const *cmd)
{
  uint8_t buffer[RMF_STREAM_OPEN_RSP_CMD_SIZE];
  apx_size_t const encoded_size = rmf_encode_stream_open_rsp_cmd(buffer, (apx_size_t)sizeof(buffer), cmd);
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_send_stream_window_update(
  apx_file_manager_worker_t *self, rmf_stream_window_update_cmd_t const *cmd)
{
  uint8_t buffer[RMF_STREAM_WINDOW_UPDATE_CMD_SIZE];
  apx_size_t const encoded_size = rmf_encode_stream_window_update_cmd(buffer, (apx_size_t)sizeof(buffer), cmd);
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_send_stream_abort(apx_file_manager_worker_t *self, rmf_stream_abort_cmd_t const *cmd)
{
  uint8_t buffer[RMF_STREAM_ABORT_CMD_SIZE];
  apx_size_t const encoded_size = rmf_encode_stream_abort_cmd(buffer, (apx_size_t)sizeof(buffer), cmd);
  apx_error_t retval = APX_NO_ERROR;
  if (encoded_size == 0u)
  {
    retval = APX_BUFFER_BOUNDARY_ERROR;
  }
  else
  {
    apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
    if (connection != NULL)
    {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(
        connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
    }
    else
    {
      retval = APX_NOT_CONNECTED_ERROR;
    }
  }
  return retval;
}

static apx_error_t run_drain_stream_data(apx_file_manager_worker_t *self, uint32_t address)
{
  apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);
  if (connection == NULL)
  {
    return APX_NOT_CONNECTED_ERROR;
  }
  apx_file_t *file = apx_file_manager_shared_find_file_by_address(self->shared, address);
  if ((file == NULL) || (!apx_file_is_stream(file)))
  {
    return APX_FILE_NOT_FOUND_ERROR;
  }
  uint32_t const start_address = apx_file_get_address_without_flags(file);
  apx_error_t retval = APX_NO_ERROR;
  while ((apx_file_get_stream_state(file) == APX_STREAM_STATE_OPEN) && apx_file_stream_can_transmit(file) &&
    (apx_file_stream_pending_block_count(file) > 0))
  {
    apx_stream_block_item_t *item = apx_file_stream_dequeue_block(file);
    if (item == NULL)
    {
      break;
    }
    retval = transmit_data_with_fragmentation(connection, start_address, item->data, item->size);
    apx_stream_block_item_delete(item);
    if (retval != APX_NO_ERROR)
    {
      break;
    }
    apx_file_stream_increment_transmitted(file);
  }
  return retval;
}

static apx_error_t transmit_data_with_fragmentation(
  apx_connection_interface_t const *connection, uint32_t address, uint8_t const *data, uint32_t size)
{
  if ((connection == NULL) || (connection->transmit_data_message == NULL))
  {
    return APX_NOT_CONNECTED_ERROR;
  }
  uint32_t max_chunk_size = size;
  if (connection->transmit_max_buffer_size != NULL)
  {
    int32_t const max_buf = connection->transmit_max_buffer_size(connection->arg);
    if ((max_buf > (int32_t)RMF_FRAME_OVERHEAD_MARGIN) && (size > ((uint32_t)max_buf - RMF_FRAME_OVERHEAD_MARGIN)))
    {
      max_chunk_size = (uint32_t)max_buf - RMF_FRAME_OVERHEAD_MARGIN;
    }
  }
  if ((size == 0u) || (size <= max_chunk_size))
  {
    int32_t bytes_available = 0;
    return connection->transmit_data_message(connection->arg, address, false, data, (int32_t)size, &bytes_available);
  }
  uint32_t offset = 0u;
  apx_error_t retval = APX_NO_ERROR;
  while (offset < size)
  {
    uint32_t const remaining = size - offset;
    uint32_t const chunk_size = (remaining > max_chunk_size) ? max_chunk_size : remaining;
    bool const more_bit = (offset + chunk_size) < size;
    int32_t bytes_available = 0;
    retval = connection->transmit_data_message(
      connection->arg, address + offset, more_bit, data + offset, (int32_t)chunk_size, &bytes_available);
    if (retval != APX_NO_ERROR)
    {
      break;
    }
    offset += chunk_size;
  }
  return retval;
}

static apx_error_t apx_file_manager_worker_process_ringbuffer_error(adt_buf_err_t error_code)
{
  apx_error_t retval = APX_NO_ERROR;
  if (error_code == BUF_E_OVERFLOW)
  {
    retval = APX_BUFFER_FULL_ERROR;
  }
  else if (error_code == BUF_E_NOT_OK)
  {
    retval = APX_MEM_ERROR;
  }
  return retval;
}

#ifndef UNIT_TEST
static apx_error_t start_worker_thread(apx_file_manager_worker_t *self)
{
  assert(self != NULL);
  if (self->worker_thread_valid == false)
  {
    self->worker_thread_valid = true;
# ifdef _WIN32
    THREAD_CREATE(self->worker_thread, worker_main, self, self->worker_thread_id);
    if (self->worker_thread == INVALID_HANDLE_VALUE)
    {
      self->worker_thread_valid = false;
      return APX_THREAD_CREATE_ERROR;
    }
# else
    int rc = THREAD_CREATE(self->worker_thread, worker_main, self);
    if (rc != 0)
    {
      self->worker_thread_valid = false;
      return APX_THREAD_CREATE_ERROR;
    }
# endif
    return APX_NO_ERROR;
  }
  return APX_INVALID_STATE_ERROR;
}

static apx_error_t stop_worker_thread(apx_file_manager_worker_t *self)
{
  if (self->worker_thread_valid == true)
  {
# ifdef _WIN32
    DWORD result;
# endif
    apx_command_t cmd = {APX_CMD_EXIT, 0, 0, {0}, NULL};
    SPINLOCK_ENTER(self->queue_lock);
    adt_rbfh_insert(&self->queue, (const uint8_t *)&cmd);
    SPINLOCK_LEAVE(self->queue_lock);
    SEMAPHORE_POST(self->semaphore);
# ifdef _WIN32
    result = WaitForSingleObject(self->worker_thread, 5000);
    if (result == WAIT_TIMEOUT)
    {
      return APX_THREAD_JOIN_TIMEOUT_ERROR;
    }
    else if (result == WAIT_FAILED)
    {
      return APX_THREAD_JOIN_ERROR;
    }
    CloseHandle(self->worker_thread);
    self->worker_thread = INVALID_HANDLE_VALUE;
# else
    if (pthread_equal(pthread_self(), self->worker_thread) == 0)
    {
      void *status;
      int s = pthread_join(self->worker_thread, &status);
      if (s != 0)
      {
        return APX_THREAD_JOIN_ERROR;
      }
    }
    else
    {
         // pthread_join attempted from pthread_self. This is not allowed
      return APX_INTERNAL_ERROR;
    }
# endif
    self->worker_thread_valid = false;
  }
  return APX_NO_ERROR;
}

static THREAD_PROTO(worker_main, arg)
{
  if (arg != NULL)
  {
    apx_file_manager_worker_t *self;
    bool is_running = true;

    self = (apx_file_manager_worker_t *)arg;

    while (is_running)
    {
      apx_connection_interface_t const *connection = apx_file_manager_shared_connection(self->shared);

# ifdef _WIN32
      DWORD result = WaitForSingleObject(self->semaphore, INFINITE);
      if (result == WAIT_OBJECT_0)
# else

      int result = sem_wait(&self->semaphore);
      if (result == 0)
# endif
      {
        uint16_t queue_length = 0u;
        SPINLOCK_ENTER(self->queue_lock);
        queue_length = adt_rbfh_length(&self->queue);
        SPINLOCK_LEAVE(self->queue_lock);
        if (queue_length > 0)
        {
          if (connection != NULL)
          {
            assert(connection->transmit_begin != NULL);
            connection->transmit_begin(connection->arg);
          }
          while (queue_length > 0)
          {
            apx_command_t cmd;
            bool rc;
            SPINLOCK_ENTER(self->queue_lock);
            adt_rbfh_remove(&self->queue, (uint8_t *)&cmd);
            SPINLOCK_LEAVE(self->queue_lock);
            rc = process_single_command(self, &cmd);
            if (!rc)
            {
              is_running = false;
              break;
            }
            SPINLOCK_ENTER(self->queue_lock);
            queue_length = adt_rbfh_length(&self->queue);
            SPINLOCK_LEAVE(self->queue_lock);
          }
          if (connection != NULL)
          {
            assert(connection->transmit_end != NULL);
            connection->transmit_end(connection->arg);
          }
        }
      }
      else
      {
        THREAD_RETURN(APX_SEMAPHORE_ERROR);
      }
    }
  }
  THREAD_RETURN(APX_NO_ERROR);
}
#endif // UNIT_TEST