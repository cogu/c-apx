#ifndef APX_PORT_H
#define APX_PORT_H
//simple port - data port with one data element
#include "apx_dataSignature.h"

#define APX_REQUIRE_PORT 0
#define APX_PROVIDE_PORT 1

typedef struct apx_port_tag{
	char *name;
	char *dataSignature; //underived data signature, this string usually contains just a type reference, e.g. "T[0]"
	apx_dataSignature_t derivedDsg; //this is the true data signature, e.g. "C(0.7)"
	char *attributes; //attribute string
	char *portSignature; //full port signature, excluding the initial 'R' or 'P'
	uint8_t portType; //APX_REQUIRE_PORT or APX_PROVIDE_PORT
	int32_t portIndex; //index of the port 0..len(ports) where it resides on its parent node
}apx_port_t;

/***************** Public Function Declarations *******************/

void apx_port_create(apx_port_t *self,uint8_t portDirection,const char *name, const char* dataSignature, const char *attributes);
void apx_port_destroy(apx_port_t *self);
apx_port_t* apx_providePort_new(const char *name, const char* dataSignature, const char *attributes);
apx_port_t* apx_requirePort_new(const char *name, const char* dataSignature, const char *attributes);
void apx_port_delete(apx_port_t *self);
void apx_port_vdelete(void *arg);

void apx_port_setDerivedDataSignature(apx_port_t *self, const char *dataSignature);
const char *apx_port_derivePortSignature(apx_port_t *self);
const char *apx_port_getPortSignature(apx_port_t *self);
int32_t apx_port_getPackLen(apx_port_t *self);
void apx_port_setPortIndex(apx_port_t *self, int32_t portIndex);
int32_t  apx_port_getPortIndex(apx_port_t *self);

#ifdef UNIT_TEST

#endif

#endif //APX_PORT_H
