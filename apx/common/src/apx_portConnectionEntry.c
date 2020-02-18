/*****************************************************************************
* \file      apx_portConnectionEntry.c
* \author    Conny Gustafsson
* \date      2019-01-23
* \brief     APX port connection information (for one port)
*
* Copyright (c) 2019 Conny Gustafsson
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
#include "apx_portConnectionEntry.h"
#include <stdlib.h>
#include <assert.h>
#include "adt_ary.h"
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
void apx_portConnectionEntry_create(apx_portConnectionEntry_t *self)
{
   if (self != 0)
   {
      self->count = 0;
      self->pAny = NULL;
   }
}

void apx_portConnectionEntry_destroy(apx_portConnectionEntry_t *self)
{
   if ( (self != 0) && (self->pAny != 0) )
   {
      if ( (self->count > 1) || (self->count < -1) )
      {
         adt_ary_delete((adt_ary_t*) self->pAny);
      }
   }
}

apx_portConnectionEntry_t *apx_portConnectionEntry_new(void)
{
   apx_portConnectionEntry_t *self = (apx_portConnectionEntry_t*) malloc(sizeof(apx_portConnectionEntry_t));
   if (self != 0)
   {
      apx_portConnectionEntry_create(self);
   }
   return self;
}

void apx_portConnectionEntry_delete(apx_portConnectionEntry_t *self)
{
   if (self != 0)
   {
      apx_portConnectionEntry_destroy(self);
      free(self);
   }
}

/**
 * Adds port connect information to an entry
 */
apx_error_t apx_portConnectionEntry_addConnection(apx_portConnectionEntry_t *self, apx_portRef_t *portDataRef)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->count < 0)
      {
         retval = APX_UNSUPPORTED_ERROR; //cannot add port connect event to an entry already containing port disconnect events
      }
      else if (self->count == 0)
      {
         self->pAny = (void*) portDataRef;
      }
      else if (self->count == 1)
      {
         //convert single entry into an array of entries
         void *tmp = self->pAny;
         self->pAny = adt_ary_new((void(*)(void*)) 0);
         if (self->pAny == 0)
         {
            retval = APX_MEM_ERROR;
         }
         else
         {
            retval = (apx_error_t) adt_ary_push( (adt_ary_t*) self->pAny, tmp);
            if (retval == APX_NO_ERROR)
            {
               retval = (apx_error_t) adt_ary_push( (adt_ary_t*) self->pAny, portDataRef);
            }
         }
      }
      else
      {
         retval = (apx_error_t) adt_ary_push( (adt_ary_t*) self->pAny, portDataRef);
      }

      if (retval == APX_NO_ERROR)
      {
         self->count++;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Adds port connect information to an entry
 */
apx_error_t apx_portConnectionEntry_removeConnection(apx_portConnectionEntry_t *self, apx_portRef_t *portDataRef)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      if (self->count > 0)
      {
         retval = APX_UNSUPPORTED_ERROR; //cannot add port disconnect event to an entry already containing port connect events
      }
      else if (self->count == 0)
      {
         self->pAny = (void*) portDataRef;
      }
      else if (self->count == -1)
      {
         //convert single entry into an array of entries
         void *tmp = self->pAny;
         self->pAny = adt_ary_new((void(*)(void*)) 0);
         if (self->pAny == 0)
         {
            retval = APX_MEM_ERROR;
         }
         else
         {
            retval = (apx_error_t) adt_ary_push( (adt_ary_t*) self->pAny, tmp);
            if (retval == APX_NO_ERROR)
            {
               retval = (apx_error_t) adt_ary_push( (adt_ary_t*) self->pAny, portDataRef);
            }
         }
      }
      else
      {
         retval = (apx_error_t) adt_ary_push( (adt_ary_t*) self->pAny, portDataRef);
      }

      if (retval == APX_NO_ERROR)
      {
         self->count--;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_portRef_t *apx_portConnectionEntry_get(apx_portConnectionEntry_t *self, int32_t index)
{
   apx_portRef_t *retval = (apx_portRef_t*) 0;
   if ( (self != 0) && (self->count != 0) && (index >= 0) )
   {
      if ( (self->count == 1) || (self->count == -1) )
      {
         if (index == 0)
         {
            retval = (apx_portRef_t*) self->pAny;
         }
      }
      else
      {
         if ( ( (self->count < 0) && (index < -self->count) ) || ( (self->count > 0) && (index < self->count) ) )
         {
            adt_ary_t *ary = (adt_ary_t*) self->pAny;
            retval = (apx_portRef_t*) adt_ary_value(ary, index);
         }
      }
   }
   return retval;
}

int32_t apx_portConnectionEntry_count(apx_portConnectionEntry_t *self)
{
   if (self != 0)
   {
      return self->count;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


