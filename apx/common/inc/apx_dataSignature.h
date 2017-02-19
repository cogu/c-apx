#ifndef APX_DATA_SIGNATURE_H
#define APX_DATA_SIGNATURE_H
#include <stdint.h>
#include "apx_dataElement.h"

#define APX_DSG_TYPE_SENDER_RECEIVER   0
#define APX_DSG_TYPE_CLIENT_SERVER     1

typedef struct apx_dataSignature_tag
{
   char *str;
   uint8_t dsgType; //this will always have value APX_DSG_TYPE_SENDER_RECEIVER until client/server has been implemented
   apx_dataElement_t *dataElement;
   //TODO: implement support for client/server interfaces here
}apx_dataSignature_t;

/***************** Public Function Declarations *******************/
apx_dataSignature_t *apx_dataSignature_new(const char *dsg);
void apx_dataSignature_delete(apx_dataSignature_t *self);
void apx_dataSignature_vdelete(void *arg);
int8_t apx_dataSignature_create(apx_dataSignature_t *self, const char *dsg);
void apx_dataSignature_destroy(apx_dataSignature_t *self);
uint32_t apx_dataSignature_packLen(apx_dataSignature_t *self);
int8_t apx_dataSignature_update(apx_dataSignature_t *self,const char *dsg);

#endif //APX_DATA_SIGNATURE_H
