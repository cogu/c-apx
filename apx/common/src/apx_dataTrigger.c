//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "apx_dataTrigger.h"
#include "apx_nodeInfo.h"
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
//static void apx_dataTriggerTable_build(apx_dataTriggerTable_t *self);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

//dataTriggerTable
int8_t apx_dataTriggerTable_create(apx_dataTriggerTable_t *self, apx_nodeInfo_t *nodeInfo, dataTriggerWriteHook_fn *writeHookFunc, void *writeHookUserArg)
{
   if ( (self != 0) && (nodeInfo != 0) )
   {
      int32_t numProvidePorts;
      self->lookupTable = 0;
      self->nodeInfo=nodeInfo;
      self->writeHookFunc=writeHookFunc;
      self->writeHookUserArg=writeHookUserArg;
      numProvidePorts = apx_nodeInfo_getNumProvidePorts(nodeInfo);
      if (numProvidePorts > 0)
      {
         apx_portDataMap_t *outDataMap = apx_nodeInfo_getOutDataMap(nodeInfo);
         if (outDataMap != 0)
         {
            size_t dataSize;
            self->lookupTableLen = (uint32_t) outDataMap->totalLen; //lookupTableLen = number of bytes in the outDataMap
            dataSize=sizeof(apx_dataTriggerFunction_t*) * self->lookupTableLen;
            //allocate lookupTableLen*sizeof(pointer), i.e. one pointer for each byte in the outDataMap.
            //This is where we trade memory for speed. We get constant lookup time, O(1) by using a little extra RAM.
            self->lookupTable = (apx_dataTriggerFunction_t**) malloc(dataSize);
            if (self->lookupTable == 0)
            {
               APX_LOG_ERROR("[APX_DATA_TRIGGER] apx_dataTriggerTable_create: malloc failed");
               errno=ENOMEM;
               return -1;
            }
            else
            {
               memset(self->lookupTable,0,dataSize);
            }
         }
      }
      return 0;
   }
   errno=EINVAL;
   return -1;
}

void apx_dataTriggerTable_destroy(apx_dataTriggerTable_t *self)
{
   if (self != 0)
   {
      if (self->lookupTable != 0)
      {
         apx_dataTriggerFunction_t **pBegin;
         apx_dataTriggerFunction_t **pNext;
         apx_dataTriggerFunction_t **pEnd;

         pBegin = self->lookupTable;
         pEnd = pBegin + self->lookupTableLen;
         for (pNext=pBegin;pNext<pEnd;pNext++)
         {
            if (*pNext != 0)
            {
               apx_dataTriggerFunction_delete(*pNext);
            }
         }
         free(self->lookupTable);
      }
   }
}

apx_dataTriggerTable_t *apx_dataTriggerTable_new(apx_nodeInfo_t *nodeInfo, dataTriggerWriteHook_fn *writeHookFunc, void *writeHookUserArg)
{
   if (nodeInfo != 0)
   {
      apx_dataTriggerTable_t *self = (apx_dataTriggerTable_t*) malloc(sizeof(apx_dataTriggerTable_t));
      if(self != 0){
         int8_t result = apx_dataTriggerTable_create(self,nodeInfo, writeHookFunc, writeHookUserArg);
         if (result != 0)
         {
            free(self);
            self=0;
         }
      }
      else{
         errno = ENOMEM;
      }
      return self;
   }
   return (apx_dataTriggerTable_t*) 0;
}

void apx_dataTriggerTable_delete(apx_dataTriggerTable_t *self)
{
   if (self != 0)
   {
      apx_dataTriggerTable_destroy(self);
      free(self);
   }
}

void apx_dataTriggerTable_vdelete(void *arg)
{
   apx_dataTriggerTable_delete( (apx_dataTriggerTable_t*) arg);
}

void apx_dataTriggerTable_updateTrigger(apx_dataTriggerTable_t *self, apx_port_t *port)
{
   if ( (self != 0) && (port != 0) )
   {
      apx_nodeInfo_t *nodeInfo = self->nodeInfo;
      assert(nodeInfo != 0);
      if (port->portType == APX_REQUIRE_PORT)
      {
         //TODO: implement later?
      }
      else if (port->portType == APX_PROVIDE_PORT)
      {
         apx_portDataMapEntry_t *dataMapEntry;
         apx_dataTriggerFunction_t *triggerFunction;
         adt_ary_t *connectorList = apx_nodeInfo_getProvidePortConnectorList(nodeInfo, port->portIndex);

         dataMapEntry = apx_portDataMap_getEntry(&nodeInfo->outDataMap, port->portIndex);
         assert(dataMapEntry != 0);
         assert(dataMapEntry->offset < (int32_t)self->lookupTableLen);
         triggerFunction = self->lookupTable[dataMapEntry->offset];
         if (triggerFunction == 0)
         {
            triggerFunction = apx_dataTriggerFunction_new(dataMapEntry->offset, dataMapEntry->length);
            if (triggerFunction != 0)
            {
               self->lookupTable[dataMapEntry->offset] = triggerFunction;
            }
            else
            {
               APX_LOG_ERROR("[APX_DATA_TRIGGER] apx_dataTriggerFunction_new returned NULL");
            }
         }
         assert(triggerFunction != 0);
         adt_ary_clear(&triggerFunction->writeInfoList);
         if (connectorList != 0)
         {
            int32_t i;
            int32_t numConnectors = adt_ary_length(connectorList);
            for (i=0;i<numConnectors;i++)
            {
               apx_nodeInfo_t *requesterNodeInfo;
               int32_t requesterPortIndex;
               apx_dataWriteInfo_t *writeInfo;
               apx_portDataMapEntry_t *requesterDataMapEntry;
               apx_portref_t *portref = (apx_portref_t*) *adt_ary_get(connectorList, i);
               requesterNodeInfo = portref->node->nodeInfo;
               requesterPortIndex =portref->port->portIndex;
               requesterDataMapEntry = apx_portDataMap_getEntry(&requesterNodeInfo->inDataMap, requesterPortIndex);
               writeInfo = apx_dataWriteInfo_new(requesterNodeInfo, requesterDataMapEntry->offset);
               if (writeInfo != 0)
               {
                  adt_ary_push(&triggerFunction->writeInfoList,writeInfo);
               }
            }
         }
      }
   }
}

apx_dataTriggerFunction_t *apx_dataTriggerTable_get(apx_dataTriggerTable_t *self, int32_t offset)
{
   if ( (self != 0) && (offset>=0) && ( ((uint32_t)offset) < (self->lookupTableLen) ) )
   {
      return self->lookupTable[offset];
   }
   errno=EINVAL;
   return (apx_dataTriggerFunction_t*) 0;
}

//dataWriteInfo
void apx_dataWriteInfo_create(apx_dataWriteInfo_t *self,apx_nodeInfo_t *nodeInfo, uint32_t destOffset)
{
   if ( (self != 0) && (nodeInfo != 0) )
   {
      self->requesterNodeInfo=nodeInfo;
      self->destOffset = destOffset;
   }
}

void apx_dataWriteInfo_destroy(apx_dataWriteInfo_t *self)
{
   //nothing to do
}

apx_dataWriteInfo_t *apx_dataWriteInfo_new(apx_nodeInfo_t *nodeInfo, uint32_t destOffset)
{
   if (nodeInfo != 0 )
   {
      apx_dataWriteInfo_t *self = (apx_dataWriteInfo_t*) malloc(sizeof(apx_dataWriteInfo_t));
      if(self != 0){
         apx_dataWriteInfo_create(self,nodeInfo, destOffset);
      }
      else{
         errno = ENOMEM;
      }
      return self;
   }
   return (apx_dataWriteInfo_t*) 0;
}

void apx_dataWriteInfo_delete(apx_dataWriteInfo_t *self)
{
   if (self != 0)
   {
      apx_dataWriteInfo_destroy(self);
      free(self);
   }
}

void apx_dataWriteInfo_vdelete(void *arg)
{
   apx_dataWriteInfo_delete( (apx_dataWriteInfo_t*) arg);
}

//apx_dataTriggerFunction
void apx_dataTriggerFunction_create(apx_dataTriggerFunction_t *self, uint32_t srcOffset, uint32_t dataLength)
{
   if(self != 0)
   {
      self->srcOffset=srcOffset;
      self->dataLength=dataLength;
      adt_ary_create(&self->writeInfoList,apx_dataWriteInfo_vdelete);
   }
}

void apx_dataTriggerFunction_destroy(apx_dataTriggerFunction_t *self)
{
   if (self != 0)
   {
      adt_ary_destroy(&self->writeInfoList);
   }
}

apx_dataTriggerFunction_t *apx_dataTriggerFunction_new(uint32_t srcOffset, uint32_t dataLength)
{
   apx_dataTriggerFunction_t *self = (apx_dataTriggerFunction_t*) malloc(sizeof(apx_dataTriggerFunction_t));
   if(self != 0)
   {
      apx_dataTriggerFunction_create(self,srcOffset, dataLength);
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_dataTriggerFunction_delete(apx_dataTriggerFunction_t *self)
{
   if (self != 0)
   {
      apx_dataTriggerFunction_destroy(self);
      free(self);
   }
}

void apx_dataTriggerFunction_vdelete(void *arg)
{
   apx_dataTriggerFunction_delete((apx_dataTriggerFunction_t*) arg);
}



//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

