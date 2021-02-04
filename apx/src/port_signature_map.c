/*****************************************************************************
* \file      port_signature_map.c
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     Port signature map
*
* Copyright (c) 2020-2021 Conny Gustafsson
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
#include "apx/port_signature_map.h"
#include "apx/node_instance.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_portSignatureMap_connect_require_ports_internal(apx_portSignatureMap_t *self, apx_nodeInstance_t *node_instance);
static apx_error_t apx_portSignatureMap_connect_provide_ports_internal(apx_portSignatureMap_t *self, apx_nodeInstance_t *node_instance);
static apx_error_t apx_portSignatureMap_insert(apx_portSignatureMap_t *self, const char *port_signature, apx_portInstance_t *port_instance);
static apx_portSignatureMapEntry_t *apx_portSignatureMap_create_new_entry(apx_portSignatureMap_t *self, const char * port_signature);
static apx_error_t apx_portSignatureMap_disconnect_require_ports_internal(apx_portSignatureMap_t *self, apx_nodeInstance_t *node_instance);
static apx_error_t apx_portSignatureMap_disconnect_provide_ports_internal(apx_portSignatureMap_t *self, apx_nodeInstance_t *node_instance);
static apx_error_t apx_portSignatureMap_remove(apx_portSignatureMap_t *self, const char *port_signature, apx_portInstance_t* port_instance);
static void apx_portSignatureMap_delete_entry(apx_portSignatureMap_t *self, const char *port_signature);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_portSignatureMap_create(apx_portSignatureMap_t *self)
{
   if (self != 0)
   {
      adt_hash_create(&self->internal_map, apx_portSignatureMapEntry_vdelete);
   }
}

void apx_portSignatureMap_destroy(apx_portSignatureMap_t *self)
{
   adt_hash_destroy(&self->internal_map);
}

apx_portSignatureMapEntry_t *apx_portSignatureMap_find(apx_portSignatureMap_t *self, const char *portSignature)
{
   if ( (self != 0) && (portSignature != 0) )
   {
      void **ppResult = adt_hash_get(&self->internal_map, portSignature);
      if (ppResult != 0)
      {
         return (apx_portSignatureMapEntry_t*) *ppResult;
      }
   }
   return (apx_portSignatureMapEntry_t*) 0;
}

int32_t apx_portSignatureMap_length(apx_portSignatureMap_t *self)
{
   if (self != 0)
   {
      return adt_hash_length(&self->internal_map);
   }
   return -1;
}

apx_portSignatureMap_t *apx_portSignatureMap_new(void)
{
   apx_portSignatureMap_t *self = (apx_portSignatureMap_t*) malloc(sizeof(apx_portSignatureMap_t));
   if (self != 0)
   {
      apx_portSignatureMap_create(self);
   }
   return self;

}

void apx_portSignatureMap_delete(apx_portSignatureMap_t *self)
{
   if (self != 0)
   {
      apx_portSignatureMap_destroy(self);
      free(self);
   }
}

apx_error_t apx_portSignatureMap_connect_provide_ports(apx_portSignatureMap_t* self, struct apx_nodeInstance_tag* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      return apx_portSignatureMap_connect_provide_ports_internal(self, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portSignatureMap_connect_require_ports(apx_portSignatureMap_t* self, struct apx_nodeInstance_tag* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      return apx_portSignatureMap_connect_require_ports_internal(self, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


apx_error_t apx_portSignatureMap_disconnect_provide_ports(apx_portSignatureMap_t* self, struct apx_nodeInstance_tag* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      return apx_portSignatureMap_disconnect_provide_ports_internal(self, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portSignatureMap_disconnect_require_ports(apx_portSignatureMap_t* self, struct apx_nodeInstance_tag* node_instance)
{
   if ( (self != NULL) && (node_instance != NULL) )
   {
      return apx_portSignatureMap_disconnect_require_ports_internal(self, node_instance);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_portSignatureMap_connect_require_ports_internal(apx_portSignatureMap_t* self, apx_nodeInstance_t* node_instance)
{
   apx_portId_t port_id;
   apx_size_t const num_require_ports = apx_nodeInstance_get_num_require_ports(node_instance);
   for(port_id = 0; port_id < num_require_ports; port_id++)
   {
      apx_portInstance_t *port_instance = apx_nodeInstance_get_require_port(node_instance, port_id);
      if (port_instance != NULL)
      {
         apx_error_t result = APX_NO_ERROR;
         bool has_dynamic_data = false;
         const char* port_signature = apx_portInstance_get_port_signature(port_instance, &has_dynamic_data);
         result = apx_portSignatureMap_insert(self, port_signature, port_instance);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_portSignatureMap_connect_provide_ports_internal(apx_portSignatureMap_t* self, apx_nodeInstance_t* node_instance)
{
   apx_portId_t port_id;
   apx_size_t const num_provide_ports = apx_nodeInstance_get_num_provide_ports(node_instance);
   for (port_id = 0; port_id < num_provide_ports; port_id++)
   {
      apx_portInstance_t* port_instance = apx_nodeInstance_get_provide_port(node_instance, port_id);
      if (port_instance != NULL)
      {
         apx_error_t result = APX_NO_ERROR;
         bool has_dynamic_data = false;
         const char* port_signature = apx_portInstance_get_port_signature(port_instance, &has_dynamic_data);
         result = apx_portSignatureMap_insert(self, port_signature, port_instance);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_portSignatureMap_insert(apx_portSignatureMap_t* self, const char* port_signature, apx_portInstance_t* port_instance)
{
   apx_portSignatureMapEntry_t *entry = NULL;
   assert(self != NULL);
   assert(port_instance != NULL);
   assert(strlen(port_signature) > 0);
   assert(port_instance != NULL);
   entry = apx_portSignatureMap_find(self, port_signature);
   if (entry == 0)
   {
      entry = apx_portSignatureMap_create_new_entry(self, port_signature);
      if (entry == 0)
      {
         return APX_MEM_ERROR;
      }
   }
   assert(entry != 0);
   if (apx_portInstance_port_type(port_instance) == APX_PROVIDE_PORT)
   {
      apx_portSignatureMapEntry_attach_provide_port(entry, port_instance, true);
      apx_portSignatureMapEntry_notify_require_ports_about_provide_port_change(entry, port_instance, APX_PORT_CONNECTED_EVENT);
   }
   else
   {
      apx_portSignatureMapEntry_attach_require_port(entry, port_instance);
      apx_portSignatureMapEntry_notify_provide_ports_about_require_port_change(entry, port_instance, APX_PORT_CONNECTED_EVENT);
   }
   return APX_NO_ERROR;
}

static apx_portSignatureMapEntry_t* apx_portSignatureMap_create_new_entry(apx_portSignatureMap_t* self, const char* port_signature)
{
   apx_portSignatureMapEntry_t *entry = apx_portSignatureMapEntry_new();
   if (entry != 0)
   {
      adt_hash_set(&self->internal_map, port_signature, entry);
   }
   return entry;
}

static apx_error_t apx_portSignatureMap_disconnect_require_ports_internal(apx_portSignatureMap_t* self, apx_nodeInstance_t* node_instance)
{
   apx_portId_t port_id;
   apx_size_t const num_require_ports = apx_nodeInstance_get_num_require_ports(node_instance);
   for (port_id = 0; port_id < num_require_ports; port_id++)
   {
      apx_portInstance_t* port_instance = apx_nodeInstance_get_require_port(node_instance, port_id);
      if (port_instance != NULL)
      {
         apx_error_t result = APX_NO_ERROR;
         bool has_dynamic_data = false;
         const char* port_signature = apx_portInstance_get_port_signature(port_instance, &has_dynamic_data);
         result = apx_portSignatureMap_remove(self, port_signature, port_instance);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_portSignatureMap_disconnect_provide_ports_internal(apx_portSignatureMap_t* self, apx_nodeInstance_t* node_instance)
{
   apx_portId_t port_id;
   apx_size_t const num_provide_ports = apx_nodeInstance_get_num_provide_ports(node_instance);
   for (port_id = 0; port_id < num_provide_ports; port_id++)
   {
      apx_portInstance_t* port_instance = apx_nodeInstance_get_provide_port(node_instance, port_id);
      if (port_instance != NULL)
      {
         apx_error_t result = APX_NO_ERROR;
         bool has_dynamic_data = false;
         const char* port_signature = apx_portInstance_get_port_signature(port_instance, &has_dynamic_data);
         result = apx_portSignatureMap_remove(self, port_signature, port_instance);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
      }
   }
   return APX_NO_ERROR;
}


static apx_error_t apx_portSignatureMap_remove(apx_portSignatureMap_t* self, const char* port_signature, apx_portInstance_t* port_instance)
{
   apx_portSignatureMapEntry_t *entry;
   assert(self != NULL);
   assert(port_signature != NULL);
   assert(strlen(port_signature) > 0);
   assert(port_instance != NULL);
   entry = apx_portSignatureMap_find(self, port_signature);
   if (entry == 0)
   {
      return APX_NOT_FOUND_ERROR;
   }
   assert(entry != 0);
   if (apx_portInstance_port_type(port_instance) == APX_PROVIDE_PORT)
   {
      apx_portSignatureMapEntry_detach_provide_port(entry, port_instance);
      apx_portSignatureMapEntry_notify_require_ports_about_provide_port_change(entry, port_instance, APX_PORT_DISCONNECTED_EVENT);
   }
   else
   {
      apx_portSignatureMapEntry_detach_require_port(entry, port_instance);
      apx_portSignatureMapEntry_notify_provide_ports_about_require_port_change(entry, port_instance, APX_PORT_DISCONNECTED_EVENT);
   }
   if (apx_portSignatureMapEntry_is_empty(entry))
   {
      apx_portSignatureMap_delete_entry(self, port_signature);
   }
   return APX_NO_ERROR;
}

static void apx_portSignatureMap_delete_entry(apx_portSignatureMap_t* self, const char* port_signature)
{
   if ( (self != 0) && (port_signature != 0) )
   {
      apx_portSignatureMapEntry_t *entry = (apx_portSignatureMapEntry_t*) adt_hash_remove(&self->internal_map, port_signature);
      if (entry != 0)
      {
         apx_portSignatureMapEntry_delete(entry);
      }
   }
}
