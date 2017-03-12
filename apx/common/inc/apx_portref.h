#ifndef APX_PORT_REF_H
#define APX_PORT_REF_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_port.h"
#include "apx_node.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portref_tag
{
   apx_node_t *node;
   apx_port_t *port;
}apx_portref_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portref_create(apx_portref_t *self,apx_node_t *node,apx_port_t *port);
int8_t apx_portref_createFromRequirePort(apx_portref_t *self,apx_node_t *node,int32_t portIndex);
int8_t apx_portref_createFromProvidePort(apx_portref_t *self,apx_node_t *node,int32_t portIndex);
void apx_portref_destroy(apx_portref_t *self);
apx_portref_t *apx_portref_new(apx_node_t *node,apx_port_t *port);
void apx_portref_delete(apx_portref_t *self);
void apx_portref_vdelete(void *arg);
int8_t apx_portref_equals(apx_portref_t *a, apx_portref_t *b);

#endif //APX_PORT_REF_H