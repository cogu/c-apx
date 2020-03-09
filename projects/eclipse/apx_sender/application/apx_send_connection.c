/*****************************************************************************
* \file      apx_send_connection.c
* \author    Conny Gustafsson
* \date      2020-03-09
* \brief     APX client connection for sending dynamic values to APX server
*
* Copyright (c) 2020 Conny Gustafsson
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
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "apx_send_connection.h"
#include "apx_eventListener.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_send_connection_onDisconnect(void *arg, apx_clientConnectionBase_t *clientConnection);
//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_send_connection_create(apx_send_connection_t *self)
{
   if (self != 0)
   {
      apx_clientEventListener_t listener;
      self->apx_definition = (adt_bytearray_t*) 0;
      self->client = apx_client_new();
      if (self->client == 0)
      {
         return APX_MEM_ERROR;
      }
      memset(&listener, 0, sizeof(listener));
      listener.arg = (void*) self;
      listener.clientDisconnect1 = apx_send_connection_onDisconnect;
      apx_client_registerEventListener(self->client, &listener);
      adt_hash_create(&self->portLookupTable, (void (*)(void*)) 0);
      MUTEX_INIT(self->mutex);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_send_connection_destroy(apx_send_connection_t *self)
{
   if (self != 0)
   {
      MUTEX_LOCK(self->mutex);
      if (self->apx_definition != 0)
      {
         adt_bytearray_delete(self->apx_definition);
      }
      if (self->client != 0)
      {
         apx_client_delete(self->client);
      }
      adt_hash_destroy(&self->portLookupTable);
      MUTEX_UNLOCK(self->mutex);
      MUTEX_DESTROY(self->mutex);
   }
}

apx_send_connection_t *apx_send_connection_new(void)
{
   apx_send_connection_t *self = (apx_send_connection_t*) malloc(sizeof(apx_send_connection_t));
   if (self != 0)
   {
      apx_error_t rc = apx_send_connection_create(self);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = (apx_send_connection_t*) 0;
      }
   }
   return self;
}

void apx_send_connection_delete(apx_send_connection_t *self)
{
   if (self != 0)
   {
      apx_send_connection_destroy(self);
      free(self);
   }
}

apx_error_t apx_send_connection_attachNode(apx_send_connection_t *self, adt_str_t *apx_definition)
{
   if ( (self != 0) && (apx_definition != 0) )
   {
      return apx_client_buildNode_cstr(self->client, adt_str_cstr(apx_definition));
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_send_connection_getLastErrorLine(apx_send_connection_t *self)
{
   if (self != 0)
   {
      return apx_client_getLastErrorLine(self->client);
   }
   return -1;
}

apx_nodeInstance_t *apx_send_connection_getLastAttachedNode(apx_send_connection_t *self)
{
   if (self != 0)
   {
      return apx_client_getLastAttachedNode(self->client);
   }
   return (apx_nodeInstance_t*) 0;
}

apx_error_t apx_send_connection_prepareProvidePorts(apx_send_connection_t *self)
{
   apx_error_t retval = APX_NO_ERROR;
   if (self != 0)
   {
      apx_nodeInstance_t *nodeInstance;
      apx_portCount_t numProvidePorts;
      assert(self->client != 0);
      MUTEX_LOCK(self->mutex);
      nodeInstance = apx_client_getLastAttachedNode(self->client);
      if (nodeInstance != 0)
      {
         apx_portId_t portId;
         numProvidePorts = apx_nodeInstance_getNumProvidePorts(nodeInstance);
         for(portId = 0; portId < numProvidePorts; portId++)
         {
            adt_str_t *portName;
            void *portHandle = apx_nodeInstance_getProvidePortHandle(nodeInstance, portId);
            portName = apx_nodeInstance_getProvidePortName(nodeInstance, portId);
            if ( (portName == 0) || (portHandle == 0) )
            {
              retval = APX_NULL_PTR_ERROR;
              break;
            }
            adt_hash_set(&self->portLookupTable, adt_str_cstr(portName), portHandle);
            adt_str_delete(portName);
         }
      }
      else
      {
         retval = APX_NULL_PTR_ERROR;
      }
      MUTEX_UNLOCK(self->mutex);
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

apx_error_t apx_send_connection_connect_unix(apx_send_connection_t *self, const char *socketPath)
{
   if (self != 0)
   {
      return apx_client_connect_unix(self->client, socketPath);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_send_connection_writeProvidePortData(apx_send_connection_t *self, const char *providePortName, dtl_dv_t *dv_value)
{
   if ( (self != 0) && (providePortName != 0) && (dv_value != 0) )
   {
      apx_error_t result;
      MUTEX_LOCK(self->mutex);
      void *portHandle = adt_hash_value(&self->portLookupTable, providePortName);
      if (portHandle == 0)
      {
         MUTEX_UNLOCK(self->mutex);
         return APX_INVALID_NAME_ERROR;
      }
      result = apx_client_writePortData(self->client, portHandle, dv_value);
      MUTEX_UNLOCK(self->mutex);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_send_connection_onDisconnect(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   printf("Disconnected from APX server\n");
}
