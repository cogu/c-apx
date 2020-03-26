/*****************************************************************************
* \file      apx_receive_connection.c
* \author    Conny Gustafsson
* \date      2020-03-09
* \brief     APX client connection that can listen to any value sent from APX server
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
#include "apx_receive_connection.h"
#include "apx_eventListener.h"
#include "dtl_json.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_receive_connection_onDisconnect(void *arg, apx_clientConnectionBase_t *clientConnection);
static void apx_receive_connection_onRequirePortWrite(void *arg, apx_nodeInstance_t *nodeInstance, apx_portId_t requirePortId, void *portHandle);
//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_receive_connection_create(apx_receive_connection_t *self)
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
      listener.clientDisconnect1 = apx_receive_connection_onDisconnect;
      listener.requirePortWrite1 = apx_receive_connection_onRequirePortWrite;
      apx_client_registerEventListener(self->client, &listener);
      MUTEX_INIT(self->mutex);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_receive_connection_destroy(apx_receive_connection_t *self)
{
   if (self != 0)
   {
      MUTEX_LOCK(self->mutex);
      if (self->client != 0)
      {
         apx_client_delete(self->client);
      }
      MUTEX_UNLOCK(self->mutex);
      MUTEX_DESTROY(self->mutex);
   }
}

apx_receive_connection_t *apx_receive_connection_new(void)
{
   apx_receive_connection_t *self = (apx_receive_connection_t*) malloc(sizeof(apx_receive_connection_t));
   if (self != 0)
   {
      apx_error_t rc = apx_receive_connection_create(self);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = (apx_receive_connection_t*) 0;
      }
   }
   return self;
}

void apx_receive_connection_delete(apx_receive_connection_t *self)
{
   if (self != 0)
   {
      apx_receive_connection_destroy(self);
      free(self);
   }
}

apx_error_t apx_receive_connection_attachNode(apx_receive_connection_t *self, adt_str_t *apx_definition)
{
   if ( (self != 0) && (apx_definition != 0) )
   {
      return apx_client_buildNode_cstr(self->client, adt_str_cstr(apx_definition));
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_receive_connection_getLastErrorLine(apx_receive_connection_t *self)
{
   if (self != 0)
   {
      return apx_client_getLastErrorLine(self->client);
   }
   return -1;
}

apx_nodeInstance_t *apx_receive_connection_getLastAttachedNode(apx_receive_connection_t *self)
{
   if (self != 0)
   {
      return apx_client_getLastAttachedNode(self->client);
   }
   return (apx_nodeInstance_t*) 0;
}


apx_error_t apx_receive_connection_connect_unix(apx_receive_connection_t *self, const char *socketPath)
{
   if (self != 0)
   {
      return apx_client_connect_unix(self->client, socketPath);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_receive_connection_onDisconnect(void *arg, apx_clientConnectionBase_t *clientConnection)
{
   printf("Disconnected from APX server\n");
}

static void apx_receive_connection_onRequirePortWrite(void *arg, apx_nodeInstance_t *nodeInstance, apx_portId_t requirePortId, void *portHandle)
{
   apx_receive_connection_t *self = (apx_receive_connection_t*) arg;
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
         adt_str_t *value = dtl_json_dumps(dv, 0, true);
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
