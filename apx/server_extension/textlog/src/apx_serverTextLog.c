/*****************************************************************************
* \file      apx_serverTextLog.c
* \author    Conny Gustafsson
* \date      2019-09-12
* \brief     Server text log
*
* Copyright (c) 2019 Conny Gustafsson
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
#include "apx_serverTextLog.h"
#include "apx_eventListener.h"
#include "apx_serverConnectionBase.h"
#include "apx_portConnectionTable.h"
#include "apx_server.h"

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
static void apx_serverTextLog_registerServerListener(apx_serverTextLog_t *self);
static void apx_serverTextLog_registerNodeDataListener(apx_serverTextLog_t *self, apx_serverConnectionBase_t *connection);
static void apx_serverTextLog_onLogEvent(void *arg, apx_logLevel_t level, const char *label, const char *msg);

static void apx_serverTextLog_onConnected(void *arg, apx_serverConnectionBase_t *connection);
static void apx_serverTextLog_onDisconnected(void *arg, apx_serverConnectionBase_t *connection);
static void apx_serverTextLog_onDefinitionDataWritten(void *arg, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len);
static void apx_serverTextLog_onOutPortDataWritten(void *arg, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len);
static void apx_serverTextLog_onNodeDataComplete(void *arg, struct apx_nodeData_tag *nodeData);
static void apx_serverTextLog_providePortsConnected(void *arg, apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable);
static void apx_serverTextLog_providePortsDisconnected(void *arg, apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable);
static void apx_serverTextLog_requirePortsConnected(void *arg, apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable);
static void apx_serverTextLog_requirePortsDisconnected(void *arg, apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable);

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
      apx_serverTextLog_registerServerListener(self);
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

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_serverTextLog_registerServerListener(apx_serverTextLog_t *self)
{
   apx_serverEventListener_t eventListener;
   memset(&eventListener, 0, sizeof(apx_serverEventListener_t));
   eventListener.arg = (void*) self;
   eventListener.serverConnect2 = apx_serverTextLog_onConnected;
   eventListener.serverDisconnect2 = apx_serverTextLog_onDisconnected;
   //eventListener.logEvent = apx_serverTextLog_onLogEvent;
   apx_server_registerEventListener(self->server, &eventListener);
}

static void apx_serverTextLog_registerNodeDataListener(apx_serverTextLog_t *self, apx_serverConnectionBase_t *connection)
{
   apx_connectionEventListener_t listener;
   memset(&listener, 0, sizeof(listener));
   listener.arg = (void*) self;
/*   listener.providePortsConnected = apx_serverTextLog_providePortsConnected;
   listener.providePortsDisconnected = apx_serverTextLog_providePortsDisconnected;
   listener.requirePortsConnected = apx_serverTextLog_requirePortsConnected;
   listener.requirePortsDisconnected = apx_serverTextLog_requirePortsDisconnected;
   listener.definitionDataWritten = apx_serverTextLog_onDefinitionDataWritten;
   listener.outPortDataWritten = apx_serverTextLog_onOutPortDataWritten;
   listener.nodeComplete = apx_serverTextLog_onNodeDataComplete;
   */
   apx_serverConnectionBase_registerEventListener(connection, &listener);
}

static void apx_serverTextLog_onLogEvent(void *arg, apx_logLevel_t level, const char *label, const char *msg)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (label != 0) && (msg != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%s] %s", label, msg);
   }
}


static void apx_serverTextLog_onConnected(void *arg, apx_serverConnectionBase_t *connection)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (connection != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%u] Client connected", apx_serverConnectionBase_getConnectionId(connection));
      apx_serverTextLog_registerNodeDataListener(self, connection);
   }
}

static void apx_serverTextLog_onDisconnected(void *arg, apx_serverConnectionBase_t *connection)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (connection != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%u] Client disconnected", apx_serverConnectionBase_getConnectionId(connection));
      //apx_serverTextLog_registerNodeDataListener(self, connection);
   }
}

static void apx_serverTextLog_onDefinitionDataWritten(void *arg, apx_nodeData_t *nodeData, uint32_t offset, uint32_t len)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (nodeData != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%u] %s: Definition data written (%u, %u)",
            apx_nodeData_getConnectionId(nodeData),
            apx_nodeData_getName(nodeData),
            (unsigned int) offset,
            (unsigned int) len);
   }
}

static void apx_serverTextLog_onOutPortDataWritten(void *arg, apx_nodeData_t *nodeData, uint32_t offset, uint32_t len)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (nodeData != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%u] %s: Outport data written (%u, %u)",
            apx_nodeData_getConnectionId(nodeData),
            apx_nodeData_getName(nodeData),
            (unsigned int) offset,
            (unsigned int) len);
   }
}

static void apx_serverTextLog_onNodeDataComplete(void *arg, apx_nodeData_t *nodeData)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (nodeData != 0) )
   {
      apx_textLogBase_printf(&self->base, "[%u] %s: Node Complete",
            apx_nodeData_getConnectionId(nodeData),
            apx_nodeData_getName(nodeData));
   }
}


static void apx_serverTextLog_providePortsConnected(void *arg, apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (nodeData != 0) && (connectionTable != 0))
   {
#if 0
      int32_t localPortId;
      apx_node_t *localNode = apx_nodeData_getNode(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connectionBase_t* connection = apx_nodeData_getConnection(nodeData);
         apx_portRef_t *portref;
         apx_portConnectionEntry_t *entry = apx_portConnectionTable_getEntry(connectionTable, localPortId);
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
#endif
   }
}

static void apx_serverTextLog_providePortsDisconnected(void *arg, apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (nodeData != 0) && (connectionTable != 0) )
   {
#if 0
      int32_t localPortId;
      apx_node_t *localNode = apx_nodeData_getNode(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connectionBase_t* connection = apx_nodeData_getConnection(nodeData);
         apx_portRef_t *portref;
         apx_portConnectionEntry_t *entry = apx_portConnectionTable_getEntry(connectionTable, localPortId);
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
#endif
   }
}

static void apx_serverTextLog_requirePortsConnected(void *arg, apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (nodeData != 0) && (connectionTable != 0))
   {
#if 0
      int32_t localPortId;
      apx_node_t *localNode = apx_nodeData_getNode(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connectionBase_t* connection = apx_nodeData_getConnection(nodeData);
         apx_portRef_t *portref;
         apx_portConnectionEntry_t *entry = apx_portConnectionTable_getEntry(connectionTable, localPortId);
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
#endif
   }
}

static void apx_serverTextLog_requirePortsDisconnected(void *arg, apx_nodeData_t *nodeData, apx_portConnectionTable_t *connectionTable)
{
   apx_serverTextLog_t *self = (apx_serverTextLog_t *) arg;
   if ( (self != 0) && (nodeData != 0) && (connectionTable != 0) )
   {
#if 0
      int32_t localPortId;
      apx_node_t *localNode = apx_nodeData_getNode(nodeData);
      for (localPortId=0; localPortId<connectionTable->numPorts; localPortId++)
      {
         apx_connectionBase_t* connection = apx_nodeData_getConnection(nodeData);
         apx_portRef_t *portref;
         apx_portConnectionEntry_t *entry = apx_portConnectionTable_getEntry(connectionTable, localPortId);
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
#endif
   }
}
