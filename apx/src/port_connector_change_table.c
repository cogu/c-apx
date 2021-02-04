/*****************************************************************************
* \file      port_connector_change_table.c
* \author    Conny Gustafsson
* \date      2019-01-31
* \brief     A list of apx_portConnectionChangeEntry_t.
*            Used to track changes in port connectors on one side of a node (Require or Provide)
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
#include "apx/port_connector_change_table.h"
#include <malloc.h>
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
apx_error_t apx_portConnectorChangeTable_create(apx_portConnectorChangeTable_t *self, apx_size_t num_ports)
{
   if ( (self != NULL) && (num_ports > 0) )
   {
      apx_size_t i;
      self->num_ports = num_ports;
      self->entries = (apx_portConnectorChangeEntry_t*) malloc(sizeof(apx_portConnectorChangeEntry_t)*num_ports);
      if (self->entries == 0)
      {
         return APX_MEM_ERROR;
      }
      for (i=0; i<self->num_ports; i++)
      {
         apx_portConnectorChangeEntry_create(&self->entries[i]);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portConnectorChangeTable_destroy(apx_portConnectorChangeTable_t *self)
{
   if ( (self != NULL) && (self->entries != 0))
   {
      apx_size_t i;
      for (i=0u; i<self->num_ports; i++)
      {
         apx_portConnectorChangeEntry_destroy(&self->entries[i]);
      }
      free(self->entries);
   }
}

apx_portConnectorChangeTable_t *apx_portConnectorChangeTable_new(int32_t num_ports)
{
   apx_portConnectorChangeTable_t *self = (apx_portConnectorChangeTable_t*) malloc(sizeof(apx_portConnectorChangeTable_t));
   if (self != NULL)
   {
      apx_error_t errorCode = apx_portConnectorChangeTable_create(self, num_ports);
      if (errorCode != APX_NO_ERROR)
      {
         free(self);
         self = 0;
      }
   }
   return self;
}

void apx_portConnectorChangeTable_delete(apx_portConnectorChangeTable_t *self)
{
   if (self != NULL)
   {
      apx_portConnectorChangeTable_destroy(self);
      free(self);
   }
}

apx_error_t apx_portConnectorChangeTable_connect(apx_portConnectorChangeTable_t* self, apx_portInstance_t* local_port, apx_portInstance_t* remote_port)
{
   if ( (self != NULL) && (local_port != NULL) && (remote_port != NULL) )
   {
      apx_portId_t port_id = apx_portInstance_port_id(local_port);
      if (port_id < self->num_ports )
      {
         return apx_portConnectorChangeEntry_add_connection(&self->entries[port_id], remote_port);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portConnectorChangeTable_disconnect(apx_portConnectorChangeTable_t* self, apx_portInstance_t* local_port, apx_portInstance_t* remote_port)
{
   if ((self != NULL) && (local_port != NULL) && (remote_port != NULL))
   {
      apx_portId_t port_id = apx_portInstance_port_id(local_port);
      if ( port_id < self->num_ports )
      {
         return apx_portConnectorChangeEntry_remove_connection(&self->entries[port_id], remote_port);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_portConnectorChangeEntry_t* apx_portConnectorChangeTable_get_entry(apx_portConnectorChangeTable_t* self, apx_portId_t port_id)
{
   if ( (self != NULL) && (port_id < self->num_ports) )
   {
      return &self->entries[port_id];
   }
   return (apx_portConnectorChangeEntry_t*) NULL;
}

apx_portInstance_t* apx_portConnectorChangeTable_get_port(apx_portConnectorChangeTable_t* self, apx_portId_t port_id, int32_t index)
{
   if ( (self != NULL) && (port_id < self->num_ports) )
   {
      return apx_portConnectorChangeEntry_get(&self->entries[port_id], index);
   }
   return (apx_portInstance_t*) NULL;
}

int32_t apx_portConnectorChangeTable_count(apx_portConnectorChangeTable_t *self, apx_portId_t port_id)
{
   if ( (self != NULL) && (port_id >= 0) && (port_id < self->num_ports) )
   {
      return apx_portConnectorChangeEntry_count(&self->entries[port_id]);
   }
   return -1;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


