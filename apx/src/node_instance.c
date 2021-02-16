/*****************************************************************************
* \file      node_instance.c
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Parent container for all things node-related.
*
* Copyright (c) 2019-2021 Conny Gustafsson
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
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <stdio.h> //DEBUG ONLY
#include "apx/node_instance.h"
#include "apx/file_manager.h"
#include "apx/node_manager.h"
#include "apx/server.h"
#include "apx/util.h"
#include "sha256.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define STACK_DATA_BUF_SIZE 256 //Bytes to allocate on stack before attempting malloc

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t calc_init_data_size(apx_portInstance_t* port_list, apx_size_t num_ports, apx_size_t* total_size);
static apx_error_t create_definition_file_info(apx_nodeInstance_t* self, rmf_fileInfo_t* file_info);
static apx_error_t create_provide_port_data_file_info(apx_nodeInstance_t* self, rmf_fileInfo_t* file_info);
static apx_error_t create_require_port_data_file_info(apx_nodeInstance_t* self, rmf_fileInfo_t* file_info);
static apx_error_t file_open_notify(apx_nodeInstance_t* self, apx_file_t *file);
static apx_error_t send_definition_data_to_file_manager(apx_nodeInstance_t* self, apx_fileManager_t* file_manager, uint32_t address);
static apx_error_t send_provide_port_data_to_file_manager(apx_nodeInstance_t* self, apx_fileManager_t* file_manager, uint32_t address);
static apx_error_t send_require_port_data_to_file_manager(apx_nodeInstance_t* self, apx_fileManager_t* file_manager, uint32_t address);
static void set_file_notification_handler(apx_nodeInstance_t* self, apx_file_t* file);
static apx_error_t file_write_notify(apx_nodeInstance_t* self, apx_file_t* file, uint32_t offset, const uint8_t* data, apx_size_t size);
static apx_error_t process_remote_write_definition_data(apx_nodeInstance_t* self, uint32_t offset, const uint8_t* data, apx_size_t size);
static apx_error_t process_remote_write_require_port_data(apx_nodeInstance_t* self, uint32_t offset, const uint8_t* data, apx_size_t size);
static apx_error_t process_remote_write_provide_port_data(apx_nodeInstance_t* self, uint32_t offset, const uint8_t* data, apx_size_t size);
static apx_error_t apx_nodeInstance_attach_to_file_manager_client_mode(apx_nodeInstance_t* self, struct apx_fileManager_tag* file_manager);
static apx_error_t apx_nodeInstance_attach_to_file_manager_server_mode(apx_nodeInstance_t* self, struct apx_fileManager_tag* file_manager);
static apx_error_t search_for_remote_provide_port_data_file(apx_nodeInstance_t* self, struct apx_fileManager_tag* file_manager);
static apx_error_t request_remote_provide_port_data(apx_nodeInstance_t* self, apx_file_t* file);
static apx_error_t request_remote_require_port_data(apx_nodeInstance_t* self, apx_file_t* file);
static apx_error_t connect_require_ports_to_server(apx_nodeInstance_t* self);
static apx_error_t remove_provide_port_connector(apx_nodeInstance_t* self, apx_portId_t provide_port_id, apx_portInstance_t* require_port);
static apx_error_t route_provide_port_data_change_to_receivers(apx_nodeInstance_t* self, uint32_t provide_data_offset, const uint8_t* provide_data, apx_size_t provide_data_size);
static apx_error_t route_provide_port_data_to_require_port(apx_portInstance_t* provide_port, apx_portInstance_t* require_port, bool do_remote_routing);
static apx_error_t remote_route_require_port_data(apx_nodeInstance_t* self, uint32_t offset, uint8_t const* data, apx_size_t size);
static apx_error_t remote_route_provide_port_data(apx_nodeInstance_t* self, uint32_t offset, uint8_t const* data, apx_size_t size);
static apx_error_t remote_route_data_to_file(apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);
static apx_error_t trigger_require_port_write_callbacks(apx_nodeInstance_t* self, uint32_t offset, const uint8_t* data, apx_size_t size);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_nodeInstance_create(apx_nodeInstance_t* self, apx_mode_t mode, char const* name)
{
   if ( (self != 0) && ( (mode == APX_CLIENT_MODE) || (mode == APX_SERVER_MODE) ))
   {
      memset(self, 0, sizeof(apx_nodeInstance_t));
      self->mode = mode;
      self->definition_data_state = APX_DATA_STATE_INIT;
      self->provide_port_data_state = APX_DATA_STATE_INIT;
      self->require_port_data_state = APX_DATA_STATE_INIT;
      self->name = STRDUP(name);
      self->num_provide_ports = 0u;
      self->num_require_ports = 0u;
      self->num_data_elements = 0u;
      self->num_computation_lists = 0u;
      self->require_port_init_data_size = 0u;
      self->provide_port_init_data_size = 0u;
      self->provide_ports = NULL;
      self->require_ports = NULL;
      self->data_elements = NULL;
      self->computation_lists = NULL;
      self->require_port_init_data = NULL;
      self->provide_port_init_data = NULL;
      self->require_port_changes = NULL;
      self->provide_port_changes = NULL;
      self->node_data = NULL;
      self->byte_port_map = NULL;
      self->parent = NULL;
      self->connector_table = NULL;
      self->server = NULL;
      self->definition_file = NULL;
      self->provide_port_data_file = NULL;
      self->require_port_data_file = NULL;
      MUTEX_INIT(self->lock);
   }
}

void apx_nodeInstance_destroy(apx_nodeInstance_t *self)
{
   if (self != 0)
   {

      if (self->name != NULL) free(self->name);
      if (self->connector_table != NULL)
      {
         apx_size_t i;
         MUTEX_LOCK(self->lock);
         for (i = 0u; i < self->num_provide_ports; i++)
         {
            apx_portConnectorList_destroy(&self->connector_table[i]);
         }
         free(self->connector_table);
         self->connector_table = NULL;
         MUTEX_UNLOCK(self->lock);
      }
      MUTEX_DESTROY(self->lock);
      if (self->provide_ports != NULL)
      {
         apx_size_t i;
         for (i = 0u; i < self->num_provide_ports; i++)
         {
            apx_portInstance_destroy(&self->provide_ports[i]);
         }
         free(self->provide_ports);
      }
      if (self->require_ports != NULL)
      {
         apx_size_t i;
         for (i = 0u; i < self->num_require_ports; i++)
         {
            apx_portInstance_destroy(&self->require_ports[i]);
         }
         free(self->require_ports);
      }
      if (self->data_elements != NULL)
      {
         apx_size_t i;
         for (i = 0u; i < self->num_data_elements; i++)
         {
            apx_dataElement_delete(self->data_elements[i]);
         }
         free(self->data_elements);
      }
      if (self->computation_lists != NULL)
      {
         apx_size_t i;
         for (i = 0u; i < self->num_computation_lists; i++)
         {
            apx_computationList_delete(self->computation_lists[i]);
         }
         free(self->computation_lists);
      }
      if (self->require_port_changes != NULL)
      {
         apx_portConnectorChangeTable_delete(self->require_port_changes);
      }

      if (self->provide_port_changes != NULL)
      {
         apx_portConnectorChangeTable_delete(self->provide_port_changes);
      }
      if (self->require_port_init_data != NULL) free(self->require_port_init_data);
      if (self->provide_port_init_data != NULL) free(self->provide_port_init_data);
      if (self->node_data != NULL) apx_nodeData_delete(self->node_data);
      if (self->byte_port_map != NULL) apx_bytePortMap_delete(self->byte_port_map);
   }
}

apx_nodeInstance_t* apx_nodeInstance_new(apx_mode_t mode, char const* name)
{
   apx_nodeInstance_t *self = (apx_nodeInstance_t*) malloc(sizeof(apx_nodeInstance_t));
   if(self != 0)
   {
      apx_nodeInstance_create(self, mode, name);
   }
   return self;
}

void apx_nodeInstance_delete(apx_nodeInstance_t *self)
{
   if(self != 0)
   {
      apx_nodeInstance_destroy(self);
      free(self);
   }
}

void apx_nodeInstance_vdelete(void *arg)
{
   apx_nodeInstance_delete((apx_nodeInstance_t*) arg);
}

char const* apx_nodeInstance_get_name(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->name;
   }
   return NULL;
}
apx_size_t apx_nodeInstance_get_num_data_elements(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->num_data_elements;
   }
   return 0u;
}

apx_size_t apx_nodeInstance_get_num_computation_lists(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->num_computation_lists;
   }
   return 0u;
}

apx_size_t apx_nodeInstance_get_num_provide_ports(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->num_provide_ports;
   }
   return 0;
}

apx_size_t apx_nodeInstance_get_num_require_ports(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->num_require_ports;
   }
   return 0u;
}

apx_size_t apx_nodeInstance_get_provide_port_init_data_size(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->provide_port_init_data_size;
   }
   return 0u;
}

apx_size_t apx_nodeInstance_get_require_port_init_data_size(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->require_port_init_data_size;
   }
   return 0u;
}

uint8_t const* apx_nodeInstance_get_provide_port_init_data(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return (uint8_t const*)self->provide_port_init_data;
   }
   return NULL;
}

uint8_t const* apx_nodeInstance_get_require_port_init_data(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return (uint8_t const*)self->require_port_init_data;
   }
   return NULL;
}

apx_portInstance_t* apx_nodeInstance_get_provide_port(apx_nodeInstance_t const* self, apx_portId_t port_id)
{
   if ( (self != NULL) && (port_id < (apx_portId_t)self->num_provide_ports) )
   {
      return &self->provide_ports[port_id];
   }
   return NULL;
}

apx_portInstance_t* apx_nodeInstance_get_require_port(apx_nodeInstance_t const* self, apx_portId_t port_id)
{
   if ((self != NULL) && (port_id < (apx_portId_t)self->num_require_ports))
   {
      return &self->require_ports[port_id];
   }
   return NULL;
}

apx_dataElement_t const* apx_nodeInstance_get_data_element(apx_nodeInstance_t const* self, apx_elementId_t id)
{
   if ((self != NULL) && (id < self->num_data_elements))
   {
      return self->data_elements[id];
   }
   return NULL;
}

apx_computationList_t const* apx_nodeInstance_get_computation_list(apx_nodeInstance_t* self, apx_computationListId_t id)
{
   if ((self != NULL) && (id < self->num_computation_lists))
   {
      return self->computation_lists[id];
   }
   return NULL;
}

apx_error_t apx_nodeInstance_alloc_port_instance_memory(apx_nodeInstance_t* self, apx_size_t num_provide_ports, apx_size_t num_require_ports)
{
   if (self != NULL)
   {
      self->num_provide_ports = num_provide_ports;
      self->num_require_ports = num_require_ports;
      if (num_provide_ports > 0u)
      {
         self->provide_ports = (apx_portInstance_t*)malloc(num_provide_ports * sizeof(apx_portInstance_t));
         if (self->provide_ports == NULL)
         {
            return APX_MEM_ERROR;
         }
      }
      if (num_require_ports > 0u)
      {
         self->require_ports = (apx_portInstance_t*)malloc(num_require_ports * sizeof(apx_portInstance_t));
         if (self->require_ports == NULL)
         {
            return APX_MEM_ERROR;
         }
      }
   }
   return APX_NO_ERROR;
}

apx_error_t apx_nodeInstance_create_provide_port(apx_nodeInstance_t* self, apx_portId_t port_id, char const* name,
   apx_program_t const* pack_program, uint32_t data_offset, uint32_t* data_size)
{
   if ((self != NULL) && (port_id < (apx_portId_t)self->num_provide_ports))
   {
      apx_portInstance_create(&self->provide_ports[port_id], self, APX_PROVIDE_PORT, port_id,
         name, pack_program, NULL);
      return apx_portInstance_derive_properties(&self->provide_ports[port_id], data_offset, data_size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_create_require_port(apx_nodeInstance_t* self, apx_portId_t port_id, char const* name,
   apx_program_t const* pack_program, apx_program_t const* unpack_program, uint32_t data_offset, uint32_t* data_size)
{
   if ((self != NULL) && (port_id < (apx_portId_t)self->num_require_ports))
   {
      apx_portInstance_create(&self->require_ports[port_id], self, APX_REQUIRE_PORT, port_id,
         name, pack_program, unpack_program);
      return apx_portInstance_derive_properties(&self->require_ports[port_id], data_offset, data_size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_alloc_init_data_memory(apx_nodeInstance_t* self, uint8_t** provide_port_data,
   apx_size_t* provide_port_data_size, uint8_t** require_port_data, apx_size_t* require_port_data_size)
{
   if ( (self != NULL) && (provide_port_data != NULL) && (provide_port_data_size != NULL) &&
      (require_port_data != NULL) && (require_port_data_size != NULL))
   {
      apx_error_t result = APX_NO_ERROR;
      apx_size_t provide_port_init_data_size = 0u;
      apx_size_t require_port_init_data_size = 0u;

      if (self->num_provide_ports > 0u)
      {

         result = calc_init_data_size(self->provide_ports, self->num_provide_ports, &provide_port_init_data_size);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
         assert(provide_port_init_data_size > 0u);
         self->provide_port_init_data_size = provide_port_init_data_size;
         self->provide_port_init_data = (uint8_t*) malloc(provide_port_init_data_size);
         if (self->provide_port_init_data == NULL)
         {
            return APX_MEM_ERROR;
         }
         memset(self->provide_port_init_data, 0u, provide_port_init_data_size);
         *provide_port_data = self->provide_port_init_data;
         *provide_port_data_size = provide_port_init_data_size;
      }
      if (self->num_require_ports > 0u)
      {
         result = calc_init_data_size(self->require_ports, self->num_require_ports, &require_port_init_data_size);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
         self->require_port_init_data_size = require_port_init_data_size;
         self->require_port_init_data = (uint8_t*)malloc(require_port_init_data_size);
         if (self->require_port_init_data == NULL)
         {
            return APX_MEM_ERROR;
         }
         memset(self->require_port_init_data, 0u, require_port_init_data_size);
         *require_port_data = self->require_port_init_data;
         *require_port_data_size = require_port_init_data_size;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_init_node_data(apx_nodeInstance_t* self, uint8_t const* definition_data, apx_size_t definition_size)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_nodeData_t* node_data = apx_nodeData_new();
      if (node_data != NULL)
      {
         retval = apx_nodeData_create_definition_data(node_data, definition_data, definition_size);
         if (retval == APX_NO_ERROR)
         {
            self->node_data = node_data;
         }
      }
      else
      {
         retval = APX_MEM_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_finalize_node_data(apx_nodeInstance_t* self)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_nodeData_t* node_data = self->node_data;
      if (node_data != NULL)
      {
         if (apx_nodeInstance_has_provide_port_data(self))
         {
            retval = apx_nodeData_create_provide_port_data(node_data, self->num_provide_ports, self->provide_port_init_data, self->provide_port_init_data_size);
         }
         if ((retval == APX_NO_ERROR) && apx_nodeInstance_has_require_port_data(self))
         {
            retval = apx_nodeData_create_require_port_data(node_data, self->num_require_ports, self->require_port_init_data, self->require_port_init_data_size);
         }
      }
      else
      {
         retval = APX_NULL_PTR_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

bool apx_nodeInstance_has_provide_port_data(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return (bool)(self->provide_port_init_data != NULL);
   }
   return false;
}

bool apx_nodeInstance_has_require_port_data(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return (bool)(self->require_port_init_data != NULL);
   }
   return false;
}

apx_nodeData_t const* apx_nodeInstance_get_const_node_data(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->node_data;
   }
   return NULL;
}

apx_nodeData_t* apx_nodeInstance_get_node_data(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->node_data;
   }
   return NULL;
}

bool apx_nodeInstance_has_node_data(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return (bool)(self->node_data != NULL);
   }
   return false;
}

apx_size_t apx_nodeInstance_get_definition_size(apx_nodeInstance_t const* self)
{
   if ( (self != NULL) && (self->node_data != NULL) )
   {
      return apx_nodeData_definition_data_size(self->node_data);
   }
   return 0u;
}

uint8_t const* apx_nodeInstance_get_definition_data(apx_nodeInstance_t const* self)
{
   if ((self != NULL) && (self->node_data != NULL))
   {
      return apx_nodeData_get_definition_data(self->node_data);
   }
   return NULL;
}

apx_error_t apx_nodeInstance_create_data_element_list(apx_nodeInstance_t* self, adt_ary_t* data_element_list)
{
   if (self != NULL)
   {
      self->num_data_elements = (apx_size_t)adt_ary_length(data_element_list);
      if (self->num_data_elements > 0u)
      {
         self->data_elements = (apx_dataElement_t**)malloc(self->num_data_elements * sizeof(apx_dataElement_t*));
         if (self->data_elements != NULL)
         {
            apx_size_t i;
            for (i = 0; i < self->num_data_elements; i++)
            {
               self->data_elements[i] = adt_ary_value(data_element_list, i);
            }
            //Take memory owmership of the copied pointers
            adt_ary_destructor_enable(data_element_list, false);
            adt_ary_clear(data_element_list);
            adt_ary_destructor_enable(data_element_list, true);
         }
         else
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_create_computation_lists(apx_nodeInstance_t* self, adt_ary_t* computation_lists)
{
   if (self != NULL)
   {
      self->num_computation_lists = (apx_size_t)adt_ary_length(computation_lists);
      if (self->num_computation_lists > 0u)
      {
         self->computation_lists = (apx_computationList_t**)malloc(self->num_computation_lists * sizeof(apx_computationList_t*));
         if (self->computation_lists != NULL)
         {
            apx_size_t i;
            for (i = 0; i < self->num_computation_lists; i++)
            {
               self->computation_lists[i] = adt_ary_value(computation_lists, i);
            }
            //Take memory owmership of the copied pointers
            adt_ary_destructor_enable(computation_lists, false);
            adt_ary_clear(computation_lists);
         }
         else
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_create_byte_port_map(apx_nodeInstance_t* self)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if ( (self->mode == APX_CLIENT_MODE) && apx_nodeInstance_has_require_port_data(self))
      {
         self->byte_port_map = apx_bytePortMap_new(self->require_port_init_data_size, self->require_ports, self->num_require_ports, &retval);
      }
      else if ((self->mode == APX_SERVER_MODE) && apx_nodeInstance_has_provide_port_data(self))
      {
         self->byte_port_map = apx_bytePortMap_new(self->provide_port_init_data_size, self->provide_ports, self->num_provide_ports, &retval);
      }
      else
      {
         //No need to create byte-port-map
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_bytePortMap_t const* apx_nodeInstance_get_byte_port_map(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->byte_port_map;
   }
   return NULL;
}

apx_dataState_t apx_nodeInstance_get_definition_data_state(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->definition_data_state;
   }
   return APX_DATA_STATE_DISCONNECTED;
}

apx_dataState_t apx_nodeInstance_get_require_port_data_state(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->require_port_data_state;
   }
   return APX_DATA_STATE_INIT;
}

apx_dataState_t apx_nodeInstance_get_provide_port_data_state(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->provide_port_data_state;
   }
   return APX_DATA_STATE_INIT;
}

void apx_nodeInstance_set_definition_data_state(apx_nodeInstance_t* self, apx_dataState_t state)
{
   if (self != NULL)
   {
      self->definition_data_state = state;
   }
}

void apx_nodeInstance_set_require_port_data_state(apx_nodeInstance_t* self, apx_dataState_t state)
{
   if (self != NULL)
   {
      self->require_port_data_state = state;
   }
}

void apx_nodeInstance_set_provide_port_data_state(apx_nodeInstance_t* self, apx_dataState_t state)
{
   if (self != NULL)
   {
      self->provide_port_data_state = state;
   }
}

void apx_nodeInstance_set_parent(apx_nodeInstance_t* self, struct apx_nodeManager_tag* parent)
{
   if (self != NULL)
   {
      self->parent = parent;
   }
}

struct apx_nodeManager_tag* apx_nodeInstance_get_parent(apx_nodeInstance_t const* self)
{
   if (self != NULL)
   {
      return self->parent;
   }
   return NULL;
}

apx_portId_t apx_nodeInstance_lookup_require_port_id(apx_nodeInstance_t const* self, apx_size_t byte_offset)
{
   if ( (self != NULL) && (self->mode == APX_CLIENT_MODE))
   {
      return apx_bytePortMap_lookup(self->byte_port_map, byte_offset);
   }
   return APX_INVALID_PORT_ID;
}

apx_portId_t apx_nodeInstance_lookup_provide_port_id(apx_nodeInstance_t const* self, apx_size_t byte_offset)
{
   if ((self != NULL) && (self->mode == APX_SERVER_MODE))
   {
      return apx_bytePortMap_lookup(self->byte_port_map, byte_offset);
   }
   return APX_INVALID_PORT_ID;
}

apx_portInstance_t* apx_nodeInstance_find(apx_nodeInstance_t const* self, char const* name)
{
   if (self != NULL)
   {
      apx_size_t i;
      if (self->num_provide_ports > 0)
      {
         for (i = 0; i < self->num_provide_ports; i++)
         {
            apx_portInstance_t* port_instance = &self->provide_ports[i];
            if (strcmp(name, apx_portInstance_name(port_instance))==0)
            {
               return port_instance;
            }
         }
      }
      if (self->num_require_ports > 0)
      {
         for (i = 0; i < self->num_require_ports; i++)
         {
            apx_portInstance_t* port_instance = &self->require_ports[i];
            if (strcmp(name, apx_portInstance_name(port_instance)) == 0)
            {
               return port_instance;
            }
         }
      }
   }
   return NULL;
}


apx_error_t apx_nodeInstance_attach_to_file_manager(apx_nodeInstance_t* self, struct apx_fileManager_tag* file_manager)
{
   if (self != NULL)
   {
      if (self->mode == APX_CLIENT_MODE)
      {
         return apx_nodeInstance_attach_to_file_manager_client_mode(self, file_manager);
      }
      else
      {
         return apx_nodeInstance_attach_to_file_manager_server_mode(self, file_manager);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_remote_file_published_notification(apx_nodeInstance_t* self, apx_file_t* file)
{
   if ((self != NULL) && (file != NULL))
   {
      apx_error_t retval = APX_NO_ERROR;
      switch (apx_file_get_apx_file_type(file))
      {
      case APX_DEFINITION_FILE_TYPE:
         set_file_notification_handler(self, file);
         break;
      case APX_REQUIRE_PORT_DATA_FILE_TYPE:
         set_file_notification_handler(self, file);
         retval = request_remote_require_port_data(self, file);
         break;
      default:
         retval = APX_NOT_IMPLEMENTED_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_nodeInstance_set_server(apx_nodeInstance_t* self, struct apx_server_tag* server)
{
   if (self != NULL)
   {
      self->server = server;
   }
}

apx_error_t apx_nodeInstance_write_provide_port_data(apx_nodeInstance_t* self, apx_size_t offset, uint8_t* data, apx_size_t size)
{
   if (self != NULL)
   {
      return remote_route_provide_port_data(self, offset, data, size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

// FileNotificationHandler API

apx_error_t apx_nodeInstance_vfile_open_notify(void* arg, apx_file_t* file)
{
   return file_open_notify((apx_nodeInstance_t*)arg, file);
}

apx_error_t apx_nodeInstance_vfile_close_notify(void* arg, apx_file_t* file)
{
   (void)arg;
   (void)file;
   return APX_NOT_IMPLEMENTED_ERROR;
}

apx_error_t apx_nodeInstance_vfile_write_notify(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   return file_write_notify((apx_nodeInstance_t*)arg, file, offset, data, size);
}

// Port Connector Change API
apx_portConnectorChangeTable_t* apx_nodeInstance_get_require_port_connector_changes(apx_nodeInstance_t* self, bool auto_create)
{
   if ( (self != NULL) && (self->num_require_ports > 0))
   {
      if ((self->require_port_changes == NULL) && (auto_create))
      {
         self->require_port_changes = apx_portConnectorChangeTable_new(self->num_require_ports);
         if ((self->require_port_changes != NULL) && (self->server != NULL))
         {
            apx_server_insert_modified_node_instance(self->server, self);
         }
      }
      return self->require_port_changes;
   }
   return (apx_portConnectorChangeTable_t*)NULL;
}

apx_portConnectorChangeTable_t* apx_nodeInstance_get_provide_port_connector_changes(apx_nodeInstance_t* self, bool auto_create)
{
   if ((self != NULL) && (self->num_provide_ports > 0))
   {
      if ((self->provide_port_changes == NULL) && (auto_create))
      {
         self->provide_port_changes = apx_portConnectorChangeTable_new(self->num_provide_ports);
         if ((self->provide_port_changes != NULL) && (self->server != NULL))
         {
            apx_server_insert_modified_node_instance(self->server, self);
         }
      }
      return self->provide_port_changes;
   }
   return (apx_portConnectorChangeTable_t*)NULL;
}

void apx_nodeInstance_clear_require_port_connector_changes(apx_nodeInstance_t* self, bool release_memory)
{
   if (self != NULL)
   {
      if (release_memory && (self->require_port_changes != NULL))
      {
         apx_portConnectorChangeTable_delete(self->require_port_changes);
      }
      self->require_port_changes = (apx_portConnectorChangeTable_t*)NULL;
   }
}

void apx_nodeInstance_clear_provide_port_connector_changes(apx_nodeInstance_t* self, bool release_memory)
{
   if (self != 0)
   {
      if (release_memory && (self->provide_port_changes != NULL))
      {
         apx_portConnectorChangeTable_delete(self->provide_port_changes);
      }
      self->provide_port_changes = (apx_portConnectorChangeTable_t*)NULL;
   }
}

apx_error_t apx_nodeInstance_handle_require_ports_disconnected(apx_nodeInstance_t* self, apx_portConnectorChangeTable_t* connector_changes)
{
   if ((self != NULL) && (connector_changes != NULL))
   {
      apx_portId_t require_port_id;
      apx_size_t num_require_ports = apx_nodeInstance_get_num_require_ports(self);
      for (require_port_id = 0; require_port_id < num_require_ports; require_port_id++)
      {
         apx_portInstance_t* require_port;
         apx_portConnectorChangeEntry_t* entry = apx_portConnectorChangeTable_get_entry(connector_changes, require_port_id);
         require_port = apx_nodeInstance_get_require_port(self, require_port_id);
         assert(require_port != NULL);
         assert(entry != NULL);
         assert(entry->count <= 0);
         if (entry->count == -1)
         {
            apx_error_t result;
            apx_portId_t provide_port_id;
            apx_nodeInstance_t* provide_node_instance;
            apx_portInstance_t* provide_port = entry->data.port_instance;
            assert(provide_port != NULL);
            provide_node_instance = provide_port->parent;
            assert(provide_node_instance != 0);
            provide_port_id = apx_portInstance_port_id(provide_port);
            apx_nodeInstance_lock_port_connector_table(provide_node_instance);
            result = remove_provide_port_connector(provide_node_instance, provide_port_id, require_port);
            apx_nodeInstance_unlock_port_connector_table(provide_node_instance);
            if (result != APX_NO_ERROR)
            {
               return result;
            }
         }
         else if (entry->count < -1)
         {
            //TODO: Handle multiple providers
            return APX_NOT_IMPLEMENTED_ERROR;
         }
         else
         {
            assert(entry->count == 0);
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

// Data Routing API
apx_error_t apx_nodeInstance_handle_require_port_connected_to_provide_port(apx_portInstance_t* require_port, apx_portInstance_t* provide_port)
{
   if ((require_port != NULL) && (provide_port != NULL))
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_nodeInstance_t* require_node;
      apx_nodeInstance_t* provide_node;
      apx_portId_t provide_port_id;
      require_node = require_port->parent;
      provide_node = provide_port->parent;
      provide_port_id = apx_portInstance_port_id(provide_port);
      assert(require_node != NULL);
      assert(provide_node != NULL);
      assert(provide_port_id < apx_nodeInstance_get_num_provide_ports(provide_node));
      MUTEX_LOCK(provide_node->lock);
      if (provide_node->connector_table != NULL)
      {
         apx_portConnectorList_t* connectors = &provide_node->connector_table[provide_port_id];
         retval = apx_portConnectorList_insert(connectors, require_port);
      }
      else
      {
         retval = APX_NULL_PTR_ERROR;
      }
      MUTEX_UNLOCK(provide_node->lock);
      if (retval == APX_NO_ERROR)
      {
         retval = route_provide_port_data_to_require_port(provide_port, require_port, false);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;

}

apx_error_t apx_nodeInstance_handle_provide_port_connected_to_require_port(apx_portInstance_t* provide_port, apx_portInstance_t* require_port)
{
   if ( (provide_port != NULL) && (require_port != NULL) && (provide_port->parent != NULL) && (require_port->parent != NULL))
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_nodeInstance_t* provide_node = provide_port->parent;
      apx_portId_t provide_port_id = apx_portInstance_port_id(provide_port);
      assert(provide_port_id < provide_node->num_provide_ports);
      assert(provide_node->connector_table != NULL);
      retval = apx_portConnectorList_insert(&provide_node->connector_table[provide_port_id], require_port);
      if (retval == APX_NO_ERROR)
      {
         retval = route_provide_port_data_to_require_port(provide_port, require_port, true);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeInstance_handle_require_port_disconnected_from_provide_port(apx_portInstance_t* require_port, apx_portInstance_t* provide_port)
{
   (void)require_port;
   (void)provide_port;
   //TODO: Implement later?
   return APX_NO_ERROR;
}

// ConnectorTable API
apx_error_t apx_nodeInstance_build_connector_table(apx_nodeInstance_t* self)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_size_t const num_provide_ports = self->num_provide_ports;
      if (num_provide_ports > 0u)
      {
         apx_portId_t port_id;
         size_t alloc_size = num_provide_ports * sizeof(apx_portConnectorList_t);
         MUTEX_LOCK(self->lock);
         self->connector_table = (apx_portConnectorList_t*)malloc(alloc_size);
         if (self->connector_table != NULL)
         {
            for (port_id = 0; port_id < num_provide_ports; port_id++)
            {
               apx_portConnectorList_create(&self->connector_table[port_id]);
            }
         }
         else
         {
            retval = APX_MEM_ERROR;
         }
         MUTEX_UNLOCK(self->lock);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_nodeInstance_lock_port_connector_table(apx_nodeInstance_t* self)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
   }
}

void apx_nodeInstance_unlock_port_connector_table(apx_nodeInstance_t* self)
{
   if (self != NULL)
   {
      MUTEX_UNLOCK(self->lock);
   }
}

apx_portConnectorList_t* apx_nodeInstance_get_provide_port_connectors(apx_nodeInstance_t* self, apx_portId_t port_id)
{
   if ( (self != NULL) && (port_id < self->num_provide_ports))
   {
      assert(self->connector_table != NULL);
      return &self->connector_table[port_id];
   }
   return NULL;
}
/*
apx_error_t apx_nodeInstance_insert_provide_port_connector(apx_nodeInstance_t* self, apx_portId_t provide_port_id, apx_portInstance_t* require_port)
{
   return APX_NOT_IMPLEMENTED_ERROR;
}

apx_error_t apx_nodeInstance_remove_provide_port_connector(apx_nodeInstance_t* self, apx_portId_t provide_port_id, apx_portInstance_t* require_port)
{
   return APX_NOT_IMPLEMENTED_ERROR;
}
*/

void apx_nodeInstance_clear_connector_table(apx_nodeInstance_t* self)
{
   if (self != NULL)
   {
      MUTEX_LOCK(self->lock);
      apx_portConnectorList_clear(self->connector_table);
      MUTEX_UNLOCK(self->lock);
   }
}

apx_portInstance_t* apx_nodeInstance_find_port_by_name(apx_nodeInstance_t const* self, char const* name)
{
   if ((self != NULL) && (name != NULL))
   {
      apx_size_t port_id;
      if (self->num_provide_ports > 0)
      {
         for (port_id = 0u; port_id < self->num_provide_ports; port_id++)
         {
            apx_portInstance_t* port_instance = &self->provide_ports[port_id];
            char const* port_name = apx_portInstance_name(port_instance);
            if ((port_name != NULL) && (strcmp(port_name, name) == 0))
            {
               return port_instance;
            }
         }
      }
      if (self->num_require_ports > 0)
      {
         for (port_id = 0u; port_id < self->num_require_ports; port_id++)
         {
            apx_portInstance_t* port_instance = &self->require_ports[port_id];
            char const* port_name = apx_portInstance_name(port_instance);
            if ((port_name != NULL) && (strcmp(port_name, name) == 0))
            {
               return port_instance;
            }
         }
      }
   }
   return NULL;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t calc_init_data_size(apx_portInstance_t* port_list, apx_size_t num_ports, apx_size_t* total_size)
{
   if ((port_list == NULL) || (num_ports == 0))
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   *total_size = 0u;
   for (apx_size_t port_id = 0u; port_id < num_ports; port_id++)
   {
      apx_size_t data_size = apx_portInstance_data_size(&port_list[port_id]);
      assert(data_size > 0u);
      *total_size += data_size;
   }
   return APX_NO_ERROR;
}

static apx_error_t create_definition_file_info(apx_nodeInstance_t* self, rmf_fileInfo_t* file_info)
{
   assert(self->node_data != NULL);
   char file_name[RMF_MAX_FILE_NAME_SIZE + 1];
   uint8_t digest_data[RMF_SHA256_SIZE];
   uint32_t file_size = (uint32_t) apx_nodeData_definition_data_size(self->node_data);
   strcpy(file_name, self->name);
   strcat(file_name, ".apx");

   sha256_calc(&digest_data[0], apx_nodeInstance_get_definition_data(self), (size_t)apx_nodeInstance_get_definition_size(self));
   return rmf_fileInfo_create(file_info, RMF_INVALID_ADDRESS, file_size, file_name,
      RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_SHA256, &digest_data[0]);
}

static apx_error_t create_provide_port_data_file_info(apx_nodeInstance_t* self, rmf_fileInfo_t* file_info)
{
   assert((self != NULL) && (file_info != NULL) && (self->provide_port_init_data_size > 0u));
   char file_name[RMF_MAX_FILE_NAME_SIZE + 1];
   strcpy(file_name, self->name);
   strcat(file_name, APX_PROVIDE_PORT_DATA_EXT);
   return rmf_fileInfo_create(file_info, RMF_INVALID_ADDRESS, self->provide_port_init_data_size, file_name,
      RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
}

static apx_error_t create_require_port_data_file_info(apx_nodeInstance_t* self, rmf_fileInfo_t* file_info)
{
   assert((self != NULL) && (file_info != NULL) && (self->require_port_init_data_size > 0u));
   char file_name[RMF_MAX_FILE_NAME_SIZE + 1];
   strcpy(file_name, self->name);
   strcat(file_name, APX_REQUIRE_PORT_DATA_EXT);
   return rmf_fileInfo_create(file_info, RMF_INVALID_ADDRESS, self->require_port_init_data_size, file_name,
      RMF_FILE_TYPE_FIXED, RMF_DIGEST_TYPE_NONE, NULL);
}

static apx_error_t file_open_notify(apx_nodeInstance_t* self, apx_file_t* file)
{
   if ( (file != NULL) && (self != NULL) && (self->node_data != NULL) )
   {
      apx_fileManager_t* file_manager = apx_file_get_file_manager(file);
      if (file_manager != NULL)
      {
         apx_error_t retval = APX_NO_ERROR;
         apx_fileType_t file_type = apx_file_get_apx_file_type(file);
         uint32_t address = apx_file_get_address_without_flags(file);
         switch (file_type)
         {
         case APX_DEFINITION_FILE_TYPE:
            retval = send_definition_data_to_file_manager(self, file_manager, address);
            break;
         case APX_PROVIDE_PORT_DATA_FILE_TYPE:
            retval = send_provide_port_data_to_file_manager(self, file_manager, address);
            break;
         case APX_REQUIRE_PORT_DATA_FILE_TYPE:
            if (self->mode == APX_SERVER_MODE)
            {
               if (self->server != NULL)
               {
                  apx_server_take_global_lock(self->server);
                  retval = connect_require_ports_to_server(self);
                  if (retval == APX_NO_ERROR)
                  {
                     apx_nodeInstance_set_require_port_data_state(self, APX_DATA_STATE_CONNECTED);
                     //TODO:Take snapshot first, then release global lock, then transmit snapshot data through file manager.
                     //This shortens the time the global lock is held.
                     retval = send_require_port_data_to_file_manager(self, file_manager, address);
                  }
                  apx_server_release_global_lock(self->server);
               }
               else
               {
                  apx_nodeInstance_set_require_port_data_state(self, APX_DATA_STATE_CONNECTED);
                  retval = send_require_port_data_to_file_manager(self, file_manager, address);
               }
            }
            break;
         default:
            retval = APX_UNSUPPORTED_ERROR;
         }
         return retval;
      }
   }
   return APX_NULL_PTR_ERROR;
}

static apx_error_t send_definition_data_to_file_manager(apx_nodeInstance_t* self, apx_fileManager_t* file_manager, uint32_t address)
{
   uint8_t const* definition_data = apx_nodeData_get_definition_data(self->node_data);
   apx_size_t const definition_size = apx_nodeData_definition_data_size(self->node_data);
   return apx_fileManager_send_local_const_data(file_manager, address, definition_data, definition_size);
}

static apx_error_t send_provide_port_data_to_file_manager(apx_nodeInstance_t* self, apx_fileManager_t* file_manager, uint32_t address)
{
   uint8_t* snapshot = apx_nodeData_take_provide_port_data_snapshot(self->node_data);
   apx_size_t const provide_port_data_size = apx_nodeData_provide_port_data_size(self->node_data);
   if (snapshot == NULL)
   {
      return APX_MEM_ERROR;
   }
   return apx_fileManager_send_local_data(file_manager, address, snapshot, provide_port_data_size);
}

static apx_error_t send_require_port_data_to_file_manager(apx_nodeInstance_t* self, apx_fileManager_t* file_manager, uint32_t address)
{
   uint8_t* snapshot = apx_nodeData_take_require_port_data_snapshot(self->node_data);
   apx_size_t const require_port_data_size = apx_nodeData_require_port_data_size(self->node_data);
   if (snapshot == NULL)
   {
      return APX_MEM_ERROR;
   }
   return apx_fileManager_send_local_data(file_manager, address, snapshot, require_port_data_size);
}


static void set_file_notification_handler(apx_nodeInstance_t* self, apx_file_t* file)
{
   apx_fileNotificationHandler_t handler;

   assert((self != NULL) && (file != NULL));
   memset(&handler, 0, sizeof(handler));
   handler.arg = (void*)self;
   handler.open_notify = apx_nodeInstance_vfile_open_notify;
   handler.close_notify = apx_nodeInstance_vfile_close_notify;
   handler.write_notify = apx_nodeInstance_vfile_write_notify;
   apx_file_set_notification_handler(file, &handler);
}

static apx_error_t file_write_notify(apx_nodeInstance_t* self, apx_file_t* file, uint32_t offset, const uint8_t* data, apx_size_t size)
{
   apx_error_t retval = APX_NO_ERROR;
   switch (apx_file_get_apx_file_type(file))
   {
   case APX_DEFINITION_FILE_TYPE:
      retval = process_remote_write_definition_data(self, offset, data, size);
      break;
   case APX_PROVIDE_PORT_DATA_FILE_TYPE:
      if (self->mode == APX_SERVER_MODE)
      {
         retval = process_remote_write_provide_port_data(self, offset, data, size);
      }
      else
      {
         retval = APX_UNSUPPORTED_ERROR;
      }
      break;
   case APX_REQUIRE_PORT_DATA_FILE_TYPE:
      retval = process_remote_write_require_port_data(self, offset, data, size);
      break;
   default:
      retval = APX_NOT_IMPLEMENTED_ERROR;
   }
   return retval;
}

static apx_error_t process_remote_write_definition_data(apx_nodeInstance_t* self, uint32_t offset, const uint8_t* data, apx_size_t size)
{
   if (self->definition_data_state == APX_DATA_STATE_WAITING_FOR_FILE_DATA)
   {
      self->definition_data_state = APX_DATA_STATE_CONNECTED;
   }
   apx_error_t retval = apx_nodeData_write_definition_data(self->node_data, offset, data, size);
   if (retval == APX_NO_ERROR)
   {
      assert(self->parent != NULL);
      retval = apx_nodeManager_on_definition_data_written(self->parent, self, offset, size);
   }
   return retval;
}

static apx_error_t process_remote_write_require_port_data(apx_nodeInstance_t* self, uint32_t offset, const uint8_t* data, apx_size_t size)
{
   if (self->require_port_data_state == APX_DATA_STATE_WAITING_FOR_FILE_DATA)
   {
      self->require_port_data_state = APX_DATA_STATE_CONNECTED;
   }
   apx_error_t retval = apx_nodeData_write_require_port_data(self->node_data, offset, data, size);
   if ( (retval == APX_NO_ERROR) && (self->mode == APX_CLIENT_MODE) )
   {
      assert(self->parent != NULL);
      retval = trigger_require_port_write_callbacks(self, offset, data, size);
   }
   return retval;
}

static apx_error_t process_remote_write_provide_port_data(apx_nodeInstance_t* self, uint32_t offset, const uint8_t* data, apx_size_t size)
{
   apx_error_t retval = APX_NO_ERROR;
   apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(self);
   assert(node_data != NULL);
   switch (self->provide_port_data_state)
   {
   case APX_DATA_STATE_INIT:
      retval = APX_INTERNAL_ERROR;
      break;
   case APX_DATA_STATE_WAITING_FILE_INFO:
      retval = APX_INTERNAL_ERROR;
      break;
   case APX_DATA_STATE_WAITING_FOR_FILE_DATA:
      retval = apx_nodeData_write_provide_port_data(node_data, offset, data, size);
      if ( retval == APX_NO_ERROR )
      {
         if (self->server != NULL)
         {
            apx_server_take_global_lock(self->server);
            retval = apx_server_connect_node_instance_provide_ports(self->server, self);
            if (retval == APX_NO_ERROR)
            {
               apx_portConnectorChangeTable_t* provide_port_changes;
               apx_nodeInstance_set_provide_port_data_state(self, APX_DATA_STATE_CONNECTED);
               provide_port_changes = apx_nodeInstance_get_provide_port_connector_changes(self, false);
               if (provide_port_changes != NULL)
               {
                  retval = apx_server_process_provide_port_connector_changes(self->server, self, provide_port_changes);
               }
            }
            if (retval == APX_NO_ERROR)
            {
               apx_nodeInstance_clear_provide_port_connector_changes(self, true); ///TODO: switch this to false once event handlers are working again
            }
            //TODO: Update port count in all affected nodes and trigger sending of port count deltas to clients
            apx_server_clear_port_connector_changes(self->server);
            apx_server_release_global_lock(self->server);
         }
         else
         {
            apx_nodeInstance_set_provide_port_data_state(self, APX_DATA_STATE_CONNECTED);
         }
      }
      break;
   case APX_DATA_STATE_CONNECTED:
      if (self->server != NULL)
      {
         retval = apx_nodeData_write_provide_port_data(node_data, offset, data, size);
         if (retval == APX_NO_ERROR)
         {
            retval = route_provide_port_data_change_to_receivers(self, offset, data, size);
         }
      }
      break;
   case APX_DATA_STATE_DISCONNECTED:
      break; //Drop all data writes in this state, we are about to close connection
   }
   return retval;
}

static apx_error_t apx_nodeInstance_attach_to_file_manager_client_mode(apx_nodeInstance_t* self, struct apx_fileManager_tag* file_manager)
{
   rmf_fileInfo_t file_info;
   apx_error_t result = APX_NO_ERROR;

   if (apx_nodeInstance_has_provide_port_data(self))
   {
      apx_nodeInstance_set_provide_port_data_state(self, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST);
      result = create_provide_port_data_file_info(self, &file_info);
      if (result == APX_NO_ERROR)
      {
         self->provide_port_data_file = apx_fileManager_create_local_file(file_manager, &file_info);
         rmf_fileInfo_destroy(&file_info);
         if (self->provide_port_data_file == NULL)
         {
            return APX_FILE_CREATE_ERROR;
         }
         set_file_notification_handler(self, self->provide_port_data_file);
      }
      else
      {
         return result;
      }
   }
   if (apx_nodeInstance_has_require_port_data(self))
   {
      apx_nodeInstance_set_require_port_data_state(self, APX_DATA_STATE_WAITING_FILE_INFO);
   }
   result = create_definition_file_info(self, &file_info);
   if (result == APX_NO_ERROR)
   {
      apx_file_t* definition_data_file = apx_fileManager_create_local_file(file_manager, &file_info);
      rmf_fileInfo_destroy(&file_info);
      if (definition_data_file == NULL)
      {
         return APX_FILE_CREATE_ERROR;
      }
      set_file_notification_handler(self, definition_data_file);
   }
   return result;
}

static apx_error_t apx_nodeInstance_attach_to_file_manager_server_mode(apx_nodeInstance_t* self, struct apx_fileManager_tag* file_manager)
{
   rmf_fileInfo_t file_info;
   apx_error_t retval = APX_NO_ERROR;

   if (apx_nodeInstance_has_provide_port_data(self))
   {
      apx_nodeInstance_set_provide_port_data_state(self, APX_DATA_STATE_WAITING_FILE_INFO);
      retval = search_for_remote_provide_port_data_file(self, file_manager);
   }
   if (apx_nodeInstance_has_require_port_data(self))
   {
      apx_nodeInstance_set_require_port_data_state(self, APX_DATA_STATE_WAITING_FOR_FILE_OPEN_REQUEST);
      retval = create_require_port_data_file_info(self, &file_info);
      if (retval == APX_NO_ERROR)
      {
         self->require_port_data_file = apx_fileManager_create_local_file(file_manager, &file_info);
         rmf_fileInfo_destroy(&file_info);
         if (self->require_port_data_file == NULL)
         {
            retval = APX_FILE_CREATE_ERROR;
         }
         else
         {
            rmf_fileInfo_t const* attached_file_info = apx_file_get_file_info(self->require_port_data_file);
            assert((attached_file_info != NULL) && (rmf_fileInfo_address_without_flags(attached_file_info) != RMF_INVALID_ADDRESS));
            set_file_notification_handler(self, self->require_port_data_file);
            retval = apx_fileManager_publish_local_file(file_manager, attached_file_info);
         }
      }
      else
      {
         return retval;
      }
   }
   return retval;
}

static apx_error_t search_for_remote_provide_port_data_file(apx_nodeInstance_t* self, struct apx_fileManager_tag* file_manager)
{
   apx_error_t retval = APX_NO_ERROR;
   adt_str_t* str = adt_str_new_cstr(apx_nodeInstance_get_name(self));
   if (str != NULL)
   {
      adt_error_t rc = adt_str_append_cstr(str, APX_PROVIDE_PORT_DATA_EXT);
      if (rc != ADT_NO_ERROR)
      {
         retval = convert_from_adt_to_apx_error(rc);
      }
      else
      {
         apx_file_t* file = apx_fileManager_find_remote_file_by_name(file_manager, adt_str_cstr(str));
         adt_str_delete(str);
         if (file != NULL)
         {
            set_file_notification_handler(self, file);
            retval = request_remote_provide_port_data(self, file);
         }
      }
   }
   if (retval != APX_NO_ERROR)
   {
      apx_fileManager_send_error_code(file_manager, retval);
   }
   return retval;
}

static apx_error_t request_remote_provide_port_data(apx_nodeInstance_t* self, apx_file_t* file)
{
   apx_fileManager_t* file_manager = apx_file_get_file_manager(file);
   assert(file_manager != NULL);
   apx_nodeInstance_set_provide_port_data_state(self, APX_DATA_STATE_WAITING_FOR_FILE_DATA);
   apx_file_open(file); //Should this be moved into file_manager?
   return apx_fileManager_send_open_file_request(file_manager, apx_file_get_address_without_flags(file));
}

static apx_error_t request_remote_require_port_data(apx_nodeInstance_t* self, apx_file_t* file)
{
   apx_fileManager_t* file_manager = apx_file_get_file_manager(file);
   assert(file_manager != NULL);
   assert(apx_nodeInstance_get_require_port_data_state(self) == APX_DATA_STATE_WAITING_FILE_INFO);
   apx_nodeInstance_set_require_port_data_state(self, APX_DATA_STATE_WAITING_FOR_FILE_DATA);
   apx_file_open(file); //Should this be moved into file_manager?
   return apx_fileManager_send_open_file_request(file_manager, apx_file_get_address_without_flags(file));
}

static apx_error_t connect_require_ports_to_server(apx_nodeInstance_t* self)
{
   apx_error_t retval = APX_NO_ERROR;
   assert( (self != NULL) && (self->server != NULL));
   retval = apx_server_connect_node_instance_require_ports(self->server, self);
   if (retval == APX_NO_ERROR)
   {
      apx_portConnectorChangeTable_t* require_port_changes;
      require_port_changes = apx_nodeInstance_get_require_port_connector_changes(self, false);
      if (require_port_changes != NULL)
      {
         retval = apx_server_process_require_port_connector_changes(self->server, self, require_port_changes);
      }
   }
   //TODO: update port counts and trigger transmission of port count delta
   apx_server_clear_port_connector_changes(self->server);
   return retval;
}

static apx_error_t route_provide_port_data_change_to_receivers(apx_nodeInstance_t* self, uint32_t provide_data_offset, const uint8_t* provide_data, apx_size_t provide_data_size)
{
   apx_error_t retval = APX_NO_ERROR;
   uint32_t end_offset = provide_data_offset + provide_data_size;
   assert(self->connector_table != NULL);
   assert(self->byte_port_map != NULL);
   MUTEX_LOCK(self->lock);
   while (provide_data_offset < end_offset)
   {
      apx_portId_t provide_port_id;
      apx_portInstance_t* provide_port = NULL;
      provide_port_id = apx_bytePortMap_lookup(self->byte_port_map, provide_data_offset);
      if (provide_port_id == APX_INVALID_PORT_ID)
      {
         fprintf(stderr, "[APX_NODE_INSTANCE] blocked write on invalid offset %u\n", provide_data_offset);
         retval = APX_INVALID_WRITE_ERROR;
         break;
      }
      else if (provide_port_id > self->num_provide_ports)
      {
         fprintf(stderr, "[APX_NODE_INSTANCE] Invalid port id detected in bytePortMap: %u\n", (unsigned int)provide_port_id);
         retval = APX_INTERNAL_ERROR;
      }
      provide_port = apx_nodeInstance_get_provide_port(self, provide_port_id);
      if (provide_port != NULL)
      {
         apx_portConnectorList_t* port_connectors;
         int32_t num_connectors;
         int32_t connector_id;
         apx_size_t provide_port_data_size = apx_portInstance_data_size(provide_port);
         assert(provide_port_data_size > 0u);
         port_connectors = &self->connector_table[provide_port_id];
         num_connectors = apx_portConnectorList_length(port_connectors);
         for (connector_id = 0; connector_id < num_connectors; connector_id++)
         {
            apx_portInstance_t* require_port = apx_portConnectorList_get(port_connectors, connector_id);
            assert( (require_port != NULL) && (require_port->parent != NULL));
            if (apx_portInstance_queue_length(provide_port) == 0u)
            {
               apx_size_t require_port_data_size = apx_portInstance_data_size(require_port);
               if (provide_port_data_size != require_port_data_size)
               {
                  retval = APX_VALUE_LENGTH_ERROR;
               }
               else
               {
                  apx_size_t require_data_offset = apx_portInstance_data_offset(require_port);
                  retval = remote_route_require_port_data(require_port->parent, require_data_offset, provide_data, provide_port_data_size);
               }
            }
            else
            {
               retval = APX_NOT_IMPLEMENTED_ERROR;
            }
         }
         provide_data_offset += provide_port_data_size;
         provide_data += provide_port_data_size;
      }
      else
      {
         fprintf(stderr, "[APX_NODE_INSTANCE] Invalid port id detected in bytePortMap: %u\n", (unsigned int)provide_port_id);
         retval = APX_INTERNAL_ERROR;
      }
   }
   MUTEX_UNLOCK(self->lock);
   return retval;
}

/*
* Note: Caller must take self->lock before calling this function
*/
static apx_error_t remove_provide_port_connector(apx_nodeInstance_t* self, apx_portId_t provide_port_id, apx_portInstance_t* require_port)
{
   assert(self->connector_table != NULL);
   if (provide_port_id < self->num_provide_ports)
   {
      apx_portConnectorList_t* connector_list = &self->connector_table[provide_port_id];
      apx_portConnectorList_remove(connector_list, require_port);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t route_provide_port_data_to_require_port(apx_portInstance_t* provide_port, apx_portInstance_t* require_port, bool do_remote_routing)
{
   apx_error_t retval = APX_NO_ERROR;
   assert( (provide_port != NULL) && provide_port->parent != NULL);
   assert( (require_port != NULL) && require_port->parent != NULL);
   if (apx_portInstance_queue_length(provide_port) == 0u)
   {
      apx_size_t const provide_port_data_size = apx_portInstance_data_size(provide_port);
      apx_size_t const require_port_data_size = apx_portInstance_data_size(require_port);
      if (provide_port_data_size == require_port_data_size)
      {
         uint8_t stack_data_buffer[STACK_DATA_BUF_SIZE];
         apx_nodeInstance_t* provide_node = provide_port->parent;
         apx_nodeInstance_t* require_node = require_port->parent;
         apx_size_t const require_data_offset = apx_portInstance_data_offset(require_port);
         apx_size_t const provide_data_offset = apx_portInstance_data_offset(provide_port);
         uint8_t* provide_port_data = &stack_data_buffer[0];
         bool use_heap_data = provide_port_data_size > STACK_DATA_BUF_SIZE ? true : false;
         if (use_heap_data)
         {
            provide_port_data = (uint8_t*)malloc(provide_port_data_size);
            if (provide_port_data == NULL)
            {
               retval = APX_MEM_ERROR;
            }
         }
         if (retval == APX_NO_ERROR)
         {
            assert((provide_node->node_data != NULL) && (require_node->node_data != NULL));
            retval = apx_nodeData_read_provide_port_data(provide_node->node_data, provide_data_offset, provide_port_data, provide_port_data_size);
         }
         if (retval == APX_NO_ERROR)
         {
            if (do_remote_routing)
            {
               retval = remote_route_require_port_data(require_node, require_data_offset, provide_port_data, provide_port_data_size);
            }
            else
            {
               retval = apx_nodeData_write_require_port_data(require_node->node_data, require_data_offset, provide_port_data, provide_port_data_size);
            }
         }
         if (use_heap_data)
         {
            free(provide_port_data);
         }
      }
      else
      {
         retval = APX_VALUE_LENGTH_ERROR;
      }
   }
   else
   {
      retval = APX_NOT_IMPLEMENTED_ERROR;
   }
   return retval;
}

static apx_error_t remote_route_require_port_data(apx_nodeInstance_t* self, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   apx_file_t* file = self->require_port_data_file;
   apx_error_t retval = apx_nodeData_write_require_port_data(self->node_data, offset, data, size);
   if ((retval == APX_NO_ERROR) && (file != NULL))
   {
      retval = remote_route_data_to_file(file, offset, data, size);
   }
   return retval;
}

static apx_error_t remote_route_provide_port_data(apx_nodeInstance_t* self, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   apx_file_t* file = self->provide_port_data_file;
   apx_error_t retval = apx_nodeData_write_provide_port_data(self->node_data, offset, data, size);
   if ((retval == APX_NO_ERROR) && (file != NULL))
   {
      retval = remote_route_data_to_file(file, offset, data, size);
   }
   return retval;
}

static apx_error_t remote_route_data_to_file(apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size)
{
   apx_error_t retval = APX_NO_ERROR;
   assert(file != NULL);

   apx_fileManager_t* file_manager = apx_file_get_file_manager(file);
   if (file_manager == NULL)
   {
      retval = APX_NULL_PTR_ERROR;
   }
   if (retval == APX_NO_ERROR)
   {
      if (!apx_file_is_open(file))
      {
         retval = APX_FILE_NOT_OPEN_ERROR;
      }
   }
   if (retval == APX_NO_ERROR)
   {
      uint8_t* allocated_buffer;
      uint32_t address = apx_file_get_address_without_flags(file) + offset;
      //TODO: use small object allocator later on
      allocated_buffer = (uint8_t*)malloc(size);
      if (allocated_buffer == NULL)
      {
         retval = APX_MEM_ERROR;
      }
      else
      {
         memcpy(allocated_buffer, data, size);
         retval = apx_fileManager_send_local_data(file_manager, address, allocated_buffer, size);
      }
   }
   return retval;
}

static apx_error_t trigger_require_port_write_callbacks(apx_nodeInstance_t* self, uint32_t offset, const uint8_t* data, apx_size_t size)
{
   apx_error_t retval = APX_NO_ERROR;
   assert((self != NULL) && (data != NULL));
   apx_size_t end_offset = offset + size;
   if (self->byte_port_map == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   while (offset < end_offset)
   {
      apx_portId_t port_id = apx_bytePortMap_lookup(self->byte_port_map, offset);
      if (port_id != APX_INVALID_PORT_ID)
      {
         apx_portInstance_t* port_instance = apx_nodeInstance_get_require_port(self, port_id);
         if (port_instance != NULL)
         {
            uint32_t const port_data_size = apx_portInstance_data_size(port_instance);
            apx_nodeManager_on_require_port_written(self->parent, port_instance, data, port_data_size);
            offset += port_data_size;
            data += port_data_size;
         }
         else
         {
            retval = APX_INVALID_WRITE_ERROR;
            break;
         }
      }
      else
      {
         retval = APX_INVALID_WRITE_ERROR;
         break;
      }
   }
   return retval;
}