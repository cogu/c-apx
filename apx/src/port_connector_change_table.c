/*****************************************************************************
* \file      port_connector_change_table.c
* \author    Conny Gustafsson
* \date      2019-01-31
* \brief     A list of apx_portConnectionChangeEntry_t.
*            Used to track changes in port connectors on one side of a node (Require or Provide)
*
* Copyright (c) 2019-2020 Conny Gustafsson
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
apx_error_t apx_portConnectorChangeTable_create(apx_portConnectorChangeTable_t *self, int32_t numPorts)
{
   if ( (self != 0) && (numPorts > 0) )
   {
      int32_t i;
      self->numPorts = numPorts;
      self->entries = (apx_portConnectorChangeEntry_t*) malloc(sizeof(apx_portConnectorChangeEntry_t)*numPorts);
      if (self->entries == 0)
      {
         return APX_MEM_ERROR;
      }
      for (i=0; i<self->numPorts; i++)
      {
         apx_portConnectorChangeEntry_create(&self->entries[i]);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portConnectorChangeTable_destroy(apx_portConnectorChangeTable_t *self)
{
   if ( (self != 0) && (self->entries != 0))
   {
      int32_t i;
      for (i=0; i<self->numPorts; i++)
      {
         apx_portConnectorChangeEntry_destroy(&self->entries[i]);
      }
      free(self->entries);
   }
}

apx_portConnectorChangeTable_t *apx_portConnectorChangeTable_new(int32_t numPorts)
{
   apx_portConnectorChangeTable_t *self = (apx_portConnectorChangeTable_t*) malloc(sizeof(apx_portConnectorChangeTable_t));
   if (self != 0)
   {
      apx_error_t errorCode = apx_portConnectorChangeTable_create(self, numPorts);
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
   if (self != 0)
   {
      apx_portConnectorChangeTable_destroy(self);
      free(self);
   }
}

apx_error_t apx_portConnectorChangeTable_connect(apx_portConnectorChangeTable_t *self, apx_portRef_t *localRef, apx_portRef_t *remoteRef)
{
   if ( (self != 0) && (localRef != 0) && (remoteRef != 0) )
   {
      apx_portId_t portId = apx_portRef_getPortId(localRef);
      if ( (portId >= 0) && (portId < self->numPorts) )
      {
         return apx_portConnectorChangeEntry_addConnection(&self->entries[portId], remoteRef);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portConnectorChangeTable_disconnect(apx_portConnectorChangeTable_t *self, apx_portRef_t *localRef, apx_portRef_t *remoteRef)
{
   if ( (self != 0) && (localRef != 0) && (remoteRef != 0) )
   {
      apx_portId_t portId = apx_portRef_getPortId(localRef);
      if ( (portId >= 0) && (portId < self->numPorts) )
      {
         return apx_portConnectorChangeEntry_removeConnection(&self->entries[portId], remoteRef);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_portConnectorChangeEntry_t *apx_portConnectorChangeTable_getEntry(apx_portConnectorChangeTable_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numPorts) )
   {
      return &self->entries[portId];
   }
   return (apx_portConnectorChangeEntry_t*) 0;
}

apx_portRef_t *apx_portConnectorChangeTable_getRef(apx_portConnectorChangeTable_t *self, apx_portId_t portId, int32_t index)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numPorts) )
   {
      return apx_portConnectorChangeEntry_get(&self->entries[portId], index);
   }
   return (apx_portRef_t*) 0;
}

int32_t apx_portConnectorChangeTable_count(apx_portConnectorChangeTable_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numPorts) )
   {
      return apx_portConnectorChangeEntry_count(&self->entries[portId]);
   }
   return 0;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


