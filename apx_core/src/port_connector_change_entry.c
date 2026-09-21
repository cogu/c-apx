/*****************************************************************************
* \file      port_connector_change_entry.c
* \author    Conny Gustafsson
* \date      2019-01-23
* \brief     APX port connection change information (for one port)
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
void apx_port_connector_change_entry_create(apx_port_connector_change_entry_t *self)
{
   if (self != NULL)
   {
      self->count = 0;
      self->data.port_instance = NULL;
   }
}

void apx_port_connector_change_entry_destroy(apx_port_connector_change_entry_t *self)
{
   if (self != NULL)
   {
      if ( (self->count < -1) || (self->count > 1) )
      {
         adt_ary_delete((adt_ary_t*) self->data.array);
      }
   }
}

apx_port_connector_change_entry_t *apx_port_connector_change_entry_new(void)
{
   apx_port_connector_change_entry_t *self = (apx_port_connector_change_entry_t*) malloc(sizeof(apx_port_connector_change_entry_t));
   if (self != NULL)
   {
      apx_port_connector_change_entry_create(self);
   }
   return self;
}

void apx_port_connector_change_entry_delete(apx_port_connector_change_entry_t *self)
{
   if (self != NULL)
   {
      apx_port_connector_change_entry_destroy(self);
      free(self);
   }
}

/**
 * Adds port connect information to an entry
 */
apx_error_t apx_port_connector_change_entry_add_connection(apx_port_connector_change_entry_t *self, apx_port_instance_t *port_instance)
{
   if (self != NULL)
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
         apx_port_instance_t *tmp = self->data.port_instance;
         self->data.array = adt_ary_new((void(*)(void*)) 0);
         if (self->data.array == NULL)
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
apx_error_t apx_port_connector_change_entry_remove_connection(apx_port_connector_change_entry_t *self, apx_port_instance_t *port_instance)
{
   if (self != NULL)
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
         apx_port_instance_t *tmp = self->data.port_instance;
         self->data.array = adt_ary_new((void(*)(void*)) 0);
         if (self->data.array == NULL)
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

apx_port_instance_t *apx_port_connector_change_entry_get(apx_port_connector_change_entry_t *self, int32_t index)
{
   apx_port_instance_t *retval = NULL;
   if ( (self != NULL) && (self->count != 0) && (index >= 0) )
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
            retval = (apx_port_instance_t*) adt_ary_value(self->data.array, index);
         }
      }
   }
   return retval;
}

int32_t apx_port_connector_change_entry_count(apx_port_connector_change_entry_t *self)
{
   if (self != NULL)
   {
      return self->count;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


