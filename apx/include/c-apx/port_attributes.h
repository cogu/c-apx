#ifndef APX_PORT_ATTRIBUTES_H
#define APX_PORT_ATTRIBUTES_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "dtl_type.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_portAttributes_tag
{
   bool isQueued;
   bool isParameter;
   bool isDynamic;
   bool isFinalized; //internal variable
   uint32_t queueLen;
   uint32_t dynLen;
   char *rawString; //raw attribute string
   dtl_dv_t *initValue;
   dtl_dv_t *properInitValue;
}apx_portAttributes_t;



//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portAttributes_create(apx_portAttributes_t *self, const char *attr);
void apx_portAttributes_destroy(apx_portAttributes_t *self);
apx_portAttributes_t* apx_portAttributes_new(const char *attr);
void apx_portAttributes_delete(apx_portAttributes_t *self);
void apx_portAttributes_vdelete(void *arg);
void apx_portAttributes_clearInitValue(apx_portAttributes_t *self);
dtl_dv_t *apx_portAttributes_getProperInitValue(apx_portAttributes_t *self);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////



#endif //APX_PORT_ATTRIBUTES_H
