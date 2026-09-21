/*****************************************************************************
* \file      data_signature.c
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX parse tree: data signature
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "apx/data_signature.h"
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
void apx_dataSignature_create(apx_dataSignature_t* self)
{
   if (self != NULL)
   {
      self->data_element = NULL;
      self->effective_data_element = NULL;
   }
}

void apx_dataSignature_destroy(apx_dataSignature_t* self)
{
   if (self != NULL)
   {
      if (self->data_element != NULL)
      {
         apx_dataElement_delete(self->data_element);
      }
      if (self->effective_data_element != NULL)
      {
         apx_dataElement_delete(self->effective_data_element);
      }
   }
}

apx_dataElement_t* apx_dataSignature_get_data_element(apx_dataSignature_t* self)
{
   if (self != NULL)
   {
      return self->data_element;
   }
   return NULL;
}

void apx_dataSignature_set_element(apx_dataSignature_t* self, apx_dataElement_t* data_element)
{
   if (self != NULL)
   {
      self->data_element = data_element;
   }
}

apx_dataElement_t* apx_dataSignature_get_effective_data_element(apx_dataSignature_t* self)
{
   if (self != NULL)
   {
      return self->effective_data_element;
   }
   return NULL;
}

void apx_dataSignature_set_effective_element(apx_dataSignature_t* self, apx_dataElement_t* data_element)
{
   if (self != NULL)
   {
      assert(self->effective_data_element == NULL);
      self->effective_data_element = data_element;
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////