/*****************************************************************************
* \file      apx_nodePortMap.c
* \author    Conny Gustafsson
* \date      2018-11-26
* \brief     Description
*
* Copyright (c) 2018 Conny Gustafsson
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
#include <string.h>
#include "apx_portDataMap.h"
#include "apx_portTriggerList.h"
#include "apx_portConnectionTable.h"

#include <stdio.h> //DEBUG ONLY
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_portDataMap_allocateMemory(apx_portDataMap_t *self);
static void apx_portDataMap_freeMemory(apx_portDataMap_t *self);
static void apx_portDataMap_createRequirePortData(apx_portDataMap_t *self, apx_nodeData_t *nodeData);
static void apx_portDataMap_createProvidePortData(apx_portDataMap_t *self, apx_nodeData_t *nodeData);
static apx_size_t apx_portDataMap_createPortDataElement(apx_portDataElement_t *attr, apx_port_t *port, apx_portId_t portId, apx_size_t offset);
static void apx_portDataMap_createPortTriggerList(apx_portDataMap_t *self);
static void apx_portDataMap_destroyPortTriggerList(apx_portDataMap_t *self);
static void apx_portDataMap_attachPortsToTriggerList(apx_portDataMap_t *self, apx_portConnectionEntry_t *entry, int32_t numPortRefs);
static void apx_portDataMap_detachPortsFromTriggerList(apx_portDataMap_t *self, apx_portConnectionEntry_t *entry, int32_t numPortRefs);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portDataMap_create(apx_portDataMap_t *self, apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) && (nodeData->node != 0))
   {
      apx_error_t errorCode;
      memset(self, 0, sizeof(apx_portDataMap_t));
      self->numRequirePorts = adt_ary_length(&nodeData->node->requirePortList);
      self->numProvidePorts = adt_ary_length(&nodeData->node->providePortList);
      errorCode = apx_portDataMap_allocateMemory(self);
      if (errorCode != APX_NO_ERROR)
      {
         apx_portDataMap_freeMemory(self);
         return errorCode;
      }
      apx_portDataMap_createRequirePortData(self, nodeData);
      apx_portDataMap_createProvidePortData(self, nodeData);
      self->requirePortPrograms = (apx_portProgramArray_t*) 0; //JIT-compiled programs not needed when ApxNode has been generated using source code generator
      self->providePortPrograms = (apx_portProgramArray_t*) 0; //JIT-compiled programs not needed when ApxNode has been generated using source code generator
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portDataMap_destroy(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      if (self->portTriggerList != 0)
      {
         apx_portDataMap_destroyPortTriggerList(self);
      }
      if (self->requirePortByteMap != 0)
      {
         apx_bytePortMap_delete(self->requirePortByteMap);
      }
      if (self->providePortByteMap != 0)
      {
         apx_bytePortMap_delete(self->providePortByteMap);
      }
      apx_portDataMap_freeMemory(self);
   }
}

apx_portDataMap_t *apx_portDataMap_new(apx_nodeData_t *nodeData)
{
   apx_portDataMap_t *self = (apx_portDataMap_t*) malloc(sizeof(apx_portDataMap_t));
   if (self != 0)
   {
      apx_error_t err = apx_portDataMap_create(self, nodeData);
      if (err != APX_NO_ERROR)
      {
         free(self);
         self =  (apx_portDataMap_t*) 0;
      }
   }
   return self;
}

void apx_portDataMap_delete(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      apx_portDataMap_destroy(self);
      free(self);
   }
}

apx_portDataElement_t *apx_portDataMap_getRequirePortDataElement(apx_portDataMap_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numRequirePorts))
   {
      return &self->requirePortDataAttributes[portId];
   }
   return (apx_portDataElement_t*) 0;
}

apx_portDataElement_t *apx_portDataMap_getProvidePortDataElement(apx_portDataMap_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numProvidePorts))
   {
      return &self->providePortDataAttributes[portId];
   }
   return (apx_portDataElement_t*) 0;
}

apx_portDataRef_t *apx_portDataMap_getRequirePortDataRef(apx_portDataMap_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numRequirePorts))
   {
      return &self->requirePortData[portId];
   }
   return (apx_portDataRef_t*) 0;
}

apx_portDataRef_t *apx_portDataMap_getProvidePortDataRef(apx_portDataMap_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numProvidePorts))
   {
      return &self->providePortData[portId];
   }
   return (apx_portDataRef_t*) 0;
}


/**
 * Initializes the self->portTriggerList member. Only used in server mode
 */
apx_error_t apx_portDataMap_initPortTriggerList(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      if (self->numProvidePorts > 0)
      {
         self->portTriggerList = (apx_portTriggerList_t*) malloc(sizeof(apx_portTriggerList_t)*self->numProvidePorts);
         if (self->portTriggerList == 0)
         {
            return APX_MEM_ERROR;
         }
         else
         {
            apx_portDataMap_createPortTriggerList(self);
         }
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portDataMap_initRequirePortByteMap(apx_portDataMap_t *self, apx_node_t *node)
{
   if (self != 0)
   {
      apx_error_t err = APX_NO_ERROR;
      self->requirePortByteMap = apx_bytePortMap_new(node, APX_REQUIRE_PORT, &err);
      if (self->requirePortByteMap == 0)
      {
         return err;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_portDataMap_initProvidePortByteMap(apx_portDataMap_t *self, apx_node_t *node)
{
   if (self != 0)
   {
      apx_error_t err = APX_NO_ERROR;
      self->providePortByteMap = apx_bytePortMap_new(node, APX_PROVIDE_PORT, &err);
      if (self->providePortByteMap == 0)
      {
         return err;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_portDataMap_updatePortTriggerList(apx_portDataMap_t *self, struct apx_portConnectionTable_tag *portConnectionTable)
{
   if ( (self != 0) && (portConnectionTable != 0) )
   {
      if ( (self->portTriggerList != 0) && (self->numProvidePorts) == portConnectionTable->numPorts)
      {
         int32_t portId;
         for(portId = 0; portId < portConnectionTable->numPorts; portId++)
         {
            apx_portConnectionEntry_t *entry = apx_portConnectionTable_getEntry(portConnectionTable, portId);
            if (entry != 0)
            {
               int32_t count = apx_portConnectionEntry_count(entry);
               if (count > 0)
               {
                  apx_portDataMap_attachPortsToTriggerList(self, entry, count);
               }
               else if (count < 0)
               {
                  apx_portDataMap_detachPortsFromTriggerList(self, entry, (-count));
               }
               else
               {
                  //MISRA
               }
            }
         }
      }
   }
}

apx_portTriggerList_t *apx_portDataMap_getPortTriggerList(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      return self->portTriggerList;
   }
   return (apx_portTriggerList_t*) 0;
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_portDataMap_allocateMemory(apx_portDataMap_t *self)
{
   if (self->numRequirePorts > 0)
   {
      self->requirePortData = (apx_portDataRef_t*) malloc(sizeof(apx_portDataRef_t)*self->numRequirePorts);
      if (self->requirePortData == 0)
      {
         return APX_MEM_ERROR;
      }
      self->requirePortDataAttributes = (apx_portDataElement_t*) malloc(sizeof(apx_portDataElement_t)*self->numRequirePorts);
      if (self->requirePortDataAttributes == 0)
      {
         return APX_MEM_ERROR;
      }
   }
   if (self->numProvidePorts > 0)
   {
      self->providePortData = (apx_portDataRef_t*) malloc(sizeof(apx_portDataRef_t)*self->numProvidePorts);
      if (self->providePortData == 0)
      {
         return APX_MEM_ERROR;
      }
      self->providePortDataAttributes = (apx_portDataElement_t*) malloc(sizeof(apx_portDataElement_t)*self->numProvidePorts);
      if (self->providePortDataAttributes == 0)
      {
         return APX_MEM_ERROR;
      }
   }
   return APX_NO_ERROR;
}

static void apx_portDataMap_freeMemory(apx_portDataMap_t *self)
{
   if (self != 0)
   {
      if (self->portTriggerList != 0)
      {
         free(self->portTriggerList);
      }
      if (self->requirePortData != 0)
      {
         free(self->requirePortData);
      }
      if (self->requirePortDataAttributes != 0)
      {
         free(self->requirePortDataAttributes);
      }
      if (self->providePortData != 0)
      {
         free(self->providePortData);
      }
      if (self->providePortDataAttributes != 0)
      {
         free(self->providePortDataAttributes);
      }
   }
}

static void apx_portDataMap_createRequirePortData(apx_portDataMap_t *self, apx_nodeData_t *nodeData)
{
   if (self->numRequirePorts > 0)
   {
      int32_t portId;
      apx_size_t offset = 0;
      for(portId=0;portId<self->numRequirePorts;portId++)
      {
         apx_portDataElement_t *attr = &self->requirePortDataAttributes[portId];
         apx_portDataRef_t *data = &self->requirePortData[portId];
         apx_uniquePortId_t uniquePortId = (uint32_t) portId;
         apx_port_t *port = apx_node_getRequirePort(nodeData->node, portId);
         offset += apx_portDataMap_createPortDataElement(attr, port, portId, offset);
         apx_portDataRef_create(data, nodeData, uniquePortId, attr);
      }
   }
}

static void apx_portDataMap_createProvidePortData(apx_portDataMap_t *self, apx_nodeData_t *nodeData)
{
   if (self->numProvidePorts > 0)
   {
      int32_t portId;
      apx_size_t offset = 0;
      for(portId=0;portId<self->numProvidePorts;portId++)
      {
         apx_portDataElement_t *attr = &self->providePortDataAttributes[portId];
         apx_portDataRef_t *data = &self->providePortData[portId];
         apx_uniquePortId_t uniquePortId = ((uint32_t) portId) | APX_PORT_ID_PROVIDE_PORT;
         apx_port_t *port = apx_node_getProvidePort(nodeData->node, portId);
         offset += apx_portDataMap_createPortDataElement(attr, port, portId, offset);
         apx_portDataRef_create(data, nodeData, uniquePortId, attr);
      }
   }
}

/**
 * Returns the total data size of the port
 */
static apx_size_t apx_portDataMap_createPortDataElement(apx_portDataElement_t *attr, apx_port_t *port, apx_portId_t portId, apx_size_t offset)
{
   apx_size_t dataElementSize;
   apx_error_t result = apx_dataSignature_calcPackLen(&port->dataSignature, &dataElementSize);
   if (result == APX_NO_ERROR)
   {
      apx_portDataElement_create(attr, port->portType, portId, offset, dataElementSize);
      return dataElementSize;
   }
   return 0u;
}

/**
 * Each provide port has its own portTriggerList. This function initializes all of the lists. (yes it's a list where each element is also a list).
 */
static void apx_portDataMap_createPortTriggerList(apx_portDataMap_t *self)
{
   int32_t portId;
   for(portId=0;portId<self->numProvidePorts;portId++)
   {
      apx_portTriggerList_create(&self->portTriggerList[portId]);
   }
}

/**
 * Destroys each portTriggerList in the portTriggerList array.
 */
static void apx_portDataMap_destroyPortTriggerList(apx_portDataMap_t *self)
{
   int32_t portId;
   for(portId=0;portId<self->numProvidePorts;portId++)
   {
      apx_portTriggerList_destroy(&self->portTriggerList[portId]);
   }
}

static void apx_portDataMap_attachPortsToTriggerList(apx_portDataMap_t *self, apx_portConnectionEntry_t *entry, int32_t numPortRefs)
{
   int32_t i;
   for(i=0; i < numPortRefs; i++)
   {
      apx_portDataRef_t *portDataRef = apx_portConnectionEntry_get(entry, i);
      apx_portTriggerList_insert(self->portTriggerList, portDataRef);
   }
}

static void apx_portDataMap_detachPortsFromTriggerList(apx_portDataMap_t *self, apx_portConnectionEntry_t *entry, int32_t numPortRefs)
{
   int32_t i;
   for(i=0; i < numPortRefs; i++)
   {
      apx_portDataRef_t *portDataRef = apx_portConnectionEntry_get(entry, i);
      apx_portTriggerList_remove(self->portTriggerList, portDataRef);
   }
}
