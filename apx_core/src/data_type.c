/*****************************************************************************
* \file      datatype.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX datatype class
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
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "apx/data_type.h"
#include "apx/types.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_dataType_t* apx_dataType_new(const char* name, int32_t line_number)
{
   apx_dataType_t* self = (apx_dataType_t*)malloc(sizeof(apx_dataType_t));
   if (self != 0)
   {
      apx_error_t result = apx_dataType_create(self, name, line_number);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = 0;
      }
   }
   return self;
}

void apx_dataType_delete(apx_dataType_t* self)
{
   if(self != 0){
      apx_dataType_destroy(self);
      free(self);
   }
}

void apx_dataType_vdelete(void* arg)
{
   apx_dataType_delete((apx_dataType_t*) arg);
}

apx_error_t apx_dataType_create(apx_dataType_t* self, const char* name, int32_t line_number)
{
   if (self != 0)
   {
      if (name != 0)
      {
         self->name = STRDUP(name);
         if (self->name == 0)
         {
            return APX_MEM_ERROR;
         }
      }
      else
      {
         self->name = 0;
      }
      apx_dataSignature_create(&self->data_signature);
      self->attributes = NULL;
      self->line_number = line_number;
   }
   return APX_NO_ERROR;
}

void apx_dataType_destroy(apx_dataType_t* self)
{
   if ( self !=0 )
   {
      if (self->name != NULL)
      {
         free(self->name);
         self->name = NULL;
      }
      apx_dataSignature_destroy(&self->data_signature);
      if (self->attributes != NULL)
      {
         apx_typeAttributes_delete(self->attributes);
         self->attributes = NULL;
      }
   }
}

apx_dataElement_t* apx_dataType_get_data_element(apx_dataType_t* self)
{
   if (self != NULL)
   {
      return apx_dataSignature_get_data_element(&self->data_signature);
   }
   return NULL;
}

int32_t apx_dataType_get_line_number(apx_dataType_t* self)
{
   if (self != 0)
   {
      return self->line_number;
   }
   return -1;
}

bool apx_dataType_has_attributes(apx_dataType_t* self)
{
   if (self != NULL)
   {
      return self->attributes == NULL ? false : true;
   }
   return false;
}

apx_error_t apx_dataType_init_attributes(apx_dataType_t* self)
{
   if ( (self != NULL) && (self->attributes == NULL) )
   {
      self->attributes = apx_typeAttributes_new();
      if (self->attributes == NULL)
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_NO_ERROR;
}

apx_typeAttributes_t* apx_dataType_get_attributes(apx_dataType_t* self)
{
   if (self != NULL)
   {
      return self->attributes;
   }
   return NULL;
}

const char* apx_dataType_get_name(apx_dataType_t* self)
{
   if (self != NULL)
   {
      return self->name;
   }
   return NULL;
}

void apx_dataType_set_id(apx_dataType_t* self, apx_typeId_t type_id)
{
   if (self != NULL)
   {
      self->type_id = type_id;
   }
}

apx_typeId_t apx_dataType_get_id(apx_dataType_t const* self)
{
   if (self != NULL)
   {
      return self->type_id;
   }
   return APX_INVALID_TYPE_ID;

}

apx_error_t apx_dataType_derive_types_on_element(apx_dataType_t* self, adt_ary_t const* type_list, adt_hash_t const* type_map)
{
   if (self != NULL)
   {
      apx_dataElement_t* data_element = apx_dataSignature_get_data_element(&self->data_signature);
      if (data_element == NULL)
      {
         return APX_NULL_PTR_ERROR;
      }
      return apx_dataElement_derive_types_on_element(data_element, type_list, type_map);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_dataType_derive_data_element(apx_dataType_t* self, struct apx_dataElement_tag** data_element, struct apx_dataElement_tag** parent)
{
   if ( (self != NULL) && (data_element != NULL) )
   {
      apx_error_t retval = APX_NO_ERROR;
      uint16_t reference_follow_count = 0u;
      apx_typeCode_t type_code;
      *data_element = apx_dataType_get_data_element(self);

      do
      {
         type_code = apx_dataElement_get_type_code(*data_element);
         assert( (type_code != APX_TYPE_CODE_REF_ID) && (type_code != APX_TYPE_CODE_REF_NAME));
         if (type_code == APX_TYPE_CODE_REF_PTR)
         {
            apx_dataType_t* data_type = apx_dataElement_get_type_ref_ptr(*data_element);
            if (data_type == NULL)
            {
               retval = APX_NULL_PTR_ERROR;
            }
            else
            {
               if (parent != NULL)
               {
                  *parent = *data_element;
               }
               *data_element = apx_dataType_get_data_element(data_type);
               if (*data_element == NULL)
               {
                  retval = APX_NULL_PTR_ERROR;
               }
               reference_follow_count++;
            }
         }
      } while ((retval == APX_NO_ERROR) &&
         (type_code == APX_TYPE_CODE_REF_PTR) &&
         (reference_follow_count <= MAX_TYPE_REF_FOLLOW_COUNT));

      if (reference_follow_count > MAX_TYPE_REF_FOLLOW_COUNT)
      {
         retval = APX_TOO_MANY_REFERENCES_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


