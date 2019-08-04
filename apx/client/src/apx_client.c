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
#include "apx_clientConnectionBase.h"
#include "apx_clientSocketConnection.h"
#include "apx_nodeDataManager.h"
#include "apx_fileManager.h"
#include "apx_parser.h"
#include "msocket.h"
#include "adt_ary.h"
#include "adt_list.h"
#include "apx_nodeData.h"
#include "apx_eventListener.h"
#include "apx_nodeInfo.h"
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
static void apx_client_triggerNodeCompleteEvent(apx_client_t *self, apx_nodeData_t *nodeData);
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
      self->eventListeners = adt_list_new((void (*)(void*)) 0);
      if (self->eventListeners == 0)
      {
         return APX_MEM_ERROR;
      }
      self->connection = (apx_clientConnectionBase_t*) 0;
      self->nodeDataList = (adt_ary_t*) 0;
      self->nodeInfoList = (adt_ary_t*) 0;
      self->parser = (apx_parser_t*) 0;
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
      if (self->nodeDataList != 0)
      {
         adt_ary_delete(self->nodeDataList);
      }
      if (self->nodeInfoList != 0)
      {
         adt_ary_delete(self->nodeInfoList);
      }
      if (self->parser != 0)
      {
         apx_parser_delete(self->parser);
      }
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
      apx_clientSocketConnection_t *socketConnection = apx_clientSocketConnection_new(socketObject, self);
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

/**
 * attached the nodeData to the local nodeManager in the client
 */
apx_error_t apx_client_attachLocalNode(apx_client_t *self, struct apx_nodeData_tag *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      if (self->nodeDataList == 0)
      {
         self->nodeDataList = adt_ary_new((void (*)(void*)) 0);
      }
      if (self->nodeDataList != 0)
      {
         adt_ary_push(self->nodeDataList, nodeData);
         return APX_NO_ERROR;
      }
      return APX_MEM_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_attachLocalNodeFromString(apx_client_t *self, const char *apx_text)
{
   if ( (self != 0) && (apx_text != 0) )
   {
      apx_nodeInfo_t *nodeInfo;
      if (self->parser == 0)
      {
         self->parser = apx_parser_new();
      }
      if (self->parser == 0)
      {
         return APX_MEM_ERROR;
      }
      if (self->nodeInfoList == 0)
      {
         self->nodeInfoList = adt_ary_new(apx_nodeInfo_vdelete);
      }
      if (self->nodeInfoList == 0)
      {
         return APX_MEM_ERROR;
      }
      nodeInfo = apx_nodeInfo_new();
      if (nodeInfo == 0)
      {
         return APX_MEM_ERROR;
      }
      else
      {
         apx_error_t errorCode = apx_nodeInfo_updateFromString(nodeInfo, self->parser, apx_text);
         if (errorCode == APX_NO_ERROR)
         {
            adt_ary_push(self->nodeInfoList, nodeInfo);
         }
         return errorCode;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_client_registerEventListener(apx_client_t *self, struct apx_clientEventListener_tag *eventListener)
{
   if (self != 0)
   {
      adt_list_insert_unique(self->eventListeners, eventListener);
   }
}

//Client internal API
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

apx_error_t apx_clientInternal_attachLocalNodes(apx_client_t *self, struct apx_nodeDataManager_tag *nodeDataManager)
{
   if ( (self != 0) && (nodeDataManager != 0) )
   {
      int32_t i;
      int32_t len;
      if (self->nodeInfoList != 0)
      {
         len = adt_ary_length(self->nodeInfoList);
         for (i=0;i<len;i++)
         {
            apx_error_t errorCode;
            apx_nodeInfo_t *nodeInfo = (apx_nodeInfo_t*) adt_ary_value(self->nodeInfoList, i);
            assert(nodeInfo != 0);
            errorCode = apx_nodeDataManager_attachFromString(nodeDataManager, nodeInfo->text);
            if (errorCode != APX_NO_ERROR)
            {
               return errorCode;
            }
         }
      }
      if (self->nodeDataList != 0)
      {
         len = adt_ary_length(self->nodeDataList);
         for (i=0;i<len;i++)
         {
            apx_error_t errorCode;
            apx_nodeData_t *nodeData = adt_ary_value(self->nodeDataList, i);
            errorCode = apx_nodeDataManager_attach(nodeDataManager, nodeData);
            if (errorCode != APX_NO_ERROR)
            {
               return errorCode;
            }
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_clientInternal_onNodeComplete(apx_client_t *self, apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      apx_client_triggerNodeCompleteEvent(self, nodeData);
   }
}

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

int32_t apx_client_getNumLocalNodes(apx_client_t *self)
{
   if (self != 0)
   {
      int32_t result = 0;
      if (self->nodeDataList != 0)
      {
         result+=adt_ary_length(self->nodeDataList);
      }
      if(self->nodeInfoList != 0)
      {
         result+=adt_ary_length(self->nodeInfoList);
      }
      return result;
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


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_client_triggerConnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   adt_list_elem_t *iter = adt_list_iter_first(self->eventListeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->clientConnected != 0))
      {
         listener->clientConnected(listener->arg, connection);
      }
      iter = adt_list_iter_next(iter);
   }
}

static void apx_client_triggerDisconnectedEventOnListeners(apx_client_t *self, apx_clientConnectionBase_t *connection)
{
   adt_list_elem_t *iter = adt_list_iter_first(self->eventListeners);
   while(iter != 0)
   {
      apx_clientEventListener_t *listener = (apx_clientEventListener_t*) iter->pItem;
      if ( (listener != 0) && (listener->clientDisconnected != 0))
      {
         listener->clientDisconnected(listener->arg, connection);
      }
      iter = adt_list_iter_next(iter);
   }
}

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
