/*****************************************************************************
* \file      apx_portTriggerList.c
* \author    Conny Gustafsson
* \date      2018-12-07
* \brief     Internal lookup table for port subscriptions
*
* Copyright (c) 2018 Conny Gustafsson
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
#include "apx_portTriggerList.h"

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
void apx_portTriggerList_create(apx_portTriggerList_t *self)
{
   if (self != 0)
   {
      adt_ary_create(&self->requirePortData, 0);
   }
}

void apx_portTriggerList_destroy(apx_portTriggerList_t *self)
{
   if (self != 0)
   {
      adt_ary_destroy(&self->requirePortData);
   }
}

apx_portTriggerList_t* apx_portTriggerList_new(void)
{
   apx_portTriggerList_t *self = (apx_portTriggerList_t*) malloc(sizeof(apx_portTriggerList_t));
   if (self != 0)
   {
      apx_portTriggerList_create(self);
   }
   return self;
}

void apx_portTriggerList_delete(apx_portTriggerList_t *self)
{
   if (self != 0)
   {
      apx_portTriggerList_destroy(self);
      free(self);
   }
}

apx_error_t apx_portTriggerList_insert(apx_portTriggerList_t *self, apx_portRef_t *portData)
{
   if ( (self != 0) && (portData != 0) )
   {
      apx_error_t retval = APX_NO_ERROR;
      adt_error_t result = adt_ary_push(&self->requirePortData, portData);
      if (result != ADT_NO_ERROR)
      {
         if (result == ADT_MEM_ERROR)
         {
            retval = APX_MEM_ERROR;
         }
         else
         {
            retval = -1; //unhandled error
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portTriggerList_remove(apx_portTriggerList_t *self, apx_portRef_t *portData)
{
   if ( (self != 0) && (portData != 0) )
   {
      (void) adt_ary_remove(&self->requirePortData, portData);
   }
}

int32_t apx_portTriggerList_length(apx_portTriggerList_t *self)
{
   if (self != 0)
   {
      return adt_ary_length(&self->requirePortData);
   }
   return -1;
}

apx_portRef_t *apx_portTriggerList_get(apx_portTriggerList_t *self, int32_t index)
{
   if (self != 0)
   {
      return (apx_portRef_t*) adt_ary_value(&self->requirePortData, index);
   }
   return (apx_portRef_t*) 0;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


