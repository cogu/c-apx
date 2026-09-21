/*****************************************************************************
* \file      type_attribute.c
* \author    Conny Gustafsson
* \date      2018-09-11
* \brief     Parse tree: APX type attributes
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <malloc.h>
#include "apx/type_attribute.h"
#include "apx/computation.h"
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

void apx_typeAttributes_create(apx_typeAttributes_t* self)
{
   if (self != NULL)
   {
      adt_ary_create(&self->computations, apx_computation_vdelete);
   }
}

void apx_typeAttributes_destroy(apx_typeAttributes_t* self)
{
   if (self != 0)
   {
      adt_ary_destroy(&self->computations);
   }
}

apx_typeAttributes_t* apx_typeAttributes_new()
{
   apx_typeAttributes_t *self = (apx_typeAttributes_t*) malloc(sizeof(apx_typeAttributes_t));
   if (self != 0)
   {
      apx_typeAttributes_create(self);
   }
   return self;
}

void apx_typeAttributes_delete(apx_typeAttributes_t* self)
{
   if (self != 0)
   {
      apx_typeAttributes_destroy(self);
      free(self);
   }
}

void apx_typeAttributes_append_computation(apx_typeAttributes_t* self, struct apx_computation_tag* computation)
{
   if ( (self != NULL) && (computation != NULL) )
   {
      adt_ary_push(&self->computations, (void*)computation);
   }
}

int32_t apx_typeAttributes_num_computations(apx_typeAttributes_t* self)
{
   if (self != NULL)
   {
      return adt_ary_length(&self->computations);
   }
   return -1;
}

apx_computation_t* apx_typeAttributes_get_computation(apx_typeAttributes_t* self, int32_t index)
{
   if (self != NULL)
   {
      return adt_ary_value(&self->computations, index);
   }
   return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


