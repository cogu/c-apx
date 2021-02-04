/*****************************************************************************
* \file      port_instance.c
* \author    Conny Gustafsson
* \date      2020-12-14
* \brief     Static information about an instantiated port (things that do not change during run-time)
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
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include "apx/port_instance.h"
#include "apx/util.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static apx_error_t process_info_from_program_header(apx_portInstance_t* self, apx_program_t const* program);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portInstance_create(apx_portInstance_t* self, struct apx_nodeInstance_tag* parent, apx_portType_t port_type, apx_portId_t port_id,
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

void apx_portInstance_destroy(apx_portInstance_t* self)
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

apx_portInstance_t* apx_portInstance_new(struct apx_nodeInstance_tag* parent, apx_portType_t port_type, apx_portId_t port_id,
   char const* name, apx_program_t const* pack_program, apx_program_t const* unpack_program)
{
   apx_portInstance_t* self = (apx_portInstance_t*)malloc(sizeof(apx_portInstance_t));
   if (self != NULL)
   {
      apx_error_t rc = apx_portInstance_create(self, parent, port_type, port_id, name, pack_program, unpack_program);
      if (rc != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_portInstance_delete(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      apx_portInstance_destroy(self);
      free(self);
   }
}

void apx_portInstance_vdelete(void* arg)
{
   apx_portInstance_delete((apx_portInstance_t*)arg);
}

struct apx_nodeInstance_tag* apx_portInstance_parent(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return self->parent;
   }
   return NULL;
}

apx_portType_t apx_portInstance_port_type(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return self->port_type;
   }
   return APX_REQUIRE_PORT;
}

apx_portId_t apx_portInstance_port_id(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return self->port_id;
   }
   return APX_INVALID_PORT_ID;
}

char const* apx_portInstance_name(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return self->name;
   }
   return NULL;
}

uint32_t apx_portInstance_data_offset(apx_portInstance_t const* self)
{
   if (self != NULL)
   {
      return self->data_offset;
   }
   return 0u;
}

uint32_t apx_portInstance_data_size(apx_portInstance_t const* self)
{
   if (self != NULL)
   {
      return self->data_size;
   }
   return 0u;
}

uint32_t apx_portInstance_queue_length(apx_portInstance_t const* self)
{
   if (self != NULL)
   {
      return self->queue_length;
   }
   return 0u;
}

uint32_t apx_portInstance_element_size(apx_portInstance_t const* self)
{
   if (self != NULL)
   {
      return self->element_size;
   }
   return 0u;
}

bool apx_portInstance_has_dynamic_data(apx_portInstance_t const* self)
{
   if (self != NULL)
   {
      return self->has_dynamic_data;
   }
   return false;
}

apx_program_t const* apx_portInstance_pack_program(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return self->pack_program;
   }
   return NULL;
}

apx_program_t const* apx_portInstance_unpack_program(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return self->pack_program;
   }
   return NULL;
}

void apx_portInstance_set_effective_element(apx_portInstance_t* self, apx_dataElement_t* data_element)
{
   if (self != NULL)
   {
      self->effective_data_element = data_element;
   }
}

apx_dataElement_t* apx_portInstance_get_effective_element(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return self->effective_data_element;
   }
   return NULL;
}

apx_elementId_t apx_portInstance_element_id(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return apx_dataElement_get_id(self->effective_data_element);
   }
   return APX_INVALID_ELEMENT_ID;
}

apx_error_t apx_portInstance_derive_properties(apx_portInstance_t* self, uint32_t offset, uint32_t* size)
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

void apx_portInstance_set_computation_list(apx_portInstance_t* self, apx_computationList_t const* computation_list)
{
   if (self != NULL)
   {
      self->computation_list = computation_list;
   }
}

apx_computationList_t const* apx_portInstance_get_computation_list(apx_portInstance_t* self)
{
   if (self != NULL)
   {
      return self->computation_list;
   }
   return NULL;
}

apx_computation_t const* apx_portInstance_get_computation(apx_portInstance_t* self, int32_t index)
{
   if ((self != NULL) && (self->computation_list != NULL))
   {
      return adt_ary_value(&self->computation_list->computations, index);
   }
   return NULL;
}

int32_t apx_portInstance_get_computation_list_length(apx_portInstance_t* self)
{
   if ((self != NULL) && (self->computation_list != NULL))
   {
      return adt_ary_length(&self->computation_list->computations);
   }
   return 0;
}

apx_computationListId_t apx_portInstance_get_computation_list_id(apx_portInstance_t* self)
{
   if ((self != NULL) && (self->computation_list != NULL))
   {
      return self->computation_list->computation_list_id;
   }
   return APX_INVALID_COMPUTATION_LIST_ID;
}

apx_error_t apx_port_instance_create_port_signature(apx_portInstance_t* self)
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
                     apx_dataElement_t* data_element = apx_portInstance_get_effective_element(self);
                     if (data_element != NULL)
                     {
                        adt_str_t* data_signature = apx_dataElement_to_string(data_element, true);
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

char const* apx_portInstance_get_port_signature(apx_portInstance_t const* self, bool *has_dynamic_data)
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
static apx_error_t process_info_from_program_header(apx_portInstance_t* self, apx_program_t const* program)
{
   if (self != NULL)
   {
      apx_programHeader_t header;
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