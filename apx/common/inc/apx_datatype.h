#ifndef APX_DATATYPE_H
#define APX_DATATYPE_H
#include <stdint.h>

typedef struct apx_datatype_tag
{
   char* pAlloc;
   uint32_t allocLen;
   char *name;
   char *dsg;
   char *attr;
}apx_datatype_t;

/***************** Public Function Declarations *******************/
apx_datatype_t* apx_datatype_new(const char *name, const char *dsg, const char *attr);
void apx_datatype_delete(apx_datatype_t *self);
void apx_datatype_vdelete(void *arg);
int8_t apx_datatype_create(apx_datatype_t *self, const char *name, const char *dsg, const char *attr);
void apx_datatype_destroy(apx_datatype_t *self);

#endif //APX_DATATYPE_H
