//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "adt_str.h"
#include "apx_router.h"
#include "apx_logging.h"
#include "apx_fileManager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_router_attachPortToPortMap(apx_router_t *self, apx_node_t *node, apx_port_t *port);
static void apx_router_detachPortFromPortMap(apx_router_t *self, apx_node_t *node, apx_port_t *port);
static bool apx_router_createDefaultPortConnector(apx_router_t *self, apx_nodeInfo_t *nodeInfo, apx_port_t *port, apx_portref_t *provideConnector);
static void apx_router_build_requireRefs(apx_nodeInfo_t *nodeInfo, adt_ary_t *requireRefs);
static void apx_router_postProcessNodes(apx_router_t *self, apx_nodeInfo_t *detachedNodeInfo);
static void apx_router_postProcessNode(apx_nodeInfo_t *extraNodeInfo, int8_t debugMode);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * constructor
 */
void apx_router_create(apx_router_t *self)
{
   if ( (self != 0) )
   {
      adt_ary_create(&self->nodeInfoList, (void(*)(void*)) 0); //weak references to apx_nodeInfo_t.
      adt_hash_create(&self->portMap, apx_routerPortMapEntry_vdelete); //hash where key is the port signature (string) and value is apx_routerPortMapEntry_t
      self->debugMode = APX_DEBUG_NONE;
   }
}

/**
 * destructor
 */
void apx_router_destroy(apx_router_t *self)
{
   if ( self != 0)
   {
      adt_ary_destroy(&self->nodeInfoList);
      adt_hash_destroy(&self->portMap);
   }
}

/**
 * attaches a nodeInfo structure to this router
 */
void apx_router_attachNodeInfo(apx_router_t *self, apx_nodeInfo_t *nodeInfo)
{
   if ( (self != 0) && (nodeInfo != 0) )
   {
      int32_t i;
      int32_t requirePortLen;
      int32_t providePortLen;
      int32_t nodeInfoListLen;
      char debugInfoStr[APX_DEBUG_INFO_MAX_LEN];

      apx_node_t *node = nodeInfo->node;

      debugInfoStr[0]=0;
      if (nodeInfo->nodeData->fileManager->debugInfo != 0)
      {
         sprintf(debugInfoStr, " (%p)", nodeInfo->nodeData->fileManager->debugInfo);
      }

      assert(node != 0);


      APX_LOG_DEBUG("[APX_ROUTER]%s Attaching %s",debugInfoStr, node->name);
      nodeInfoListLen = adt_ary_length(&self->nodeInfoList);
      requirePortLen = adt_ary_length(&node->requirePortList);
      providePortLen = adt_ary_length(&node->providePortList);
      //1. Is the node already attached?
      for (i=0;i<nodeInfoListLen;i++)
      {
         apx_nodeInfo_t *elem = (apx_nodeInfo_t*) *adt_ary_get(&self->nodeInfoList,i);
         if (elem == nodeInfo)
         {
            //node already attached, ignore request
            APX_LOG_WARNING("[APX_ROUTER]%s Node with name %s is already attached",debugInfoStr, node->name);
            return;
         }
      }

      //This is a new node.
      //2. add this nodeInfo to the nodeInfoList
      adt_ary_push(&self->nodeInfoList,nodeInfo);

      //3. register all require ports into the portMap
      for (i=0;i<requirePortLen;i++)
      {
         apx_port_t *port = apx_node_getRequirePort(node,i);
         apx_router_attachPortToPortMap(self,node,port);
      }
      //4. register all provide ports into the portMap
      for (i=0;i<providePortLen;i++)
      {
         apx_port_t *port = apx_node_getProvidePort(node,i);
         apx_router_attachPortToPortMap(self,node,port);
      }
      //5. create connectors using default connection rules (latest attached node is provider of a signal)
      for (i=0;i<requirePortLen;i++)
      {
         apx_port_t *port = apx_node_getRequirePort(node,i);
         apx_router_createDefaultPortConnector(self,nodeInfo,port,0);
      }
      for (i=0;i<providePortLen;i++)
      {
         apx_port_t *port = apx_node_getProvidePort(node,i);
         apx_router_createDefaultPortConnector(self,nodeInfo,port,0);
      }
      //6. loop through all nodes and check for flags (flags indicate extra postprocessing steps are required)
      apx_router_postProcessNodes(self,0);
   }
}

/**
 * detaches a nodeInfo structure from the router
 */
void apx_router_detachNodeInfo(apx_router_t *self, apx_nodeInfo_t *nodeInfo)
{
   if ( (self != 0) && (nodeInfo != 0) )
   {
      int8_t found=0;
      int32_t i;
      int32_t requirePortLen;
      int32_t providePortLen;
      int32_t nodeInfoListLen;
      int32_t numRequireRefs;
      adt_ary_t requireRefs; //array of apx_portref_t*
      apx_node_t *node = nodeInfo->node;
      char debugInfoStr[APX_DEBUG_INFO_MAX_LEN];
      assert(node != 0);

      debugInfoStr[0]=0;
      if (nodeInfo->nodeData->fileManager->debugInfo != 0)
      {
         sprintf(debugInfoStr, " (%p)", nodeInfo->nodeData->fileManager->debugInfo);
      }

      APX_LOG_DEBUG("[APX_ROUTER]%s Detaching %s", debugInfoStr, node->name);
      nodeInfoListLen = adt_ary_length(&self->nodeInfoList);
      requirePortLen = adt_ary_length(&node->requirePortList);
      providePortLen = adt_ary_length(&node->providePortList);
      for (i=0;i<nodeInfoListLen;i++)
      {
         apx_nodeInfo_t *elem = (apx_nodeInfo_t*) *adt_ary_get(&self->nodeInfoList,i);
         if (elem == nodeInfo)
         {
            found = 1;
            //found the node, remove it from the list
            adt_ary_splice(&self->nodeInfoList,i,1);
            break; //important to break for-loop here since splice modifies the list (invalidating the nodeInfoListLen variable)
         }
      }

      if (found == 0)
      {
         return; //user tried to detach a node that wasn't attached in the first place
      }

      //1. Detach all ports from the portMap
      for (i=0;i<requirePortLen;i++)
      {
         apx_port_t *port = apx_node_getRequirePort(node,i);
         apx_router_detachPortFromPortMap(self,node,port);
      }
      for (i=0;i<providePortLen;i++)
      {
         apx_port_t *port = apx_node_getProvidePort(node,i);
         apx_router_detachPortFromPortMap(self,node,port);
      }

      //2. follow all connectors reaching out from this node and determine what other nodes will be affected by this delete
      adt_ary_create(&requireRefs,apx_portref_vdelete);

      apx_router_build_requireRefs(nodeInfo,&requireRefs);

      //3. disconnect require and provide ports
      for (i=0;i<requirePortLen;i++)
      {
         apx_nodeInfo_disconnectRequirePort(nodeInfo,i);
      }
      for (i=0;i<providePortLen;i++)
      {
         apx_nodeInfo_disconnectProvidePort(nodeInfo,i);
      }

      //4. for the deleted connectors, try to reroute using default rule
      numRequireRefs = adt_ary_length(&requireRefs);
      for (i=0;i<numRequireRefs;i++)
      {
         apx_nodeInfo_t *requesterNodeInfo; //this is the node that requested the signal this node provided
         apx_portref_t *requireConnector;
         apx_portref_t *portref = (apx_portref_t*) *adt_ary_get(&requireRefs,i);
         requesterNodeInfo = portref->node->nodeInfo;
         requireConnector = apx_nodeInfo_getRequirePortConnector(requesterNodeInfo, portref->port->portIndex);
         assert(requireConnector == 0);
         //This is now an empty connector due to the fact that our detached nodeInfo was the provider of that signal.
         //Try to reroute the signal from a different source
         (void)apx_router_createDefaultPortConnector(self,requesterNodeInfo,portref->port,0);
      }
      adt_ary_destroy(&requireRefs);
      apx_router_postProcessNodes(self,nodeInfo);
   }
}

void apx_router_setDebugMode(apx_router_t *self, int8_t debugMode)
{
   if (self != 0)
   {
      self->debugMode = debugMode;
   }
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_router_attachPortToPortMap(apx_router_t *self, apx_node_t *node, apx_port_t *port)
{
   if ( (self != 0) && (node != 0) && (port != 0) )
   {
      apx_routerPortMapEntry_t *portMapEntry = (apx_routerPortMapEntry_t*) 0;
      void **ptr;
      const char *psg = apx_port_getPortSignature(port);
      ptr = adt_hash_get(&self->portMap,psg,0);
      if (ptr == 0)
      {
         //no entry, create new entry
         portMapEntry = apx_routerPortMapEntry_new();
         adt_hash_set(&self->portMap,psg,0,portMapEntry);
      }
      else
      {
         portMapEntry = (apx_routerPortMapEntry_t*) *ptr;
      }
      if (portMapEntry != 0)
      {
         apx_routerPortMapEntry_insertPort(portMapEntry,node,port);
      }
   }
}

static void apx_router_detachPortFromPortMap(apx_router_t *self, apx_node_t *node, apx_port_t *port)
{
   if ( (self != 0) && (node != 0) && (port != 0) )
   {
      apx_routerPortMapEntry_t *portMapEntry = (apx_routerPortMapEntry_t*) 0;
      void **ptr;
      const char *psg = apx_port_getPortSignature(port);
      ptr = adt_hash_get(&self->portMap,psg,0);
      if (ptr == 0)
      {
         //no entry, this is a weird situation
      }
      else
      {
         portMapEntry = (apx_routerPortMapEntry_t*) *ptr;
      }
      if (portMapEntry != 0)
      {
         apx_routerPortMapEntry_removePort(portMapEntry,node,port);
      }
   }
}

/**
 * use the apx_routerPortMap to try and complete the connector for this port
 */
static bool apx_router_createDefaultPortConnector(apx_router_t *self, apx_nodeInfo_t *nodeInfo, apx_port_t *port, apx_portref_t *provideConnector)
{
   if ( (self != 0) && (nodeInfo != 0) && (port != 0) )
   {
      apx_routerPortMapEntry_t *portMapEntry = (apx_routerPortMapEntry_t*) 0;
      //char debugInfoStr[APX_DEBUG_INFO_MAX_LEN];
      void **ptr;
      const char *psg = apx_port_getPortSignature(port);
      //debugInfoStr[0]=0;
/*      if (nodeInfo->nodeData->fileManager->debugInfo != 0)
      {
         sprintf(debugInfoStr, " (%p)", nodeInfo->nodeData->fileManager->debugInfo);
      }*/
      ptr = adt_hash_get(&self->portMap,psg,0);
      if (ptr == 0)
      {
         //no entry, this is a weird situation
      }
      else
      {
         portMapEntry = (apx_routerPortMapEntry_t*) *ptr;


         if (port->portType == APX_REQUIRE_PORT)
         {
            //if port is a require port, try to find a matching provide port.
            int32_t numProvidePorts;
            numProvidePorts = adt_ary_length(&portMapEntry->providePorts);
            if (numProvidePorts > 0)
            {
               //OK, there is at least one provider for this signal.
               //The default rule is to connect to the last of the available providers (index = numProvidePorts-1)
               apx_portref_t *portref = apx_routerPortMapEntry_getProvidePortById(portMapEntry,numProvidePorts-1);
               if ( (portref != 0) && (portref->node != 0) && (portref->port != 0) )
               {
                  apx_nodeInfo_t *providerNodeInfo = portref->node->nodeInfo;

                  if (providerNodeInfo != 0)
                  {
                     if (self->debugMode > APX_DEBUG_NONE)
                     {
                        int32_t requirePortOffset;
                        int32_t providePortOffset;
                        requirePortOffset = apx_nodeInfo_getInPortDataOffset(nodeInfo, port->portIndex);
                        providePortOffset = apx_nodeInfo_getOutPortDataOffset(providerNodeInfo, portref->port->portIndex);
                        APX_LOG_DEBUG("   %s/%s[%d] -> %s/%s[%d]", portref->node->name, portref->port->name, providePortOffset,
                              nodeInfo->node->name,port->name, requirePortOffset);
                     }
                     apx_nodeInfo_connectPort(providerNodeInfo, portref->port->portIndex, nodeInfo, port->portIndex);
                     if (provideConnector != 0)
                     {
                        provideConnector->node=portref->node;
                        provideConnector->port=portref->port;
                     }
                     return true;
                  }
               }
            }
         }
         else if(port->portType == APX_PROVIDE_PORT)
         {
            //if port is a provide port, try to find all require ports and try to connect them to this new provider port
            int32_t numRequirePorts;
            int32_t i;
            numRequirePorts = adt_ary_length(&portMapEntry->requirePorts);
            for (i=0;i<numRequirePorts;i++)
            {
               apx_portref_t *portref = apx_routerPortMapEntry_getRequirePortById(portMapEntry,i);
               if ( (portref != 0) && (portref->node != 0) && (portref->port != 0) )
               {
                  apx_portref_t *requireConnector;
                  int32_t requirePortIndex;
                  apx_nodeInfo_t *requireNodeInfo = portref->node->nodeInfo;
                  requirePortIndex = portref->port->portIndex;
                  //Ok, now we have the nodeInfo and the portIndex of the requester node, check to see if this port is unconnected
                  requireConnector = apx_nodeInfo_getRequirePortConnector(requireNodeInfo,requirePortIndex);
                  if (requireConnector != 0)
                  {
                     //the require port already has a connection, the default rule is to override the requireConnector
                     //and reroute it to our new provide port.
                     apx_nodeInfo_disconnectRequirePort(requireNodeInfo,requireConnector->port->portIndex);
                  }
                  if (self->debugMode > APX_DEBUG_NONE)
                  {
                     int32_t requirePortOffset;
                     int32_t providePortOffset;
                     requirePortOffset = apx_nodeInfo_getInPortDataOffset(requireNodeInfo, requirePortIndex);
                     providePortOffset = apx_nodeInfo_getOutPortDataOffset(nodeInfo, port->portIndex);
                     APX_LOG_DEBUG("   %s/%s[%d] -> %s/%s[%d]",nodeInfo->node->name, port->name, providePortOffset,
                           portref->node->name,portref->port->name, requirePortOffset);
                  }
                  apx_nodeInfo_connectPort(nodeInfo, port->portIndex, portref->node->nodeInfo, portref->port->portIndex);
               }
            }
            return true;
         }
         else
         {
            //MISRA
         }
      }
   }
   return false;
}



static void apx_router_build_requireRefs(apx_nodeInfo_t *nodeInfo, adt_ary_t *requireRefs)
{
   int32_t providePortLen;
   int32_t i;


   providePortLen = adt_ary_length(&nodeInfo->node->providePortList);

   for (i=0;i<providePortLen;i++)
   {
      adt_ary_t *connectorList = apx_nodeInfo_getProvidePortConnectorList(nodeInfo,i);
      if (connectorList != 0)
      {
         int32_t j;
         int32_t numConnectors = adt_ary_length(connectorList);
         for (j=0;j<numConnectors;j++)
         {
            apx_portref_t *connector = (apx_portref_t*) *adt_ary_get(connectorList,j);
            if (connector != 0)
            {
               apx_portref_t *portref = apx_portref_new(connector->node,connector->port);
               if (portref != 0)
               {
                  adt_ary_push(requireRefs,(void*) portref);
               }
            }
         }
      }
   }
}

static void apx_router_postProcessNodes(apx_router_t *self, apx_nodeInfo_t *detachedNodeInfo)
{
   int32_t numNodes;
   int32_t i;
   numNodes = adt_ary_length(&self->nodeInfoList);
   for (i=0;i<numNodes;i++)
   {
      apx_nodeInfo_t *nodeInfo = (apx_nodeInfo_t*) *adt_ary_get(&self->nodeInfoList,i);
      assert(nodeInfo != 0);
      apx_router_postProcessNode(nodeInfo, self->debugMode);
   }
   if (detachedNodeInfo != 0) //detachedNodeInfo is the nodeInfo that is no longer in self->nodeInfoList
   {
      apx_router_postProcessNode(detachedNodeInfo, self->debugMode);
   }
}

static void apx_router_postProcessNode(apx_nodeInfo_t *nodeInfo, int8_t debugMode)
{
   if (nodeInfo->pendingProvidePortFlags>0)
   {
      int32_t i;
      int32_t numProvidePorts = adt_ary_length(&nodeInfo->node->providePortList);

      for(i=0;i<numProvidePorts;i++)
      {
         if (nodeInfo->providePortFlags[i] != 0)
         {
            adt_ary_t *connectorList;
            apx_port_t *port = apx_node_getProvidePort(nodeInfo->node,i);
            assert(port != 0);
            if (debugMode == APX_DEBUG_NONE)
            {
               //1. generate debug printout describing the change in connection status
               connectorList = apx_nodeInfo_getProvidePortConnectorList(nodeInfo,i);
               if (connectorList != 0)
               {
                  int32_t numConnectors;
                  numConnectors = adt_ary_length(connectorList);
                  if (numConnectors > 0)
                  {
                     adt_str_t *str;
                     int32_t j;
                     bool first=true;
                     str = adt_str_new();
                     for(j=0;j<numConnectors;j++)
                     {
                        apx_portref_t *portref;
                        if (first)
                        {
                           first=false;
                        }
                        else
                        {
                           adt_str_append_cstr(str,", ");
                        }
                        portref = (apx_portref_t*) *adt_ary_get(connectorList,j);
                        assert(portref != 0);
                        adt_str_append_cstr(str,portref->node->name);
                        adt_str_push(str,'/');
                        adt_str_append_cstr(str,portref->port->name);
                     }
                     APX_LOG_DEBUG("   %s/%s -> %s",nodeInfo->node->name, port->name, adt_str_cstr(str));
                     adt_str_delete(str);
                  }
                  else
                  {
                     APX_LOG_DEBUG("   %s/%s -> (null)",nodeInfo->node->name, port->name);
                  }
               }
            }
            //2. recalculate data triggers for this port
            apx_nodeInfo_updateDataTriggers(nodeInfo,i);

            //3. decrease counter, when it reaches zero we have processed all flags.
            if ( (--nodeInfo->pendingProvidePortFlags) == 0)
            {
               break; //processed all pending flags
            }
         }
      }
      //clear flags
      memset(nodeInfo->providePortFlags,0,numProvidePorts);
   }
   if (nodeInfo->pendingRequirePortFlags>0)
   {
      int32_t i;
      int32_t numRequirePorts = adt_ary_length(&nodeInfo->node->requirePortList);

      for(i=0;i<numRequirePorts;i++)
      {
         if (nodeInfo->requirePortFlags[i] != 0)
         {
            //1. generate debug printout describing the change in connection status
            if (debugMode == APX_DEBUG_NONE)
            {
               apx_portref_t *portref;
               apx_port_t *port = apx_node_getRequirePort(nodeInfo->node,i);
               assert(port != 0);
               portref = apx_nodeInfo_getRequirePortConnector(nodeInfo,i);
               if (portref == 0)
               {
                  APX_LOG_DEBUG("   (null) -> %s/%s",nodeInfo->node->name, port->name);
               }
               else
               {
                  //APX_LOG_DEBUG("%s/%s -> %s/%s",portref->node->name, portref->port->name, nodeInfo->node->name, port->name);
               }
            }
            //2. decrease counter, when it reaches zero we have processed all flags.
            if ( (--nodeInfo->pendingRequirePortFlags) == 0)
            {
               break; //processed all pending flags
            }
         }
      }
      //clear flags
      memset(nodeInfo->requirePortFlags,0,numRequirePorts);
   }
}
