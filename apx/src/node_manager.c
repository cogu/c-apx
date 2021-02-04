/*****************************************************************************
* \file      node_manager.c
* \author    Conny Gustafsson
* \date      2019-12-29
* \brief     Manager for apx_nodeInstance objects
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
#include <malloc.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include "apx/node_manager.h"
#include "apx/vm.h"
#include "apx/connection_base.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t create_node_instance(apx_nodeManager_t* self, apx_node_t const* node, uint8_t const* definition_data, apx_size_t definition_size);
static apx_error_t build_node_instance(apx_nodeManager_t* self, apx_nodeInstance_t* node_instance, apx_node_t const* node);
static void attach_node(apx_nodeManager_t* self, apx_nodeInstance_t* node_instance);
static apx_error_t create_ports_on_node_instance(apx_nodeManager_t* self, apx_nodeInstance_t* node_instance, apx_node_t const* node,
   apx_size_t* expected_provide_port_data_size, apx_size_t* expected_require_port_data_size);
static apx_error_t create_init_data_on_node_instance(apx_nodeInstance_t* node_instance, apx_node_t const* node,
   apx_size_t expected_provide_port_data_size, size_t expected_require_port_data_size);
static apx_error_t create_port_init_data(apx_vm_t* vm, apx_portInstance_t* port_instance, dtl_dv_t* value, uint8_t* data, size_t data_size);
static apx_error_t create_data_element_list_on_node_instance(apx_nodeInstance_t* node_instance, apx_node_t const* node);
static apx_error_t update_data_element_list_on_port(adt_ary_t* list, adt_hash_t* map,
   apx_portInstance_t* port_instance, apx_port_t const* parsed_port);
static apx_error_t create_computation_list_on_node_instance(apx_nodeInstance_t* node_instance, apx_node_t const* node);
static apx_error_t update_computation_list_on_port(adt_ary_t* list, adt_hash_t* map,
   apx_portInstance_t* port_instance, apx_port_t const* parsed_port);
static apx_error_t create_port_signatures_on_node_instance(apx_nodeInstance_t* node_instance);
static apx_error_t init_node_instance_from_file_info(apx_nodeManager_t* self, rmf_fileInfo_t const* file_info, bool* file_open_request);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_nodeManager_create(apx_nodeManager_t *self, apx_mode_t mode)
{
   if ( (self != 0) && ( (mode == APX_CLIENT_MODE) || (mode == APX_SERVER_MODE) ) )
   {
      self->mode = mode;
      self->last_attached = NULL;
      self->parent_connection = NULL;
      apx_compiler_create(&self->compiler);
      apx_istream_create(&self->stream);
      apx_parser_create(&self->parser, &self->stream);
      adt_hash_create(&self->instance_map, apx_nodeInstance_vdelete);
      MUTEX_INIT(self->lock);
   }
}

void apx_nodeManager_destroy(apx_nodeManager_t *self)
{
   if (self != 0)
   {
      apx_parser_destroy(&self->parser);
      apx_istream_destroy(&self->stream);
      adt_hash_destroy(&self->instance_map);
      apx_compiler_destroy(&self->compiler);
      MUTEX_DESTROY(self->lock);
   }
}

apx_nodeManager_t *apx_nodeManager_new(apx_mode_t mode)
{
   apx_nodeManager_t *self = (apx_nodeManager_t*) malloc(sizeof(apx_nodeManager_t));
   if (self != 0)
   {
      apx_nodeManager_create(self, mode);
   }
   return self;
}

void apx_nodeManager_delete(apx_nodeManager_t *self)
{
   if (self != 0)
   {
      apx_nodeManager_destroy(self);
      free(self);
   }
}


//Client-side API
apx_error_t apx_nodeManager_build_node(apx_nodeManager_t* self, char const* definition_text)
{
   if ((self != NULL) && (definition_text != NULL))
   {
      size_t definition_size = strlen(definition_text);
      apx_error_t result = apx_parser_parse_cstr(&self->parser, definition_text);
      apx_node_t* node = NULL;
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      node = apx_parser_take_last_node(&self->parser);
      assert(node != NULL);
      result = create_node_instance(self, node, (uint8_t const*)definition_text, (apx_size_t)definition_size);
      apx_node_delete(node);
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//Server-side API
apx_error_t apx_nodeManager_init_node_from_file_info(apx_nodeManager_t* self, rmf_fileInfo_t const* file_info, bool* file_open_request)
{
   if ( (self != NULL) && (file_info != NULL) && (file_open_request != NULL))
   {
      if (rmf_fileInfo_name_ends_with(file_info, ".apx"))
      {
         return init_node_instance_from_file_info(self, file_info, file_open_request);
      }
      return APX_INVALID_FILE_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_nodeManager_build_node_from_data(apx_nodeManager_t* self, apx_nodeInstance_t* node_instance)
{
   if (self != NULL && (node_instance != NULL))
   {
      apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
      if (node_data == NULL)
      {
         return APX_NULL_PTR_ERROR;
      }
      size_t definition_size = (size_t)apx_nodeData_definition_data_size(node_data);
      if (definition_size == 0u)
      {
         return APX_LENGTH_ERROR;
      }
      uint8_t* definition_data = apx_nodeData_take_definition_data_snapshot(node_data);
      if (definition_data == NULL)
      {
         return APX_MEM_ERROR;
      }
      apx_error_t result = apx_parser_parse_bstr(&self->parser, definition_data, definition_data + definition_size);
      apx_node_t* node = NULL;
      free(definition_data);
      if (result == APX_NO_ERROR)
      {
         node = apx_parser_take_last_node(&self->parser);
         assert(node != NULL);
         result = build_node_instance(self, node_instance, node);
         apx_node_delete(node);
      }
      if (result != APX_NO_ERROR)
      {
         char const* name = apx_nodeInstance_get_name(node_instance);
         if (name != NULL)
         {
            adt_hash_remove(&self->instance_map, name);
            apx_nodeInstance_delete(node_instance);
         }
      }
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//Common API
struct apx_nodeInstance_tag* apx_nodeManager_get_last_attached(apx_nodeManager_t const* self)
{
   if (self != NULL)
   {
      return self->last_attached;
   }
   return NULL;
}

apx_size_t apx_nodeManager_length(apx_nodeManager_t const* self)
{
   if (self != NULL)
   {
      return adt_hash_length(&self->instance_map);
   }
   return 0u;
}

adt_ary_t* apx_nodeManager_get_nodes(apx_nodeManager_t* self)
{
   if (self != NULL)
   {
      adt_ary_t* array = adt_ary_new(NULL);
      if (array != NULL)
      {
         (void)adt_hash_values(&self->instance_map, array);
      }
      return array;
   }
   return NULL;
}

struct apx_nodeInstance_tag* apx_nodeManager_find(apx_nodeManager_t const* self, char const* name)
{
   if (self != NULL)
   {
      return adt_hash_value(&self->instance_map, name);
   }
   return NULL;

}

void apx_nodeManager_set_connection(apx_nodeManager_t* self, struct apx_connectionBase_tag* connection)
{
   if (self != NULL)
   {
      self->parent_connection = connection;
   }
}

struct apx_connectionBase_tag* apx_nodeManager_get_connection(apx_nodeManager_t const* self)
{
   if (self != NULL)
   {
      return self->parent_connection;
   }
   return NULL;

}

apx_error_t apx_nodeManager_on_definition_data_written(apx_nodeManager_t* self, struct apx_nodeInstance_tag* node_instance, uint32_t offset, apx_size_t size)
{
   if (self != NULL && node_instance != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
      if (node_data == NULL)
      {
         retval = APX_NULL_PTR_ERROR;
      }
      if ((offset == 0u) && (size == apx_nodeData_definition_data_size(node_data)))
      {
         retval = apx_nodeManager_build_node_from_data(self, node_instance);
         if (retval == APX_NO_ERROR)
         {
            if (self->parent_connection != NULL)
            {
               apx_fileManager_t* file_manager = apx_connectionBase_get_file_manager(self->parent_connection);
               if (file_manager != NULL)
               {
                  apx_nodeInstance_attach_to_file_manager(node_instance, file_manager);
               }
            }
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_nodeManager_on_require_port_data_written(apx_nodeManager_t* self, struct apx_nodeInstance_tag* node_instance, uint32_t offset, apx_size_t size)
{
   (void)self;
   (void)node_instance;
   (void)offset;
   (void)size;
}

void apx_nodeManager_on_provide_port_data_written(apx_nodeManager_t* self, struct apx_nodeInstance_tag* node_instance, uint32_t offset, apx_size_t size)
{
   (void)self;
   (void)node_instance;
   (void)offset;
   (void)size;
}

int32_t apx_nodeManager_values(apx_nodeManager_t* self, adt_ary_t* array)
{
   if ((self != 0) && (array != 0))
   {
      int32_t retval;
      MUTEX_LOCK(self->lock);
      retval = adt_hash_values(&self->instance_map, array);
      MUTEX_UNLOCK(self->lock);
      return retval;
   }
   return -1;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t create_node_instance(apx_nodeManager_t* self, apx_node_t const* node, uint8_t const* definition_data, apx_size_t definition_size)
{
   assert(self != NULL);
   if ( (node == NULL) || (definition_data == NULL) || (definition_size == 0u))
   {
      return APX_INVALID_ARGUMENT_ERROR;
   }
   apx_error_t result = APX_NO_ERROR;
   apx_nodeInstance_t* node_instance = apx_nodeInstance_new(self->mode, apx_node_get_name(node));
   result = apx_nodeInstance_init_node_data(node_instance, definition_data, definition_size);
   if (result == APX_NO_ERROR)
   {
      result = build_node_instance(self, node_instance, node);
   }
   if (result == APX_NO_ERROR)
   {
      attach_node(self, node_instance);
   }
   else
   {
      apx_nodeInstance_delete(node_instance);
   }
   return result;
}

static apx_error_t build_node_instance(apx_nodeManager_t* self, apx_nodeInstance_t* node_instance, apx_node_t const* node)
{
   apx_size_t expected_provide_port_data_size = 0u;
   apx_size_t expected_require_port_data_size = 0u;
   apx_error_t result = create_ports_on_node_instance(self, node_instance, node, &expected_provide_port_data_size, &expected_require_port_data_size);
   if (result == APX_NO_ERROR)
   {
      result = create_init_data_on_node_instance(node_instance, node, expected_provide_port_data_size, expected_require_port_data_size);
   }
   if (result == APX_NO_ERROR)
   {
      result = apx_nodeInstance_finalize_node_data(node_instance);
   }
   if (result == APX_NO_ERROR)
   {
      result = apx_nodeInstance_create_byte_port_map(node_instance);
   }
   if ((result == APX_NO_ERROR) && (self->mode == APX_SERVER_MODE))
   {
      result = apx_nodeInstance_build_connector_table(node_instance);
   }
   return result;
}

static void attach_node(apx_nodeManager_t* self, apx_nodeInstance_t* node_instance)
{
   assert(self != NULL);
   if (node_instance != NULL)
   {
      adt_hash_set(&self->instance_map, apx_nodeInstance_get_name(node_instance), (void*)node_instance);
      apx_nodeInstance_set_parent(node_instance, self);
      self->last_attached = node_instance;
      if (self->parent_connection != NULL)
      {
         apx_connectionBase_node_created_notification(self->parent_connection, node_instance);
      }
   }
}

static apx_error_t create_ports_on_node_instance(apx_nodeManager_t* self, apx_nodeInstance_t* node_instance, apx_node_t const* node,
   apx_size_t* expected_provide_port_data_size, apx_size_t* expected_require_port_data_size)
{
   assert( (self != NULL) && (node_instance != NULL) && (node != NULL) &&
      (expected_provide_port_data_size != NULL) && (expected_require_port_data_size != NULL));
   apx_error_t result = APX_NO_ERROR;
   apx_size_t const num_provide_ports = (apx_size_t)apx_node_num_provide_ports(node);
   apx_size_t const num_require_ports = (apx_size_t)apx_node_num_require_ports(node);
   apx_size_t data_offset = 0u;
   result = apx_nodeInstance_alloc_port_instance_memory(node_instance, num_provide_ports, num_require_ports);
   if (result == APX_NO_ERROR)
   {
      apx_size_t port_id;
      for (port_id = 0u; port_id < num_provide_ports; port_id++)
      {
         apx_port_t* port = apx_node_get_provide_port(node, port_id);
         apx_program_t* pack_program;
         apx_size_t data_size = 0u;
         assert(port != NULL);
         pack_program = apx_compiler_compile_port(&self->compiler, port, APX_PACK_PROGRAM, &result);
         if (result == APX_NO_ERROR)
         {
            result = apx_nodeInstance_create_provide_port(node_instance, port_id, port->name, pack_program, data_offset, &data_size);
            if (result == APX_NO_ERROR)
            {
               data_offset += data_size;
            }
            else
            {
               APX_PROGRAM_DELETE(pack_program);
               break;
            }
         }
      }
      *expected_provide_port_data_size = data_offset;
      data_offset = 0u;
      if (result == APX_NO_ERROR)
      {
         for (port_id = 0u; port_id < num_require_ports; port_id++)
         {
            apx_port_t* port = apx_node_get_require_port(node, port_id);
            apx_program_t* pack_program = NULL;
            apx_program_t* unpack_program = NULL;
            apx_size_t data_size = 0u;
            assert(port != NULL);
            pack_program = apx_compiler_compile_port(&self->compiler, port, APX_PACK_PROGRAM, &result);
            if (result == APX_NO_ERROR)
            {
               unpack_program = apx_compiler_compile_port(&self->compiler, port, APX_UNPACK_PROGRAM, &result);
            }
            if (result == APX_NO_ERROR)
            {
               result = apx_nodeInstance_create_require_port(node_instance, port_id, port->name, pack_program, unpack_program, data_offset, &data_size);
               if (result == APX_NO_ERROR)
               {
                  data_offset += data_size;
               }
               else
               {
                  APX_PROGRAM_DELETE(pack_program);
                  APX_PROGRAM_DELETE(unpack_program);
                  break;
               }
            }
         }
         *expected_require_port_data_size = data_offset;
      }
   }
   if (result == APX_NO_ERROR)
   {
      result = create_data_element_list_on_node_instance(node_instance, node);
   }
   if (result == APX_NO_ERROR)
   {
      result = create_computation_list_on_node_instance(node_instance, node);
   }
   if ( (result == APX_NO_ERROR) && (self->mode == APX_SERVER_MODE) )
   {
      result = create_port_signatures_on_node_instance(node_instance);
   }
   return result;
}

static apx_error_t create_init_data_on_node_instance(apx_nodeInstance_t* node_instance, apx_node_t const* node,
   apx_size_t expected_provide_port_data_size, size_t expected_require_port_data_size)
{
   apx_size_t provide_port_data_size = 0u;
   apx_size_t require_port_data_size = 0u;
   uint8_t* provide_port_data = NULL;
   uint8_t* require_port_data = NULL;
   apx_error_t result = apx_nodeInstance_alloc_init_data_memory(node_instance, &provide_port_data, &provide_port_data_size,
      &require_port_data, &require_port_data_size);
   if (result != APX_NO_ERROR)
   {
      return result;
   }
   if ((provide_port_data_size != expected_provide_port_data_size) ||
      (require_port_data_size != expected_require_port_data_size))
   {
      return APX_LENGTH_ERROR;
   }
   apx_size_t const num_provide_ports = (apx_size_t)apx_node_num_provide_ports(node);
   apx_size_t const num_require_ports = (apx_size_t)apx_node_num_require_ports(node);
   if ((num_provide_ports > 0u) && (provide_port_data == NULL))
   {
      return APX_MEM_ERROR;
   }
   if ((num_require_ports > 0u) && (require_port_data == NULL))
   {
      return APX_MEM_ERROR;
   }
   size_t data_offset = 0u;
   apx_vm_t vm;
   apx_error_t retval = APX_NO_ERROR;
   retval = apx_vm_create(&vm);
   if (retval == APX_NO_ERROR)
   {
      apx_size_t port_id;
      for (port_id = 0u; port_id < num_provide_ports; port_id++)
      {
         apx_port_t* parsed_port = apx_node_get_provide_port(node, port_id);
         apx_portInstance_t* port_instance = apx_nodeInstance_get_provide_port(node_instance, port_id);
         if ((parsed_port == NULL) || (port_instance == NULL))
         {
            retval = APX_NULL_PTR_ERROR;
            break;
         }
         uint32_t data_size = apx_portInstance_data_size(port_instance);
         assert(data_size > 0u);
         dtl_dv_t* proper_init_value = apx_port_get_proper_init_value(parsed_port);
         if (proper_init_value != NULL)
         {
            retval = create_port_init_data(&vm, port_instance, proper_init_value, provide_port_data + data_offset, data_size);
            if (retval != APX_NO_ERROR)
            {
               break;
            }
         }
         data_offset += data_size;
      }
      data_offset = 0u;
      if (retval == APX_NO_ERROR)
      {
         for (port_id = 0u; port_id < num_require_ports; port_id++)
         {
            apx_port_t* parsed_port = apx_node_get_require_port(node, port_id);
            apx_portInstance_t* port_instance = apx_nodeInstance_get_require_port(node_instance, port_id);
            if ((parsed_port == NULL) || (port_instance == NULL))
            {
               retval = APX_NULL_PTR_ERROR;
               break;
            }
            uint32_t data_size = apx_portInstance_data_size(port_instance);
            assert(data_size > 0u);
            dtl_dv_t* proper_init_value = apx_port_get_proper_init_value(parsed_port);
            if (proper_init_value != NULL)
            {
               retval = create_port_init_data(&vm, port_instance, proper_init_value, require_port_data + data_offset, data_size);
               if (retval != APX_NO_ERROR)
               {
                  break;
               }
            }
            data_offset += data_size;
         }
      }
      apx_vm_destroy(&vm);
   }
   return retval;
}

static apx_error_t create_port_init_data(apx_vm_t* vm, apx_portInstance_t* port_instance, dtl_dv_t* value, uint8_t* data, size_t data_size)
{
   assert((port_instance != NULL) && (value != NULL) && (data != NULL));
   apx_program_t const* pack_program = apx_portInstance_pack_program(port_instance);
   apx_error_t result = apx_vm_select_program(vm, pack_program);
   if (result != APX_NO_ERROR)
   {
      return result;
   }
   result = apx_vm_set_write_buffer(vm, data, (uint32_t) data_size);
   if (result != APX_NO_ERROR)
   {
      return result;
   }
   return apx_vm_pack_value(vm, value);
}

static apx_error_t create_data_element_list_on_node_instance(apx_nodeInstance_t* node_instance, apx_node_t const* node)
{
   apx_size_t const num_provide_ports = (apx_size_t)apx_node_num_provide_ports(node);
   apx_size_t const num_require_ports = (apx_size_t)apx_node_num_require_ports(node);
   adt_ary_t data_element_list;
   adt_hash_t data_element_map;
   apx_size_t port_id;
   apx_error_t retval = APX_NO_ERROR;

   adt_ary_create(&data_element_list, apx_dataElement_vdelete);
   adt_hash_create(&data_element_map, NULL);
   for (port_id = 0u; port_id < num_provide_ports; port_id++)
   {
      apx_port_t* parsed_port = apx_node_get_provide_port(node, port_id);
      apx_portInstance_t* port_instance = apx_nodeInstance_get_provide_port(node_instance, port_id);
      if ((parsed_port == NULL) || (port_instance == NULL))
      {
         retval = APX_NULL_PTR_ERROR;
         break;
      }
      retval = update_data_element_list_on_port(&data_element_list, &data_element_map, port_instance, parsed_port);
      if (retval != APX_NO_ERROR)
      {
         break;
      }
   }
   if (retval == APX_NO_ERROR)
   {
      for (port_id = 0u; port_id < num_require_ports; port_id++)
      {
         apx_port_t* parsed_port = apx_node_get_require_port(node, port_id);
         apx_portInstance_t* port_instance = apx_nodeInstance_get_require_port(node_instance, port_id);
         if ((parsed_port == NULL) || (port_instance == NULL))
         {
            retval = APX_NULL_PTR_ERROR;
            break;
         }
         retval = update_data_element_list_on_port(&data_element_list, &data_element_map, port_instance, parsed_port);
         if (retval != APX_NO_ERROR)
         {
            break;
         }
      }
   }
   if (retval == APX_NO_ERROR)
   {
      if (adt_ary_length(&data_element_list) > 0u)
      {
         retval = apx_nodeInstance_create_data_element_list(node_instance, &data_element_list);
      }
   }
   adt_ary_destroy(&data_element_list);
   adt_hash_destroy(&data_element_map);
   return retval;
}

static apx_error_t update_data_element_list_on_port(adt_ary_t* list, adt_hash_t* map,
   apx_portInstance_t* port_instance, apx_port_t const* parsed_port)
{
   assert((port_instance != NULL) && (parsed_port != NULL));
   apx_size_t current_length = (apx_size_t)adt_ary_length(list);
   apx_dataElement_t* data_element = apx_port_get_effective_data_element(parsed_port);
   if (data_element == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   adt_str_t* signature = apx_dataElement_to_string(data_element, false);
   apx_dataElement_t* existing_item = (apx_dataElement_t*) adt_hash_value(map, adt_str_cstr(signature));
   if (existing_item == NULL)
   {
      apx_dataElement_t* clone = apx_dataElement_clone(data_element);
      if (clone == NULL)
      {
         adt_str_delete(signature);
         return APX_MEM_ERROR;
      }
      data_element = clone;
      apx_dataElement_set_id(data_element, (apx_elementId_t)current_length);
      adt_hash_set(map, adt_str_cstr(signature), data_element);
      adt_ary_push(list, data_element);
   }
   else
   {
      data_element = existing_item;
   }
   adt_str_delete(signature);
   apx_portInstance_set_effective_element(port_instance, data_element);
   return APX_NO_ERROR;
}

static apx_error_t create_computation_list_on_node_instance(apx_nodeInstance_t* node_instance, apx_node_t const* node)
{
   apx_size_t const num_provide_ports = (apx_size_t)apx_node_num_provide_ports(node);
   apx_size_t const num_require_ports = (apx_size_t)apx_node_num_require_ports(node);
   adt_ary_t computation_lists;
   adt_hash_t computation_map;
   apx_size_t port_id;
   apx_error_t retval = APX_NO_ERROR;

   adt_ary_create(&computation_lists, apx_dataElement_vdelete);
   adt_hash_create(&computation_map, NULL);
   for (port_id = 0u; port_id < num_provide_ports; port_id++)
   {
      apx_port_t* parsed_port = apx_node_get_provide_port(node, port_id);
      apx_portInstance_t* port_instance = apx_nodeInstance_get_provide_port(node_instance, port_id);
      if ((parsed_port == NULL) || (port_instance == NULL))
      {
         retval = APX_NULL_PTR_ERROR;
         break;
      }
      retval = update_computation_list_on_port(&computation_lists, &computation_map, port_instance, parsed_port);
      if (retval != APX_NO_ERROR)
      {
         break;
      }
   }
   if (retval == APX_NO_ERROR)
   {
      for (port_id = 0u; port_id < num_require_ports; port_id++)
      {
         apx_port_t* parsed_port = apx_node_get_require_port(node, port_id);
         apx_portInstance_t* port_instance = apx_nodeInstance_get_require_port(node_instance, port_id);
         if ((parsed_port == NULL) || (port_instance == NULL))
         {
            retval = APX_NULL_PTR_ERROR;
            break;
         }
         retval = update_computation_list_on_port(&computation_lists, &computation_map, port_instance, parsed_port);
         if (retval != APX_NO_ERROR)
         {
            break;
         }
      }
   }
   if (retval == APX_NO_ERROR)
   {
      if (adt_ary_length(&computation_lists) > 0u)
      {
         retval = apx_nodeInstance_create_computation_lists(node_instance, &computation_lists);
      }
   }
   adt_ary_destroy(&computation_lists);
   adt_hash_destroy(&computation_map);
   return retval;
}

static apx_error_t update_computation_list_on_port(adt_ary_t* list, adt_hash_t* map,
   apx_portInstance_t* port_instance, apx_port_t const* parsed_port)
{
   assert((port_instance != NULL) && (parsed_port != NULL));
   apx_size_t current_length = (apx_size_t)adt_ary_length(list);
   apx_typeAttributes_t* attributes = apx_port_get_referenced_type_attributes(parsed_port);
   int32_t const num_computations = apx_typeAttributes_num_computations(attributes);
   if ((attributes != NULL) && (num_computations > 0))
   {
      adt_str_t signature;
      apx_error_t retval = APX_NO_ERROR;
      bool first = true;
      int32_t i;
      adt_str_create(&signature);
      for (i=0; i < num_computations; i++)
      {
         apx_computation_t const* computation = apx_typeAttributes_get_computation(attributes, i);
         assert(computation != NULL);
         if (first)
         {
            first = false;
         }
         else
         {
            adt_str_push(&signature, ',');
         }
         adt_str_t* computation_str = apx_computation_to_string(computation);
         if (computation_str != NULL)
         {
            adt_str_append(&signature, computation_str);
            adt_str_delete(computation_str);
         }
         else
         {
            retval = APX_NULL_PTR_ERROR;
            break;
         }
      }
      if (retval == APX_NO_ERROR)
      {
         apx_computationList_t* computation_list = NULL;
         apx_computationList_t* existing_item = (apx_computationList_t*)adt_hash_value(map, adt_str_cstr(&signature));
         if (existing_item == NULL)
         {
            computation_list = apx_computationList_new();
            if (computation_list != NULL)
            {
               for (i = 0; i < num_computations; i++)
               {
                  apx_computation_t const* computation = apx_typeAttributes_get_computation(attributes, i);
                  assert(computation != NULL);
                  retval = apx_computationList_append_clone_of_computation(computation_list, computation);
                  if (retval != APX_NO_ERROR)
                  {
                     break;
                  }
               }
               if (retval == APX_NO_ERROR)
               {
                  apx_computationList_set_id(computation_list, current_length);
                  adt_hash_set(map, adt_str_cstr(&signature), computation_list);
                  adt_ary_push(list, computation_list);
               }
            }
            else
            {
               retval = APX_MEM_ERROR;
            }
         }
         else
         {
            computation_list = existing_item;
         }
         if (retval == APX_NO_ERROR)
         {
            apx_portInstance_set_computation_list(port_instance, computation_list);
         }
      }
      adt_str_destroy(&signature);
      return retval;
   }
   return APX_NO_ERROR;
}

static apx_error_t init_node_instance_from_file_info(apx_nodeManager_t* self, rmf_fileInfo_t const* file_info, bool* file_open_request)
{
   assert(self != NULL);
   assert(file_info != NULL);
   assert(file_open_request != NULL);
   apx_error_t result = APX_NO_ERROR;
   size_t name_size;
   char* base_name = rmf_fileInfo_base_name(file_info);
   if (base_name == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   name_size = strlen(base_name);
   if (name_size == 0u)
   {
      result = APX_NAME_MISSING_ERROR;
   }
   else if (name_size > RMF_MAX_FILE_NAME_SIZE)
   {
      result = APX_NAME_TOO_LONG_ERROR;
   }
   else
   {
      apx_nodeInstance_t* node_instance = apx_nodeInstance_new(self->mode, base_name);
      if (result == APX_NO_ERROR)
      {
         apx_size_t definition_size = (apx_size_t)rmf_fileInfo_size(file_info);
         result = apx_nodeInstance_init_node_data(node_instance, NULL, definition_size);
      }
      if (result == APX_NO_ERROR)
      {
         apx_nodeData_t* node_data = apx_nodeInstance_get_node_data(node_instance);
         if (node_data == NULL)
         {
            result = APX_NULL_PTR_ERROR;
         }
         else
         {
            apx_nodeData_set_checksum_data(node_data, rmf_fileInfo_digest_type(file_info), rmf_fileInfo_digest_data(file_info));
         }
      }
      if (result == APX_NO_ERROR)
      {
         //TODO: Check node cache if we have a cached version of the node (level1 or level2 cache)
         *file_open_request = true;
      }
      if (result == APX_NO_ERROR)
      {
         attach_node(self, node_instance);
      }
      else
      {
         apx_nodeInstance_delete(node_instance);
      }
   }
   free(base_name);
   return result;
}

static apx_error_t create_port_signatures_on_node_instance(apx_nodeInstance_t* node_instance)
{
   apx_size_t const num_provide_ports = apx_nodeInstance_get_num_provide_ports(node_instance);
   apx_size_t const num_require_ports = apx_nodeInstance_get_num_require_ports(node_instance);
   apx_size_t port_id;
   apx_error_t retval = APX_NO_ERROR;

   for (port_id = 0u; port_id < num_provide_ports; port_id++)
   {
      apx_portInstance_t* port_instance = apx_nodeInstance_get_provide_port(node_instance, port_id);
      if (port_instance == NULL)
      {
         retval = APX_NULL_PTR_ERROR;
         break;
      }
      retval = apx_port_instance_create_port_signature(port_instance);
      if (retval != APX_NO_ERROR)
      {
         break;
      }
   }
   if (retval == APX_NO_ERROR)
   {
      for (port_id = 0u; port_id < num_require_ports; port_id++)
      {
         apx_portInstance_t* port_instance = apx_nodeInstance_get_require_port(node_instance, port_id);
         if (port_instance == NULL)
         {
            retval = APX_NULL_PTR_ERROR;
            break;
         }
         retval = apx_port_instance_create_port_signature(port_instance);
         if (retval != APX_NO_ERROR)
         {
            break;
         }
      }
   }
   return retval;
}