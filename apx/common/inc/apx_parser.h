#ifndef APX_PARSER_H
#define APX_PARSER_H
#include <stdint.h>
#include "apx_node.h"
#include "adt_ary.h"

typedef struct apx_parser_tag
{
   adt_ary_t nodeList;
   apx_node_t *currentNode;
}apx_parser_t;
/***************** Public Function Declarations *******************/
void apx_parser_create(apx_parser_t *self);
void apx_parser_destroy(apx_parser_t *self);
int32_t apx_parser_getNumNodes(apx_parser_t *self);
apx_node_t *apx_parser_getNode(apx_parser_t *self, int32_t index);
void apx_parser_clearNodes(apx_parser_t *self);
#if defined(_WIN32) || defined(__GNUC__)
apx_node_t *apx_parser_parseFile(apx_parser_t *self, const char *filename);
#endif

//event handlers
void apx_parser_open(apx_parser_t *self);
void apx_parser_close(apx_parser_t *self);
void apx_parser_node(apx_parser_t *self, const char *name); //N"<name>"
void apx_parser_datatype(apx_parser_t *self, const char *name, const char *dsg, const char *attr);
void apx_parser_require(apx_parser_t *self, const char *name, const char *dsg, const char *attr);
void apx_parser_provide(apx_parser_t *self, const char *name, const char *dsg, const char *attr);
void apx_parser_node_end(apx_parser_t *self);

//void event handlers
void apx_parser_vopen(void *arg);
void apx_parser_vclose(void *arg);
void apx_parser_vnode(void *arg, const char *name); //N"<name>"
void apx_parser_vdatatype(void *arg, const char *name, const char *dsg, const char *attr);
void apx_parser_vrequire(void *arg, const char *name, const char *dsg, const char *attr);
void apx_parser_vprovide(void *arg, const char *name, const char *dsg, const char *attr);
void apx_parser_vnode_end(void *arg);


#endif //APX_PARSER_H
