/*****************************************************************************
* \file      port_connector_list.c
* \author    Conny Gustafsson
* \date      2018-12-07
* \brief     Internal lookup table for port subscriptions
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
void apx_portConnectorList_create(apx_port_connector_list_t *self)
{
   if (self != NULL)
   {
      adt_ary_create(&self->require_ports, 0);
   }
}

void apx_portConnectorList_destroy(apx_port_connector_list_t *self)
{
   if (self != NULL)
   {
      adt_ary_destroy(&self->require_ports);
   }
}

apx_port_connector_list_t* apx_portConnectorList_new(void)
{
   apx_port_connector_list_t *self = (apx_port_connector_list_t*) malloc(sizeof(apx_port_connector_list_t));
   if (self != NULL)
   {
      apx_portConnectorList_create(self);
   }
   return self;
}

void apx_portConnectorList_delete(apx_port_connector_list_t *self)
{
   if (self != NULL)
   {
      apx_portConnectorList_destroy(self);
      free(self);
   }
}

apx_error_t apx_portConnectorList_insert(apx_port_connector_list_t* self, apx_port_instance_t* port_instance)
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

void apx_portConnectorList_remove(apx_port_connector_list_t* self, apx_port_instance_t* port_instance)
{
   if ((self != NULL) && (port_instance != NULL))
   {
      (void) adt_ary_remove(&self->require_ports, port_instance);
   }
}

void apx_portConnectorList_clear(apx_port_connector_list_t *self)
{
   if (self != NULL)
   {
      adt_ary_clear(&self->require_ports);
   }
}

int32_t apx_portConnectorList_length(apx_port_connector_list_t *self)
{
   if (self != NULL)
   {
      return adt_ary_length(&self->require_ports);
   }
   return -1;
}

apx_port_instance_t* apx_portConnectorList_get(apx_port_connector_list_t* self, int32_t index)
{
   if (self != NULL)
   {
      return (apx_port_instance_t*) adt_ary_value(&self->require_ports, index);
   }
   return (apx_port_instance_t*) NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


