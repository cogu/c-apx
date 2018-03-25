
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "apx_nodeInfo.h"
#include "apx_dataTrigger.h"
#include "apx_fileManager.h"
#include "apx_nodeData.h"
#include "apx_logging.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_nodeInfo_connectRequirePortInternal(apx_nodeInfo_t *self, int32_t portIndex, apx_node_t *providerNode, apx_port_t *providerPort);
static void apx_nodeInfo_connectProvidePortInternal(apx_nodeInfo_t *self, int32_t portIndex, apx_node_t *requesterNode, apx_port_t *requirePort);
static void apx_nodeInfo_disconnectRequirePortInternal(apx_nodeInfo_t *requesterNodeInfo, int32_t requesterPortIndex);
static void apx_nodeInfo_disconnectProvidePortInternal(apx_nodeInfo_t *providerNodeInfo, int32_t providerPortIndex, apx_portref_t *portref);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_nodeInfo_create(apx_nodeInfo_t *self, apx_node_t *node)
{
   if ( (self != 0) && (node != 0) )
   {
      int32_t numRequirePorts;
      int32_t numProvidePorts;
      int32_t requireDataLen;
      int32_t provideDataLen;
      adt_ary_create(&self->requireConnectors,apx_portref_vdelete);
      adt_ary_create(&self->provideConnectors,adt_ary_vdelete);
      apx_portDataMap_create(&self->inDataMap);
      apx_portDataMap_create(&self->outDataMap);
      self->providePortFlags=0;
      self->requirePortFlags=0;
      self->pendingProvidePortFlags=0;
      self->pendingRequirePortFlags=0;
      self->node=node;
      node->nodeInfo=self;
      self->isWeakRef_node = true; //default true
      numRequirePorts = adt_ary_length(&self->node->requirePortList);
      numProvidePorts = adt_ary_length(&self->node->providePortList);
      adt_ary_resize(&self->requireConnectors,numRequirePorts);
      adt_ary_resize(&self->provideConnectors,numProvidePorts);
      apx_node_finalize(node);
      apx_portDataMap_build(&self->inDataMap,node,APX_REQUIRE_PORT);
      apx_portDataMap_build(&self->outDataMap,node,APX_PROVIDE_PORT);
      requireDataLen = apx_portDataMap_getDataLen(&self->inDataMap);
      provideDataLen = apx_portDataMap_getDataLen(&self->outDataMap);
      //GFX_LOG_INFO("%s/require: %d\n",node->name,requireDataLen);
      //GFX_LOG_INFO("%s/provide: %d\n",node->name,provideDataLen);
      if(requireDataLen < 0)
      {
         APX_LOG_ERROR("[APX_NODE_INFO] %s", "apx_portDataMap_getDataLen for requirePortDataMap return negative result");
         requireDataLen=0;
      }
      if(provideDataLen < 0)
      {
         APX_LOG_ERROR("[APX_NODE_INFO] %s", "apx_portDataMap_getDataLen for providePortDataMap return negative result");
         provideDataLen=0;
      }
      self->inPortDataLen = requireDataLen;
      self->outPortDataLen = provideDataLen;
      self->requirePortFlags = (uint8_t*) malloc(numRequirePorts); //allocate numRequirePorts bytes
      self->providePortFlags = (uint8_t*) malloc(numProvidePorts); //allocate numProvidePorts bytes
      if (self->requirePortFlags != 0)
      {
         memset(self->requirePortFlags,0,numRequirePorts);
      }
      else
      {
         APX_LOG_ERROR("[APX_NODE_INFO] (%d): malloc failed\n", (int) __LINE__);
      }
      if (self->providePortFlags != 0)
      {
         memset(self->providePortFlags,0,numProvidePorts);
      }
      else
      {
         APX_LOG_ERROR("[APX_NODE_INFO] (%d) : malloc failed\n", (int) __LINE__);
      }
      apx_dataTriggerTable_create(&self->outDataTriggerTable,self,0,0);
      self->nodeData = (apx_nodeData_t*) 0; //default NULL
   }

}

void apx_nodeInfo_destroy(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      adt_ary_destroy(&self->requireConnectors);
      adt_ary_destroy(&self->provideConnectors);
      apx_portDataMap_destroy(&self->inDataMap);
      apx_portDataMap_destroy(&self->outDataMap);
      apx_dataTriggerTable_destroy(&self->outDataTriggerTable);
      if (self->requirePortFlags != 0)
      {
         free(self->requirePortFlags);
      }
      if (self->providePortFlags != 0)
      {
         free(self->providePortFlags);
      }
      if ( (self->isWeakRef_node == false) && (self->node != 0) )
      {
         apx_node_delete(self->node);
      }
   }
}

apx_nodeInfo_t *apx_nodeInfo_new(apx_node_t *node)
{
   apx_nodeInfo_t *self = (apx_nodeInfo_t*) malloc(sizeof(apx_nodeInfo_t));
   if(self != 0)
   {
      apx_nodeInfo_create(self,node);
   }
   else{
      errno = ENOMEM;
   }
   return self;
}
void apx_nodeInfo_delete(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      apx_nodeInfo_destroy(self);
      free(self);
   }
}

void apx_nodeInfo_vdelete(void *arg)
{
   apx_nodeInfo_delete( (apx_nodeInfo_t*) arg );
}

/**
 * creates a connector between providerPort and requesterPort
 *
 */
void apx_nodeInfo_connectPort(apx_nodeInfo_t *providerNodeInfo, int32_t providerPortIndex, apx_nodeInfo_t *requesterNodeInfo, int32_t requesterPortIndex)
{
   if ( (providerNodeInfo != 0) && (requesterNodeInfo != 0) && (providerNodeInfo->node != 0) && (requesterNodeInfo->node != 0) )
   {
      int32_t numProvidePorts;
      int32_t numRequirePorts;
      numProvidePorts = adt_ary_length(&providerNodeInfo->node->providePortList);
      numRequirePorts = adt_ary_length(&requesterNodeInfo->node->requirePortList);
      if ( (providerPortIndex>=0) && (providerPortIndex<numProvidePorts) &&
           (requesterPortIndex>=0) && (requesterPortIndex<numRequirePorts) )
      {
         apx_port_t *providePort;
         apx_port_t *requirePort;
         providePort = (apx_port_t *) *adt_ary_get(&providerNodeInfo->node->providePortList,providerPortIndex);
         requirePort = (apx_port_t *) *adt_ary_get(&requesterNodeInfo->node->requirePortList,requesterPortIndex);

         //create connection point on the provide port
         apx_nodeInfo_connectProvidePortInternal(providerNodeInfo,providerPortIndex,requesterNodeInfo->node,requirePort);

         //create connection point on the require port
         apx_nodeInfo_connectRequirePortInternal(requesterNodeInfo,requesterPortIndex,providerNodeInfo->node,providePort);

         //set dirty flag on provide port in order to trigger recalculation of data triggers

         //set event flag on require port (for later processing)
         providerNodeInfo->providePortFlags[providerPortIndex] |= APX_PORT_EVENT_CONNECTED;
         providerNodeInfo->pendingProvidePortFlags++;

         //set event flag on require port (for later processing)
         requesterNodeInfo->requirePortFlags[requesterPortIndex] |= APX_PORT_EVENT_CONNECTED;
         requesterNodeInfo->pendingRequirePortFlags++;
      }
   }
}

/**
 * removes connection point on provide port
 */
void apx_nodeInfo_disconnectProvidePort(apx_nodeInfo_t *providerNodeInfo, int32_t providerPortIndex)
{
   if ( (providerNodeInfo != 0) && (providerNodeInfo->node != 0) )
   {
      int32_t numProvidePorts;
      int32_t numProvideConnectionPoints;
      numProvidePorts = adt_ary_length(&providerNodeInfo->node->providePortList);
      numProvideConnectionPoints = adt_ary_length(&providerNodeInfo->provideConnectors);
      if ( (providerPortIndex>=0) && (providerPortIndex<numProvidePorts) &&
           (providerPortIndex<numProvideConnectionPoints) )
      {
         adt_ary_t *connectionList = (adt_ary_t*) *adt_ary_get(&providerNodeInfo->provideConnectors,providerPortIndex);
         int32_t i;
         int32_t end;
         end = adt_ary_length(connectionList);
         for(i=0;i<end;i++)
         {
            apx_nodeInfo_t *requesterNodeInfo;
            int32_t requesterPortIndex;
            apx_portref_t *portref = (apx_portref_t*) *adt_ary_get(connectionList,i);
            requesterNodeInfo = portref->node->nodeInfo;
            requesterPortIndex = portref->port->portIndex;
            //We cannot use the normal function apx_nodeInfo_disconnectRequirePort here, it will cause a recursive loop.
            //Instead we use the internal function apx_nodeInfo_disconnectRequirePortInternal.
            apx_nodeInfo_disconnectRequirePortInternal(requesterNodeInfo,requesterPortIndex);
            requesterNodeInfo->requirePortFlags[requesterPortIndex] |= APX_PORT_EVENT_DISCONNECTED;
            requesterNodeInfo->pendingRequirePortFlags++;
         }
         //now all connections to connectList should be cleared, time to delete connectionList entirely
         //First set the connectorList pointer to NULL (i.e. we detach the object from the list)
         adt_ary_set(&providerNodeInfo->provideConnectors,providerPortIndex, (void*) 0);
         //Now delete the detached object, the virtual destructor will take care of freeing memory for the internal objects in the list
         adt_ary_delete(connectionList);
         //finally set the disconnected event for later processing where we will need to update data trigger tables
         providerNodeInfo->providePortFlags[providerPortIndex] |= APX_PORT_EVENT_DISCONNECTED;
         providerNodeInfo->pendingProvidePortFlags++;
      }
   }
}

void apx_nodeInfo_disconnectRequirePort(apx_nodeInfo_t *requesterNodeInfo, int32_t requesterPortIndex)
{
   if ( (requesterNodeInfo != 0) && (requesterNodeInfo->node != 0) )
   {
      int32_t numRequirePorts;
      int32_t numRequireConnectionPoints;
      numRequirePorts = adt_ary_length(&requesterNodeInfo->node->requirePortList);
      numRequireConnectionPoints = adt_ary_length(&requesterNodeInfo->requireConnectors);
      if ( (requesterPortIndex>=0) && (requesterPortIndex<numRequirePorts) &&
           (requesterPortIndex<numRequireConnectionPoints) )
      {
         apx_portref_t *requireConnector; //connector from a require port
         apx_portref_t provideConnector; //connector from a provide port

         int32_t providerPortIndex;
         requireConnector = apx_nodeInfo_getRequirePortConnector(requesterNodeInfo,requesterPortIndex);
         apx_nodeInfo_clearRequirePortConnector(requesterNodeInfo,requesterPortIndex);
         if (requireConnector != 0)
         {
            //create provideConnector
            apx_nodeInfo_t *providerNodeInfo;
            providerNodeInfo = requireConnector->node->nodeInfo;
            providerPortIndex = requireConnector->port->portIndex;
            apx_portref_create(&provideConnector,requesterNodeInfo->node, (apx_port_t*) *adt_ary_get(&requesterNodeInfo->node->requirePortList, requesterPortIndex));
            //We cannot use the normal function apx_nodeInfo_disconnectProvidePort here, it will cause a recursive loop.
            //Instead we use the internal function apx_nodeInfo_disconnectProvidePortInternal.
            apx_nodeInfo_disconnectProvidePortInternal(providerNodeInfo,providerPortIndex,&provideConnector);
            apx_portref_delete(requireConnector);
            apx_portref_destroy(&provideConnector);
            //set event flags for later processing when we will update the data trigger tables
            requesterNodeInfo->requirePortFlags[requesterPortIndex] |= APX_PORT_EVENT_DISCONNECTED;
            providerNodeInfo->providePortFlags[providerPortIndex] |= APX_PORT_EVENT_DISCONNECTED;
            providerNodeInfo->pendingProvidePortFlags++;
            requesterNodeInfo->pendingRequirePortFlags++;
         }
         else
         {
            //this port had no connector
         }

      }
   }
}

adt_ary_t *apx_nodeInfo_getProvidePortConnectorList(apx_nodeInfo_t *self, int32_t providerPortIndex)
{
   if (self != 0)
   {
      int32_t numProvideConnectors = adt_ary_length(&self->provideConnectors);
      if ( (providerPortIndex >= 0) && (providerPortIndex < numProvideConnectors) )
      {
         return (adt_ary_t*) *adt_ary_get(&self->provideConnectors,providerPortIndex);
      }
   }
   return (adt_ary_t *) 0;
}

apx_portref_t *apx_nodeInfo_getRequirePortConnector(apx_nodeInfo_t *self, int32_t requesterPortIndex)
{
   if (self != 0)
   {
      int32_t numRequireConnectors = adt_ary_length(&self->requireConnectors);
      if ( (requesterPortIndex >= 0) && (requesterPortIndex < numRequireConnectors) )
      {
         return (apx_portref_t*) *adt_ary_get(&self->requireConnectors,requesterPortIndex);
      }
   }
   return (apx_portref_t*) 0;
}

void apx_nodeInfo_clearRequirePortConnector(apx_nodeInfo_t *self, int32_t requesterPortIndex)
{
   if (self != 0)
   {
      adt_ary_set(&self->requireConnectors,requesterPortIndex, (void*) 0);
   }
}

apx_node_t *apx_nodeInfo_getNode(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->node;
   }
   return 0;
}

int32_t apx_nodeInfo_getNumRequirePorts(apx_nodeInfo_t *self)
{
   if ( self != 0 )
   {
      if (self->node != 0)
      {
         return adt_ary_length(&self->node->requirePortList);
      }
   }
   else
   {
      errno=EINVAL; //only set errno if self==0
   }
   return -1;
}

int32_t apx_nodeInfo_getNumProvidePorts(apx_nodeInfo_t *self)
{
   if ( self != 0 )
   {
      if (self->node != 0)
      {
         return adt_ary_length(&self->node->providePortList);
      }
   }
   else
   {
      errno=EINVAL; //only set errno if self==0
   }
   return -1;
}

/**
 * recalculates data triggers for a specific port chosen by \param providePortIndex
 */
void apx_nodeInfo_updateDataTriggers(apx_nodeInfo_t *self, int32_t providePortIndex)
{
   if ( (self != 0) && (self->node != 0) )
   {
      int32_t numProvidePorts = adt_ary_length(&self->node->providePortList);
      if ( (providePortIndex>=0) && (providePortIndex < numProvidePorts) )
      {
         apx_port_t *port = apx_node_getProvidePort(self->node, providePortIndex);
         assert(port != 0);
         apx_dataTriggerTable_updateTrigger(&self->outDataTriggerTable,port);
      }
   }


}

apx_portDataMap_t *apx_nodeInfo_getOutDataMap(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return &self->outDataMap;
   }
   errno=EINVAL;
   return 0;
}

int32_t apx_nodeInfo_getInPortDataOffset(apx_nodeInfo_t *self, int32_t requirePortIndex)
{
   if ( self != 0)
   {
      apx_portDataMapEntry_t *entry = apx_portDataMap_getEntry(&self->inDataMap, requirePortIndex);
      if (entry != 0)
      {
         return entry->offset;
      }
   }
   errno=EINVAL;
   return -1;
}

int32_t apx_nodeInfo_getOutPortDataOffset(apx_nodeInfo_t *self, int32_t providePortIndex)
{
   if ( self != 0)
   {
      apx_portDataMapEntry_t *entry = apx_portDataMap_getEntry(&self->outDataMap, providePortIndex);
      if (entry != 0)
      {
         return entry->offset;
      }
   }
   errno=EINVAL;
   return -1;
}


int32_t apx_nodeInfo_getInPortDataLen(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->inPortDataLen;
   }
   return -1;
}

int32_t apx_nodeInfo_getOutPortDataLen(apx_nodeInfo_t *self)
{
   if (self != 0)
   {
      return self->outPortDataLen;
   }
   return -1;
}

/**
 * returns dataTriggerFunction based on offset
 */
apx_dataTriggerFunction_t *apx_nodeInfo_getTriggerFunction(apx_nodeInfo_t *self, int32_t offset)
{
   if ( (self != 0) )
   {
      return apx_dataTriggerTable_get(&self->outDataTriggerTable, offset);
   }
   errno=EINVAL;
   return (apx_dataTriggerFunction_t*) 0;
}

/**
 * for each require port in self, copy init data from the outDataBuf of connected provide port
 */
void apx_nodeInfo_copyInitDataFromProvideConnectors(apx_nodeInfo_t *self)
{
   if ( (self != 0) && (self->nodeData != 0) && (self->nodeData->inPortDataBuf != 0) )
   {
      int32_t numRequirePorts;
      int32_t requirePortIndex;
      apx_nodeData_t *requireNodeData = self->nodeData;

      numRequirePorts = apx_nodeInfo_getNumRequirePorts(self);
      for (requirePortIndex=0;requirePortIndex<numRequirePorts;requirePortIndex++)
      {

         apx_portref_t *portref = apx_nodeInfo_getRequirePortConnector(self, requirePortIndex);


         if (portref != 0)
         {
            apx_portDataMapEntry_t *requirePortEntry;
            apx_nodeInfo_t *provideNodeInfo = portref->node->nodeInfo;
            apx_nodeData_t *provideNodeData;
            int32_t providePortIndex;
            requirePortEntry = apx_portDataMap_getEntry(&self->inDataMap, requirePortIndex);
            assert(requirePortEntry != 0);
            providePortIndex = portref->port->portIndex;
            assert( (provideNodeInfo != 0) && (providePortIndex>=0) );
            provideNodeData = provideNodeInfo->nodeData;
            if ( (provideNodeData!=0) && (provideNodeData->outPortDataBuf != 0) )
            {
               apx_portDataMapEntry_t *providePortEntry;
               providePortEntry = apx_portDataMap_getEntry(&provideNodeInfo->outDataMap, providePortIndex);
               assert(providePortEntry != 0);
               //array bounds check
               if ( ( (uint32_t)providePortEntry->offset >= provideNodeData->outPortDataLen) ||  ( ((uint32_t)(providePortEntry->offset+providePortEntry->length)) > provideNodeData->outPortDataLen) )
               {
                  APX_LOG_ERROR("[APX_NODE_INFO] offset/length in providePortEntry for %s/%s is outside outPortDataLen", portref->node->name, portref->port->name);
               }
               else
               {
                  if ( ((uint32_t)requirePortEntry->offset >= requireNodeData->inPortDataLen) ||  ((uint32_t)(requirePortEntry->offset+requirePortEntry->length) > requireNodeData->inPortDataLen))
                  {
                     APX_LOG_ERROR("[APX_NODE_INFO] offset/length in requirePortEntry for %s/%s is outside inPortDataLen", self->node->name, requirePortEntry->port->name);
                  }
                  else
                  {
                     memcpy(&requireNodeData->inPortDataBuf[requirePortEntry->offset], &provideNodeData->outPortDataBuf[providePortEntry->offset], providePortEntry->length);
                  }
               }
            }
         }
      }
   }
}

void apx_nodeInfo_setNodeData(apx_nodeInfo_t *self, apx_nodeData_t *nodeData)
{
   if (self != 0)
   {
      self->nodeData = nodeData;
   }   
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * connect one of our require ports to another nodes' provide port
 */
static void apx_nodeInfo_connectRequirePortInternal(apx_nodeInfo_t *self, int32_t portIndex, apx_node_t *providerNode, apx_port_t *providerPort)
{
   if ( (self != 0) && (providerNode != 0) && (providerPort != 0) )
   {
      int32_t numPorts;
      int32_t numConnections;
      numPorts = adt_ary_length(&self->node->requirePortList);
      numConnections = adt_ary_length(&self->requireConnectors);
      if (numPorts != numConnections)
      {
         //node object is out of sync with requireConnections list
         adt_ary_resize(&self->requireConnectors,numPorts);
      }
      if ( (portIndex >= 0) && (portIndex<numPorts) )
      {
         apx_portref_t *ref = apx_portref_new(providerNode, providerPort);
         adt_ary_set(&self->requireConnectors, portIndex, ref);
      }
   }
}

/**
 * connect one of the provide ports on self to another nodes' require port
 */
static void apx_nodeInfo_connectProvidePortInternal(apx_nodeInfo_t *self, int32_t portIndex, apx_node_t *requesterNode, apx_port_t *requirePort)
{
   if ( (self != 0) && (requesterNode != 0) && (requirePort != 0) )
   {
      int32_t numPorts;
      int32_t numConnections;
      adt_ary_t *outerConnectionList = &self->provideConnectors;
      numPorts = adt_ary_length(&self->node->providePortList);
      numConnections = adt_ary_length(outerConnectionList);
      if (numPorts != numConnections)
      {
         //node object is out of sync with provideConnections list
         adt_ary_resize(outerConnectionList,numPorts);
      }
      if ( (portIndex >= 0) && (portIndex<numPorts) )
      {
         apx_portref_t *newConnection = apx_portref_new(requesterNode, requirePort);
         adt_ary_t *innerConnectionList;
         /*
          * For provide ports it gets a little trickier than for require ports.
          * Each provide port can provide its output value to many receivers (multiplicity 0..*).
          * In order to support this we need to allow multiple connections to reach out from each of our provide ports.
          * We implement this using a list of lists where each element in the outer list represents one provide port and
          * each inner list contains all the connectors reaching out from that provide port.
          *
          * self->provideConnections (outerConnectionList)
          * |
          * +--- provide port 1: adt_ary_t<apx_portref_t>
          * |                     |
          * |                     +--- connector 0 apx_portref_t(requesterNode1,requesterPort1)
          * |                     |
          * |                     +--- connector 1 apx_portref_t(requesterNode2,reqesterPort2)
          * |
          * +--- provide port 2: adt_ary_t<apx_portref_t>
            |                     |
          * |                     +--- connector 0 apx_portref_t(requesterNode3,requesterPort3)
          * |                     |
          * |                     +--- connector 1 apx_portref_t(requesterNode4,requesterPort4)
          * |
          * ...
          * +--- provide port n
          */

         //1. check for empty list
         innerConnectionList = (adt_ary_t*) *adt_ary_get(outerConnectionList,portIndex);
         if (innerConnectionList == 0)
         {
            //innerConnectionList does not exist for this port, create inner list and append new item to it
            innerConnectionList = adt_ary_new(apx_portref_vdelete);
            adt_ary_push(innerConnectionList, newConnection);
            adt_ary_set(outerConnectionList, portIndex,innerConnectionList);
         }
         else
         {
            //innerConnectionList already exists for this port, grab it and search for duplicates before appending
            int32_t end;
            int32_t i;
            end = adt_ary_length(innerConnectionList);
            for(i=0;i<end;i++)
            {
               apx_portref_t *ref = (apx_portref_t*) *adt_ary_get(innerConnectionList,i);
               if (apx_portref_equals(ref,newConnection) != 0)
               {
                  //identical connection found, don't add it again
                  return;
               }
            }
            //add new connection
            adt_ary_push(innerConnectionList, newConnection);
         }
      }
   }
}

/**
 * Clears connection point for a require port on a nodeInfo.
 * After the operation the connection point on the requesterPortIndex is NULL.
 */
static void apx_nodeInfo_disconnectRequirePortInternal(apx_nodeInfo_t *requesterNodeInfo, int32_t requesterPortIndex)
{
   if ( requesterNodeInfo != 0 )
   {
      int32_t numConnectionPoints;
      numConnectionPoints = adt_ary_length(&requesterNodeInfo->requireConnectors);
      if ( (requesterPortIndex >= 0) && (requesterPortIndex<numConnectionPoints) )
      {
         apx_portref_t *portref = (apx_portref_t*) *adt_ary_get(&requesterNodeInfo->requireConnectors, requesterPortIndex);
         //set pointer to NULL then delete the object
         adt_ary_set(&requesterNodeInfo->requireConnectors, requesterPortIndex, (void*) 0);
         apx_portref_delete(portref);
      }
   }
}

/**
 * Clears connection point for a provide port on a nodeInfo.
 * After the operation the connection point on the providerPortIndex is NULL.
 */
static void apx_nodeInfo_disconnectProvidePortInternal(apx_nodeInfo_t *providerNodeInfo, int32_t providerPortIndex, apx_portref_t *portref)
{
   if ( providerNodeInfo != 0 )
   {
      int32_t numConnectionPoints;
      numConnectionPoints = adt_ary_length(&providerNodeInfo->provideConnectors);
      if ( (providerPortIndex >= 0) && (providerPortIndex<numConnectionPoints) )
      {
         adt_ary_t *innerList = (adt_ary_t*) *adt_ary_get(&providerNodeInfo->provideConnectors, providerPortIndex);
         if (innerList != 0)
         {
            int32_t i;
            int32_t end;

            end = adt_ary_length(innerList);
            for (i=0;i<end;i++)
            {
               apx_portref_t *other = (apx_portref_t*) *adt_ary_get(innerList,i);
               if ( apx_portref_equals(portref,other) != 0)
               {
                  //found element
                  adt_ary_splice(innerList,i,1);
                  return; //cannot continue loop, we just modified the list. This is OK since duplicates are not allowed
               }
            }
         }
      }
   }
}

