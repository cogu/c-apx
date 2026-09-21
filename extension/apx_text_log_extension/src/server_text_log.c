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
      register_server_listener(self);
   }
}

void apx_server_text_log_destroy(apx_server_text_log_t *self)
{
   if (self != NULL)
   {
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
   apx_server_register_event_listener(self->server, &eventListener);
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


#if 0
static void apx_server_text_log_provide_ports_connected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t *connectionTable)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (node_instance != NULL) && (connectionTable != NULL))
   {
      int32_t localPortId;
      apx_node_t *localNode = apx_node_data_get_node(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connection_base_t* connection = apx_node_data_get_connection(nodeData);
         apx_port_ref_t *portref;
         apx_port_connection_entry_t *entry = apx_port_connector_change_table_get_entry(connectionTable, localPortId);
         portref = apx_port_connection_entry_get(entry, 0);
         if (portref != NULL)
         {
            int32_t remotePortId;
            apx_port_t *localPort;
            apx_port_t *remotePort;
            apx_node_t *remoteNode = apx_node_data_get_node(portref->nodeData);
            remotePortId = apx_port_data_ref_get_port_id(portref);
            localPort = apx_node_get_provide_port(localNode, localPortId);
            remotePort = apx_node_get_require_port(remoteNode, remotePortId);
            if ( (localPort != NULL) && (remotePort) )
            {

               apx_text_log_base_printf(&self->base, "[%d] %s.%s --> %s.%s",
                       apx_connection_base_get_connection_id(connection),
                       localNode->name,
                       localPort->name,
                       remoteNode->name,
                       remotePort->name);
            }
         }
      }
   }
}

static void apx_server_text_log_provide_ports_disconnected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t *connectionTable)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (node_instance != NULL) && (connectionTable != NULL) )
   {
      int32_t localPortId;
      apx_node_t *localNode = apx_node_data_get_node(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connection_base_t* connection = apx_node_data_get_connection(nodeData);
         apx_port_ref_t *portref;
         apx_port_connection_entry_t *entry = apx_port_connector_change_table_get_entry(connectionTable, localPortId);
         portref = apx_port_connection_entry_get(entry, 0);
         if (portref != NULL)
         {
            int32_t remotePortId;
            apx_port_t *localPort;
            apx_port_t *remotePort;
            apx_node_t *remoteNode = apx_node_data_get_node(portref->nodeData);
            remotePortId = apx_port_data_ref_get_port_id(portref);
            localPort = apx_node_get_provide_port(localNode, localPortId);
            remotePort = apx_node_get_require_port(remoteNode, remotePortId);
            if ( (localPort != NULL) && (remotePort) )
            {
               apx_text_log_base_printf(&self->base, "[%d] %s.%s -!-> %s.%s",
                       apx_connection_base_get_connection_id(connection),
                       localNode->name,
                       localPort->name,
                       remoteNode->name,
                       remotePort->name);
            }
         }
      }
   }
}

static void apx_server_text_log_require_ports_connected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t *connectionTable)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (node_instance != NULL) && (connectionTable != NULL))
   {
      int32_t localPortId;
      apx_node_t *localNode = apx_node_data_get_node(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connection_base_t* connection = apx_node_data_get_connection(nodeData);
         apx_port_ref_t *portref;
         apx_port_connection_entry_t *entry = apx_port_connector_change_table_get_entry(connectionTable, localPortId);
         portref = apx_port_connection_entry_get(entry, 0);
         if (portref != NULL)
         {
            int32_t remotePortId;
            apx_port_t *localPort;
            apx_port_t *remotePort;
            apx_node_t *remoteNode = apx_node_data_get_node(portref->nodeData);
            remotePortId = apx_port_data_ref_get_port_id(portref);
            localPort = apx_node_get_require_port(localNode, localPortId);
            remotePort = apx_node_get_provide_port(remoteNode, remotePortId);
            if ( (localPort != NULL) && (remotePort) )
            {

               apx_text_log_base_printf(&self->base, "[%d] %s.%s <-- %s.%s",
                       apx_connection_base_get_connection_id(connection),
                       localNode->name,
                       localPort->name,
                       remoteNode->name,
                       remotePort->name);
            }
         }
      }
   }
}

static void apx_server_text_log_require_ports_disconnected(void *arg, apx_node_instance_t* node_instance, apx_port_connector_change_table_t *connectionTable)
{
   apx_server_text_log_t *self = (apx_server_text_log_t *) arg;
   if ( (self != NULL) && (node_instance != NULL) && (connectionTable != NULL) )
   {
      int32_t localPortId;
      apx_node_t *localNode = apx_node_data_get_node(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connection_base_t* connection = apx_node_data_get_connection(nodeData);
         apx_port_ref_t *portref;
         apx_port_connection_entry_t *entry = apx_port_connector_change_table_get_entry(connectionTable, localPortId);
         portref = apx_port_connection_entry_get(entry, 0);
         if (portref != NULL)
         {
            int32_t remotePortId;
            apx_port_t *localPort;
            apx_port_t *remotePort;
            apx_node_t *remoteNode = apx_node_data_get_node(portref->nodeData);
            remotePortId = apx_port_data_ref_get_port_id(portref);
            localPort = apx_node_get_require_port(localNode, localPortId);
            remotePort = apx_node_get_provide_port(remoteNode, remotePortId);
            if ( (localPort != NULL) && (remotePort) )
            {
               apx_text_log_base_printf(&self->base, "[%d] %s.%s -!-> %s.%s",
                       apx_connection_base_get_connection_id(connection),
                       localNode->name,
                       localPort->name,
                       remoteNode->name,
                       remotePort->name);
            }
         }
      }
   }
}
#endif
