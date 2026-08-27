/*****************************************************************************
* \file      type_attribute.c
* \author    Conny Gustafsson
* \date      2018-09-11
* \brief     Parse tree: APX type attributes
*
* Copyright (c) 2018-2020 Conny Gustafsson
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


