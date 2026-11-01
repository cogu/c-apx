/*****************************************************************************
 * \file      apx_log_client.c
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     APX log streaming client implementation
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_log_client.h"
#include "apx/file.h"
#include "apx/file_manager.h"
#include "apx/remotefile.h"
#include "apx/util.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_LOG_SERVER_LOG_FILE_NAME "server.log"
#define APX_LOG_STREAM_BLOCK_SIZE (64u * 1024u)
#define APX_LOG_STREAM_WINDOW_BLOCKS 64u

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void on_client_greeting_accepted(void *arg, apx_client_connection_t *connection);
static void on_client_disconnected(void *arg, apx_client_connection_t *connection);
static apx_error_t on_client_file_published(void *arg, apx_file_t *file);
static apx_error_t on_log_file_written(
  void *arg, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t size);
static apx_error_t on_log_file_closed(void *arg, apx_file_t *file);

static struct timespec compute_deadline(uint32_t timeout_ms)
{
  struct timespec ts;
  uint64_t const total_ns = apx_get_unix_time_ns() + ((uint64_t)timeout_ms * 1000000ULL);
  ts.tv_sec = (time_t)(total_ns / 1000000000ULL);
  ts.tv_nsec = (long)(total_ns % 1000000000ULL);
  return ts;
}

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_log_client_create(apx_log_client_t *self)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  memset(self, 0, sizeof(apx_log_client_t));
  MUTEX_INIT(self->lock);
  pthread_cond_init(&self->cond, NULL);

  self->server_log_address = RMF_INVALID_ADDRESS;

  apx_error_t err = apx_client_socket_connection_create(&self->connection, NULL, APX_CONNECTION_TYPE_MONITOR);
  if (err != APX_NO_ERROR)
  {
    MUTEX_DESTROY(self->lock);
    pthread_cond_destroy(&self->cond);
    return err;
  }

  apx_client_socket_connection_set_greeting_accepted_notification(&self->connection, on_client_greeting_accepted, self);
  apx_client_socket_connection_set_disconnected_notification(&self->connection, on_client_disconnected, self);
  apx_client_socket_connection_set_file_published_notification(&self->connection, on_client_file_published, self);

  return APX_NO_ERROR;
}

void apx_log_client_destroy(apx_log_client_t *self)
{
  if (self != NULL)
  {
    apx_log_client_disconnect(self);
    apx_client_socket_connection_destroy(&self->connection);

    MUTEX_DESTROY(self->lock);
    pthread_cond_destroy(&self->cond);
  }
}

apx_log_client_t *apx_log_client_new(void)
{
  apx_log_client_t *self = (apx_log_client_t *)malloc(sizeof(apx_log_client_t));
  if (self != NULL)
  {
    apx_error_t err = apx_log_client_create(self);
    if (err != APX_NO_ERROR)
    {
      free(self);
      return NULL;
    }
  }
  return self;
}

void apx_log_client_delete(apx_log_client_t *self)
{
  if (self != NULL)
  {
    apx_log_client_destroy(self);
    free(self);
  }
}

apx_error_t apx_log_client_connect_unix(apx_log_client_t *self, const char *socket_path)
{
  if (self == NULL || socket_path == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  return apx_client_socket_connection_connect_unix(&self->connection, socket_path);
}

apx_error_t apx_log_client_connect_tcp(apx_log_client_t *self, const char *address, uint16_t port)
{
  if (self == NULL || address == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  return apx_client_socket_connection_connect_tcp(&self->connection, address, port);
}

#if defined(MSOCKET_ENABLE_TLS)
apx_error_t apx_log_client_connect_tls(
  apx_log_client_t *self, const char *address, uint16_t port, const msocket_tls_config_t *tls_config)
{
  if (self == NULL || address == NULL || tls_config == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  return apx_client_socket_connection_connect_tls(&self->connection, address, port, tls_config);
}
#endif

void apx_log_client_disconnect(apx_log_client_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->is_connected = false;
    self->is_greeting_accepted = false;
    self->is_server_log_open = false;
    MUTEX_UNLOCK(self->lock);
    apx_connection_base_stop(&self->connection.base.base);
    apx_connection_base_close(&self->connection.base.base);
  }
}

apx_error_t apx_log_client_wait_ready(apx_log_client_t *self, uint32_t timeout_ms)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  struct timespec deadline = compute_deadline(timeout_ms);
  MUTEX_LOCK(self->lock);
  while (!self->is_server_log_open)
  {
    if (self->is_greeting_accepted && !self->is_connected)
    {
      MUTEX_UNLOCK(self->lock);
      return APX_NOT_CONNECTED_ERROR;
    }
    int rc = pthread_cond_timedwait(&self->cond, &self->lock, &deadline);
    if (rc == ETIMEDOUT)
    {
      MUTEX_UNLOCK(self->lock);
      return APX_TIMEOUT_ERROR;
    }
  }
  MUTEX_UNLOCK(self->lock);
  return APX_NO_ERROR;
}

bool apx_log_client_is_connected(const apx_log_client_t *self)
{
  if (self != NULL)
  {
    return self->is_connected;
  }
  return false;
}

void apx_log_client_set_event_callback(apx_log_client_t *self, apx_log_client_event_cb_t *cb, void *arg)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->event_cb = cb;
    self->event_arg = arg;
    MUTEX_UNLOCK(self->lock);
  }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE NOTIFICATION HANDLERS
//////////////////////////////////////////////////////////////////////////////

static void on_client_greeting_accepted(void *arg, apx_client_connection_t *connection)
{
  (void)connection;
  apx_log_client_t *self = (apx_log_client_t *)arg;
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->is_connected = true;
    self->is_greeting_accepted = true;
    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }
}

static void on_client_disconnected(void *arg, apx_client_connection_t *connection)
{
  (void)connection;
  apx_log_client_t *self = (apx_log_client_t *)arg;
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->is_connected = false;
    self->is_greeting_accepted = false;
    self->is_server_log_open = false;
    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }
}

static apx_error_t on_client_file_published(void *arg, apx_file_t *file)
{
  apx_log_client_t *self = (apx_log_client_t *)arg;
  if (self == NULL || file == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  const char *fname = rmf_file_info_name(&file->file_info);
  if (fname == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  apx_file_manager_t *fm = apx_client_connection_get_file_manager(&self->connection.base);
  if (fm == NULL)
  {
    return APX_NULL_PTR_ERROR;
  }

  uint32_t const clean_addr = apx_file_get_address_without_flags(file);

  if (strcmp(fname, APX_LOG_SERVER_LOG_FILE_NAME) == 0)
  {
    MUTEX_LOCK(self->lock);
    self->server_log_address = clean_addr;

    apx_file_notification_handler_t log_handler;
    memset(&log_handler, 0, sizeof(log_handler));
    log_handler.arg = self;
    log_handler.write_notify = (apx_file_write_notify_func *)on_log_file_written;
    log_handler.close_notify = (apx_file_open_close_notify_func *)on_log_file_closed;
    apx_file_set_notification_handler(file, &log_handler);

    apx_file_manager_open_stream(fm, clean_addr, APX_LOG_STREAM_BLOCK_SIZE, APX_LOG_STREAM_WINDOW_BLOCKS);
    self->is_server_log_open = true;

    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }

  return APX_NO_ERROR;
}

static apx_error_t on_log_file_written(void *arg, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t size)
{
  (void)file;
  (void)offset;
  apx_log_client_t *self = (apx_log_client_t *)arg;
  if (self == NULL || data == NULL || size == 0u)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  MUTEX_LOCK(self->lock);
  apx_log_client_event_cb_t *cb = self->event_cb;
  void *cb_arg = self->event_arg;
  MUTEX_UNLOCK(self->lock);

  if (cb != NULL)
  {
    cb(cb_arg, data, size);
  }

  return APX_NO_ERROR;
}

static apx_error_t on_log_file_closed(void *arg, apx_file_t *file)
{
  (void)file;
  apx_log_client_t *self = (apx_log_client_t *)arg;
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    self->is_server_log_open = false;
    pthread_cond_broadcast(&self->cond);
    MUTEX_UNLOCK(self->lock);
  }
  return APX_NO_ERROR;
}
