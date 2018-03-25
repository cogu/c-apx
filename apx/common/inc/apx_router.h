#ifndef APX_ROUTER_H
#define APX_ROUTER_H
#include <stdint.h>
#include "apx_port.h"
#include "apx_node.h"
#include "apx_nodeInfo.h"
#include "adt_ary.h"
#include "adt_hash.h"
#include "apx_routerPortMapEntry.h"

typedef struct apx_router_tag
{
   adt_ary_t nodeInfoList; //list of apx_nodeInto_t
   adt_hash_t portMap; //hash of apx_routerPortMapEntry_t
   int8_t debugMode;
}apx_router_t;

/***************** Public Function Declarations *******************/
void apx_router_create(apx_router_t *self);
void apx_router_destroy(apx_router_t *self);

void apx_router_attachNodeInfo(apx_router_t *self, apx_nodeInfo_t *nodeInfo);
void apx_router_detachNodeInfo(apx_router_t *self, apx_nodeInfo_t *nodeInfo);
void apx_router_setDebugMode(apx_router_t *self, int8_t debugMode);

#endif //APX_ROUTER_H
