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
#include "apx_nodeDataManager.h"
#include "apx_fileManager.h"
#include "apx_parser.h"
#include "msocket.h"
#include "adt_ary.h"
#include "adt_list.h"
#include "adt_hash.h"
#include "apx_nodeData.h"
#include "apx_eventListener.h"
#include "apx_compiler.h"
#include "apx_portDataMap.h"

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
static apx_error_t apx_client_compilePortPrograms(apx_client_t *self, apx_nodeData_t *nodeData, apx_uniquePortId_t *errPortId);
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
      self->nodeDataMap = adt_hash_new((void (*)(void*)) 0);
      self->nodeDataList = (adt_ary_t*) 0;
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
      if (self->nodeDataMap != 0)
      {
         adt_hash_delete(self->nodeDataMap);
      }
      if (self->nodeDataList != 0)
      {
         adt_ary_delete(self->nodeDataList);
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
 * Attach existing nodeData object to this client
 */
apx_error_t apx_client_attachLocalNode(apx_client_t *self,  apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      assert(self->nodeDataMap != 0);
      const char *name = apx_nodeData_getName(nodeData);
      if (name != 0)
      {
         adt_hash_set(self->nodeDataMap, name, nodeData);
         return APX_NO_ERROR;
      }
      else
      {
         return APX_NAME_MISSING_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_client_createLocalNode_cstr(apx_client_t *self, const char *apx_text)
{
   if ( (self != 0) && (apx_text != 0) )
   {
      apx_error_t result = APX_NO_ERROR;
      apx_nodeData_t *nodeData;
      if (self->parser == 0)
      {
         self->parser = apx_parser_new();
      }
      if (self->parser == 0)
      {
         return APX_MEM_ERROR;
      }
      if (self->nodeDataList == 0)
      {
         self->nodeDataList = adt_ary_new(apx_nodeData_vdelete);
      }
      if (self->nodeDataList == 0)
      {
         return APX_MEM_ERROR;
      }
      nodeData = apx_nodeData_makeFromString(self->parser, apx_text, &result);
      if ( (nodeData == 0) || (result != APX_NO_ERROR) )
      {
         if (nodeData != 0)
         {
            apx_nodeData_delete(nodeData);
         }
         return result;
      }
      else
      {
         nodeData->isDynamic = true;
         if (nodeData->portDataMap == 0)
         {
            result = apx_nodeData_createPortDataMap(nodeData, APX_CLIENT_MODE);
            if (result != APX_NO_ERROR)
            {
               apx_nodeData_delete(nodeData);
               return result;
            }
            else
            {
               apx_uniquePortId_t errPortId = 0;
               result = apx_client_compilePortPrograms(self, nodeData, &errPortId);
               if (result != APX_NO_ERROR)
               {
                  if (result == APX_MEM_ERROR)
                  {
                     fprintf(stderr, "%s: Compile MEM_ERROR\n", apx_nodeData_getName(nodeData));
                  }
                  else
                  {
                     int32_t portId = (int32_t) (errPortId & APX_PORT_ID_MASK);
                     const char *portType = (errPortId & APX_PORT_ID_PROVIDE_PORT)? "P" : "R";
                     fprintf(stderr, "%s.%s[%d]: Compile error %d\n", apx_nodeData_getName(nodeData), portType, (int) portId, (int) result);
                  }
                  apx_nodeData_delete(nodeData);
               }
               else
               {
                  adt_error_t rc;
                  rc = adt_ary_push(self->nodeDataList, nodeData);
                  if (rc != ADT_NO_ERROR)
                  {
                     apx_nodeData_delete(nodeData);
                     return (apx_error_t) rc;
                  }
                  result = apx_client_attachLocalNode(self, nodeData);
               }
               return result;
            }
         }
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

apx_error_t apx_clientInternal_attachLocalNodes(apx_client_t *self, apx_nodeDataManager_t *nodeDataManager)
{
   if ( (self != 0) && (nodeDataManager != 0) )
   {
      int32_t i;
      int32_t numItems;
      adt_ary_t values;

      adt_ary_create(&values, (void (*)(void*)) 0);
      numItems = adt_hash_values(self->nodeDataMap, &values);
      for (i=0; i< numItems; i++)
      {
         apx_error_t errorCode;
         apx_nodeData_t *nodeData = (apx_nodeData_t*) adt_ary_value(&values, i);
         assert(nodeData != 0);
         errorCode = apx_nodeDataManager_attach(nodeDataManager, nodeData);
         if (errorCode != APX_NO_ERROR)
         {
            adt_ary_destroy(&values);
            return errorCode;
         }
      }
      adt_ary_destroy(&values);
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

int32_t apx_client_getNumAttachedNodes(apx_client_t *self)
{
   if (self != 0)
   {
      return adt_hash_length(self->nodeDataMap);
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

apx_nodeData_t *apx_client_getDynamicNode(apx_client_t *self, int32_t index)
{
   if (self != 0)
   {
      void **ptr = adt_ary_get(self->nodeDataList, index);
      if (ptr != 0)
      {
         return (apx_nodeData_t*) *ptr;
      }
   }
   return (apx_nodeData_t*) 0;
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

static apx_error_t apx_client_compilePortPrograms(apx_client_t *self, apx_nodeData_t *nodeData, apx_uniquePortId_t *errPortId)
{
   bool usePackPrograms = true;
   bool useUnpackPrograms = false;
   apx_error_t retval = APX_NO_ERROR;
   apx_portDataMap_t *portDataMap;
   apx_node_t *node;
   apx_compiler_t compiler;
   node = apx_nodeData_getNode(nodeData);
   portDataMap = apx_nodeData_getPortDataMap(nodeData);


   if ( (node == 0) || (portDataMap == 0) )
   {
      return APX_NULL_PTR_ERROR;
   }
   apx_compiler_create(&compiler);
   if(usePackPrograms)
   {
      retval = apx_portDataMap_createPackPrograms(portDataMap, &compiler, node, errPortId);
   }
   if( (retval == APX_NO_ERROR) && (useUnpackPrograms) )
   {
      //retval = apx_portDataMap_createUnpackPrograms(portDataMap, &compiler, node, errPortId);
      retval = APX_NO_ERROR;
   }
   apx_compiler_destroy(&compiler);
   return retval;
}
