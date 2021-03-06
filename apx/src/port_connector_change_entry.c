/*****************************************************************************
* \file      port_connector_change_entry.h
* \author    Conny Gustafsson
* \date      2019-01-23
* \brief     APX port connection change information (for one port)
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
#include <stdlib.h>
#include <assert.h>
#include "apx/port_connector_change_entry.h"
#include "apx/util.h"

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
/**
 * Constructor
 */
void apx_portConnectorChangeEntry_create(apx_portConnectorChangeEntry_t *self)
{
   if (self != 0)
   {
      self->count = 0;
      self->data.port_instance = NULL;
   }
}

void apx_portConnectorChangeEntry_destroy(apx_portConnectorChangeEntry_t *self)
{
   if (self != 0)
   {
      if ( (self->count < -1) || (self->count > 1) )
      {
         adt_ary_delete((adt_ary_t*) self->data.array);
      }
   }
}

apx_portConnectorChangeEntry_t *apx_portConnectorChangeEntry_new(void)
{
   apx_portConnectorChangeEntry_t *self = (apx_portConnectorChangeEntry_t*) malloc(sizeof(apx_portConnectorChangeEntry_t));
   if (self != 0)
   {
      apx_portConnectorChangeEntry_create(self);
   }
   return self;
}

void apx_portConnectorChangeEntry_delete(apx_portConnectorChangeEntry_t *self)
{
   if (self != 0)
   {
      apx_portConnectorChangeEntry_destroy(self);
      free(self);
   }
}

/**
 * Adds port connect information to an entry
 */
apx_error_t apx_portConnectorChangeEntry_add_connection(apx_portConnectorChangeEntry_t *self, apx_portInstance_t *port_instance)
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
         self->data.port_instance = port_instance;
      }
      else if (self->count == 1)
      {
         //convert single entry into an array of entries
         apx_portInstance_t *tmp = self->data.port_instance;
         self->data.array = adt_ary_new((void(*)(void*)) 0);
         if (self->data.array == 0)
         {
            retval = APX_MEM_ERROR;
         }
         else
         {
            retval = convert_from_adt_to_apx_error(adt_ary_push( (adt_ary_t*) self->data.array, (void*) tmp));
            if (retval == APX_NO_ERROR)
            {
               retval = convert_from_adt_to_apx_error(adt_ary_push( (adt_ary_t*) self->data.array, port_instance));
            }
         }
      }
      else
      {
         retval = convert_from_adt_to_apx_error(adt_ary_push( (adt_ary_t*) self->data.array, port_instance));
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
 * Adds port disconnect information to an entry
 */
apx_error_t apx_portConnectorChangeEntry_remove_connection(apx_portConnectorChangeEntry_t *self, apx_portInstance_t *port_instance)
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
         self->data.port_instance = port_instance;
      }
      else if (self->count == -1)
      {
         //convert single entry into an array of entries
         apx_portInstance_t *tmp = self->data.port_instance;
         self->data.array = adt_ary_new((void(*)(void*)) 0);
         if (self->data.array == 0)
         {
            retval = APX_MEM_ERROR;
         }
         else
         {
            retval = convert_from_adt_to_apx_error(adt_ary_push( (adt_ary_t*) self->data.array, (void*) tmp));
            if (retval == APX_NO_ERROR)
            {
               retval = convert_from_adt_to_apx_error(adt_ary_push( (adt_ary_t*) self->data.array, port_instance));
            }
         }
      }
      else
      {
         retval = convert_from_adt_to_apx_error(adt_ary_push( (adt_ary_t*) self->data.array, port_instance));
      }

      if (retval == APX_NO_ERROR)
      {
         self->count--;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_portInstance_t *apx_portConnectorChangeEntry_get(apx_portConnectorChangeEntry_t *self, int32_t index)
{
   apx_portInstance_t *retval = (apx_portInstance_t*) 0;
   if ( (self != 0) && (self->count != 0) && (index >= 0) )
   {
      if ( (self->count == 1) || (self->count == -1) )
      {
         if (index == 0)
         {
            retval = self->data.port_instance;
         }
      }
      else
      {
         if ( ( (self->count < 0) && (index < -self->count) ) || ( (self->count > 0) && (index < self->count) ) )
         {
            retval = (apx_portInstance_t*) adt_ary_value(self->data.array, index);
         }
      }
   }
   return retval;
}

int32_t apx_portConnectorChangeEntry_count(apx_portConnectorChangeEntry_t *self)
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


