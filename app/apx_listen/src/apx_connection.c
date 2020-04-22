/*****************************************************************************
* \file      apx_connection.c
* \author    Conny Gustafsson
* \date      2020-04-14
* \brief     APX client connection
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
#include "apx_connection.h"
#include "apx_eventListener.h"
#include "dtl_json.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_connection_onConnect(void *arg, apx_clientConnectionBase_t *clientConnection);
static void apx_connection_onDisconnect(void *arg, apx_clientConnectionBase_t *clientConnection);
static void apx_connection_onRequirePortWrite(void *arg, apx_nodeInstance_t *nodeInstance, apx_portId_t requirePortId, void *portHandle);
static apx_error_t apx_connection_prepareProvidePorts(apx_connection_t *self, apx_nodeInstance_t *nodeInstance);
static apx_error_t apx_connection_prepareRequirePorts(apx_connection_t *self, apx_nodeInstance_t *nodeInstance);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_connection_create(apx_connection_t *self)
{
   if (self != 0)
   {
      apx_clientEventListener_t listener;
      self->client = apx_client_new();
      if (self->client == 0)
      {
         return APX_MEM_ERROR;
      }
      memset(&listener, 0, sizeof(listener));
      listener.arg = (void*) self;
      listener.clientConnect1 = apx_connection_onConnect;
      listener.clientDisconnect1 = apx_connection_onDisconnect;
      listener.requirePortWrite1 = apx_connection_onRequirePortWrite;
      apx_client_registerEventListener(self->client, &listener);
      adt_hash_create(&self->providePortLookupTable, (void (*)(void*)) 0);
      adt_ary_create(&self->requirePortLookupTable, (void (*)(void*)) 0);
      adt_ary_create(&self->requirePortNames, adt_str_vdelete);
      MUTEX_INIT(self->mutex);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_connection_destroy(apx_connection_t *self)
{
   if (self != 0)
   {
      MUTEX_LOCK(self->mutex);
      if (self->client != 0)
      {
         apx_client_delete(self->client);
      }
      adt_hash_destroy(&self->providePortLookupTable);
      adt_ary_destroy(&self->requirePortLookupTable);
      adt_ary_destroy(&self->requirePortNames);
      MUTEX_UNLOCK(self->mutex);
      MUTEX_DESTROY(self->mutex);
   }
}

apx_connection_t *apx_connection_new(void)
{
   apx_connection_t *self = (apx_connection_t*) malloc(sizeof(apx_connection_t));
   if (self != 0)
   {
      apx_error_t rc = apx_connection_create(self);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = (apx_connection_t*) 0;
      }
   }
   return self;
}

void apx_connection_delete(apx_connection_t *self)
{
   if (self != 0)
   {
      apx_connection_destroy(self);
      free(self);
   }
}

void apx_connection_disconnect(apx_connection_t *self)
{
   if (self != 0)
   {
      apx_client_disconnect(self->client);
   }
}

apx_error_t apx_connection_attachNode(apx_connection_t *self, adt_str_t *apx_definition)
{
   if ( (self != 0) && (apx_definition != 0) )
   {
      MUTEX_LOCK(self->mutex);
      apx_error_t retval = apx_client_buildNode_cstr(self->client, adt_str_cstr(apx_definition));
      if (retval == APX_NO_ERROR)
      {
         apx_nodeInstance_t *nodeInstance = apx_client_getLastAttachedNode(self->client);
         if (nodeInstance != 0)
         {
            retval = apx_connection_prepareProvidePorts(self, nodeInstance);
            if (retval == APX_NO_ERROR)
            {
               retval = apx_connection_prepareRequirePorts(self, nodeInstance);
            }
         }
         else
         {
            retval = APX_NULL_PTR_ERROR;
         }
      }
      MUTEX_UNLOCK(self->mutex);
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_connection_getLastErrorLine(apx_connection_t *self)
{
   if (self != 0)
   {
      return apx_client_getLastErrorLine(self->client);
   }
   return -1;
}

apx_nodeInstance_t *apx_connection_getLastAttachedNode(apx_connection_t *self)
{
   if (self != 0)
   {
      return apx_client_getLastAttachedNode(self->client);
   }
   return (apx_nodeInstance_t*) 0;
}

apx_error_t apx_connection_connect_unix(apx_connection_t *self, const char *socketPath)
{
   if (self != 0)
   {
      return apx_client_connect_unix(self->client, socketPath);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_connection_connect_tcp(apx_connection_t *self, const char *address, uint16_t port)
{
   if (self != 0)
   {
      return apx_client_connect_tcp(self->client, address, port);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_connection_writeProvidePortData(apx_connection_t *self, const char *providePortName, dtl_dv_t *dv_value)
{
   if ( (self != 0) && (providePortName != 0) && (dv_value != 0) )
   {
      apx_error_t result;
      MUTEX_LOCK(self->mutex);
      void *portHandle = adt_hash_value(&self->providePortLookupTable, providePortName);
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
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_connection_onConnect(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   printf("[APX-CONNECTION] connected to APX server\n");
}

static void apx_connection_onDisconnect(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   printf("[APX-CONNECTION] Disconnected from APX server\n");
}

static void apx_connection_onRequirePortWrite(void *arg, apx_nodeInstance_t *nodeInstance, apx_portId_t requirePortId, void *portHandle)
{
   apx_connection_t *self = (apx_connection_t*) arg;
   if (self != 0)
   {
      apx_error_t result;
      adt_str_t *port_name;
      dtl_dv_t *dv = 0;
      MUTEX_LOCK(self->mutex);
      result = apx_client_readPortData(self->client, portHandle, &dv);
      if (result != APX_NO_ERROR)
      {
         printf("apx_client_readPortData failed with error code %d\n", (int) result);
         return;
      }
      port_name = apx_nodeInstance_getRequirePortName(nodeInstance, requirePortId);
      MUTEX_UNLOCK(self->mutex);
      if ( (dv != 0) && (port_name != 0) )
      {
         adt_str_t *value = dtl_json_dumps(dv, 0, false);
         if (value != 0)
         {
            printf("%s: %s\n", adt_str_cstr(port_name), adt_str_cstr(value));
            fflush(stdout);
            adt_str_delete(value);
         }
      }
      if (dv != 0)
      {
         dtl_dec_ref(dv);
      }
      if (port_name != 0)
      {
         adt_str_delete(port_name);
      }
   }
}

static apx_error_t apx_connection_prepareProvidePorts(apx_connection_t *self, apx_nodeInstance_t *nodeInstance)
{
   apx_error_t retval = APX_NO_ERROR;
   if ( (self != 0) && (nodeInstance != 0) )
   {
      apx_portCount_t numProvidePorts;
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
         adt_hash_set(&self->providePortLookupTable, adt_str_cstr(portName), portHandle);
         adt_str_delete(portName);
      }
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

static apx_error_t apx_connection_prepareRequirePorts(apx_connection_t *self, apx_nodeInstance_t *nodeInstance)
{
   apx_error_t retval = APX_NO_ERROR;
   if ( (self != 0) && (nodeInstance != 0) )
   {
      apx_portCount_t numRequirePorts;
      numRequirePorts = apx_nodeInstance_getNumRequirePorts(nodeInstance);
      if (numRequirePorts > 0)
      {
         adt_error_t rc;
         rc = adt_ary_resize(&self->requirePortLookupTable, (int32_t) numRequirePorts);
         if (rc != ADT_NO_ERROR)
         {
            if (rc == ADT_MEM_ERROR)
            {
               retval = APX_MEM_ERROR;
            }
            else
            {
               retval = APX_GENERIC_ERROR;
            }
         }
         rc = adt_ary_resize(&self->requirePortNames, (int32_t) numRequirePorts);
         if (rc != ADT_NO_ERROR)
         {
            if (rc == ADT_MEM_ERROR)
            {
               retval = APX_MEM_ERROR;
            }
            else
            {
               retval = APX_GENERIC_ERROR;
            }
         }
         if (retval == APX_NO_ERROR)
         {
            apx_portId_t portId;
            for(portId = 0; portId < numRequirePorts; portId++)
            {
               adt_str_t *portName;
               void *portHandle = apx_nodeInstance_getRequirePortHandle(nodeInstance, portId);
               portName = apx_nodeInstance_getRequirePortName(nodeInstance, portId);
               if ( (portName == 0) || (portHandle == 0) )
               {
                  retval = APX_NULL_PTR_ERROR;
                  break;
               }
               (void) adt_ary_set(&self->requirePortLookupTable, (int32_t) portId, portHandle);
               (void) adt_ary_set(&self->requirePortNames, (int32_t) portId, portName);
            }
         }
      }
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

