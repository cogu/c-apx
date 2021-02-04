/*****************************************************************************
* \file      port_connector_list.c
* \author    Conny Gustafsson
* \date      2018-12-07
* \brief     Internal lookup table for port subscriptions
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
#include <malloc.h>
#include "apx/port_connector_list.h"

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
void apx_portConnectorList_create(apx_portConnectorList_t *self)
{
   if (self != NULL)
   {
      adt_ary_create(&self->require_ports, 0);
   }
}

void apx_portConnectorList_destroy(apx_portConnectorList_t *self)
{
   if (self != NULL)
   {
      adt_ary_destroy(&self->require_ports);
   }
}

apx_portConnectorList_t* apx_portConnectorList_new(void)
{
   apx_portConnectorList_t *self = (apx_portConnectorList_t*) malloc(sizeof(apx_portConnectorList_t));
   if (self != NULL)
   {
      apx_portConnectorList_create(self);
   }
   return self;
}

void apx_portConnectorList_delete(apx_portConnectorList_t *self)
{
   if (self != NULL)
   {
      apx_portConnectorList_destroy(self);
      free(self);
   }
}

apx_error_t apx_portConnectorList_insert(apx_portConnectorList_t* self, apx_portInstance_t* port_instance)
{
   if ( (self != NULL) && (port_instance != NULL) )
   {
      apx_error_t retval = APX_NO_ERROR;
      adt_error_t result = adt_ary_push(&self->require_ports, (void*)port_instance);
      if (result != ADT_NO_ERROR)
      {
         if (result == ADT_MEM_ERROR)
         {
            retval = APX_MEM_ERROR;
         }
         else
         {
            retval = APX_GENERIC_ERROR; //unhandled error
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portConnectorList_remove(apx_portConnectorList_t* self, apx_portInstance_t* port_instance)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      (void) adt_ary_remove(&self->require_ports, port_instance);
   }
}

void apx_portConnectorList_clear(apx_portConnectorList_t *self)
{
   if (self != NULL)
   {
      adt_ary_clear(&self->require_ports);
   }
}

int32_t apx_portConnectorList_length(apx_portConnectorList_t *self)
{
   if (self != NULL)
   {
      return adt_ary_length(&self->require_ports);
   }
   return -1;
}

apx_portInstance_t* apx_portConnectorList_get(apx_portConnectorList_t* self, int32_t index)
{
   if (self != NULL)
   {
      return (apx_portInstance_t*) adt_ary_value(&self->require_ports, index);
   }
   return (apx_portInstance_t*) NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


