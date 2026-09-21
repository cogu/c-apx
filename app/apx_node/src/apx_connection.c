/*****************************************************************************
* \file      apx_connection.c
* \author    Conny Gustafsson
* \date      2020-04-14
* \brief     APX client connection
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include "apx_connection.h"
#include "apx/event_listener.h"
#include "apx/client.h"
#include "dtl_json.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_connection_on_connect(void *arg, apx_client_connection_t *client_connection);
static void apx_connection_on_disconnect(void *arg, apx_client_connection_t *client_connection);
static void apx_connection_on_require_port_write(void* arg, struct apx_port_instance_tag* port_instance, uint8_t const* data, apx_size_t size);
static apx_error_t apx_connection_prepare_provide_ports(apx_connection_t *self, apx_node_instance_t *node_instance);

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
   if (self != NULL)
   {
      apx_client_event_listener_t listener;
      self->client = apx_client_new();
      if (self->client == NULL)
      {
         return APX_MEM_ERROR;
      }
      memset(&listener, 0, sizeof(listener));
      listener.arg = (void*) self;
      listener.connected = apx_connection_on_connect;
      listener.disconnected = apx_connection_on_disconnect;
      listener.require_port_write = apx_connection_on_require_port_write;
      apx_client_register_event_listener(self->client, &listener);
      adt_hash_create(&self->provide_port_lookup_table, NULL);
      MUTEX_INIT(self->mutex);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_connection_destroy(apx_connection_t *self)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->mutex);
      if (self->client != NULL)
      {
         apx_client_delete(self->client);
      }
      adt_hash_destroy(&self->provide_port_lookup_table);
      MUTEX_UNLOCK(self->mutex);
      MUTEX_DESTROY(self->mutex);
   }
}

apx_connection_t *apx_connection_new(void)
{
   apx_connection_t *self = (apx_connection_t*) malloc(sizeof(apx_connection_t));
   if (self != NULL)
   {
      apx_error_t rc = apx_connection_create(self);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_connection_delete(apx_connection_t *self)
{
   if (self != NULL)
   {
      apx_connection_destroy(self);
      free(self);
   }
}

void apx_connection_disconnect(apx_connection_t *self)
{
   if (self != NULL)
   {
      apx_client_disconnect(self->client);
   }
}

apx_error_t apx_connection_attachNode(apx_connection_t *self, adt_str_t *apx_definition)
{
   if ( (self != NULL) && (apx_definition != NULL) )
   {
      MUTEX_LOCK(self->mutex);
      apx_error_t retval = apx_client_build_node(self->client, adt_str_cstr(apx_definition));
      if (retval == APX_NO_ERROR)
      {
         apx_node_instance_t *nodeInstance = apx_client_get_last_attached_node(self->client);
         if (nodeInstance != NULL)
         {
            retval = apx_connection_prepare_provide_ports(self, nodeInstance);
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
   if (self != NULL)
   {
      return apx_client_get_error_line(self->client);
   }
   return -1;
}

apx_node_instance_t *apx_connection_getLastAttachedNode(apx_connection_t *self)
{
   if (self != NULL)
   {
      return apx_client_get_last_attached_node(self->client);
   }
   return NULL;
}

#ifndef _WIN32
apx_error_t apx_connection_connect_unix(apx_connection_t *self, const char *socketPath)
{
   if (self != NULL)
   {
      return apx_client_connect_unix(self->client, socketPath);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
#endif

apx_error_t apx_connection_connect_tcp(apx_connection_t *self, const char *address, uint16_t port)
{
   if (self != NULL)
   {
      return apx_client_connect_tcp(self->client, address, port);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_connection_writeProvidePortData(apx_connection_t *self, const char *providePortName, dtl_dv_t *dv_value)
{
   if ( (self != NULL) && (providePortName != NULL) && (dv_value != NULL) )
   {
      apx_error_t result;
      MUTEX_LOCK(self->mutex);
      apx_port_instance_t *port_instance = (apx_port_instance_t*) adt_hash_value(&self->provide_port_lookup_table, providePortName);
      if (port_instance == NULL)
      {
         MUTEX_UNLOCK(self->mutex);
         return APX_INVALID_NAME_ERROR;
      }
      result = apx_client_write_port_data(self->client, port_instance, dv_value);
      MUTEX_UNLOCK(self->mutex);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_connection_on_connect(void* arg, apx_client_connection_t* client_connection)
{
   (void)arg;
   (void)client_connection;
   printf("[APX-CONNECTION] connected to APX server\n");
}

static void apx_connection_on_disconnect(void* arg, apx_client_connection_t* client_connection)
{
   (void)arg;
   printf("[APX-CONNECTION] Disconnected from APX server\n");
}

static void apx_connection_on_require_port_write(void* arg, struct apx_port_instance_tag* port_instance, uint8_t const* data, apx_size_t size)
{
   apx_connection_t* self = (apx_connection_t*)arg;
   (void)data;
   (void)size;
   if ( (self != NULL) && (port_instance != NULL) )
   {
      apx_error_t result;
      char const* port_name;
      dtl_dv_t* dv = NULL;
      MUTEX_LOCK(self->mutex);
      port_name = apx_portInstance_name(port_instance);
      result = apx_client_read_port_data(self->client, port_instance, &dv);
      MUTEX_UNLOCK(self->mutex);
      if (result != APX_NO_ERROR)
      {
         printf("apx_client_read_port_data failed with error code %d\n", (int)result);
         return;
      }
      if ((dv != NULL) && (port_name != NULL))
      {
         adt_str_t* value = dtl_json_dumps(dv, 0, false);
         if (value != NULL)
         {
            printf("\"%s\": %s\n", port_name, adt_str_cstr(value));
            fflush(stdout);
            adt_str_delete(value);
         }
      }
      if (dv != NULL)
      {
         dtl_dec_ref(dv);
      }
   }
}

static apx_error_t apx_connection_prepare_provide_ports(apx_connection_t* self, apx_node_instance_t* node_instance)
{
   apx_error_t retval = APX_NO_ERROR;
   if ( (self != NULL) && (node_instance != NULL) )
   {
      apx_size_t num_provide_ports;
      apx_port_id_t port_id;
      num_provide_ports = apx_nodeInstance_get_num_provide_ports(node_instance);
      for(port_id = 0; port_id < num_provide_ports; port_id++)
      {
         char const *port_name = NULL;
         apx_port_instance_t *port_instance = apx_nodeInstance_get_provide_port(node_instance, port_id);
         if (port_instance != NULL)
         {
            port_name = apx_portInstance_name(port_instance);
         }

         if ( port_name == NULL)
         {
            retval = APX_NULL_PTR_ERROR;
            break;
         }
         adt_hash_set(&self->provide_port_lookup_table, port_name, (void*) port_instance);
      }
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}
