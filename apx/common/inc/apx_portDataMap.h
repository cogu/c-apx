#ifndef APX_PORT_DATA_MAP_H
#define APX_PORT_DATA_MAP_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_port.h"
#include "apx_node.h"
#include "adt_ary.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

/**
 * metadata describing what is in this port (data offset, and length)
 */
typedef struct apx_portDataMapEntry_tag
{
   apx_port_t *port; //pointer to the port
   int32_t offset; //data offset
   int32_t length; //data length
}apx_portDataMapEntry_t;

typedef struct apx_portDataMap_tag
{
   adt_ary_t elements; //list of apx_portDataMapEntry_t object
   apx_node_t *node; //pointer to parent node
   int8_t mapType; //APX_REQUIRE_DATA_MAP or APX_PROVIDE_DATA_MAP
   int32_t totalLen; //totalLen=sum([x.length for x in elements]
}apx_portDataMap_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portDataMapEntry_create(apx_portDataMapEntry_t *self, apx_port_t *port, int32_t offset, int32_t length);
void apx_portDataMapEntry_destroy(apx_portDataMapEntry_t *self);
apx_portDataMapEntry_t *apx_portDataMapEntry_new(apx_port_t *port,int32_t offset, int32_t length);
void apx_portDataMapEntry_delete(apx_portDataMapEntry_t *self);
void apx_portDataMapEntry_vdelete(void *arg);

void apx_portDataMap_create(apx_portDataMap_t *self);
void apx_portDataMap_destroy(apx_portDataMap_t *self);
apx_portDataMap_t *apx_portDataMap_new(void);
void apx_portDataMap_delete(apx_portDataMap_t *self);
void apx_portDataMap_vdelete(void *arg);
int8_t apx_portDataMap_build(apx_portDataMap_t *self, apx_node_t *node, uint8_t portType);
int32_t apx_portDataMap_getDataLen(apx_portDataMap_t *self);

apx_portDataMapEntry_t *apx_portDataMap_getEntry(apx_portDataMap_t *self, int32_t portIndex);

#endif //APX_PORT_DATA_MAP_H
