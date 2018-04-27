#ifndef APX_PORT_TRIGGER_H
#define APX_PORT_TRIGGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "adt_bytearray.h"
#include "apx_portDataMap.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//forward declaration
struct apx_nodeInfo_tag;


typedef void (dataTriggerWriteHook_fn)(void *arg, struct apx_nodeInfo_tag *providerNodeInfo, struct apx_nodeInfo_tag *requesterNodeInfo, uint8_t *data, uint32_t srcOffset, uint32_t destOffset, uint32_t length);

typedef struct apx_dataWriteInfo_tag
{
   struct apx_nodeInfo_tag *requesterNodeInfo;
   uint32_t destOffset;       //byte offset into inDataBuffer of requesterNodeInfo
} apx_dataWriteInfo_t;

typedef struct apx_dataTriggerFunction_tag
{
   uint32_t srcOffset;        //offset in bytes to where the signal data actually starts (this is only used if partial signal update is supported)
   uint32_t dataLength;       //expected length of data (this is just for error checking)
   adt_ary_t writeInfoList;   //array of apx_dataWriteInfo_t
}apx_dataTriggerFunction_t;

typedef struct apx_dataTriggerTable_tag
{
   //Note: the lookupTable contains duplicate pointer objects, we cannot delete its items like we normally do using virtual destructor.

   apx_dataTriggerFunction_t  **lookupTable;     //array of apx_dataTriggerFunction_t*,
   uint32_t                   lookupTableLen;    //should be identical to number of bytes in the outPortMap (from corresponding NodeInfo)
   dataTriggerWriteHook_fn    *writeHookFunc;
   void                       *writeHookUserArg;
   struct apx_nodeInfo_tag    *nodeInfo;         //The nodeInfo where this triggertable is attached to
}apx_dataTriggerTable_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//dataTriggerTable
int8_t apx_dataTriggerTable_create(apx_dataTriggerTable_t *self, struct apx_nodeInfo_tag *nodeInfo, dataTriggerWriteHook_fn *writeHookFunc, void *writeHookUserArg);
void apx_dataTriggerTable_destroy(apx_dataTriggerTable_t *self);
apx_dataTriggerTable_t *apx_dataTriggerTable_new(struct apx_nodeInfo_tag *nodeInfo, dataTriggerWriteHook_fn *writeHookFunc, void *writeHookUserArg);
void apx_dataTriggerTable_delete(apx_dataTriggerTable_t *self);
void apx_dataTriggerTable_vdelete(void *arg);
void apx_dataTriggerTable_updateTrigger(apx_dataTriggerTable_t *self, apx_port_t *port);
apx_dataTriggerFunction_t *apx_dataTriggerTable_get(const apx_dataTriggerTable_t *self, int32_t offset);

//apx_dataTriggerFunction
void apx_dataTriggerFunction_create(apx_dataTriggerFunction_t *self, uint32_t srcOffset, uint32_t dataLength);
void apx_dataTriggerFunction_destroy(apx_dataTriggerFunction_t *self);
apx_dataTriggerFunction_t *apx_dataTriggerFunction_new(uint32_t srcOffset, uint32_t dataLength);
void apx_dataTriggerFunction_delete(apx_dataTriggerFunction_t *self);
void apx_dataTriggerFunction_vdelete(void *arg);

//dataWriteInfo
void apx_dataWriteInfo_create(apx_dataWriteInfo_t *self,struct apx_nodeInfo_tag *nodeInfo, uint32_t destOffset);
void apx_dataWriteInfo_destroy(apx_dataWriteInfo_t *self);
apx_dataWriteInfo_t *apx_dataWriteInfo_new(struct apx_nodeInfo_tag *nodeInfo, uint32_t destOffset);
void apx_dataWriteInfo_delete(apx_dataWriteInfo_t *self);
void apx_dataWriteInfo_vdelete(void *arg);

#endif //APX_PORT_TRIGGER_H
