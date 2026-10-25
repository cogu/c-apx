/*****************************************************************************
 * \file      server_monitor.c
 * \author    Conny Gustafsson
 * \date      2026-10-25
 * \brief     Server monitor extension core
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/server_monitor.h"
#include "apx/extension/monitor_session.h"
#include "apx/server.h"
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
static void register_server_listener(apx_server_monitor_t *self);
static void unregister_server_listener(apx_server_monitor_t *self);
static void register_connection_listener(apx_server_monitor_t *self, apx_server_connection_t *connection);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_server_monitor_create(apx_server_monitor_t *self, apx_server_t *server)
{
  if (self != NULL)
  {
    self->server = server;
    self->server_listener_handle = NULL;
    self->num_active_log_streams = 0;
    self->num_active_port_watches = 0;
    MUTEX_INIT(self->lock);
    adt_list_create(&self->sessions, apx_monitor_session_vdelete);
    if (server != NULL)
    {
      register_server_listener(self);
    }
  }
}

void apx_server_monitor_destroy(apx_server_monitor_t *self)
{
  if (self != NULL)
  {
    unregister_server_listener(self);
    MUTEX_LOCK(self->lock);
    adt_list_destroy(&self->sessions);
    MUTEX_UNLOCK(self->lock);
    MUTEX_DESTROY(self->lock);
    self->server = NULL;
  }
}

apx_server_monitor_t *apx_server_monitor_new(apx_server_t *server)
{
  apx_server_monitor_t *self = (apx_server_monitor_t *)malloc(sizeof(apx_server_monitor_t));
  if (self != NULL)
  {
    apx_server_monitor_create(self, server);
  }
  return self;
}

void apx_server_monitor_delete(apx_server_monitor_t *self)
{
  if (self != NULL)
  {
    apx_server_monitor_destroy(self);
    free(self);
  }
}

void apx_server_monitor_vdelete(void *arg) { apx_server_monitor_delete((apx_server_monitor_t *)arg); }

int32_t apx_server_monitor_num_sessions(apx_server_monitor_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    int32_t const count = (int32_t)adt_list_length(&self->sessions);
    MUTEX_UNLOCK(self->lock);
    return count;
  }
  return 0;
}

apx_monitor_session_t *apx_server_monitor_create_session(
  apx_server_monitor_t *self, apx_server_connection_t *connection)
{
  if ((self == NULL) || (connection == NULL))
  {
    return NULL;
  }

  apx_monitor_session_t *session = apx_monitor_session_new(self, connection);
  if (session != NULL)
  {
    MUTEX_LOCK(self->lock);
    adt_list_insert(&self->sessions, session);
    MUTEX_UNLOCK(self->lock);
    apx_monitor_session_publish_files(session);
  }
  return session;
}

void apx_server_monitor_remove_session(apx_server_monitor_t *self, apx_monitor_session_t *session)
{
  if ((self != NULL) && (session != NULL))
  {
    MUTEX_LOCK(self->lock);
    adt_list_remove(&self->sessions, session);
    MUTEX_UNLOCK(self->lock);
  }
}

apx_monitor_session_t *apx_server_monitor_find_session(apx_server_monitor_t *self, apx_server_connection_t *connection)
{
  if ((self != NULL) && (connection != NULL))
  {
    MUTEX_LOCK(self->lock);
    adt_list_elem_t *iter = adt_list_iter_first(&self->sessions);
    while (iter != NULL)
    {
      apx_monitor_session_t *session = (apx_monitor_session_t *)iter->pItem;
      if (apx_monitor_session_get_connection(session) == connection)
      {
        MUTEX_UNLOCK(self->lock);
        return session;
      }
      iter = adt_list_iter_next(iter);
    }
    MUTEX_UNLOCK(self->lock);
  }
  return NULL;
}

void apx_server_monitor_on_new_connection(void *arg, apx_server_connection_t *connection)
{
  apx_server_monitor_t *self = (apx_server_monitor_t *)arg;
  if ((self != NULL) && (connection != NULL))
  {
    register_connection_listener(self, connection);
    if (apx_server_connection_get_connection_type(connection) == APX_CONNECTION_TYPE_MONITOR)
    {
      apx_server_monitor_create_session(self, connection);
    }
  }
}

void apx_server_monitor_on_connection_closed(void *arg, apx_server_connection_t *connection)
{
  apx_server_monitor_t *self = (apx_server_monitor_t *)arg;
  if ((self != NULL) && (connection != NULL))
  {
    apx_monitor_session_t *session = apx_server_monitor_find_session(self, connection);
    if (session != NULL)
    {
      apx_server_monitor_remove_session(self, session);
    }
  }
}

void apx_server_monitor_on_protocol_header_accepted(void *arg, apx_connection_base_t *connection)
{
  apx_server_monitor_t *self = (apx_server_monitor_t *)arg;
  if ((self != NULL) && (connection != NULL))
  {
    apx_server_connection_t *server_conn = (apx_server_connection_t *)connection;
    if (apx_server_connection_get_connection_type(server_conn) == APX_CONNECTION_TYPE_MONITOR)
    {
      if (apx_server_monitor_find_session(self, server_conn) == NULL)
      {
        apx_server_monitor_create_session(self, server_conn);
      }
    }
  }
}

void apx_server_monitor_on_file_published(
  void *arg, apx_connection_base_t *connection, const struct rmf_file_info_tag *file_info)
{
  (void)arg;
  (void)connection;
  (void)file_info;
}

void apx_server_monitor_on_file_revoked(
  void *arg, apx_connection_base_t *connection, const struct rmf_file_info_tag *file_info)
{
  (void)arg;
  (void)connection;
  (void)file_info;
}

void apx_server_monitor_on_file_opened(
  void *arg, apx_connection_base_t *connection, const struct rmf_file_info_tag *file_info)
{
  (void)arg;
  (void)connection;
  (void)file_info;
}

void apx_server_monitor_on_file_closed(
  void *arg, apx_connection_base_t *connection, const struct rmf_file_info_tag *file_info)
{
  (void)arg;
  (void)connection;
  (void)file_info;
}

void apx_server_monitor_on_file_written(
  void *arg, apx_connection_base_t *connection, apx_file_t *file, uint32_t offset, const uint8_t *data, apx_size_t size)
{
  (void)arg;
  (void)connection;
  (void)file;
  (void)offset;
  (void)data;
  (void)size;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_server_monitor_t *self)
{
  assert(self->server != NULL);
  apx_server_event_listener_t listener;
  memset(&listener, 0, sizeof(listener));
  listener.arg = self;
  listener.new_connection = apx_server_monitor_on_new_connection;
  listener.connection_closed = apx_server_monitor_on_connection_closed;
  self->server_listener_handle = apx_server_register_event_listener(self->server, &listener);
}

static void unregister_server_listener(apx_server_monitor_t *self)
{
  if ((self->server != NULL) && (self->server_listener_handle != NULL))
  {
    apx_server_unregister_event_listener(self->server, self->server_listener_handle);
    self->server_listener_handle = NULL;
  }
}

static void register_connection_listener(apx_server_monitor_t *self, apx_server_connection_t *connection)
{
  apx_server_connection_event_listener_t listener;
  memset(&listener, 0, sizeof(listener));
  listener.arg = self;
  listener.protocol_header_accepted = apx_server_monitor_on_protocol_header_accepted;
  listener.file_published = apx_server_monitor_on_file_published;
  listener.file_revoked = apx_server_monitor_on_file_revoked;
  listener.file_opened = apx_server_monitor_on_file_opened;
  listener.file_closed = apx_server_monitor_on_file_closed;
  listener.file_written = apx_server_monitor_on_file_written;
  apx_server_connection_register_event_listener(connection, &listener);
}