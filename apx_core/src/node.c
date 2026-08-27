/*****************************************************************************
* \file      node.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX (parse tree) node
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#include <assert.h>
#include <string.h>
#include "apx/node.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t derive_types_on_ports(apx_node_t* self, adt_ary_t* ports);
static apx_error_t derive_proper_init_values_on_ports(apx_node_t* self, adt_ary_t* ports);
static apx_error_t expand_data_elements_on_ports(apx_node_t* self, adt_ary_t* ports);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_node_create(apx_node_t* self, const char* name)
{
   if (self != NULL)
   {
      adt_ary_create(&self->data_types, apx_dataType_vdelete);
      adt_ary_create(&self->require_ports, apx_port_vdelete);
      adt_ary_create(&self->provide_ports, apx_port_vdelete);
      adt_hash_create(&self->type_map, NULL);
      adt_hash_create(&self->port_map, NULL);
      self->name = NULL;
      apx_node_set_name(self, name);
      self->is_finalized = false;
      self->last_error_line = 0;
   }
}

void apx_node_destroy(apx_node_t* self)
{
   if (self != NULL)
   {
      if (self->name != NULL)
      {
         free(self->name);
      }
      adt_ary_destroy(&self->data_types);
      adt_ary_destroy(&self->require_ports);
      adt_ary_destroy(&self->provide_ports);
      adt_hash_destroy(&self->type_map);
      adt_hash_destroy(&self->port_map);
   }
}

apx_node_t* apx_node_new(const char* name)
{
   apx_node_t* self = (apx_node_t*)malloc(sizeof(apx_node_t));
   if (self != 0)
   {
      apx_node_create(self, name);
   }
   return self;
}

void apx_node_delete(apx_node_t* self)
{
   if (self != 0) {
      apx_node_destroy(self);
      free(self);
   }
}

void apx_node_vdelete(void* arg)
{
   apx_node_delete((apx_node_t*)arg);
}

apx_error_t apx_node_append_data_type(apx_node_t* self, apx_dataType_t* data_type)
{
   if ((self != NULL) && (data_type != NULL))
   {
      const char* name = apx_dataType_get_name(data_type);
      if (name == NULL)
      {
         return APX_NAME_MISSING_ERROR;
      }
      apx_dataType_t* found = adt_hash_value(&self->type_map, name);
      if (found != NULL)
      {
         return APX_TYPE_ALREADY_EXIST_ERROR;
      }
      else
      {
         apx_dataType_set_id(data_type, (apx_typeId_t)adt_ary_length(&self->data_types));
         adt_ary_push(&self->data_types, data_type);
         adt_hash_set(&self->type_map, name, data_type);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_node_append_port(apx_node_t* self, apx_port_t* port)
{
   if ((self != NULL) && (port != NULL))
   {
      const char* name = apx_port_get_name(port);
      if (name == NULL)
      {
         return APX_NAME_MISSING_ERROR;
      }
      apx_dataType_t* found = adt_hash_value(&self->port_map, name);
      if (found != NULL)
      {
         return APX_PORT_ALREADY_EXIST_ERROR;
      }
      else
      {
         if (apx_port_get_port_type(port) == APX_PROVIDE_PORT)
         {
            apx_port_set_id(port, (apx_portId_t)adt_ary_length(&self->provide_ports));
            adt_ary_push(&self->provide_ports, port);
         }
         else
         {
            apx_port_set_id(port, (apx_portId_t)adt_ary_length(&self->require_ports));
            adt_ary_push(&self->require_ports, port);
         }
         adt_hash_set(&self->port_map, name, port);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_node_set_name(apx_node_t* self, const char* name)
{
   if (self != NULL)
   {
      if (self->name != NULL)
      {
         free(self->name);
      }
      if (name != NULL)
      {
         self->name = STRDUP(name);
      }
      else
      {
         self->name = NULL;
      }
   }
}

const char* apx_node_get_name(const apx_node_t* self)
{
   if (self != 0)
   {
      return self->name;
   }
   return (const char*)0;
}

int32_t apx_node_num_data_types(const apx_node_t* self)
{
   if (self != NULL)
   {
      return adt_ary_length(&self->data_types);
   }
   return -1;
}

int32_t apx_node_num_require_ports(const apx_node_t* self)
{
   if (self != NULL)
   {
      return adt_ary_length(&self->require_ports);
   }
   return -1;
}

int32_t apx_node_num_provide_ports(const apx_node_t* self)
{
   if (self != NULL)
   {
      return adt_ary_length(&self->provide_ports);
   }
   return -1;
}

apx_dataType_t* apx_node_get_data_type(const apx_node_t* self, apx_typeId_t type_id)
{
   if (self != NULL)
   {
      return adt_ary_value(&self->data_types, (int32_t)type_id);
   }
   return NULL;
}

apx_port_t* apx_node_get_require_port(const apx_node_t* self, apx_portId_t port_id)
{
   if (self != NULL)
   {
      return adt_ary_value(&self->require_ports, (int32_t)port_id);
   }
   return NULL;
}

apx_port_t* apx_node_get_provide_port(const apx_node_t* self, apx_portId_t port_id)
{
   if (self != NULL)
   {
      return adt_ary_value(&self->provide_ports, (int32_t)port_id);
   }
   return NULL;
}

apx_dataType_t* apx_node_get_last_data_type(const apx_node_t* self)
{
   if ( (self != NULL) && !adt_ary_is_empty(&self->data_types))
   {
      return adt_ary_value(&self->data_types, -1);
   }
   return NULL;
}

apx_port_t* apx_node_get_last_require_port(const apx_node_t* self)
{
   if (self != NULL && !adt_ary_is_empty(&self->require_ports))
   {
      return adt_ary_value(&self->require_ports, -1);
   }
   return NULL;
}

apx_port_t* apx_node_get_last_provide_port(const apx_node_t* self)
{
   if (self != NULL && !adt_ary_is_empty(&self->provide_ports))
   {
      return adt_ary_value(&self->provide_ports, -1);
   }
   return NULL;
}
apx_error_t apx_node_finalize(apx_node_t* self)
{
   if (self != NULL)
   {
      if (self->is_finalized)
      {
         return APX_NO_ERROR;
      }
      self->last_error_line = -1;
      apx_error_t retval = derive_types_on_ports(self, &self->provide_ports);
      if (retval == APX_NO_ERROR)
      {
         retval = derive_types_on_ports(self, &self->require_ports);
      }
      if (retval == APX_NO_ERROR)
      {
         retval = expand_data_elements_on_ports(self, &self->provide_ports);
      }
      if (retval == APX_NO_ERROR)
      {
         retval = expand_data_elements_on_ports(self, &self->require_ports);
      }
      if (retval == APX_NO_ERROR)
      {
         retval = derive_proper_init_values_on_ports(self, &self->provide_ports);
      }
      if (retval == APX_NO_ERROR)
      {
         retval = derive_proper_init_values_on_ports(self, &self->require_ports);
      }
      if (retval == APX_NO_ERROR)
      {
         self->is_finalized = true;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

int32_t apx_node_get_last_error_line(const apx_node_t* self)
{
   if (self != NULL)
   {
      return self->last_error_line;
   }
   return -1;
}

dtl_dv_t* apx_port_get_proper_init_value(apx_port_t* self)
{
   if (self != NULL)
   {
      return self->proper_init_value;
   }
   return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t derive_types_on_ports(apx_node_t* self, adt_ary_t* ports)
{
   int32_t num_ports;
   int32_t port_id;
   assert( (self != NULL) && (ports != NULL));
   num_ports = adt_ary_length(ports);

   for (port_id = 0; port_id < num_ports; port_id++)
   {
      apx_error_t result;
      apx_port_t* port = (apx_port_t*) adt_ary_value(ports, port_id);
      assert(port != NULL);

      result = apx_port_derive_types(port, &self->data_types, &self->type_map);
      if (result != APX_NO_ERROR)
      {
         self->last_error_line = port->line_number;
         return result;
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t derive_proper_init_values_on_ports(apx_node_t* self, adt_ary_t* ports)
{
   int32_t num_ports;
   int32_t port_id;
   assert((self != NULL) && (ports != NULL));
   num_ports = adt_ary_length(ports);

   for (port_id = 0; port_id < num_ports; port_id++)
   {
      apx_error_t result;
      apx_port_t* port = (apx_port_t*)adt_ary_value(ports, port_id);
      assert(port != NULL);

      result = apx_port_derive_proper_init_value(port);
      if (result != APX_NO_ERROR)
      {
         self->last_error_line = port->line_number;
         return result;
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t expand_data_elements_on_ports(apx_node_t* self, adt_ary_t* ports)
{
   int32_t num_ports;
   int32_t port_id;
   assert((self != NULL) && (ports != NULL));
   num_ports = adt_ary_length(ports);

   for (port_id = 0; port_id < num_ports; port_id++)
   {
      apx_error_t result;
      apx_port_t* port = (apx_port_t*)adt_ary_value(ports, port_id);
      assert(port != NULL);

      result = apx_port_flatten_data_element(port);
      if (result != APX_NO_ERROR)
      {
         self->last_error_line = port->line_number;
         return result;
      }
   }
   return APX_NO_ERROR;
}