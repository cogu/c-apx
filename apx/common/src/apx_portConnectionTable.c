/*****************************************************************************
* \file      apx_portConnectionTable.c
* \author    Conny Gustafsson
* \date      2019-01-31
* \brief     Description
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
#include "apx_portConnectionTable.h"
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
apx_error_t apx_portConnectionTable_create(apx_portConnectionTable_t *self, int32_t numPorts)
{
   if ( (self != 0) && (numPorts > 0) )
   {
      int32_t i;
      self->numPorts = numPorts;
      self->connections = (apx_portConnectionEntry_t*) malloc(sizeof(apx_portConnectionEntry_t)*numPorts);
      if (self->connections == 0)
      {
         return APX_MEM_ERROR;
      }
      for (i=0; i<self->numPorts; i++)
      {
         apx_portConnectionEntry_create(&self->connections[i]);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portConnectionTable_destroy(apx_portConnectionTable_t *self)
{
   if ( (self != 0) && (self->connections != 0))
   {
      int32_t i;
      for (i=0; i<self->numPorts; i++)
      {
         apx_portConnectionEntry_destroy(&self->connections[i]);
      }
      free(self->connections);
   }
}

apx_portConnectionTable_t *apx_portConnectionTable_new(int32_t numPorts)
{
   apx_portConnectionTable_t *self = (apx_portConnectionTable_t*) malloc(sizeof(apx_portConnectionTable_t));
   if (self != 0)
   {
      apx_error_t errorCode = apx_portConnectionTable_create(self, numPorts);
      if (errorCode != APX_NO_ERROR)
      {
         free(self);
         self = 0;
      }
   }
   return self;
}

void apx_portConnectionTable_delete(apx_portConnectionTable_t *self)
{
   if (self != 0)
   {
      apx_portConnectionTable_destroy(self);
      free(self);
   }
}

apx_error_t apx_portConnectionTable_connect(apx_portConnectionTable_t *self, apx_portRef_t *localRef, apx_portRef_t *remoteRef)
{
   if ( (self != 0) && (localRef != 0) && (remoteRef != 0) )
   {
      apx_portId_t portId = apx_portDataRef_getPortId(localRef);
      if ( (portId >= 0) && (portId < self->numPorts) )
      {
         return apx_portConnectionEntry_addConnection(&self->connections[portId], remoteRef);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portConnectionTable_disconnect(apx_portConnectionTable_t *self, apx_portRef_t *localRef, apx_portRef_t *remoteRef)
{
   if ( (self != 0) && (localRef != 0) && (remoteRef != 0) )
   {
      apx_portId_t portId = apx_portDataRef_getPortId(localRef);
      if ( (portId >= 0) && (portId < self->numPorts) )
      {
         return apx_portConnectionEntry_removeConnection(&self->connections[portId], remoteRef);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/*
apx_error_t apx_portConnectionTable_addConnection(apx_portConnectionTable_t *self, apx_portId_t portId, apx_portRef_t *portDataRef)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numPorts) )
   {
      return apx_portConnectionEntry_addConnection(&self->connections[portId], portDataRef);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portConnectionTable_removeConnection(apx_portConnectionTable_t *self, apx_portId_t portId, apx_portRef_t *portDataRef)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numPorts) )
   {
      return apx_portConnectionEntry_removeConnection(&self->connections[portId], portDataRef);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
*/

apx_portConnectionEntry_t *apx_portConnectionTable_getEntry(apx_portConnectionTable_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numPorts) )
   {
      return &self->connections[portId];
   }
   return (apx_portConnectionEntry_t*) 0;
}

apx_portRef_t *apx_portConnectionTable_getRef(apx_portConnectionTable_t *self, apx_portId_t portId, int32_t index)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numPorts) )
   {
      return apx_portConnectionEntry_get(&self->connections[portId], index);
   }
   return (apx_portRef_t*) 0;
}

int32_t apx_portConnectionTable_count(apx_portConnectionTable_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId >= 0) && (portId < self->numPorts) )
   {
      return apx_portConnectionEntry_count(&self->connections[portId]);
   }
   return 0;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


