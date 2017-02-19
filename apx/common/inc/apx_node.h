#ifndef APX_NODE_H
#define APX_NODE_H
#include <stdint.h>
#include "adt_ary.h"
#include "apx_datatype.h"
#include "apx_port.h"


struct apx_node_t;
struct apx_nodeInfo_tag;

typedef struct apx_node_t{
   adt_ary_t datatypeList;
   adt_ary_t requirePortList;
   adt_ary_t providePortList;
   int8_t lastPortError;
   int8_t lastPortId;
   int16_t lastPortType;
   char *name;
   struct apx_nodeInfo_tag *nodeInfo;
} apx_node_t;


struct apx_portref_tag;
/***************** Public Function Declarations *******************/
//constructor/destructor
apx_node_t *apx_node_new(const char *name);
void apx_node_delete(apx_node_t *self);
void apx_node_vdelete(void *arg);
void apx_node_create(apx_node_t *self,const char *name);
void apx_node_destroy(apx_node_t *self);

//node functions
void apx_node_setName(apx_node_t *self, const char *name);

//datatype functions
apx_datatype_t *apx_node_createDataType(apx_node_t *self, const char* name, const char *dsg, const char *attr);
//port functions
apx_port_t *apx_node_createRequirePort(apx_node_t *self, const char* name, const char *dsg, const char *attr);
apx_port_t *apx_node_createProvidePort(apx_node_t *self, const char* name, const char *dsg, const char *attr);
int8_t apx_node_resolvePortSignatures(apx_node_t *self);
apx_port_t *apx_node_getRequirePort(apx_node_t *self, int32_t portIndex);
apx_port_t *apx_node_getProvidePort(apx_node_t *self, int32_t portIndex);


#endif //APX_NODE_H
