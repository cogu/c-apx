/*****************************************************************************
 * \file      monitor_session.c
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Per-client monitor session
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/monitor_session.h"
#include "apx/connection_base.h"
#include "apx/extension/diag_dispatcher.h"
#include "apx/extension/log_streamer.h"
#include "apx/extension/server_monitor.h"
#include "apx/file.h"
#include "apx/file_info.h"
#include "apx/file_manager.h"
#include "apx/server_connection.h"
#include <assert.h>
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
static apx_error_t on_file_open_notify(void *arg, apx_file_t *file);
static apx_error_t on_file_close_notify(void *arg, apx_file_t *file);
static apx_error_t on_file_write_notify(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *src, uint32_t len);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_monitor_session_create(
  apx_monitor_session_t *self, apx_server_monitor_t *monitor, apx_server_connection_t *connection)
{
  if (self != NULL)
  {
    self->monitor = monitor;
    self->connection = connection;
    self->is_server_log_open = false;
    self->is_diag_response_open = false;
    self->is_diag_notify_open = false;
    MUTEX_INIT(self->lock);
    self->log_streamer = apx_log_streamer_new(self);
    self->diag_dispatcher = apx_diag_dispatcher_new(self);
  }
}

void apx_monitor_session_destroy(apx_monitor_session_t *self)
{
  if (self != NULL)
  {
    if (self->log_streamer != NULL)
    {
      apx_log_streamer_delete(self->log_streamer);
      self->log_streamer = NULL;
    }
    if (self->diag_dispatcher != NULL)
    {
      apx_diag_dispatcher_delete(self->diag_dispatcher);
      self->diag_dispatcher = NULL;
    }
    MUTEX_DESTROY(self->lock);
    self->monitor = NULL;
    self->connection = NULL;
  }
}

apx_monitor_session_t *apx_monitor_session_new(apx_server_monitor_t *monitor, apx_server_connection_t *connection)
{
  apx_monitor_session_t *self = (apx_monitor_session_t *)malloc(sizeof(apx_monitor_session_t));
  if (self != NULL)
  {
    apx_monitor_session_create(self, monitor, connection);
  }
  return self;
}

void apx_monitor_session_delete(apx_monitor_session_t *self)
{
  if (self != NULL)
  {
    apx_monitor_session_destroy(self);
    free(self);
  }
}

void apx_monitor_session_vdelete(void *arg) { apx_monitor_session_delete((apx_monitor_session_t *)arg); }

apx_error_t apx_monitor_session_publish_files(apx_monitor_session_t *self)
{
  if ((self == NULL) || (self->connection == NULL))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  apx_file_manager_t *file_manager = apx_connection_base_get_file_manager(&self->connection->base);
  if (file_manager == NULL)
  {
    return APX_NULL_PTR_ERROR;
  }

  apx_file_notification_handler_t handler;
  memset(&handler, 0, sizeof(handler));
  handler.arg = self;
  handler.open_notify = on_file_open_notify;
  handler.close_notify = on_file_close_notify;
  handler.write_notify = on_file_write_notify;

  char const *stream_files[] = {
    APX_SERVER_LOG_FILE_NAME,
    APX_DIAG_RESPONSE_FILE_NAME,
    APX_DIAG_NOTIFY_FILE_NAME,
  };
  size_t const num_files = sizeof(stream_files) / sizeof(stream_files[0]);

  for (size_t i = 0; i < num_files; ++i)
  {
    rmf_file_info_t file_info;
    apx_error_t const err = rmf_file_info_create(&file_info, RMF_INVALID_ADDRESS, APX_MONITOR_DEFAULT_STREAM_FILE_SIZE,
      stream_files[i], RMF_FILE_TYPE_STREAM, RMF_DIGEST_TYPE_NONE, NULL);
    if (err != APX_NO_ERROR)
    {
      return err;
    }
    apx_file_t *local_file = apx_file_manager_create_local_file(file_manager, &file_info);
    if (local_file == NULL)
    {
      rmf_file_info_destroy(&file_info);
      return APX_MEM_ERROR;
    }
    apx_file_set_notification_handler(local_file, &handler);
    apx_error_t const pub_err = apx_file_manager_publish_local_file(file_manager, &local_file->file_info);
    rmf_file_info_destroy(&file_info);
    if (pub_err != APX_NO_ERROR)
    {
      return pub_err;
    }
  }

  return APX_NO_ERROR;
}

void apx_monitor_session_on_file_opened(apx_monitor_session_t *self, apx_file_t *file)
{
  if ((self != NULL) && (file != NULL))
  {
    const char *name = rmf_file_info_name(&file->file_info);
    if (strcmp(name, APX_SERVER_LOG_FILE_NAME) == 0)
    {
      MUTEX_LOCK(self->lock);
      self->is_server_log_open = true;
      MUTEX_UNLOCK(self->lock);
      if (self->log_streamer != NULL)
      {
        apx_log_streamer_start(self->log_streamer);
      }
    }
    else if (strcmp(name, APX_DIAG_RESPONSE_FILE_NAME) == 0)
    {
      MUTEX_LOCK(self->lock);
      self->is_diag_response_open = true;
      MUTEX_UNLOCK(self->lock);
    }
    else if (strcmp(name, APX_DIAG_NOTIFY_FILE_NAME) == 0)
    {
      MUTEX_LOCK(self->lock);
      self->is_diag_notify_open = true;
      MUTEX_UNLOCK(self->lock);
    }
  }
}

void apx_monitor_session_on_file_closed(apx_monitor_session_t *self, apx_file_t *file)
{
  if ((self != NULL) && (file != NULL))
  {
    const char *name = rmf_file_info_name(&file->file_info);
    if (strcmp(name, APX_SERVER_LOG_FILE_NAME) == 0)
    {
      MUTEX_LOCK(self->lock);
      self->is_server_log_open = false;
      MUTEX_UNLOCK(self->lock);
      if (self->log_streamer != NULL)
      {
        apx_log_streamer_stop(self->log_streamer);
      }
    }
    else if (strcmp(name, APX_DIAG_RESPONSE_FILE_NAME) == 0)
    {
      MUTEX_LOCK(self->lock);
      self->is_diag_response_open = false;
      MUTEX_UNLOCK(self->lock);
    }
    else if (strcmp(name, APX_DIAG_NOTIFY_FILE_NAME) == 0)
    {
      MUTEX_LOCK(self->lock);
      self->is_diag_notify_open = false;
      MUTEX_UNLOCK(self->lock);
    }
  }
}

void apx_monitor_session_on_file_written(
  apx_monitor_session_t *self, apx_file_t *file, uint32_t offset, const uint8_t *data, apx_size_t size)
{
  if ((self != NULL) && (file != NULL))
  {
    const char *name = rmf_file_info_name(&file->file_info);
    if (strcmp(name, APX_DIAG_REQUEST_FILE_NAME) == 0)
    {
      if (self->diag_dispatcher != NULL)
      {
        apx_diag_dispatcher_on_request_written(self->diag_dispatcher, offset, data, size);
      }
    }
  }
}

apx_server_connection_t *apx_monitor_session_get_connection(apx_monitor_session_t *self)
{
  if (self != NULL)
  {
    return self->connection;
  }
  return NULL;
}

apx_log_streamer_t *apx_monitor_session_get_log_streamer(apx_monitor_session_t *self)
{
  if (self != NULL)
  {
    return self->log_streamer;
  }
  return NULL;
}

apx_diag_dispatcher_t *apx_monitor_session_get_diag_dispatcher(apx_monitor_session_t *self)
{
  if (self != NULL)
  {
    return self->diag_dispatcher;
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t on_file_open_notify(void *arg, apx_file_t *file)
{
  apx_monitor_session_t *self = (apx_monitor_session_t *)arg;
  apx_monitor_session_on_file_opened(self, file);
  return APX_NO_ERROR;
}

static apx_error_t on_file_close_notify(void *arg, apx_file_t *file)
{
  apx_monitor_session_t *self = (apx_monitor_session_t *)arg;
  apx_monitor_session_on_file_closed(self, file);
  return APX_NO_ERROR;
}

static apx_error_t on_file_write_notify(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *src, uint32_t len)
{
  apx_monitor_session_t *self = (apx_monitor_session_t *)arg;
  apx_monitor_session_on_file_written(self, file, offset, src, len);
  return APX_NO_ERROR;
}
