/*****************************************************************************
* \file      apx_portSignatureMap.h
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     An element in an apx_portSignatureMap_t
*
* Copyright (c) 2020 Conny Gustafsson
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
#include <assert.h>
#include "apx_error.h"
#include "apx_portSignatureMapEntry.h"
#include "apx_nodeInstance.h"
#include "apx_portConnectorChangeTable.h"
#include "apx_nodeData.h"
#include "apx_port.h"

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
void apx_portSignatureMapEntry_create(apx_portSignatureMapEntry_t *self)
{
   if ( self != 0 )
   {
      self->preferredProvider = (apx_portRef_t*) 0;
      adt_list_create(&self->requirePortRef, (void (*)(void*)) 0);
      adt_list_create(&self->providePortRef, (void (*)(void*)) 0);
   }
}

void apx_portSignatureMapEntry_destroy(apx_portSignatureMapEntry_t *self)
{
   if (self != 0)
   {
      adt_list_destroy(&self->requirePortRef);
      adt_list_destroy(&self->providePortRef);
   }
}

apx_portSignatureMapEntry_t *apx_portSignatureMapEntry_new(void)
{
   apx_portSignatureMapEntry_t *self = (apx_portSignatureMapEntry_t*) malloc(sizeof(apx_portSignatureMapEntry_t));
   if(self != 0)
   {
      apx_portSignatureMapEntry_create(self);
   }
   return self;
}

void apx_portSignatureMapEntry_delete(apx_portSignatureMapEntry_t *self)
{
   if (self != 0)
   {
      apx_portSignatureMapEntry_destroy(self);
      free(self);
   }
}

void apx_portSignatureMapEntry_vdelete(void *arg)
{
   apx_portSignatureMapEntry_delete((apx_portSignatureMapEntry_t*) arg);
}

void apx_portSignatureMapEntry_attachRequirePort(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef)
{
   if ((self != 0) && (portRef != 0))
   {
      adt_list_insert(&self->requirePortRef, portRef);
   }
}

void apx_portSignatureMapEntry_attachProvidePort(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef, bool isPreferred)
{
   if ((self != 0) && (portRef != 0))
   {
      adt_list_insert(&self->providePortRef, portRef);
      if (isPreferred)
      {
         apx_portSignatureMapEntry_setPreferredProvider(self, portRef);
      }
   }
}

void apx_portSignatureMapEntry_detachRequirePort(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef)
{
   if ((self != 0) && (portRef != 0))
   {
      adt_list_remove(&self->requirePortRef, portRef);
   }
}

void apx_portSignatureMapEntry_detachProvidePort(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef)
{
   if ((self != 0) && (portRef != 0))
   {
      adt_list_remove(&self->providePortRef, portRef);
   }
}


bool apx_portSignatureMapEntry_isEmpty(apx_portSignatureMapEntry_t *self)
{
   if (self != 0)
   {
      return (bool) ( (adt_list_is_empty(&self->providePortRef) == true) &&  (adt_list_is_empty(&self->requirePortRef) == true));
   }
   return false;
}

/**
 * Returns the PortDataRef for the first connected provider or NULL in case no providers are connected
 */
apx_portRef_t *apx_portSignatureMapEntry_getFirstProvider(apx_portSignatureMapEntry_t *self)
{
   if (self != 0)
   {
      return (apx_portRef_t*) adt_list_first(&self->providePortRef);
   }
   return (apx_portRef_t*) 0;
}

apx_portRef_t *apx_portSignatureMapEntry_getLastProvider(apx_portSignatureMapEntry_t *self)
{
   if (self != 0)
   {
      return (apx_portRef_t*) adt_list_last(&self->providePortRef);
   }
   return (apx_portRef_t*) 0;
}

void apx_portSignatureMapEntry_setPreferredProvider(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef)
{
   if ( (self != 0) && (portRef != 0) )
   {
      self->preferredProvider = portRef;
   }
}

apx_portRef_t *apx_portSignatureMapEntry_getPreferredProvider(apx_portSignatureMapEntry_t *self)
{
   if (self != 0)
   {
      return self->preferredProvider;
   }
   return (apx_portRef_t*) 0;
}

void apx_portSignatureMapEntry_notifyRequirePortsAboutProvidePortChange(apx_portSignatureMapEntry_t *self, apx_portRef_t *providePortRef, apx_portConnectorEvent_t eventType)
{
   if ( (self != 0) && (providePortRef != 0) && ( (eventType == APX_PORT_CONNECTED_EVENT) || (eventType == APX_PORT_DISCONNECTED_EVENT) ) )
   {
      if (adt_list_length(&self->requirePortRef) > 0)
      {
         adt_list_elem_t *iter;
         apx_portConnectorChangeTable_t *providePortChangeTable;
         apx_portConnectorChangeEntry_t *providePortChangeEntry;
         apx_portConnectorChangeEntry_actionFunc *actionFunc;
         assert(providePortRef->nodeInstance != 0);

         actionFunc = (eventType == APX_PORT_CONNECTED_EVENT)? apx_portConnectorChangeEntry_addConnection : apx_portConnectorChangeEntry_removeConnection;

         providePortChangeTable = apx_nodeInstance_getProvidePortConnectorChanges(providePortRef->nodeInstance, true);
         assert(providePortChangeTable != 0);
         providePortChangeEntry = apx_portConnectorChangeTable_getEntry(providePortChangeTable, apx_portRef_getPortId(providePortRef));
         assert(providePortChangeEntry != 0);

         for(iter = adt_list_iter_first(&self->requirePortRef); iter != 0; iter = adt_list_iter_next(iter))
         {
            apx_portConnectorChangeTable_t *requirePortChangeTable;
            apx_portConnectorChangeEntry_t *requirePortChangeEntry;
            apx_portRef_t *requirePortRef = (apx_portRef_t*) iter->pItem;
            assert(requirePortRef != 0);
            assert(requirePortRef->nodeInstance != 0);
            requirePortChangeTable = apx_nodeInstance_getRequirePortConnectorChanges(requirePortRef->nodeInstance, true);
            assert(requirePortChangeTable != 0);
            requirePortChangeEntry = apx_portConnectorChangeTable_getEntry(requirePortChangeTable, apx_portRef_getPortId(requirePortRef));
            assert(requirePortChangeEntry != 0);
            actionFunc(requirePortChangeEntry, providePortRef);
            actionFunc(providePortChangeEntry, requirePortRef);
         }
      }
   }
}

void apx_portSignatureMapEntry_notifyProvidePortsAboutRequirePortChange(apx_portSignatureMapEntry_t *self, apx_portRef_t *requirePortRef, apx_portConnectorEvent_t eventType)
{
   if ( (self != 0) && (requirePortRef != 0) && ( (eventType == APX_PORT_CONNECTED_EVENT) || (eventType == APX_PORT_DISCONNECTED_EVENT) ) )
   {
      if (adt_list_length(&self->providePortRef) > 0)
      {
         adt_list_elem_t *iter;
         apx_portConnectorChangeTable_t *requirePortChangeTable;
         apx_portConnectorChangeEntry_t *requirePortChangeEntry;

         apx_portConnectorChangeEntry_actionFunc *actionFunc;
         assert(requirePortRef->nodeInstance != 0);

         actionFunc = (eventType == APX_PORT_CONNECTED_EVENT)? apx_portConnectorChangeEntry_addConnection : apx_portConnectorChangeEntry_removeConnection;

         requirePortChangeTable = apx_nodeInstance_getRequirePortConnectorChanges(requirePortRef->nodeInstance, true);
         assert(requirePortChangeTable != 0);
         requirePortChangeEntry = apx_portConnectorChangeTable_getEntry(requirePortChangeTable, apx_portRef_getPortId(requirePortRef));
         assert(requirePortChangeTable != 0);

         for(iter = adt_list_iter_first(&self->providePortRef); iter != 0; iter = adt_list_iter_next(iter))
         {
            apx_portConnectorChangeTable_t *providePortChangeTable;
            apx_portConnectorChangeEntry_t *providePortChangeEntry;
            apx_portRef_t *providePortRef = (apx_portRef_t*) iter->pItem;
            assert(requirePortRef != 0);
            assert(requirePortRef->nodeInstance != 0);
            providePortChangeTable = apx_nodeInstance_getProvidePortConnectorChanges(requirePortRef->nodeInstance, true);
            assert(providePortChangeTable != 0);
            providePortChangeEntry = apx_portConnectorChangeTable_getEntry(providePortChangeTable, apx_portRef_getPortId(providePortRef));
            assert(providePortChangeEntry != 0);
            actionFunc(requirePortChangeEntry, providePortRef);
            actionFunc(providePortChangeEntry, requirePortRef);
         }
      }
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

