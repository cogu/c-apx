#ifndef APX_DATAELEMENT_H
#define APX_DATAELEMENT_H
#include <stdint.h>
#include "adt_ary.h"
#include "dtl_type.h"
#include "apx_error.h"

#define APX_BASE_TYPE_NONE     -1
#define APX_BASE_TYPE_UINT8     0 //'C'
#define APX_BASE_TYPE_UINT16    1 //'S'
#define APX_BASE_TYPE_UINT32    2 //'L'
#define APX_BASE_TYPE_UINT64    3 //'U'
#define APX_BASE_TYPE_SINT8     4 //'c'
#define APX_BASE_TYPE_SINT16    5 //'s'
#define APX_BASE_TYPE_SINT32    6 //'l'
#define APX_BASE_TYPE_SINT64    7 //'u'
#define APX_BASE_TYPE_STRING    8 //'a'
#define APX_BASE_TYPE_RECORD    9 //"{}"


typedef struct apx_dataElement_tag
{
   char *name;
   int8_t baseType;
   uint32_t arrayLen;
   uint32_t packLen;
   union {
      uint32_t u32;
      int32_t  s32;
   }min;
   union {
      uint32_t u32;
      int32_t  s32;
   }max;
   adt_ary_t *childElements; //NULL for all cases except when baseType is exactly == APX_BASE_TYPE_RECORD
}apx_dataElement_t;

/***************** Public Function Declarations *******************/
apx_dataElement_t *apx_dataElement_new(int8_t baseType, const char *name);
void apx_dataElement_delete(apx_dataElement_t *self);
void apx_dataElement_vdelete(void *arg);
int8_t apx_dataElement_create(apx_dataElement_t *self, int8_t baseType, const char *name);
void apx_dataElement_destroy(apx_dataElement_t *self);
void apx_dataElement_initRecordType(apx_dataElement_t *self);
uint8_t *apx_dataElement_pack_dv(apx_dataElement_t *self, uint8_t *pBegin, uint8_t *pEnd, dtl_dv_t *dv);

void apx_dataElement_setArrayLen(apx_dataElement_t *self, uint32_t arrayLen);
uint32_t apx_dataElement_getArrayLen(apx_dataElement_t *self);
void apx_dataElement_appendChild(apx_dataElement_t *self, apx_dataElement_t *child);
int32_t apx_dataElement_getNumChild(apx_dataElement_t *self);
apx_dataElement_t *apx_dataElement_getChildAt(apx_dataElement_t *self, int32_t index);
#endif //APX_DATAELEMENT_H
