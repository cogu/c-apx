#ifndef APX_NODE_INFO_H
#define APX_NODE_INFO_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_node.h"
#include "apx_port.h"
#include "apx_portref.h"
#include "apx_portDataBuffer.h"
#include "adt_ary.h"
#include "apx_portDataMap.h"
#include "apx_dataTrigger.h"
#include "apx_nodeData.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//forward declarations
struct apx_nodeData_tag;

typedef struct apx_nodeInfo_tag
{
   apx_node_t *node; //weak/strong pointer to associated node
   bool isWeakRef_node; //selects the weak/strong property of node
   adt_ary_t requireConnectors; //array of strong references to apx_portref_t elements
   adt_ary_t provideConnectors; //array of strong references to adt_ary_t elements, where each element in turn is an array of apx_portref_t elements
   apx_portDataMap_t inDataMap; //describes the length and offset of each of the input/require ports
   apx_portDataMap_t outDataMap; //describes the length and offset of each of the output/provide ports
   int32_t inPortDataLen; //number of bytes needed to create nodeData->inPortDataBuf
   int32_t outPortDataLen; //number of bytes needed to create nodeData->outPortDataBuf
   uint8_t *requirePortFlags; //internal flags for require ports (used for dirty flags when ports are connected/disconnected) (one byte per port)
   uint8_t *providePortFlags; //internal flags for provide ports (used for dirty flags when port are connected/disconnected) (one byte per port)
   uint32_t pendingRequirePortFlags; //number of modified requirePortFlags since last check (this is an optimization to reduce some linear search time)
   uint32_t pendingProvidePortFlags; //number of modified providePortFlags since last check (this is an optimization to reduce some linear search time)
   apx_dataTriggerTable_t outDataTriggerTable; //trigger table routines
   struct apx_nodeData_tag *nodeData; //weak pointer to associated nodeData
} apx_nodeInfo_t;

#define APX_PORT_EVENT_NONE         0
#define APX_PORT_EVENT_CONNECTED    1
#define APX_PORT_EVENT_DISCONNECTED 2

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeInfo_create(apx_nodeInfo_t *self, apx_node_t *node);
void apx_nodeInfo_destroy(apx_nodeInfo_t *self);
apx_nodeInfo_t *apx_nodeInfo_new(apx_node_t *node);
void apx_nodeInfo_delete(apx_nodeInfo_t *self);
void apx_nodeInfo_vdelete(void *arg);

void apx_nodeInfo_connectPort(apx_nodeInfo_t *providerNodeInfo, int32_t providerPortIndex, apx_nodeInfo_t *requesterNodeInfo, int32_t requesterPortIndex);
void apx_nodeInfo_disconnectProvidePort(apx_nodeInfo_t *providerNodeInfo, int32_t providerPortIndex);
void apx_nodeInfo_disconnectRequirePort(apx_nodeInfo_t *requesterNodeInfo, int32_t requesterPortIndex);

adt_ary_t *apx_nodeInfo_getProvidePortConnectorList(apx_nodeInfo_t *self, int32_t providerPortIndex);
apx_portref_t *apx_nodeInfo_getRequirePortConnector(apx_nodeInfo_t *self, int32_t requesterPortIndex);
void apx_nodeInfo_clearRequirePortConnector(apx_nodeInfo_t *self, int32_t requesterPortIndex);
apx_node_t *apx_nodeInfo_getNode(apx_nodeInfo_t *self);
int32_t apx_nodeInfo_getNumRequirePorts(apx_nodeInfo_t *self);
int32_t apx_nodeInfo_getNumProvidePorts(apx_nodeInfo_t *self);
void apx_nodeInfo_updateDataTriggers(apx_nodeInfo_t *self,int32_t providePortIndex);
apx_portDataMap_t *apx_nodeInfo_getOutDataMap(apx_nodeInfo_t *self);
int32_t apx_nodeInfo_getInPortDataLen(apx_nodeInfo_t *self);
int32_t apx_nodeInfo_getOutPortDataLen(apx_nodeInfo_t *self);
apx_dataTriggerFunction_t *apx_nodeInfo_getTriggerFunction(apx_nodeInfo_t *self, int32_t offset);
void apx_nodeInfo_copyInitDataFromProvideConnectors(apx_nodeInfo_t *self);
#endif //APX_NODE_INFO_H
