/*****************************************************************************
* \file      port_instance.c
* \author    Conny Gustafsson
* \date      2020-12-14
* \brief     Static information about an instantiated port (things that do not change during run-time)
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include "apx/port_instance.h"
#include "apx/node_instance.h"
#include "apx/util.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static apx_error_t process_info_from_program_header(apx_port_instance_t* self, apx_program_t const* program);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_port_instance_create(apx_port_instance_t* self, struct apx_node_instance_tag* parent, apx_port_type_t port_type, apx_port_id_t port_id,
   char const* name, apx_program_t const* pack_program, apx_program_t const* unpack_program)
{
   if (self != NULL)
   {
      self->parent = parent;
      self->port_type = port_type;
      self->port_id = port_id;
      self->pack_program = pack_program;
      self->unpack_program = unpack_program;
      self->effective_data_element = NULL;
      self->data_offset = 0u;
      self->data_size = 0u;
      self->queue_length = 0u;
      self->element_size = 0u;
      self->has_dynamic_data = false;
      self->computation_list = NULL;
      self->port_signature = NULL;
      if (name != NULL)
      {
         self->name = STRDUP(name);
         if (self->name == NULL)
         {
            return APX_MEM_ERROR;
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_port_instance_destroy(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      if (self->name != NULL)
      {
         free(self->name);
      }
      if (self->pack_program != NULL)
      {
         APX_PROGRAM_DELETE((apx_program_t*)self->pack_program);
      }
      if (self->unpack_program != NULL)
      {
         APX_PROGRAM_DELETE((apx_program_t*)self->unpack_program);
      }
      if (self->port_signature != NULL)
      {
         free(self->port_signature);
      }
   }
}

apx_port_instance_t* apx_port_instance_new(struct apx_node_instance_tag* parent, apx_port_type_t port_type, apx_port_id_t port_id,
   char const* name, apx_program_t const* pack_program, apx_program_t const* unpack_program)
{
   apx_port_instance_t* self = (apx_port_instance_t*)malloc(sizeof(apx_port_instance_t));
   if (self != NULL)
   {
      apx_error_t rc = apx_port_instance_create(self, parent, port_type, port_id, name, pack_program, unpack_program);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_port_instance_delete(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      apx_port_instance_destroy(self);
      free(self);
   }
}

void apx_port_instance_vdelete(void* arg)
{
   apx_port_instance_delete((apx_port_instance_t*)arg);
}

struct apx_node_instance_tag* apx_port_instance_parent(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return self->parent;
   }
   return NULL;
}

apx_port_type_t apx_port_instance_port_type(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return self->port_type;
   }
   return APX_REQUIRE_PORT;
}

apx_port_id_t apx_port_instance_port_id(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return self->port_id;
   }
   return APX_INVALID_PORT_ID;
}

char const* apx_port_instance_name(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return self->name;
   }
   return NULL;
}

uint32_t apx_port_instance_data_offset(apx_port_instance_t const* self)
{
   if (self != NULL)
   {
      return self->data_offset;
   }
   return 0u;
}

uint32_t apx_port_instance_data_size(apx_port_instance_t const* self)
{
   if (self != NULL)
   {
      return self->data_size;
   }
   return 0u;
}

uint32_t apx_port_instance_queue_length(apx_port_instance_t const* self)
{
   if (self != NULL)
   {
      return self->queue_length;
   }
   return 0u;
}

uint32_t apx_port_instance_element_size(apx_port_instance_t const* self)
{
   if (self != NULL)
   {
      return self->element_size;
   }
   return 0u;
}

bool apx_port_instance_has_dynamic_data(apx_port_instance_t const* self)
{
   if (self != NULL)
   {
      return self->has_dynamic_data;
   }
   return false;
}

apx_program_t const* apx_port_instance_pack_program(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return self->pack_program;
   }
   return NULL;
}

apx_program_t const* apx_port_instance_unpack_program(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return self->unpack_program;
   }
   return NULL;
}

void apx_port_instance_set_effective_element(apx_port_instance_t* self, apx_data_element_t* data_element)
{
   if (self != NULL)
   {
      self->effective_data_element = data_element;
   }
}

apx_data_element_t* apx_port_instance_get_effective_element(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return self->effective_data_element;
   }
   return NULL;
}

apx_element_id_t apx_port_instance_element_id(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return apx_data_element_get_id(self->effective_data_element);
   }
   return APX_INVALID_ELEMENT_ID;
}

apx_error_t apx_port_instance_derive_properties(apx_port_instance_t* self, uint32_t offset, uint32_t* size)
{
   if ( (self != NULL) && (size != NULL) )
   {
      apx_error_t result = APX_NO_ERROR;
      result = process_info_from_program_header(self, (self->port_type == APX_PROVIDE_PORT) ? self->pack_program : self->unpack_program);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      self->data_offset = offset;
      *size = self->data_size;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_port_instance_set_computation_list(apx_port_instance_t* self, apx_computation_list_t const* computation_list)
{
   if (self != NULL)
   {
      self->computation_list = computation_list;
   }
}

apx_computation_list_t const* apx_port_instance_get_computation_list(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      return self->computation_list;
   }
   return NULL;
}

apx_computation_t const* apx_port_instance_get_computation(apx_port_instance_t* self, int32_t index)
{
   if ((self != NULL) && (self->computation_list != NULL))
   {
      return adt_ary_value(&self->computation_list->computations, index);
   }
   return NULL;
}

int32_t apx_port_instance_get_computation_list_length(apx_port_instance_t* self)
{
   if ((self != NULL) && (self->computation_list != NULL))
   {
      return adt_ary_length(&self->computation_list->computations);
   }
   return 0;
}

apx_computation_list_id_t apx_port_instance_get_computation_list_id(apx_port_instance_t* self)
{
   if ((self != NULL) && (self->computation_list != NULL))
   {
      return self->computation_list->computation_list_id;
   }
   return APX_INVALID_COMPUTATION_LIST_ID;
}

apx_error_t apx_port_instance_create_port_signature(apx_port_instance_t* self)
{
   if (self != NULL)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->port_signature == NULL)
      {
         adt_str_t* str = adt_str_new();
         if (str != NULL)
         {
            retval = convert_from_adt_to_apx_error(adt_str_push(str, '"'));
            if (retval == APX_NO_ERROR)
            {
               retval = convert_from_adt_to_apx_error(adt_str_append_cstr(str, self->name));
               if (retval == APX_NO_ERROR)
               {
                  retval = convert_from_adt_to_apx_error(adt_str_push(str, '"'));
                  if (retval == APX_NO_ERROR)
                  {
                     apx_data_element_t* data_element = apx_port_instance_get_effective_element(self);
                     if (data_element != NULL)
                     {
                        adt_str_t* data_signature = apx_data_element_to_string(data_element, true);
                        if (data_signature != NULL)
                        {
                           retval = convert_from_adt_to_apx_error(adt_str_append(str, data_signature));
                           adt_str_delete(data_signature);
                        }
                        else
                        {
                           retval = APX_NULL_PTR_ERROR;
                        }
                     }
                     else
                     {
                        retval = APX_NULL_PTR_ERROR;
                     }
                  }
               }
            }
            if (retval == APX_NO_ERROR)
            {
               size_t size = (size_t) adt_str_size(str);
               if (size > 0)
               {
                  self->port_signature = (char*)malloc(size + 1); //Add 1 for null-terminator
                  if (self->port_signature == NULL)
                  {
                     retval = APX_MEM_ERROR;
                  }
                  else
                  {
                     memcpy(self->port_signature, adt_str_data(str), size);
                     self->port_signature[size] = 0;
                  }
               }
            }
            adt_str_delete(str);
         }
         else
         {
            retval = APX_MEM_ERROR;
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;

}

char const* apx_port_instance_get_port_signature(apx_port_instance_t const* self, bool *has_dynamic_data)
{
   if ( (self != NULL) && (has_dynamic_data != NULL) )
   {
      *has_dynamic_data = self->has_dynamic_data;
      return self->port_signature;
   }
   return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t process_info_from_program_header(apx_port_instance_t* self, apx_program_t const* program)
{
   if (self != NULL)
   {
      apx_program_header_t header;
      apx_error_t result;
      uint8_t const* begin;
      uint8_t const* end;
      uint8_t const* next = NULL;
      if (program == NULL)
      {
         return APX_NULL_PTR_ERROR;
      }
      begin = adt_bytearray_data(program);
      end = begin + (size_t)adt_bytearray_length(program);
      result = apx_program_decode_header(begin, end, &next, &header);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      self->data_size = header.data_size;
      self->has_dynamic_data = header.has_dynamic_data;
      self->queue_length = header.queue_length;
      self->element_size = header.element_size;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_port_count_t apx_port_instance_connection_count(apx_port_instance_t const* self)
{
   if ((self != NULL) && (self->parent != NULL))
   {
      return apx_node_instance_get_port_connection_count(self->parent, self->port_type, self->port_id);
   }
   return 0u;
}

apx_port_count_t apx_port_instance_get_connection_count(apx_port_instance_t const* self)
{
   return apx_port_instance_connection_count(self);
}