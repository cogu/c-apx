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
#include "adt_bytearray.h"
#include "apx/connection_base.h"
#include "apx/extension/monitor_session.h"
#include "apx/file_manager.h"
#include "apx/node_manager.h"
#include "apx/numheader.h"
#include "apx/remotefile.h"
#include "apx/server.h"
#include "apx/server_connection.h"
#include "bstr.h"
#include <assert.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define SEND_BUFFER_GROW_SIZE 4096u

#ifdef UNIT_TEST
# define SOCKET_TYPE testsocket_t
# define SOCKET_OBJECT_DELETE testsocket_delete
# define SOCKET_START_IO(x)
# define SOCKET_SET_HANDLER testsocket_set_server_handler
# define SOCKET_SEND testsocket_server_send
# define SOCKET_OBJECT_CLOSE(x)
#else
# define SOCKET_OBJECT_DELETE msocket_delete
# define SOCKET_TYPE msocket_t
# define SOCKET_START_IO(x) msocket_start_io(x)
# define SOCKET_SET_HANDLER msocket_set_handler
# define SOCKET_SEND msocket_send
# define SOCKET_OBJECT_CLOSE(x) msocket_close(x)
#endif

typedef struct apx_monitor_server_connection_tag
{
  apx_server_connection_t base;
  apx_node_manager_t node_manager;
  adt_bytearray_t send_buffer;
  apx_size_t default_buffer_size;
  apx_size_t pending_bytes;
  SOCKET_TYPE *socket_object;
  MUTEX_T lock;
} apx_monitor_server_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_server_monitor_t *self);
static void unregister_server_listener(apx_server_monitor_t *self);
static void register_connection_listener(apx_server_monitor_t *self, apx_server_connection_t *connection);
static void apx_server_monitor_on_stream_accept(void *arg, struct msocket_server_tag *srv, void *sock);

// Monitor server connection methods
static apx_error_t apx_monitor_server_connection_create(
  apx_monitor_server_connection_t *self, SOCKET_TYPE *socket_object);
static void apx_monitor_server_connection_destroy(apx_monitor_server_connection_t *self);
static apx_monitor_server_connection_t *apx_monitor_server_connection_new(SOCKET_TYPE *socket_object);
static void apx_monitor_server_connection_delete(apx_monitor_server_connection_t *self);
static void apx_monitor_server_connection_vdestroy(void *arg);
static void apx_monitor_server_connection_vstart(void *arg);
static void apx_monitor_server_connection_vclose(void *arg);

static void register_socket_handler(apx_monitor_server_connection_t *self, SOCKET_TYPE *socket_object);
static void create_connection_interface_vtable(
  apx_monitor_server_connection_t *self, apx_connection_interface_t *interface);
static void socket_disconnected_notification(void *arg, void *socket);
static msocket_error_t socket_data_notification(void *arg, void *socket, const uint8_t *data, const uint32_t num_bytes,
  uint32_t *consumed_bytes, uint32_t *msg_size_hint);

static int32_t connection_transmit_max_bytes_available(apx_monitor_server_connection_t *self);
static int32_t connection_transmit_current_bytes_available(apx_monitor_server_connection_t *self);
static void connection_transmit_begin(apx_monitor_server_connection_t *self);
static void connection_transmit_end(apx_monitor_server_connection_t *self);
static apx_error_t connection_transmit_data_message(apx_monitor_server_connection_t *self, uint32_t write_address,
  bool more_bit, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available);
static apx_error_t connection_transmit_direct_message(
  apx_monitor_server_connection_t *self, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available);
static void connection_send_packet(apx_monitor_server_connection_t *self);

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
    self->is_enabled = true;
    self->tcp_port = 0u;
    self->bind_address = NULL;
    self->socket_file = NULL;
    self->is_tcp_started = false;
    self->is_unix_started = false;
    self->tls_enabled = false;
    msocket_tls_config_create(&self->tls_config);
    msocket_tls_config_set_require_client_cert(&self->tls_config, true);
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
    apx_server_monitor_stop(self);
    unregister_server_listener(self);
    MUTEX_LOCK(self->lock);
    adt_list_destroy(&self->sessions);
    MUTEX_UNLOCK(self->lock);
    MUTEX_DESTROY(self->lock);
    if (self->socket_file != NULL)
    {
      free(self->socket_file);
      self->socket_file = NULL;
    }
    if (self->bind_address != NULL)
    {
      free(self->bind_address);
      self->bind_address = NULL;
    }
    msocket_tls_config_destroy(&self->tls_config);
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

apx_error_t apx_server_monitor_configure(apx_server_monitor_t *self, dtl_hv_t *config)
{
  if ((self == NULL) || (config == NULL))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }

  bool conversion_ok = false;
  dtl_sv_t *sv_enabled = (dtl_sv_t *)dtl_hv_get_cstr(config, "enabled");
  if (sv_enabled != NULL)
  {
    bool enabled = dtl_sv_to_bool(sv_enabled, &conversion_ok);
    if (conversion_ok)
    {
      self->is_enabled = enabled;
    }
  }

  dtl_sv_t *sv_socket_file = (dtl_sv_t *)dtl_hv_get_cstr(config, "socket-file");
  if (sv_socket_file == NULL)
  {
    sv_socket_file = (dtl_sv_t *)dtl_hv_get_cstr(config, "unix-file");
  }
  if (sv_socket_file != NULL)
  {
    const char *path = dtl_sv_to_cstr(sv_socket_file, &conversion_ok);
    if (conversion_ok && (path != NULL) && (*path != '\0'))
    {
      if (self->socket_file != NULL)
      {
        free(self->socket_file);
      }
      self->socket_file = STRDUP(path);
    }
  }

  dtl_sv_t *sv_tcp_port = (dtl_sv_t *)dtl_hv_get_cstr(config, "tcp-port");
  if (sv_tcp_port == NULL)
  {
    sv_tcp_port = (dtl_sv_t *)dtl_hv_get_cstr(config, "port");
  }
  if (sv_tcp_port != NULL)
  {
    uint32_t port = dtl_sv_to_u32(sv_tcp_port, &conversion_ok);
    if (conversion_ok && (port <= 65535u))
    {
      self->tcp_port = (uint16_t)port;
    }
  }

  dtl_sv_t *sv_bind_address = (dtl_sv_t *)dtl_hv_get_cstr(config, "bind-address");
  if (sv_bind_address != NULL)
  {
    const char *addr = dtl_sv_to_cstr(sv_bind_address, &conversion_ok);
    if (conversion_ok && (addr != NULL) && (*addr != '\0'))
    {
      if (self->bind_address != NULL)
      {
        free(self->bind_address);
      }
      self->bind_address = STRDUP(addr);
    }
  }

  dtl_dv_t *tls_dv = dtl_hv_get_cstr(config, "tls");
  if ((tls_dv != NULL) && (dtl_dv_type(tls_dv) == DTL_DV_HASH))
  {
    dtl_hv_t *tls_hv = (dtl_hv_t *)tls_dv;
    dtl_sv_t *sv_server_cert = (dtl_sv_t *)dtl_hv_get_cstr(tls_hv, "server-cert");
    dtl_sv_t *sv_server_key = (dtl_sv_t *)dtl_hv_get_cstr(tls_hv, "server-key");
    dtl_sv_t *sv_ca_cert = (dtl_sv_t *)dtl_hv_get_cstr(tls_hv, "ca-cert");
    dtl_sv_t *sv_require_client_cert = (dtl_sv_t *)dtl_hv_get_cstr(tls_hv, "require-client-cert");

    if ((sv_server_cert != NULL) && (sv_server_key != NULL))
    {
      const char *server_cert = dtl_sv_to_cstr(sv_server_cert, &conversion_ok);
      const char *server_key = dtl_sv_to_cstr(sv_server_key, &conversion_ok);
      if ((server_cert != NULL) && (server_key != NULL))
      {
        msocket_tls_config_set_server_cert(&self->tls_config, server_cert, server_key);
        self->tls_enabled = true;
      }
    }
    if (sv_ca_cert != NULL)
    {
      const char *ca_cert = dtl_sv_to_cstr(sv_ca_cert, &conversion_ok);
      if (ca_cert != NULL)
      {
        msocket_tls_config_set_ca_cert(&self->tls_config, ca_cert);
      }
    }
    if (sv_require_client_cert != NULL)
    {
      bool req = dtl_sv_to_bool(sv_require_client_cert, &conversion_ok);
      if (conversion_ok)
      {
        msocket_tls_config_set_require_client_cert(&self->tls_config, req);
      }
    }
  }

  return APX_NO_ERROR;
}

apx_error_t apx_server_monitor_start(apx_server_monitor_t *self)
{
  if (self == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!self->is_enabled)
  {
    return APX_NO_ERROR;
  }

#ifndef _WIN32
  if ((self->socket_file != NULL) && !self->is_unix_started)
  {
    msocket_handler_t unix_handler;
    memset(&unix_handler, 0, sizeof(unix_handler));
    unix_handler.stream_accept = apx_server_monitor_on_stream_accept;

    msocket_server_create(&self->unix_server, MSOCKET_ADDR_UNIX, NULL);
    msocket_server_disable_cleanup(&self->unix_server);
    msocket_server_set_handler(&self->unix_server, &unix_handler, self);
    msocket_server_unix_start(&self->unix_server, self->socket_file);
    self->is_unix_started = true;
    if (self->server != NULL)
    {
      apx_server_monitor_log(self->server, APX_LOG_LEVEL_INFO, "Listening on UNIX socket %s", self->socket_file);
    }
  }
#endif

  if ((self->tcp_port > 0u) && !self->is_tcp_started)
  {
    msocket_handler_t tcp_handler;
    memset(&tcp_handler, 0, sizeof(tcp_handler));
    tcp_handler.stream_accept = apx_server_monitor_on_stream_accept;

    msocket_server_create(&self->tcp_server, MSOCKET_ADDR_INET, NULL);
    msocket_server_disable_cleanup(&self->tcp_server);
    msocket_server_set_handler(&self->tcp_server, &tcp_handler, self);

    if (self->tls_enabled)
    {
      msocket_error_t rc = msocket_server_start_tls(&self->tcp_server, self->tcp_port, &self->tls_config);
      if (rc != MSOCKET_NO_ERROR)
      {
        msocket_server_destroy(&self->tcp_server);
        return APX_GENERIC_ERROR;
      }
      self->is_tcp_started = true;
      if (self->server != NULL)
      {
        apx_server_monitor_log(
          self->server, APX_LOG_LEVEL_INFO, "Listening on TLS TCP port %u", (unsigned int)self->tcp_port);
      }
    }
    else
    {
      msocket_server_start(&self->tcp_server, self->bind_address, 0, self->tcp_port);
      self->is_tcp_started = true;
      if (self->server != NULL)
      {
        apx_server_monitor_log(
          self->server, APX_LOG_LEVEL_INFO, "Listening on TCP port %u", (unsigned int)self->tcp_port);
      }
    }
  }

  return APX_NO_ERROR;
}

void apx_server_monitor_stop(apx_server_monitor_t *self)
{
  if (self != NULL)
  {
#ifndef _WIN32
    if (self->is_unix_started)
    {
      msocket_server_destroy(&self->unix_server);
      self->is_unix_started = false;
    }
#endif
    if (self->is_tcp_started)
    {
      msocket_server_destroy(&self->tcp_server);
      self->is_tcp_started = false;
    }
  }
}

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
      if (apx_server_monitor_find_session(self, connection) == NULL)
      {
        apx_server_monitor_create_session(self, connection);
      }
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

#ifdef UNIT_TEST
void apx_server_monitor_accept_testsocket(apx_server_monitor_t *self, testsocket_t *sock)
{
  if ((self != NULL) && (sock != NULL))
  {
    apx_server_monitor_on_stream_accept(self, NULL, (void *)sock);
  }
}
#endif

void apx_server_monitor_log(apx_server_t *server, apx_log_level_t level, const char *format, ...)
{
  if (server != NULL)
  {
    va_list args;
    va_start(args, format);
    apx_server_log_vwrite(server, level, APX_MONITOR_SERVER_LABEL, format, args);
    va_end(args);
  }
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

static void apx_server_monitor_on_stream_accept(void *arg, struct msocket_server_tag *srv, void *sock)
{
  apx_server_monitor_t *self = (apx_server_monitor_t *)arg;
  (void)srv;
  if ((self != NULL) && (sock != NULL))
  {
    if (self->server != NULL)
    {
      apx_server_monitor_log(self->server, APX_LOG_LEVEL_DEBUG, "New incoming monitor connection");
    }
    apx_monitor_server_connection_t *conn = apx_monitor_server_connection_new((SOCKET_TYPE *)sock);
    if (conn != NULL)
    {
      apx_server_connection_set_connection_type(&conn->base, APX_CONNECTION_TYPE_MONITOR);
      apx_server_accept_connection(self->server, &conn->base);
    }
    else
    {
      SOCKET_OBJECT_DELETE((SOCKET_TYPE *)sock);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// MONITOR SERVER CONNECTION
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_monitor_server_connection_create(
  apx_monitor_server_connection_t *self, SOCKET_TYPE *socket_object)
{
  if ((self != NULL) && (socket_object != NULL))
  {
    apx_connection_base_vtable_t base_vtable;
    apx_connection_interface_t conn_interface;
    MUTEX_INIT(self->lock);
    self->default_buffer_size = SEND_BUFFER_GROW_SIZE;
    self->pending_bytes = 0u;
    self->socket_object = socket_object;

    apx_connection_base_vtable_create(&base_vtable, apx_monitor_server_connection_vdestroy,
      apx_monitor_server_connection_vstart, apx_monitor_server_connection_vclose);
    create_connection_interface_vtable(self, &conn_interface);

    apx_error_t retval = apx_server_connection_create(&self->base, &base_vtable, &conn_interface);
    if (retval == APX_NO_ERROR)
    {
      adt_bytearray_create(&self->send_buffer);
      register_socket_handler(self, socket_object);
      apx_node_manager_create(&self->node_manager, APX_SERVER_MODE);
      apx_server_connection_attach_node_manager(&self->base, &self->node_manager);
    }
    return retval;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

static void apx_monitor_server_connection_destroy(apx_monitor_server_connection_t *self)
{
  if (self != NULL)
  {
    apx_server_connection_destroy(&self->base);
    adt_bytearray_destroy(&self->send_buffer);
    apx_node_manager_destroy(&self->node_manager);
    SOCKET_OBJECT_DELETE(self->socket_object);
    MUTEX_DESTROY(self->lock);
  }
}

static apx_monitor_server_connection_t *apx_monitor_server_connection_new(SOCKET_TYPE *socket_object)
{
  apx_monitor_server_connection_t *self =
    (apx_monitor_server_connection_t *)malloc(sizeof(apx_monitor_server_connection_t));
  if (self != NULL)
  {
    apx_error_t err = apx_monitor_server_connection_create(self, socket_object);
    if (err != APX_NO_ERROR)
    {
      free(self);
      return NULL;
    }
  }
  return self;
}

static void apx_monitor_server_connection_delete(apx_monitor_server_connection_t *self)
{
  if (self != NULL)
  {
    apx_monitor_server_connection_destroy(self);
    free(self);
  }
}

static void apx_monitor_server_connection_vdestroy(void *arg)
{
  apx_monitor_server_connection_destroy((apx_monitor_server_connection_t *)arg);
}

static void apx_monitor_server_connection_vstart(void *arg)
{
  apx_monitor_server_connection_t *self = (apx_monitor_server_connection_t *)arg;
  if (self != NULL)
  {
    assert(self->socket_object != NULL);
    apx_server_connection_start(&self->base);
    SOCKET_START_IO(self->socket_object);
  }
}

static void apx_monitor_server_connection_vclose(void *arg)
{
  apx_monitor_server_connection_t *self = (apx_monitor_server_connection_t *)arg;
  if (self != NULL)
  {
    SOCKET_OBJECT_CLOSE(self->socket_object);
  }
}

static void register_socket_handler(apx_monitor_server_connection_t *self, SOCKET_TYPE *socket_object)
{
  msocket_handler_t handler_table;
  memset(&handler_table, 0, sizeof(handler_table));
  handler_table.stream_data = socket_data_notification;
  handler_table.stream_disconnected = socket_disconnected_notification;
  SOCKET_SET_HANDLER(socket_object, &handler_table, self);
}

static void create_connection_interface_vtable(
  apx_monitor_server_connection_t *self, apx_connection_interface_t *interface)
{
  memset(interface, 0, sizeof(apx_connection_interface_t));
  interface->arg = (void *)self;
  interface->transmit_max_buffer_size = (int32_t(*)(void *))connection_transmit_max_bytes_available;
  interface->transmit_current_bytes_avaiable = (int32_t(*)(void *))connection_transmit_current_bytes_available;
  interface->transmit_begin = (void (*)(void *))connection_transmit_begin;
  interface->transmit_end = (void (*)(void *))connection_transmit_end;
  interface->transmit_data_message =
    (apx_error_t(*)(void *, uint32_t, bool, const uint8_t *, int32_t, int32_t *))connection_transmit_data_message;
  interface->transmit_direct_message =
    (apx_error_t(*)(void *, const uint8_t *, int32_t, int32_t *))connection_transmit_direct_message;
}

static void socket_disconnected_notification(void *arg, void *socket)
{
  (void)socket;
  apx_monitor_server_connection_t *self = (apx_monitor_server_connection_t *)arg;
  if (self != NULL)
  {
    if (self->base.parent != NULL)
    {
      apx_server_monitor_log(self->base.parent, APX_LOG_LEVEL_INFO, "[%u] Monitor client disconnected",
        apx_server_connection_get_connection_id(&self->base));
      apx_server_detach_connection(self->base.parent, &self->base);
    }
  }
}

static msocket_error_t socket_data_notification(void *arg, void *socket, const uint8_t *data, const uint32_t num_bytes,
  uint32_t *consumed_bytes, uint32_t *msg_size_hint)
{
  (void)socket;
  apx_monitor_server_connection_t *self = (apx_monitor_server_connection_t *)arg;
  if (self == NULL)
  {
    return MSOCKET_INVALID_ARGUMENT_ERROR;
  }
  int retval = apx_server_connection_on_data_received(&self->base, data, num_bytes, consumed_bytes, msg_size_hint);
  if ((retval == 0) && (self->base.parent != NULL) && (consumed_bytes != NULL) && (*consumed_bytes > 0u))
  {
    apx_server_monitor_log(self->base.parent, APX_LOG_LEVEL_DEBUG, "[%u] Received %d bytes",
      apx_server_connection_get_connection_id(&self->base), (int)*consumed_bytes);
  }
  return (retval == 0) ? MSOCKET_NO_ERROR : MSOCKET_GENERIC_ERROR;
}

static int32_t connection_transmit_max_bytes_available(apx_monitor_server_connection_t *self)
{
  if (self != NULL)
  {
    return (int32_t)self->default_buffer_size;
  }
  return -1;
}

static int32_t connection_transmit_current_bytes_available(apx_monitor_server_connection_t *self)
{
  if (self != NULL)
  {
    return (int32_t)(adt_bytearray_length(&self->send_buffer) - self->pending_bytes);
  }
  return -1;
}

static void connection_transmit_begin(apx_monitor_server_connection_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->lock);
    if (adt_bytearray_length(&self->send_buffer) < self->default_buffer_size)
    {
      adt_bytearray_resize(&self->send_buffer, self->default_buffer_size);
    }
    self->pending_bytes = 0u;
    assert(adt_bytearray_length(&self->send_buffer) >= self->default_buffer_size);
  }
}

static void connection_transmit_end(apx_monitor_server_connection_t *self)
{
  if (self != NULL)
  {
    if (self->pending_bytes > 0u)
    {
      connection_send_packet(self);
    }
    MUTEX_UNLOCK(self->lock);
  }
}

static apx_error_t connection_transmit_data_message(apx_monitor_server_connection_t *self, uint32_t write_address,
  bool more_bit, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available)
{
  if (self != NULL)
  {
    uint8_t header[NUMHEADER32_LONG_SIZE + RMF_HIGH_ADDR_SIZE];
    apx_size_t const address_size = rmf_needed_encoding_size(write_address);
    apx_size_t const payload_size = address_size + msg_size;
    if (payload_size > self->default_buffer_size)
    {
      return APX_MSG_TOO_LARGE_ERROR;
    }
    apx_size_t const header1_size = numheader_encode32(header, sizeof(header), payload_size);
    assert(header1_size > 0);
    apx_size_t const header2_size =
      rmf_address_encode(header + header1_size, sizeof(header) - header1_size, write_address, more_bit);
    assert(header2_size == address_size);
    apx_size_t const bytes_to_send = header1_size + header2_size + msg_size;
    apx_size_t const buffer_available = ((apx_size_t)adt_bytearray_length(&self->send_buffer)) - self->pending_bytes;
    if (bytes_to_send > buffer_available)
    {
      connection_send_packet(self);
      assert(self->pending_bytes == 0u);
    }
    memcpy(adt_bytearray_data(&self->send_buffer) + self->pending_bytes, header, header1_size + header2_size);
    self->pending_bytes += (header1_size + header2_size);
    memcpy(adt_bytearray_data(&self->send_buffer) + self->pending_bytes, msg_data, msg_size);
    self->pending_bytes += msg_size;
    *bytes_available = (int32_t)(((apx_size_t)adt_bytearray_length(&self->send_buffer)) - self->pending_bytes);
    return APX_NO_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t connection_transmit_direct_message(
  apx_monitor_server_connection_t *self, const uint8_t *msg_data, int32_t msg_size, int32_t *bytes_available)
{
  if (self != NULL)
  {
    uint8_t header[NUMHEADER32_LONG_SIZE];
    if (msg_size > ((int32_t)self->default_buffer_size))
    {
      return APX_MSG_TOO_LARGE_ERROR;
    }
    apx_size_t const header_size = numheader_encode32(header, sizeof(header), msg_size);
    apx_size_t const bytes_to_send = header_size + msg_size;
    apx_size_t const buffer_available = ((apx_size_t)adt_bytearray_length(&self->send_buffer)) - self->pending_bytes;
    if (bytes_to_send > buffer_available)
    {
      connection_send_packet(self);
      assert(self->pending_bytes == 0u);
    }
    memcpy(adt_bytearray_data(&self->send_buffer) + self->pending_bytes, header, header_size);
    self->pending_bytes += header_size;
    memcpy(adt_bytearray_data(&self->send_buffer) + self->pending_bytes, msg_data, msg_size);
    self->pending_bytes += msg_size;
    *bytes_available = (int32_t)(((apx_size_t)adt_bytearray_length(&self->send_buffer)) - self->pending_bytes);
    return APX_NO_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

static void connection_send_packet(apx_monitor_server_connection_t *self)
{
  if ((self->socket_object != NULL) && (self->pending_bytes > 0u))
  {
    uint8_t const *data = adt_bytearray_const_data(&self->send_buffer);
    if (self->base.parent != NULL)
    {
      apx_server_monitor_log(self->base.parent, APX_LOG_LEVEL_DEBUG, "[%u] Sending %d bytes",
        apx_server_connection_get_connection_id(&self->base), (int)self->pending_bytes);
    }
    SOCKET_SEND(self->socket_object, data, (uint32_t)self->pending_bytes);
    adt_bytearray_clear(&self->send_buffer);
    self->pending_bytes = 0u;
  }
}