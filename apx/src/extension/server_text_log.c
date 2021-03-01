/*****************************************************************************
* \file      server_text_log.c
* \author    Conny Gustafsson
* \date      2019-09-12
* \brief     Server text log
*
* Copyright (c) 2019-2020 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
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
static void register_server_listener(apx_serverTextLog_t *self);
static void register_connection_listener(apx_serverTextLog_t *self, apx_serverConnection_t *connection);
static void apx_serverTextLog_onLogEvent(void *arg, apx_logLevel_t level, const char *label, const char *msg);

static void apx_serverTextLog_on_new_connection(void *arg, apx_serverConnection_t *connection);
static void apx_serverTextLog_on_connection_closed(void *arg, apx_serverConnection_t *connection);
static void on_protocol_header_accepted(apx_serverTextLog_t* self, apx_serverConnection_t* connection);
static void on_file_published(apx_serverTextLog_t* self, apx_serverConnection_t* connection, const struct rmf_fileInfo_tag* file_info);
static void on_file_revoked(apx_serverTextLog_t* self, apx_serverConnection_t* connection, const struct rmf_fileInfo_tag* file_info);


//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_serverTextLog_create(apx_serverTextLog_t *self, struct apx_server_tag *server)
{
   if ( (self != 0) && (server != 0) )
   {
      apx_textLogBase_create(&self->base);
      self->server = server;
      register_server_listener(self);
   }
}

void apx_serverTextLog_destroy(apx_serverTextLog_t *self)
{
   if (self != 0)
   {
      apx_textLogBase_destroy(&self->base);
   }
}

apx_serverTextLog_t *apx_serverTextLog_new(struct apx_server_tag *server)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t*) malloc(sizeof(apx_serverTextLog_t));
   if(self != 0)
   {
      apx_serverTextLog_create(self, server);
   }
   return self;
}

void apx_serverTextLog_delete(apx_serverTextLog_t *self)
{
   if(self != 0)
   {
      apx_serverTextLog_destroy(self);
      free(self);
   }
}

void apx_serverTextLog_enableFile(apx_serverTextLog_t *self, const char *path)
{
   if ( (self != 0) && (path != 0) )
   {
      apx_textLogBase_enableFile(&self->base, path );
   }
}

void apx_serverTextLog_enableStdOut(apx_serverTextLog_t *self)
{
   if (self != 0)
   {
      apx_textLogBase_enableStdout(&self->base);
   }
}

void apx_serverTextLog_enableSysLog(apx_serverTextLog_t *self, const char *label)
{
   if (self != 0)
   {
      apx_textLogBase_enableSysLog(&self->base, label);
   }
}

void apx_serverTextLog_closeAll(apx_serverTextLog_t *self)
{
   if (self != 0)
   {
      apx_textLogBase_closeAll(&self->base);
   }
}

void apx_serverTextLog_virtual_on_protocol_header_accepted(void* arg, struct apx_connectionBase_tag* connection)
{
   apx_serverTextLog_t* self = (apx_serverTextLog_t*)arg;
   if ((self != NULL) && connection != NULL)
   {
      on_protocol_header_accepted(self, (apx_serverConnection_t*)connection);
   }
}

void apx_serverTextLog_virtual_on_file_published(void* arg, struct apx_connectionBase_tag* connection, const struct rmf_fileInfo_tag* file_info)
{
   apx_serverTextLog_t* self = (apx_serverTextLog_t*)arg;
   if ((self != NULL) && (connection != NULL) && (file_info != NULL))
   {
      on_file_published(self, (apx_serverConnection_t*)connection, file_info);
   }
}

void apx_serverTextLog_virtual_on_file_revoked(void* arg, struct apx_connectionBase_tag* connection, const struct rmf_fileInfo_tag* file_info)
{
   apx_serverTextLog_t* self = (apx_serverTextLog_t*)arg;
   if ((self != NULL) && (connection != NULL) && (file_info != NULL))
   {
      on_file_revoked(self, (apx_serverConnection_t*)connection, file_info);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void register_server_listener(apx_serverTextLog_t *self)
{
   apx_serverEventListener_t eventListener;
   memset(&eventListener, 0, sizeof(apx_serverEventListener_t));
   eventListener.arg = (void*) self;
   eventListener.new_connection2 = apx_serverTextLog_on_new_connection;
   eventListener.connection_closed2 = apx_serverTextLog_on_connection_closed;
   apx_server_register_event_listener(self->server, &eventListener);
}

static void register_connection_listener(apx_serverTextLog_t *self, apx_serverConnection_t *connection)
{
   apx_connectionEventListener_t listener;
   memset(&listener, 0, sizeof(listener));
   listener.arg = (void*) self;
   listener.protocol_header_accepted = apx_serverTextLog_virtual_on_protocol_header_accepted;
   listener.file_published = apx_serverTextLog_virtual_on_file_published;
   listener.file_revoked = apx_serverTextLog_virtual_on_file_revoked;
   apx_serverConnection_register_event_listener(connection, &listener);
}

static void apx_serverTextLog_onLogEvent(void *arg, apx_logLevel_t level, const char *label, const char *msg)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   (void)level;
   if ( (self != 0) && (label != 0) && (msg != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%s] %s", label, msg);
   }
}


static void apx_serverTextLog_on_new_connection(void *arg, apx_serverConnection_t *connection)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (connection != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%u] Client connected", apx_serverConnection_get_connection_id(connection));
      register_connection_listener(self, connection);
   }
}

static void apx_serverTextLog_on_connection_closed(void *arg, apx_serverConnection_t *connection)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (connection != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%u] Client disconnected", apx_serverConnection_get_connection_id(connection));      
   }
}

static void on_protocol_header_accepted(apx_serverTextLog_t* self, apx_serverConnection_t* connection)
{
   apx_textLogBase_printf(&self->base, "[%u] Header Accepted", apx_serverConnection_get_connection_id(connection));
}

static void on_file_published(apx_serverTextLog_t* self, apx_serverConnection_t* connection, const struct rmf_fileInfo_tag* file_info)
{
   (void)file_info;
   apx_textLogBase_printf(&self->base, "[%u] New file published", apx_serverConnection_get_connection_id(connection));
}

static void on_file_revoked(apx_serverTextLog_t* self, apx_serverConnection_t* connection, const struct rmf_fileInfo_tag* file_info)
{
   (void)self;
   (void)connection;
   (void)file_info;
}


#if 0
static void apx_serverTextLog_providePortsConnected(void *arg, apx_nodeInstance_t* node_instance, apx_portConnectorChangeTable_t *connectionTable)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (node_instance != 0) && (connectionTable != 0))
   {
      int32_t localPortId;
      apx_node_t *localNode = apx_nodeData_getNode(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connectionBase_t* connection = apx_nodeData_getConnection(nodeData);
         apx_portRef_t *portref;
         apx_portConnectionEntry_t *entry = apx_portConnectorChangeTable_getEntry(connectionTable, localPortId);
         portref = apx_portConnectionEntry_get(entry, 0);
         if (portref != 0)
         {
            int32_t remotePortId;
            apx_port_t *localPort;
            apx_port_t *remotePort;
            apx_node_t *remoteNode = apx_nodeData_getNode(portref->nodeData);
            remotePortId = apx_portDataRef_getPortId(portref);
            localPort = apx_node_getProvidePort(localNode, localPortId);
            remotePort = apx_node_getRequirePort(remoteNode, remotePortId);
            if ( (localPort != 0) && (remotePort) )
            {

               apx_textLogBase_printf(&self->base, "[%d] %s.%s --> %s.%s",
                       apx_connectionBase_getConnectionId(connection),
                       localNode->name,
                       localPort->name,
                       remoteNode->name,
                       remotePort->name);
            }
         }
      }
   }
}

static void apx_serverTextLog_providePortsDisconnected(void *arg, apx_nodeInstance_t* node_instance, apx_portConnectorChangeTable_t *connectionTable)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (node_instance != 0) && (connectionTable != 0) )
   {
      int32_t localPortId;
      apx_node_t *localNode = apx_nodeData_getNode(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connectionBase_t* connection = apx_nodeData_getConnection(nodeData);
         apx_portRef_t *portref;
         apx_portConnectionEntry_t *entry = apx_portConnectorChangeTable_getEntry(connectionTable, localPortId);
         portref = apx_portConnectionEntry_get(entry, 0);
         if (portref != 0)
         {
            int32_t remotePortId;
            apx_port_t *localPort;
            apx_port_t *remotePort;
            apx_node_t *remoteNode = apx_nodeData_getNode(portref->nodeData);
            remotePortId = apx_portDataRef_getPortId(portref);
            localPort = apx_node_getProvidePort(localNode, localPortId);
            remotePort = apx_node_getRequirePort(remoteNode, remotePortId);
            if ( (localPort != 0) && (remotePort) )
            {
               apx_textLogBase_printf(&self->base, "[%d] %s.%s -!-> %s.%s",
                       apx_connectionBase_getConnectionId(connection),
                       localNode->name,
                       localPort->name,
                       remoteNode->name,
                       remotePort->name);
            }
         }
      }
   }
}

static void apx_serverTextLog_requirePortsConnected(void *arg, apx_nodeInstance_t* node_instance, apx_portConnectorChangeTable_t *connectionTable)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (node_instance != 0) && (connectionTable != 0))
   {
      int32_t localPortId;
      apx_node_t *localNode = apx_nodeData_getNode(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connectionBase_t* connection = apx_nodeData_getConnection(nodeData);
         apx_portRef_t *portref;
         apx_portConnectionEntry_t *entry = apx_portConnectorChangeTable_getEntry(connectionTable, localPortId);
         portref = apx_portConnectionEntry_get(entry, 0);
         if (portref != 0)
         {
            int32_t remotePortId;
            apx_port_t *localPort;
            apx_port_t *remotePort;
            apx_node_t *remoteNode = apx_nodeData_getNode(portref->nodeData);
            remotePortId = apx_portDataRef_getPortId(portref);
            localPort = apx_node_getRequirePort(localNode, localPortId);
            remotePort = apx_node_getProvidePort(remoteNode, remotePortId);
            if ( (localPort != 0) && (remotePort) )
            {

               apx_textLogBase_printf(&self->base, "[%d] %s.%s <-- %s.%s",
                       apx_connectionBase_getConnectionId(connection),
                       localNode->name,
                       localPort->name,
                       remoteNode->name,
                       remotePort->name);
            }
         }
      }
   }
}

static void apx_serverTextLog_requirePortsDisconnected(void *arg, apx_nodeInstance_t* node_instance, apx_portConnectorChangeTable_t *connectionTable)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (node_instance != 0) && (connectionTable != 0) )
   {
      int32_t localPortId;
      apx_node_t *localNode = apx_nodeData_getNode(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connectionBase_t* connection = apx_nodeData_getConnection(nodeData);
         apx_portRef_t *portref;
         apx_portConnectionEntry_t *entry = apx_portConnectorChangeTable_getEntry(connectionTable, localPortId);
         portref = apx_portConnectionEntry_get(entry, 0);
         if (portref != 0)
         {
            int32_t remotePortId;
            apx_port_t *localPort;
            apx_port_t *remotePort;
            apx_node_t *remoteNode = apx_nodeData_getNode(portref->nodeData);
            remotePortId = apx_portDataRef_getPortId(portref);
            localPort = apx_node_getRequirePort(localNode, localPortId);
            remotePort = apx_node_getProvidePort(remoteNode, remotePortId);
            if ( (localPort != 0) && (remotePort) )
            {
               apx_textLogBase_printf(&self->base, "[%d] %s.%s -!-> %s.%s",
                       apx_connectionBase_getConnectionId(connection),
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
