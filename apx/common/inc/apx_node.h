#ifndef APX_NODE_H
#define APX_NODE_H
#include <stdint.h>
#include "adt_ary.h"
#include "adt_bytearray.h"
#include "apx_datatype.h"
#include "apx_port.h"
#include "apx_attributeParser.h"
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif

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
   bool isFinalized;
   apx_attributeParser_t attributeParser;
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
const char *apx_node_getName(apx_node_t *self);

//datatype functions
apx_datatype_t *apx_node_createDataType(apx_node_t *self, const char* name, const char *dsg, const char *attr);
//port functions
apx_port_t *apx_node_createRequirePort(apx_node_t *self, const char* name, const char *dsg, const char *attr);
apx_port_t *apx_node_createProvidePort(apx_node_t *self, const char* name, const char *dsg, const char *attr);
int8_t apx_node_finalize(apx_node_t *self);
apx_port_t *apx_node_getRequirePort(apx_node_t *self, int32_t portIndex);
apx_port_t *apx_node_getProvidePort(apx_node_t *self, int32_t portIndex);
int32_t apx_node_getNumRequirePorts(apx_node_t *self);
int32_t apx_node_getNumProvidePorts(apx_node_t *self);
adt_bytearray_t *apx_node_createPortInitData(apx_node_t *self, apx_port_t *port);
int32_t apx_node_fillPortInitData(apx_node_t *self, apx_port_t *port, adt_bytearray_t *output);

#endif //APX_NODE_H
