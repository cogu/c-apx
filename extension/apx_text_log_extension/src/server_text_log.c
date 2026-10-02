/*****************************************************************************
* \file      server_text_log.c
* \author    Conny Gustafsson
* \date      2019-09-12
* \brief     Server text log
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <stdio.h>


#include "adt_str.h"
#include "apx/extension/server_text_log.h"
#include "apx/event_listener.h"
#include "apx/server_connection.h"
#include "apx/port_connector_change_table.h"
#include "apx/server.h"
#include "apx/node_instance.h"
#include "apx/port_instance.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_server_text_log_t *self);
static void register_connection_listener(apx_server_text_log_t *self, apx_server_connection_t *connection);
static void apx_server_text_log_on_log_event(void *arg, apx_log_level_t level, const char *label, const char *msg);

static void apx_server_text_log_on_new_connection(void *arg, apx_server_connection_t *connection);
static void apx_server_text_log_on_connection_closed(void *arg, apx_server_connection_t *connection);
static void on_protocol_header_accepted(apx_server_text_log_t* self, apx_server_connection_t* connection);
static void on_file_published(apx_server_text_log_t* self, apx_server_connection_t* connection, const struct rmf_file_info_tag* file_info);
static void on_file_revoked(apx_server_text_log_t* self, apx_server_connection_t* connection, const struct rmf_file_info_tag* file_info);
static void apx_server_text_log_provide_ports_connected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t const *connector_changes);
static void apx_server_text_log_provide_ports_disconnected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t const *connector_changes);
static void apx_server_text_log_require_ports_connected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t const *connector_changes);
static void apx_server_text_log_require_ports_disconnected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t const *connector_changes);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_server_text_log_create(apx_server_text_log_t *self, struct apx_server_tag *server)
{
   if ( (self != NULL) && (server != 0) )
   {
      apx_text_log_base_create(&self->base);
      self->server = server;
      self->server_listener_handle = NULL;
      register_server_listener(self);
   }
}

void apx_server_text_log_destroy(apx_server_text_log_t *self)
{
   if (self != NULL)
   {
      if ((self->server != NULL) && (self->server_listener_handle != NULL))
      {
         apx_server_unregister_event_listener(self->server, self->server_listener_handle);
         self->server_listener_handle = NULL;
      }
      apx_text_log_base_destroy(&self->base);
   }
}

apx_server_text_log_t *apx_server_text_log_new(struct apx_server_tag *server)
{
   apx_server_text_log_t *self = (apx_server_text_log_t*) malloc(sizeof(apx_server_text_log_t));
   if(self != NULL)
   {
      apx_server_text_log_create(self, server);
   }
   return self;
}

void apx_server_text_log_delete(apx_server_text_log_t *self)
{
   if(self != NULL)
   {
      apx_server_text_log_destroy(self);
      free(self);
   }
}

void apx_server_text_log_enable_file(apx_server_text_log_t *self, const char *path)
{
   if ( (self != NULL) && (path != NULL) )
   {
      apx_text_log_base_enable_file(&self->base, path );
   }
}

void apx_server_text_log_enable_std_out(apx_server_text_log_t *self)
{
   if (self != NULL)
   {
      apx_text_log_base_enable_stdout(&self->base);
   }
}

void apx_server_text_log_enable_sys_log(apx_server_text_log_t *self, const char *label)
{
   if (self != NULL)
   {
      apx_text_log_base_enable_sys_log(&self->base, label);
   }
}

void apx_server_text_log_close_all(apx_server_text_log_t *self)
{
   if (self != NULL)
   {
      apx_text_log_base_close_all(&self->base);
   }
}

void apx_server_text_log_virtual_on_protocol_header_accepted(void* arg, struct apx_connection_base_tag* connection)
{
   apx_server_text_log_t* self = (apx_server_text_log_t*)arg;
   if ((self != NULL) && connection != NULL)
   {
      on_protocol_header_accepted(self, (apx_server_connection_t*)connection);
   }
}

void apx_server_text_log_virtual_on_file_published(void* arg, struct apx_connection_base_tag* connection, const struct rmf_file_info_tag* file_info)
{
   apx_server_text_log_t* self = (apx_server_text_log_t*)arg;
   if ((self != NULL) && (connection != NULL) && (file_info != NULL))
   {
      on_file_published(self, (apx_server_connection_t*)connection, file_info);
   }
}

void apx_server_text_log_virtual_on_file_revoked(void* arg, struct apx_connection_base_tag* connection, const struct rmf_file_info_tag* file_info)
{
   apx_server_text_log_t* self = (apx_server_text_log_t*)arg;
   if ((self != NULL) && (connection != NULL) && (file_info != NULL))
   {
      on_file_revoked(self, (apx_server_connection_t*)connection, file_info);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_server_text_log_t *self)
{
   apx_server_event_listener_t eventListener;
   memset(&eventListener, 0, sizeof(apx_server_event_listener_t));
   eventListener.arg = (void*) self;
   eventListener.new_connection = apx_server_text_log_on_new_connection;
   eventListener.connection_closed = apx_server_text_log_on_connection_closed;
   eventListener.server_write_log = apx_server_text_log_on_log_event;
   eventListener.require_ports_connected = apx_server_text_log_require_ports_connected;
   eventListener.provide_ports_connected = apx_server_text_log_provide_ports_connected;
   eventListener.require_ports_disconnected = apx_server_text_log_require_ports_disconnected;
   eventListener.provide_ports_disconnected = apx_server_text_log_provide_ports_disconnected;
   self->server_listener_handle = apx_server_register_event_listener(self->server, &eventListener);
}

static void register_connection_listener(apx_server_text_log_t *self, apx_server_connection_t *connection)
{
   apx_server_connection_event_listener_t listener;
   memset(&listener, 0, sizeof(listener));
   listener.arg = (void*) self;
   listener.protocol_header_accepted = apx_server_text_log_virtual_on_protocol_header_accepted;
   listener.file_published = apx_server_text_log_virtual_on_file_published;
   listener.file_revoked = apx_server_text_log_virtual_on_file_revoked;
   apx_server_connection_register_event_listener(connection, &listener);
}

static void apx_server_text_log_on_log_event(void *arg, apx_log_level_t level, const char *label, const char *msg)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   (void)level;
   if ( (self != NULL) && (label != NULL) && (msg != NULL) )
   {
      apx_text_log_base_printf(&self->base, "[%s] %s", label, msg);
   }
}


static void apx_server_text_log_on_new_connection(void *arg, apx_server_connection_t *connection)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (connection != NULL) )
   {
      apx_text_log_base_printf(&self->base, "[%u] New connection", apx_server_connection_get_connection_id(connection));
      register_connection_listener(self, connection);
   }
}

static void apx_server_text_log_on_connection_closed(void *arg, apx_server_connection_t *connection)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (connection != NULL) )
   {
      apx_text_log_base_printf(&self->base, "[%u] Connection closed", apx_server_connection_get_connection_id(connection));
   }
}

static void on_protocol_header_accepted(apx_server_text_log_t* self, apx_server_connection_t* connection)
{
   apx_text_log_base_printf(&self->base, "[%u] Header Accepted", apx_server_connection_get_connection_id(connection));
}

static void on_file_published(apx_server_text_log_t* self, apx_server_connection_t* connection, const struct rmf_file_info_tag* file_info)
{
   (void)file_info;
   apx_text_log_base_printf(&self->base, "[%u] New file published: %s", apx_server_connection_get_connection_id(connection), rmf_file_info_name(file_info));
}

static void on_file_revoked(apx_server_text_log_t* self, apx_server_connection_t* connection, const struct rmf_file_info_tag* file_info)
{
   (void)self;
   (void)connection;
   (void)file_info;
}


static void apx_server_text_log_provide_ports_connected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t const *connector_changes)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (node_instance != NULL) && (connector_changes != NULL) )
   {
      apx_port_id_t local_port_id;
      uint32_t conn_id = apx_node_instance_get_connection_id(node_instance);
      char const *local_node_name = apx_node_instance_get_name(node_instance);
      for (local_port_id = 0; local_port_id < connector_changes->num_ports; local_port_id++)
      {
         int32_t count = apx_port_connector_change_table_count((apx_port_connector_change_table_t*)connector_changes, local_port_id);
         if (count > 0)
         {
            apx_port_instance_t *local_port = apx_node_instance_get_provide_port(node_instance, local_port_id);
            char const *local_port_name = (local_port != NULL) ? apx_port_instance_name(local_port) : "<unknown>";
            int32_t i;
            for (i = 0; i < count; i++)
            {
               apx_port_instance_t *remote_port = apx_port_connector_change_table_get_port((apx_port_connector_change_table_t*)connector_changes, local_port_id, i);
               if (remote_port != NULL)
               {
                  apx_node_instance_t *remote_node = apx_port_instance_parent(remote_port);
                  char const *remote_node_name = (remote_node != NULL) ? apx_node_instance_get_name(remote_node) : "<unknown>";
                  char const *remote_port_name = apx_port_instance_name(remote_port);
                  apx_text_log_base_printf(&self->base, "[%u] %s.%s --> %s.%s",
                          conn_id,
                          local_node_name,
                          local_port_name,
                          remote_node_name,
                          remote_port_name);
               }
            }
         }
      }
   }
}

static void apx_server_text_log_provide_ports_disconnected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t const *connector_changes)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (node_instance != NULL) && (connector_changes != NULL) )
   {
      apx_port_id_t local_port_id;
      uint32_t conn_id = apx_node_instance_get_connection_id(node_instance);
      char const *local_node_name = apx_node_instance_get_name(node_instance);
      for (local_port_id = 0; local_port_id < connector_changes->num_ports; local_port_id++)
      {
         int32_t count = apx_port_connector_change_table_count((apx_port_connector_change_table_t*)connector_changes, local_port_id);
         if (count < 0)
         {
            apx_port_instance_t *local_port = apx_node_instance_get_provide_port(node_instance, local_port_id);
            char const *local_port_name = (local_port != NULL) ? apx_port_instance_name(local_port) : "<unknown>";
            int32_t num_changes = -count;
            int32_t i;
            for (i = 0; i < num_changes; i++)
            {
               apx_port_instance_t *remote_port = apx_port_connector_change_table_get_port((apx_port_connector_change_table_t*)connector_changes, local_port_id, i);
               if (remote_port != NULL)
               {
                  apx_node_instance_t *remote_node = apx_port_instance_parent(remote_port);
                  char const *remote_node_name = (remote_node != NULL) ? apx_node_instance_get_name(remote_node) : "<unknown>";
                  char const *remote_port_name = apx_port_instance_name(remote_port);
                  apx_text_log_base_printf(&self->base, "[%u] %s.%s -!-> %s.%s",
                          conn_id,
                          local_node_name,
                          local_port_name,
                          remote_node_name,
                          remote_port_name);
               }
            }
         }
      }
   }
}

static void apx_server_text_log_require_ports_connected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t const *connector_changes)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (node_instance != NULL) && (connector_changes != NULL) )
   {
      apx_port_id_t local_port_id;
      uint32_t conn_id = apx_node_instance_get_connection_id(node_instance);
      char const *local_node_name = apx_node_instance_get_name(node_instance);
      for (local_port_id = 0; local_port_id < connector_changes->num_ports; local_port_id++)
      {
         int32_t count = apx_port_connector_change_table_count((apx_port_connector_change_table_t*)connector_changes, local_port_id);
         if (count > 0)
         {
            apx_port_instance_t *local_port = apx_node_instance_get_require_port(node_instance, local_port_id);
            char const *local_port_name = (local_port != NULL) ? apx_port_instance_name(local_port) : "<unknown>";
            int32_t i;
            for (i = 0; i < count; i++)
            {
               apx_port_instance_t *remote_port = apx_port_connector_change_table_get_port((apx_port_connector_change_table_t*)connector_changes, local_port_id, i);
               if (remote_port != NULL)
               {
                  apx_node_instance_t *remote_node = apx_port_instance_parent(remote_port);
                  char const *remote_node_name = (remote_node != NULL) ? apx_node_instance_get_name(remote_node) : "<unknown>";
                  char const *remote_port_name = apx_port_instance_name(remote_port);
                  apx_text_log_base_printf(&self->base, "[%u] %s.%s <-- %s.%s",
                          conn_id,
                          local_node_name,
                          local_port_name,
                          remote_node_name,
                          remote_port_name);
               }
            }
         }
      }
   }
}

static void apx_server_text_log_require_ports_disconnected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t const *connector_changes)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (node_instance != NULL) && (connector_changes != NULL) )
   {
      apx_port_id_t local_port_id;
      uint32_t conn_id = apx_node_instance_get_connection_id(node_instance);
      char const *local_node_name = apx_node_instance_get_name(node_instance);
      for (local_port_id = 0; local_port_id < connector_changes->num_ports; local_port_id++)
      {
         int32_t count = apx_port_connector_change_table_count((apx_port_connector_change_table_t*)connector_changes, local_port_id);
         if (count < 0)
         {
            apx_port_instance_t *local_port = apx_node_instance_get_require_port(node_instance, local_port_id);
            char const *local_port_name = (local_port != NULL) ? apx_port_instance_name(local_port) : "<unknown>";
            int32_t num_changes = -count;
            int32_t i;
            for (i = 0; i < num_changes; i++)
            {
               apx_port_instance_t *remote_port = apx_port_connector_change_table_get_port((apx_port_connector_change_table_t*)connector_changes, local_port_id, i);
               if (remote_port != NULL)
               {
                  apx_node_instance_t *remote_node = apx_port_instance_parent(remote_port);
                  char const *remote_node_name = (remote_node != NULL) ? apx_node_instance_get_name(remote_node) : "<unknown>";
                  char const *remote_port_name = apx_port_instance_name(remote_port);
                  apx_text_log_base_printf(&self->base, "[%u] %s.%s -!-> %s.%s",
                          conn_id,
                          local_node_name,
                          local_port_name,
                          remote_node_name,
                          remote_port_name);
               }
            }
         }
      }
   }
}
