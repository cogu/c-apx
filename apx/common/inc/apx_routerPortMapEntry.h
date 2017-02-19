#ifndef APX_PORTMAP_ENTRY_H
#define APX_PORTMAP_ENTRY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_node.h"
#include "adt_ary.h"
#include "apx_portref.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct portMapEntry_tag
{
   adt_ary_t requirePorts; //list of apx_portref_t (all require ports that maps to this signal)
   adt_ary_t providePorts; //list of apx_portref_t (all provide ports that maps to this signal)
}apx_routerPortMapEntry_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_routerPortMapEntry_create(apx_routerPortMapEntry_t *self);
void apx_routerPortMapEntry_destroy(apx_routerPortMapEntry_t *self);
apx_routerPortMapEntry_t *apx_routerPortMapEntry_new(void);
void apx_routerPortMapEntry_delete(apx_routerPortMapEntry_t *self);
void apx_routerPortMapEntry_vdelete(void *arg);

int8_t apx_routerPortMapEntry_insertPort(apx_routerPortMapEntry_t *self, apx_node_t *node, apx_port_t *port);
int8_t apx_routerPortMapEntry_insertProvidePort(apx_routerPortMapEntry_t *self, apx_node_t *node, int32_t portIndex);
int8_t apx_routerPortMapEntry_insertRequirePort(apx_routerPortMapEntry_t *self, apx_node_t *node, int32_t portIndex);
int8_t apx_routerPortMapEntry_removePort(apx_routerPortMapEntry_t *self, apx_node_t *node, apx_port_t *port);

apx_portref_t *apx_routerPortMapEntry_getProvidePortById(apx_routerPortMapEntry_t *self,int32_t index);
apx_portref_t *apx_routerPortMapEntry_getRequirePortById(apx_routerPortMapEntry_t *self,int32_t index);


#endif //APX_PORTMAP_ENTRY_H
