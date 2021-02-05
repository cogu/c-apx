/*****************************************************************************
* \file      client.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX client class
*
* Copyright (c) 2017-2021 Conny Gustafsson
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
#include <assert.h>
#include <stdio.h>
#include "apx/client.h"
#include "apx/client_internal.h"
#include "apx/client_connection.h"
#include "apx/socket_client_connection.h"
#include "apx/node_manager.h"
#include "apx/file_manager.h"
#include "apx/parser.h"
#include "apx/node_instance.h"
#include "apx/vm.h"
#include "msocket.h"
#include "adt_ary.h"
#include "adt_list.h"
#include "adt_hash.h"
#include "apx/event_listener.h"
#include "apx/compiler.h"
#include "pack.h"
#ifdef UNIT_TEST
#include "testsocket.h"
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MAX_STACK_BUFFER_SIZE 256u
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_client_trigger_connected_event_on_listeners(apx_client_t *self, apx_clientConnection_t *connection);
static void apx_client_trigger_disconnected_event_on_listeners(apx_client_t *self, apx_clientConnection_t *connection);
static void apx_client_attach_local_nodes_to_connection(apx_client_t *self);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_client_create(apx_client_t *self)
{
   if( self != NULL )
   {
      self->event_listeners = adt_list_new(apx_clientEventListener_vdelete);
      if (self->event_listeners == 0)
      {
         return APX_MEM_ERROR;
      }
      self->connection = (apx_clientConnection_t*) NULL;
      self->vm = (apx_vm_t*) NULL;
      self->node_manager = apx_nodeManager_new(APX_CLIENT_MODE);
      self->is_connected = false;
      MUTEX_INIT(self->lock);
      MUTEX_INIT(self->event_listener_lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_client_destroy(apx_client_t *self)
{
   if (self != NULL)
   {
      bool isConnected;
      MUTEX_LOCK(self->lock);
      isConnected = self->is_connected;
      MUTEX_UNLOCK(self->lock);
      if (isConnected)
      {
         apx_client_disconnect(self);
      }
      adt_list_delete(self->event_listeners);
      if (self->connection != 0)
      {
         apx_connectionBase_delete(&self->connection->base);
      }
      if (self->node_manager != 0)
      {
         apx_nodeManager_delete(self->node_manager);
      }
      if (self->vm != 0)
      {
         apx_vm_delete(self->vm);
      }
      MUTEX_DESTROY(self->lock);
      MUTEX_DESTROY(self->event_listener_lock);
   }
}

apx_client_t DLL_PUBLIC *apx_client_new(void)
{
   apx_client_t *self = (apx_client_t*) malloc(sizeof(apx_client_t));
   if(self != NULL)
   {
      apx_error_t result = apx_client_create(self);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void DLL_PUBLIC apx_client_delete(apx_client_t *self)
{
   if (self != NULL)
   {
      apx_client_destroy(self);
      free(self);
   }
}

void DLL_PUBLIC apx_client_vdelete(void *arg)
{
   apx_client_delete((apx_client_t*) arg);
}

#ifdef UNIT_TEST
apx_error_t apx_client_connect_testsocket(apx_client_t *self, struct testsocket_tag *socketObject)
{
   if (self != NULL)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new(socketObject);
      if (socketConnection)
      {
         apx_error_t result;
         apx_client_attach_connection(self, &socketConnection->base);
         result = APX_NO_ERROR;//apx_clientSocketConnection_connect(socketConnection);
         if (result == APX_NO_ERROR)
         {
            MUTEX_LOCK(self->lock);
            self->is_connected = true;
            MUTEX_UNLOCK(self->lock);
            testsocket_onConnect(socketObject);
         }
         return result;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
#else

/**
 * On connection error, the user can retreive the actual error using errno on Linux and WSAGetLastError on Windows
 */
apx_error_t apx_client_connect_tcp(apx_client_t *self, const char *address, uint16_t port)
{
   if (self != NULL)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new((msocket_t*) 0);
      if (socketConnection != 0)
      {
         apx_error_t result;
         apx_client_attachConnection(self, (apx_clientConnection_t*) socketConnection);
         result = apx_clientConnection_tcp_connect(socketConnection, address, port);
         if (result == APX_NO_ERROR)
         {
            MUTEX_LOCK(self->lock);
            self->is_connected = true;
            MUTEX_UNLOCK(self->lock);
         }
         return result;
      }
      else
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

# ifndef _WIN32
apx_error_t apx_client_connect_unix(apx_client_t *self, const char *socketPath)
{
   if (self != NULL)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new((msocket_t*) 0);
      if (socketConnection != 0)
      {
         apx_error_t result;
         apx_client_attachConnection(self, (apx_clientConnection_t*) socketConnection);
         result = apx_clientConnection_unix_connect(socketConnection, socketPath);
         if (result == APX_NO_ERROR)
         {
            MUTEX_LOCK(self->lock);
            self->is_connected = true;
            MUTEX_UNLOCK(self->lock);
         }
         return result;
      }
      else
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
# endif

#endif

void apx_client_disconnect(apx_client_t *self)
{
   if ( (self != NULL) && (self->connection != 0))
   {
      apx_connectionBase_close(&self->connection->base);
      apx_connectionBase_stop(&self->connection->base);
      MUTEX_LOCK(self->lock);
      self->is_connected = false;
      MUTEX_UNLOCK(self->lock);
   }
}


void* apx_client_register_event_listener(apx_client_t *self, struct apx_clientEventListener_tag *listener)
{
   if ( (self != NULL) && (listener != 0))
   {
      void *handle = (void*) apx_clientEventListener_clone(listener);
      if (handle != 0)
      {
         MUTEX_LOCK(self->event_listener_lock);
         adt_list_insert(self->event_listeners, handle);
         MUTEX_UNLOCK(self->event_listener_lock);
      }
      return handle;
   }
   return (void*) 0;
}


void apx_client_unregister_event_listener(apx_client_t *self, void *handle)
{
   if ( (self != NULL) && (handle != 0) )
   {
      bool deleteSuccess = false;
      MUTEX_LOCK(self->event_listener_lock);
      deleteSuccess = adt_list_remove(self->event_listeners, handle);
      MUTEX_UNLOCK(self->event_listener_lock);
      if (deleteSuccess)
      {
         apx_clientEventListener_vdelete(handle);
      }
   }
}

int32_t apx_client_get_num_attached_nodes(apx_client_t *self)
{
   if (self != NULL)
   {
      int32_t retval;
      MUTEX_LOCK(self->lock);
      retval = apx_nodeManager_length(self->node_manager);
      MUTEX_UNLOCK(self->lock);
      return retval;
   }
   return -1;
}

int32_t apx_client_get_num_event_listeners(apx_client_t *self)
{
   if (self != NULL)
   {
      int32_t retval;
      MUTEX_LOCK(self->event_listener_lock);
      retval = adt_list_length(self->event_listeners);
      MUTEX_UNLOCK(self->event_listener_lock);
      return retval;
   }
   return -1;
}

void apx_client_attach_connection(apx_client_t *self, apx_clientConnection_t *connection)
{
   if ( (self != NULL) && (connection != 0) )
   {
      self->connection = connection;
      if (connection->client != self)
      {
         connection->client = self;
      }
      apx_client_attach_local_nodes_to_connection(self);
   }
}

apx_clientConnection_t *apx_client_get_connection(apx_client_t *self)
{
   if (self != NULL)
   {
      return self->connection;
   }
   return (apx_clientConnection_t*) 0;
}

apx_error_t apx_client_build_node(apx_client_t *self, const char *definition_text)
{
   if (self != NULL && definition_text != 0)
   {
      return apx_nodeManager_build_node(self->node_manager, definition_text);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_client_get_last_error_line(apx_client_t *self)
{
   if (self != NULL)
   {
      return -1;
   }
   return -1;
}

apx_nodeInstance_t *apx_client_get_last_attached_node(apx_client_t *self)
{
   if (self != NULL)
   {
      return apx_nodeManager_get_last_attached(self->node_manager);
   }
   return (apx_nodeInstance_t*) 0;
}

struct apx_fileManager_tag *apx_client_get_file_manager(apx_client_t *self)
{
   if ( (self != NULL) && (self->connection != NULL))
   {
      return &self->connection->base.file_manager;
   }
   return (apx_fileManager_t*) 0;
}

struct apx_nodeManager_tag *apx_client_get_node_manager(apx_client_t *self)
{
   if (self != NULL)
   {
      return self->node_manager;
   }
   return (apx_nodeManager_t*) 0;
}

/*** Port Handle API ***/
void *apx_client_get_port_handle(apx_client_t *self, const char *nodeName, const char *portName)
{
   if ( (self != NULL) && (portName != NULL))
   {
      apx_nodeInstance_t *nodeInstance = 0;
      if (nodeName == 0)
      {
         nodeInstance = apx_client_get_last_attached_node(self);
      }
      else
      {
         nodeInstance = apx_nodeManager_find(self->node_manager, nodeName);
      }
      if (nodeInstance != 0)
      {
         return (void*) apx_nodeInstance_find_port_by_name(nodeInstance, portName);
      }
   }
   return NULL;
}

void *apx_client_get_provide_port_handle_by_id(apx_client_t *self, const char *nodeName, apx_portId_t providePortId)
{
   if ( (self != NULL) && (providePortId >= 0))
   {
      apx_nodeInstance_t *nodeInstance = 0;
      if (nodeName == 0)
      {
         nodeInstance = apx_client_get_last_attached_node(self);
      }
      else
      {
         nodeInstance = apx_nodeManager_find(self->node_manager, nodeName);
      }
      if (nodeInstance != 0)
      {
         return (void*)apx_nodeInstance_get_provide_port(nodeInstance, providePortId);
      }
   }
   return (void*) NULL;
}

void *apx_client_get_require_port_handle_by_id(apx_client_t *self, const char *nodeName, apx_portId_t requirePortId)
{
   if ( (self != NULL) && (requirePortId >= 0))
   {
      apx_nodeInstance_t *nodeInstance = 0;
      if (nodeName == 0)
      {
         nodeInstance = apx_client_get_last_attached_node(self);
      }
      else
      {
         nodeInstance = apx_nodeManager_find(self->node_manager, nodeName);
      }
      if (nodeInstance != 0)
      {
         return (void*)apx_nodeInstance_get_require_port(nodeInstance, requirePortId);
      }
   }
   return (void*) 0;
}
/////////////////////// BEGIN CLIENT INTERNAL API /////////////////////
void apx_clientInternal_onConnect(apx_client_t *self, apx_clientConnection_t *connection)
{
   if ( (self != NULL) && (connection != 0) )
   {
      apx_client_trigger_connected_event_on_listeners(self, connection);
   }
}

void apx_clientInternal_onDisconnect(apx_client_t *self, apx_clientConnection_t *connection)
{
   if ( (self != NULL) && (connection != 0) )
   {
      apx_client_trigger_disconnected_event_on_listeners(self, connection);
   }
}

void apx_clientInternal_requirePortDataWriteNotify(apx_client_t* self, apx_clientConnection_t* connection, apx_nodeInstance_t* nodeInstance, uint32_t offset, const uint8_t* data, uint32_t len)
{
   (void)self;
   (void)connection;
   (void)nodeInstance;
   (void)offset;
   (void)data;
   (void)len;
}

/////////////////////// END CLIENT INTERNAL API /////////////////////

/////////////////////// BEGIN UNIT TEST API /////////////////////
#ifdef UNIT_TEST

#define APX_CLIENT_RUN_CYCLES 10

void apx_client_run(apx_client_t *self)
{
   if (self!=0 && (self->connection != 0))
   {
      int32_t i;
      for(i=0;i<APX_CLIENT_RUN_CYCLES;i++)
      {
         apx_clientConnection_run(self->connection);
      }
   }
}
#endif

/////////////////////// END UNIT TEST API /////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_client_trigger_connected_event_on_listeners(apx_client_t *self, apx_clientConnection_t *connection)
{
   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(self->event_listeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->clientConnect1 != 0))
      {
         listener->clientConnect1(listener->arg, connection);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);
}

static void apx_client_trigger_disconnected_event_on_listeners(apx_client_t *self, apx_clientConnection_t *connection)
{
   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(self->event_listeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->clientDisconnect1 != 0))
      {
         listener->clientDisconnect1(listener->arg, connection);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);
}
/*
static void apx_client_triggerRequirePortDataWriteEventOnListeners(apx_client_t *self, apx_nodeInstance_t *nodeInstance, apx_portId_t requirePortId, void *portHandle)
{
   MUTEX_LOCK(self->event_listener_lock);
   adt_list_elem_t *iter = adt_list_iter_first(self->event_listeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->requirePortWrite1 != 0))
      {
         listener->requirePortWrite1(listener->arg, nodeInstance, requirePortId, portHandle);
      }
      iter = adt_list_iter_next(iter);
   }
   MUTEX_UNLOCK(self->event_listener_lock);
}
*/

static void apx_client_attach_local_nodes_to_connection(apx_client_t *self)
{
   if (self->connection != 0)
   {
      adt_ary_t *nodeList = adt_ary_new( (void(*)(void*)) 0);
      if (nodeList != 0)
      {
         int32_t i;
         int32_t numNodes;
         numNodes = apx_nodeManager_values(self->node_manager, nodeList);
         for (i=0; i<numNodes; i++)
         {
            apx_nodeInstance_t *nodeInstance = (apx_nodeInstance_t*) adt_ary_value(nodeList, i);
            apx_clientConnection_attach_node_instance(self->connection, nodeInstance);
         }
         adt_ary_delete(nodeList);
      }
   }
}




