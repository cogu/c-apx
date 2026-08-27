/*****************************************************************************
* \file      port.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Parse tree APX port
*
* Copyright (c) 2017-2018 Conny Gustafsson
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
#include <assert.h>
#include "apx/port.h"
#include "apx/data_type.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t derive_data_element(apx_port_t* self, apx_dataElement_t** data_element, apx_dataElement_t** parent);




//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_port_create(apx_port_t* self, apx_portType_t port_type, const char* name, int32_t line_number)
{
   if (self != NULL)
   {
      self->port_type = port_type;
      self->port_id = APX_INVALID_PORT_ID;
      self->line_number = line_number;
      self->name = (name != 0) ? STRDUP(name) : 0;
      apx_dataSignature_create(&self->data_signature);
      self->attributes = NULL;
      self->proper_init_value = NULL;
   }
}

void apx_port_destroy(apx_port_t* self)
{
   if (self != NULL)
   {
      apx_dataSignature_destroy(&self->data_signature);
      if (self->name != NULL)
      {
         free(self->name);
      }
      if (self->proper_init_value != NULL)
      {
         dtl_dec_ref(self->proper_init_value);
      }
      if (self->attributes != NULL)
      {
         apx_portAttributes_delete(self->attributes);
      }
   }
}

apx_port_t* apx_port_new(apx_portType_t port_type, const char* name, int32_t line_number)
{
   apx_port_t* self = (apx_port_t*)malloc(sizeof(apx_port_t));
   if (self != NULL)
   {
      apx_port_create(self, port_type, name, line_number);
   }
   return self;
}

void apx_port_delete(apx_port_t* self)
{
   if (self != NULL)
   {
      apx_port_destroy(self);
      free(self);
   }
}

void apx_port_vdelete(void* arg)
{
   apx_port_delete((apx_port_t*)arg);
}

apx_dataElement_t* apx_port_get_data_element(apx_port_t const* self)
{
   if (self != NULL)
   {
      return self->data_signature.data_element;
   }
   return NULL;
}

apx_dataElement_t* apx_port_get_effective_data_element(apx_port_t const* self)
{
   if (self != NULL)
   {
      return self->data_signature.effective_data_element;
   }
   return NULL;
}

apx_portAttributes_t* apx_port_get_attributes(apx_port_t* self)
{
   if (self != NULL)
   {
      return self->attributes;
   }
   return NULL;
}

bool apx_port_has_attributes(apx_port_t* self)
{
   if (self != NULL)
   {
      return self->attributes == NULL? false : true;
   }
   return false;
}

apx_error_t apx_port_init_attributes(apx_port_t* self)
{
   if ((self != NULL) && (self->attributes == NULL))
   {
      self->attributes = apx_portAttributes_new();
      if (self->attributes == NULL)
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_NO_ERROR;
}

apx_typeAttributes_t* apx_port_get_referenced_type_attributes(apx_port_t const* self)
{
   if (self != NULL)
   {
      apx_dataElement_t* data_element = apx_port_get_data_element(self);
      assert(data_element != NULL);
      apx_typeCode_t type_code = apx_dataElement_get_type_code(data_element);
      if (type_code == APX_TYPE_CODE_REF_PTR)
      {
         apx_dataType_t* data_type = apx_dataElement_get_type_ref_ptr(data_element);
         assert(data_type != NULL);
         return apx_dataType_get_attributes(data_type);
      }
   }
   return NULL;
}

apx_error_t apx_port_derive_types(apx_port_t* self, adt_ary_t const* type_list, adt_hash_t const* type_map)
{
   apx_dataElement_t* data_element = apx_port_get_data_element(self);
   if (data_element == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   return apx_dataElement_derive_types_on_element(data_element, type_list, type_map);
}

apx_error_t apx_port_derive_proper_init_value(apx_port_t* self)
{
   apx_error_t result = APX_NO_ERROR;
   dtl_dv_t* derived_init_value = NULL;
   apx_dataElement_t* data_element = apx_port_get_effective_data_element(self);
   if (data_element == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   if (self->attributes != NULL)
   {
      dtl_dv_t* parsed_init_value = apx_portAttributes_get_init_value(self->attributes);
      if (parsed_init_value != NULL)
      {
         result = apx_dataElement_derive_proper_init_value(data_element, parsed_init_value, &derived_init_value);
         if (result != APX_NO_ERROR)
         {
            return result;
         }
      }
   }
   if (derived_init_value != NULL)
   {
      self->proper_init_value = derived_init_value;
   }
   return APX_NO_ERROR;
}

bool apx_port_is_queued(apx_port_t* self)
{
   if ((self != NULL) && (self->attributes != NULL))
   {
      return apx_portAttributes_is_queued(self->attributes);
   }
   return false;
}

bool apx_port_is_parameter(apx_port_t* self)
{
   if ((self != NULL) && (self->attributes != NULL))
   {
      return apx_portAttributes_is_parameter(self->attributes);
   }
   return false;
}

uint32_t apx_port_get_queue_length(apx_port_t* self)
{
   if ((self != NULL) && (self->attributes != NULL))
   {
      return apx_portAttributes_get_queue_length(self->attributes);
   }
   return 0;
}

apx_error_t apx_port_flatten_data_element(apx_port_t* self)
{
   apx_error_t result;
   apx_dataElement_t* parent_element = NULL;
   apx_dataElement_t* cloned_element = NULL;
   apx_dataElement_t* data_element = apx_port_get_data_element(self);
   if (data_element == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   result = derive_data_element(self, &data_element, &parent_element);
   if (result != APX_NO_ERROR)
   {
      return result;
   }

   cloned_element = apx_dataElement_clone(data_element);
   if (cloned_element == NULL)
   {
      return APX_MEM_ERROR;
   }

   if (parent_element != NULL)
   {
      bool const parent_is_array = apx_dataElement_is_array(parent_element);
      if (parent_is_array && apx_dataElement_is_array(cloned_element))
      {
         return APX_PARSE_ERROR; //Illegal in APX to create an array-reference to array-element.
      }
      else if (parent_is_array)
      {
         //Handle array-reference to data element
         apx_dataElement_set_array_length(cloned_element, apx_dataElement_get_array_length(parent_element));
         if (apx_dataElement_is_dynamic_array(parent_element))
         {
            apx_dataElement_set_dynamic_array(cloned_element);
         }
      }
   }
   apx_dataSignature_set_effective_element(&self->data_signature, cloned_element);

   return APX_NO_ERROR;
}

const char* apx_port_get_name(apx_port_t const* self)
{
   if (self != NULL)
   {
      return self->name;
   }
   return NULL;
}

apx_portType_t apx_port_get_port_type(const apx_port_t* self)
{
   if (self != NULL)
   {
      return self->port_type;
   }
   return APX_REQUIRE_PORT;
}

void apx_port_set_id(apx_port_t* self, apx_portId_t port_id)
{
   if (self != NULL)
   {
      self->port_id = port_id;
   }
}

apx_portId_t apx_port_get_id(apx_port_t* self)
{
   if (self != NULL)
   {
      return self->port_id;
   }
   return APX_INVALID_PORT_ID;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t derive_data_element(apx_port_t* self, apx_dataElement_t** data_element, apx_dataElement_t** parent)
{
   apx_error_t retval = APX_NO_ERROR;
   assert((self != NULL) && (data_element != NULL) );
   *data_element = apx_port_get_data_element(self);
   if (*data_element == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   if (apx_dataElement_get_type_code(*data_element) == APX_TYPE_CODE_REF_PTR)
   {
      apx_dataType_t* data_type = apx_dataElement_get_type_ref_ptr(*data_element);
      if (parent != NULL)
      {
         *parent = *data_element;
      }
      if (data_type != NULL)
      {
         retval = apx_dataType_derive_data_element(data_type, data_element, parent);
         if (*data_element == NULL)
         {
            return APX_NULL_PTR_ERROR;
         }
      }
      else
      {
         retval = APX_NULL_PTR_ERROR;
      }
   }
   return retval;
}

