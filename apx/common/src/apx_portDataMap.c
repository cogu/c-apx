/*****************************************************************************
* \file      apx_nodePortMap.c
* \author    Conny Gustafsson
* \date      2018-11-26
* \brief     Various data structures associated with an apx_nodeData_t for use in apx client and server
*
* Copyright (c) 2018-2019 Conny Gustafsson
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
static apx_error_t apx_portDataMap_allocateMemory(apx_portDataMap_t *self, bool hasPortPrograms);
static void apx_portDataMap_freeMemory(apx_portDataMap_t *self);
static void apx_portDataMap_createRequirePortData(apx_portDataMap_t *self, apx_nodeData_t *nodeData);
static void apx_portDataMap_createProvidePortData(apx_portDataMap_t *self, apx_nodeData_t *nodeData);
static apx_size_t apx_portDataMap_createPortDataProps(apx_portDataProps_t *attr, apx_port_t *port, apx_portId_t portId, apx_size_t offset);
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
      errorCode = apx_portDataMap_allocateMemory(self, nodeData->isDynamic);
      if (errorCode != APX_NO_ERROR)
      {
         apx_portDataMap_freeMemory(self);
         return errorCode;
      }
      apx_portDataMap_createRequirePortData(self, nodeData);
      apx_portDataMap_createProvidePortData(self, nodeData);
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
      if (self->requirePortPackPrograms)
      {
         int32_t portId;
         for(portId = 0; portId < self->numRequirePorts; portId++)
         {
            adt_bytes_delete(self->requirePortPackPrograms[portId]);
            self->requirePortPackPrograms[portId] = (adt_bytes_t*) 0;
         }
      }
      if (self->providePortPackPrograms)
      {
         int32_t portId;
         for(portId = 0; portId < self->numProvidePorts; portId++)
         {
            adt_bytes_delete(self->providePortPackPrograms[portId]);
            self->providePortPackPrograms[portId] = (adt_bytes_t*) 0;
         }
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

apx_portDataProps_t *apx_portDataMap_getRequirePortDataProps(apx_portDataMap_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numRequirePorts))
   {
      return &self->requirePortDataProps[portId];
   }
   return (apx_portDataProps_t*) 0;
}

apx_portDataProps_t *apx_portDataMap_getProvidePortDataProps(apx_portDataMap_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numProvidePorts))
   {
      return &self->providePortDataProps[portId];
   }
   return (apx_portDataProps_t*) 0;
}

apx_portDataRef_t *apx_portDataMap_getRequirePortDataRef(apx_portDataMap_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numRequirePorts))
   {
      return &self->requirePortDataRef[portId];
   }
   return (apx_portDataRef_t*) 0;
}

apx_portDataRef_t *apx_portDataMap_getProvidePortDataRef(apx_portDataMap_t *self, apx_portId_t portId)
{
   if ( (self != 0) && (portId>=0) && (portId < self->numProvidePorts))
   {
      return &self->providePortDataRef[portId];
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

apx_error_t apx_portDataMap_createPackPrograms(apx_portDataMap_t *self, apx_compiler_t *compiler, apx_node_t *node, apx_uniquePortId_t *errPortId)
{
   apx_error_t retval = APX_NO_ERROR;
   if ( (self != 0) && (compiler != 0) && (node != 0) && (errPortId != 0) )
   {
      int32_t portIndex;
      adt_bytearray_t program;
      adt_bytearray_create(&program, APX_PROGRAM_GROW_SIZE);
      if (self->numRequirePorts > 0)
      {
         if (self->requirePortPackPrograms == 0)
         {
            return APX_NULL_PTR_ERROR;
         }
         for(portIndex=0; portIndex < self->numRequirePorts; portIndex++)
         {
            apx_dataElement_t *dataElement;
            apx_error_t compilationResult;
            apx_port_t *port = apx_node_getRequirePort(node, portIndex);
            assert(port != 0);
            dataElement = apx_port_getDerivedDataElement(port);
            assert(dataElement != 0);

            adt_bytearray_clear(&program);
            apx_compiler_begin_packProgram(compiler, &program);
            compilationResult = apx_compiler_compilePackDataElement(compiler, dataElement);
            if (compilationResult == APX_NO_ERROR)
            {
               adt_bytes_t *tmp;
               tmp = adt_bytearray_bytes(&program);
               if (tmp == 0)
               {
                  retval = APX_MEM_ERROR;
                  break;
               }
               self->requirePortPackPrograms[portIndex] = tmp;
            }
            else
            {
               retval = compilationResult;
               break;
            }
            (*errPortId)++;
         }
      }

      if ( (retval == APX_NO_ERROR) && (self->numProvidePorts > 0) )
      {
         if (self->providePortPackPrograms == 0)
         {
            return APX_NULL_PTR_ERROR;
         }
         *errPortId = (apx_uniquePortId_t) APX_PORT_ID_PROVIDE_PORT;
         for(portIndex=0; portIndex < self->numProvidePorts; portIndex++)
         {
            apx_dataElement_t *dataElement;
            apx_error_t compilationResult;
            apx_port_t *port = apx_node_getProvidePort(node, portIndex);
            assert(port != 0);
            dataElement = apx_port_getDerivedDataElement(port);
            assert(dataElement != 0);

            adt_bytearray_clear(&program);
            apx_compiler_begin_packProgram(compiler, &program);
            compilationResult = apx_compiler_compilePackDataElement(compiler, dataElement);
            if (compilationResult == APX_NO_ERROR)
            {
               adt_bytes_t *tmp;
               tmp = adt_bytearray_bytes(&program);
               if (tmp == 0)
               {
                  retval = APX_MEM_ERROR;
                  break;
               }
               self->providePortPackPrograms[portIndex] = tmp;
            }
            else
            {
               retval = compilationResult;
               break;
            }
            (*errPortId)++;
         }
      }
      adt_bytearray_destroy(&program);
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_portDataMap_allocateMemory(apx_portDataMap_t *self, bool hasPortPrograms)
{
   if (self->numRequirePorts > 0)
   {
      self->requirePortDataRef = (apx_portDataRef_t*) malloc(sizeof(apx_portDataRef_t)*self->numRequirePorts);
      if (self->requirePortDataRef == 0)
      {
         return APX_MEM_ERROR;
      }
      self->requirePortDataProps = (apx_portDataProps_t*) malloc(sizeof(apx_portDataProps_t)*self->numRequirePorts);
      if (self->requirePortDataProps == 0)
      {
         return APX_MEM_ERROR;
      }
      if (hasPortPrograms)
      {
         size_t numBytes = sizeof(adt_bytes_t*) * self->numRequirePorts;
         self->requirePortPackPrograms = (adt_bytes_t**) malloc(numBytes);
         if (self->requirePortPackPrograms == 0)
         {
            return APX_MEM_ERROR;
         }
         memset(self->requirePortPackPrograms, 0, numBytes);
      }
   }
   if (self->numProvidePorts > 0)
   {
      self->providePortDataRef = (apx_portDataRef_t*) malloc(sizeof(apx_portDataRef_t)*self->numProvidePorts);
      if (self->providePortDataRef == 0)
      {
         return APX_MEM_ERROR;
      }
      self->providePortDataProps = (apx_portDataProps_t*) malloc(sizeof(apx_portDataProps_t)*self->numProvidePorts);
      if (self->providePortDataProps == 0)
      {
         return APX_MEM_ERROR;
      }
      if (hasPortPrograms)
      {
         size_t numBytes = sizeof(adt_bytes_t*) * self->numProvidePorts;
         self->providePortPackPrograms = (adt_bytes_t**) malloc(numBytes);
         if (self->providePortPackPrograms == 0)
         {
            return APX_MEM_ERROR;
         }
         memset(self->providePortPackPrograms, 0, numBytes);
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
      if (self->requirePortDataRef != 0)
      {
         free(self->requirePortDataRef);
      }
      if (self->requirePortDataProps != 0)
      {
         free(self->requirePortDataProps);
      }
      if (self->providePortDataRef != 0)
      {
         free(self->providePortDataRef);
      }
      if (self->providePortDataProps != 0)
      {
         free(self->providePortDataProps);
      }
      if (self->requirePortPackPrograms != 0)
      {
         free(self->requirePortPackPrograms);
      }
      if (self->providePortPackPrograms != 0)
      {
         free(self->providePortPackPrograms);
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
         apx_portDataProps_t *props = &self->requirePortDataProps[portId];
         apx_portDataRef_t *dataRef = &self->requirePortDataRef[portId];
         apx_uniquePortId_t uniquePortId = (uint32_t) portId;
         apx_port_t *port = apx_node_getRequirePort(nodeData->node, portId);
         offset += apx_portDataMap_createPortDataProps(props, port, portId, offset);
         apx_portDataRef_create(dataRef, nodeData, uniquePortId, props);
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
         apx_portDataProps_t *props = &self->providePortDataProps[portId];
         apx_portDataRef_t *dataRef = &self->providePortDataRef[portId];
         apx_uniquePortId_t uniquePortId = ((uint32_t) portId) | APX_PORT_ID_PROVIDE_PORT;
         apx_port_t *port = apx_node_getProvidePort(nodeData->node, portId);
         offset += apx_portDataMap_createPortDataProps(props, port, portId, offset);
         apx_portDataRef_create(dataRef, nodeData, uniquePortId, props);
      }
   }
}

/**
 * Returns the total data size of the port
 */
static apx_size_t apx_portDataMap_createPortDataProps(apx_portDataProps_t *attr, apx_port_t *port, apx_portId_t portId, apx_size_t offset)
{
   apx_size_t dataSize;
   apx_error_t result = apx_dataSignature_calcPackLen(&port->dataSignature, &dataSize);
   if (result == APX_NO_ERROR)
   {
      apx_portDataProps_create(attr, port->portType, portId, offset, dataSize);
      return dataSize;
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
