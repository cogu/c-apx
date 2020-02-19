/*****************************************************************************
* \file      apx_portSignatureMap.c
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     Port signature map
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
#include <assert.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h> //DEBUG ONLY
#include "apx_portSignatureMap.h"
#include "apx_nodeInstance.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif
//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_portSignatureMap_connectRequirePortsInternal(apx_portSignatureMap_t *self, apx_nodeInstance_t *nodeInstance, apx_nodeInfo_t *nodeInfo);
static apx_error_t apx_portSignatureMap_connectProvidePortsInternal(apx_portSignatureMap_t *self, apx_nodeInstance_t *nodeInstance, apx_nodeInfo_t *nodeInfo);
static apx_error_t apx_portSignatureMap_insert(apx_portSignatureMap_t *self, const char *portSignature, apx_portRef_t *portRef);
static apx_portSignatureMapEntry_t *apx_portSignatureMap_createNewEntry(apx_portSignatureMap_t *self, const char *portSignature);
static apx_error_t apx_portSignatureMap_remove(apx_portSignatureMap_t *self, const char *portSignature);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_portSignatureMap_create(apx_portSignatureMap_t *self)
{
   if (self != 0)
   {
      adt_hash_create(&self->internalMap, apx_portSignatureMapEntry_vdelete);
   }
}

void apx_portSignatureMap_destroy(apx_portSignatureMap_t *self)
{
   adt_hash_destroy(&self->internalMap);
}

apx_portSignatureMapEntry_t *apx_portSignatureMap_find(apx_portSignatureMap_t *self, const char *portSignature)
{
   if ( (self != 0) && (portSignature != 0) )
   {
      void **ppResult = adt_hash_get(&self->internalMap, portSignature);
      if (ppResult != 0)
      {
         return (apx_portSignatureMapEntry_t*) *ppResult;
      }
   }
   return (apx_portSignatureMapEntry_t*) 0;
}

int32_t apx_portSignatureMap_length(apx_portSignatureMap_t *self)
{
   if (self != 0)
   {
      return adt_hash_length(&self->internalMap);
   }
   return -1;
}

apx_portSignatureMap_t *apx_portSignatureMap_new(void)
{
   apx_portSignatureMap_t *self = (apx_portSignatureMap_t*) malloc(sizeof(apx_portSignatureMap_t));
   if (self != 0)
   {
      apx_portSignatureMap_create(self);
   }
   return self;

}

void apx_portSignatureMap_delete(apx_portSignatureMap_t *self)
{
   if (self != 0)
   {
      apx_portSignatureMap_destroy(self);
      free(self);
   }
}

apx_error_t apx_portSignatureMap_connectProvidePorts(apx_portSignatureMap_t *self, struct apx_nodeInstance_tag *nodeInstance)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {

      apx_nodeInfo_t *nodeInfo = apx_nodeInstance_getNodeInfo(nodeInstance);
      apx_error_t retval;
      if (nodeInfo == 0)
      {
         return APX_NULL_PTR_ERROR;
      }
      retval = apx_portSignatureMap_connectProvidePortsInternal(self, nodeInstance, nodeInfo);
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portSignatureMap_connectRequirePorts(apx_portSignatureMap_t *self, struct apx_nodeInstance_tag *nodeInstance)
{
   if ( (self != 0) && (nodeInstance != 0) )
   {

      apx_nodeInfo_t *nodeInfo = apx_nodeInstance_getNodeInfo(nodeInstance);
      apx_error_t retval;
      if (nodeInfo == 0)
      {
         return APX_NULL_PTR_ERROR;
      }
      retval = apx_portSignatureMap_connectRequirePortsInternal(self, nodeInstance, nodeInfo);
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portSignatureMap_disconnectPorts(apx_portSignatureMap_t *self, struct apx_nodeInstance_tag *nodeInstance);

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_portSignatureMap_connectRequirePortsInternal(apx_portSignatureMap_t *self, apx_nodeInstance_t *nodeInstance, apx_nodeInfo_t *nodeInfo)
{
   apx_portId_t portId;
   apx_portCount_t numRequirePorts = apx_nodeInfo_getNumRequirePorts(nodeInfo);
   for(portId = 0; portId < numRequirePorts; portId++)
   {
      const char *portSignature;
      apx_error_t rc;
      apx_portRef_t *portRef;
      portSignature = apx_nodeInfo_getRequirePortSignature(nodeInfo, portId);
      portRef = apx_nodeInstance_getRequirePortRef(nodeInstance, portId);
      rc = apx_portSignatureMap_insert(self, portSignature, portRef);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_portSignatureMap_connectProvidePortsInternal(apx_portSignatureMap_t *self, apx_nodeInstance_t *nodeInstance, apx_nodeInfo_t *nodeInfo)
{
   apx_portId_t portId;
   apx_portCount_t numProvidePorts = apx_nodeInfo_getNumProvidePorts(nodeInfo);
   for(portId = 0; portId < numProvidePorts; portId++)
   {
      const char *portSignature;
      apx_error_t rc;
      apx_portRef_t *portRef;
      portSignature = apx_nodeInfo_getProvidePortSignature(nodeInfo, portId);
      portRef = apx_nodeInstance_getProvidePortRef(nodeInstance, portId);
      rc = apx_portSignatureMap_insert(self, portSignature, portRef);
      if (rc != APX_NO_ERROR)
      {
         return rc;
      }
   }
   return APX_NO_ERROR;
}

static apx_error_t apx_portSignatureMap_insert(apx_portSignatureMap_t *self, const char *portSignature, apx_portRef_t *portRef)
{
   apx_portSignatureMapEntry_t *entry = apx_portSignatureMap_find(self, portSignature);
   assert(self != 0);
   assert(portSignature != 0);
   assert(strlen(portSignature) > 0);
   assert(portRef != 0);
   if (entry == 0)
   {
      entry = apx_portSignatureMap_createNewEntry(self, portSignature);
      if (entry == 0)
      {
         return APX_MEM_ERROR;
      }
   }
   assert(entry != 0);
   if (apx_portRef_isProvidePort(portRef))
   {
      apx_portSignatureMapEntry_attachProvidePort(entry, portRef, true);
      apx_portSignatureMapEntry_notifyRequirePortsAboutProvidePortChange(entry, portRef, APX_PORT_CONNECTED_EVENT);
   }
   else
   {
      apx_portSignatureMapEntry_attachRequirePort(entry, portRef);
      apx_portSignatureMapEntry_notifyProvidePortsAboutRequirePortChange(entry, portRef, APX_PORT_CONNECTED_EVENT);
   }
   return APX_NO_ERROR;
}

static apx_portSignatureMapEntry_t *apx_portSignatureMap_createNewEntry(apx_portSignatureMap_t *self, const char *portSignature)
{
   apx_portSignatureMapEntry_t *entry = apx_portSignatureMapEntry_new();
   if (entry != 0)
   {
      adt_hash_set(&self->internalMap, portSignature, entry);
   }
   return entry;
}

static apx_error_t apx_portSignatureMap_remove(apx_portSignatureMap_t *self, const char *portSignature)
{
   if ( (self != 0) && (portSignature != 0) )
   {
      apx_portSignatureMapEntry_t *entry = (apx_portSignatureMapEntry_t*) adt_hash_remove(&self->internalMap, portSignature);
      if (entry != 0)
      {
         apx_portSignatureMapEntry_delete(entry);
         return APX_NO_ERROR;
      }
      return APX_NOT_FOUND_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}
