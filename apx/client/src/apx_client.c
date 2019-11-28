/*****************************************************************************
* \file      apx_client.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX client class
*
* Copyright (c) 2017-2018 Conny Gustafsson
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
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "apx_client.h"
#include "apx_clientInternal.h"
#include "apx_clientConnectionBase.h"
#include "apx_clientSocketConnection.h"
#include "apx_nodeManager.h"
#include "apx_fileManager2.h"
#include "apx_parser.h"
#include "apx_nodeInstance.h"
#include "msocket.h"
#include "adt_ary.h"
#include "adt_list.h"
#include "adt_hash.h"
#include "apx_eventListener2.h"
#include "apx_compiler.h"

#ifdef UNIT_TEST
#include "testsocket.h"
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_client_triggerConnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection);
static void apx_client_triggerDisconnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection);
static void apx_client_attachLocalNodesToConnection(apx_client_t *self);
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
   if( self != 0 )
   {
      self->eventListeners = adt_list_new(apx_clientEventListener2_vdelete);
      if (self->eventListeners == 0)
      {
         return APX_MEM_ERROR;
      }
      self->connection = (apx_clientConnectionBase_t*) 0;
      self->parser = (apx_parser_t*) 0;
      //The node manager in this class is the true manager of the nodeInstances. Therefore we set useWeakRef argument to false.
      self->nodeManager = apx_nodeManager_new(APX_CLIENT_MODE, false);
      SPINLOCK_INIT(self->lock);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_client_destroy(apx_client_t *self)
{
   if (self != 0)
   {
      adt_list_delete(self->eventListeners);
      if (self->connection != 0)
      {
         apx_connectionBase_delete(&self->connection->base);
      }
      if (self->nodeManager != 0)
      {
         apx_nodeManager_delete(self->nodeManager);
      }
      if (self->parser != 0)
      {
         apx_parser_delete(self->parser);
      }
      SPINLOCK_DESTROY(self->lock);
   }
}

apx_client_t *apx_client_new(void)
{
   apx_client_t *self = (apx_client_t*) malloc(sizeof(apx_client_t));
   if(self != 0)
   {
      int8_t result = apx_client_create(self);
      if (result != 0)
      {
         free(self);
         self=0;
      }
   }
   return self;
}

void apx_client_delete(apx_client_t *self)
{
   if (self != 0)
   {
      apx_client_destroy(self);
      free(self);
   }
}

void apx_client_vdelete(void *arg)
{
   apx_client_delete((apx_client_t*) arg);
}

#ifdef UNIT_TEST
apx_error_t apx_client_socketConnect(apx_client_t *self, struct testsocket_tag *socketObject)
{
   if (self != 0)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new(socketObject);
      if (socketConnection)
      {
         apx_error_t result;
         self->connection = (apx_clientConnectionBase_t*) socketConnection;
         result = apx_clientSocketConnection_connect(socketConnection);
         if (result == APX_NO_ERROR)
         {
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
apx_error_t apx_client_connectTcp(apx_client_t *self, const char *address, uint16_t port)
{
   if (self != 0)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new((msocket_t*) 0, self);
      if (socketConnection != 0)
      {
         self->connection = (apx_clientConnectionBase_t*)socketConnection;
         apx_client_updateBaseConnectionOnNodes(self);
         return apx_clientConnection_tcp_connect(socketConnection, address, port);
      }
      else
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

# ifndef _WIN32
apx_error_t apx_client_connectUnix(apx_client_t *self, const char *socketPath)
{
   if (self != 0)
   {
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new((msocket_t*) 0, self);
      if (socketConnection != 0)
      {
         self->connection = (apx_clientConnectionBase_t*)socketConnection;
         apx_client_updateBaseConnectionOnNodes(self);
         return apx_clientConnection_unix_connect(socketConnection, socketPath);
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
   if ( (self != 0) && (self->connection != 0))
   {
      apx_connectionBase_close(&self->connection->base);
   }
}


void* apx_client_registerEventListener(apx_client_t *self, struct apx_clientEventListener2_tag *listener)
{
   if ( (self != 0) && (listener != 0))
   {
      void *handle = (void*) apx_clientEventListener2_clone(listener);
      if (handle != 0)
      {
         SPINLOCK_ENTER(self->lock);
         adt_list_insert(self->eventListeners, handle);
         SPINLOCK_LEAVE(self->lock);
      }
      return handle;
   }
   return (void*) 0;
}


void apx_client_unregisterEventListener(apx_client_t *self, void *handle)
{
   if ( (self != 0) && (handle != 0) )
   {
      bool deleteSuccess = false;
      SPINLOCK_ENTER(self->lock);
      deleteSuccess = adt_list_remove(self->eventListeners, handle);
      SPINLOCK_LEAVE(self->lock);
      if (deleteSuccess)
      {
         apx_clientEventListener2_vdelete(handle);
      }
   }
}

int32_t apx_client_getNumAttachedNodes(apx_client_t *self)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->lock);
      retval = apx_nodeManager_length(self->nodeManager);
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return -1;
}

int32_t apx_client_getNumEventListeners(apx_client_t *self)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->lock);
      retval = adt_list_length(self->eventListeners);
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return -1;
}

void apx_client_attachConnection(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      self->connection = connection;
      if (connection->client != self)
      {
         connection->client = self;
      }
      apx_client_attachLocalNodesToConnection(self);
   }
}

apx_clientConnectionBase_t *apx_client_getConnection(apx_client_t *self)
{
   if (self != 0)
   {
      return self->connection;
   }
   return (apx_clientConnectionBase_t*) 0;
}

apx_error_t apx_client_buildNode_cstr(apx_client_t *self, const char *definition_text)
{
   if (self != 0 && definition_text != 0)
   {
      return apx_nodeManager_buildNode_cstr(self->nodeManager, definition_text);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_nodeInstance_t *apx_client_getLastAttachedNode(apx_client_t *self)
{
   if (self != 0)
   {
      return apx_nodeManager_getLastAttached(self->nodeManager);
   }
   return (apx_nodeInstance_t*) 0;
}

struct apx_fileManager2_tag *apx_client_getFileManager(apx_client_t *self)
{
   if (self != 0 && self->connection != 0)
   {
      return &self->connection->base.fileManager;
   }
   return (apx_fileManager2_t*) 0;
}

struct apx_nodeManager_tag *apx_client_getNodeManager(apx_client_t *self)
{
   if (self != 0 && self->connection != 0)
   {
      return &self->connection->base.nodeManager;
   }
   return (apx_nodeManager_t*) 0;
}

/////////////////////// BEGIN CLIENT INTERNAL API /////////////////////
void apx_clientInternal_onConnect(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      apx_client_triggerConnectedEventOnListeners(self, connection);
   }
}

void apx_clientInternal_onDisconnect(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      apx_client_triggerDisconnectedEventOnListeners(self, connection);
   }
}
/*
void apx_clientInternal_onNodeComplete(apx_client_t *self, apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      apx_client_triggerNodeCompleteEvent(self, nodeData);
   }
}
*/

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
         apx_clientConnectionBase_run(self->connection);
      }
   }
}
#endif

/////////////////////// END UNIT TEST API /////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_client_triggerConnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   adt_list_elem_t *iter = adt_list_iter_first(self->eventListeners);
   while(iter != 0)
   {
      apx_clientEventListener2_t *listener = (apx_clientEventListener2_t*) iter->pItem;
      if ( (listener != 0) && (listener->clientConnect2 != 0))
      {
         listener->clientConnect2(listener->arg, connection);
      }
      iter = adt_list_iter_next(iter);
   }
}

static void apx_client_triggerDisconnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   adt_list_elem_t *iter = adt_list_iter_first(self->eventListeners);
   while(iter != 0)
   {
      apx_clientEventListener2_t *listener = (apx_clientEventListener2_t*) iter->pItem;
      if ( (listener != 0) && (listener->clientDisconnect2 != 0))
      {
         listener->clientDisconnect2(listener->arg, connection);
      }
      iter = adt_list_iter_next(iter);
   }
}
/*
static void apx_client_triggerNodeCompleteEvent(apx_client_t *self, apx_nodeData_t *nodeData)
{
   adt_list_elem_t *iter = adt_list_iter_first(self->eventListeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->nodeCompleted != 0))
      {
         listener->nodeCompleted(listener->arg, nodeData);
      }
      iter = adt_list_iter_next(iter);
   }
}
*/

static void apx_client_attachLocalNodesToConnection(apx_client_t *self)
{
   if (self->connection != 0)
   {
      adt_ary_t *nodeList = adt_ary_new( (void(*)(void*)) 0);
      if (nodeList != 0)
      {
         int32_t i;
         int32_t numNodes;
         numNodes = apx_nodeManager_values(self->nodeManager, nodeList);
         for (i=0; i<numNodes; i++)
         {
            apx_nodeInstance_t *nodeInstance = (apx_nodeInstance_t*) adt_ary_value(nodeList, i);
            apx_clientConnectionBase_attachNodeInstance(self->connection, nodeInstance);
         }
         adt_ary_delete(nodeList);
      }
   }
}

