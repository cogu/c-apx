/*****************************************************************************
* \file      apx_port.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX port class
*
* Copyright (c) 2017-2018 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
#ifndef APX_PORT_H
#define APX_PORT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_dataSignature.h"
#include "apx_portAttributes.h"
#include "apx_error.h"
#include "apx_types.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct adt_ary_tag;
struct adt_hash_tag;

typedef struct apx_port_tag
{
   char *name;
   apx_dataSignature_t dataSignature; //Container for both raw and derived data signature
   apx_portAttributes_t *portAttributes; //port attributes object, includes the raw attributes string
   char *derivedPortSignature; //Derived port signature, excluding the initial 'R' or 'P'
   int32_t lineNumber; //line number in the APX-file where this port is defines. Special value 0 is used in case this port was created without an APX-file
   apx_portId_t portId; //index of the port 0..len(ports) where it resides on its parent node
   apx_portType_t portType; //APX_REQUIRE_PORT or APX_PROVIDE_PORT
} apx_port_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_port_t* apx_providePort_new(const char *name, const char* dataSignature, const char *attributes, int32_t lineNumber, apx_error_t *errorCode);
apx_port_t* apx_requirePort_new(const char *name, const char* dataSignature, const char *attributes, int32_t lineNumber,  apx_error_t *errorCode);
apx_error_t apx_port_create(apx_port_t *self, apx_portType_t portType, const char *name, const char* dataSignature, const char *attributes, int32_t lineNumber);
void apx_port_destroy(apx_port_t *self);
void apx_port_delete(apx_port_t *self);
void apx_port_vdelete(void *arg);

apx_error_t apx_port_resolveTypes(apx_port_t *self, struct adt_ary_tag *typeList, struct adt_hash_tag *typeMap);
apx_error_t apx_port_updateDerivedPortSignature(apx_port_t *self);
const char *apx_port_getDerivedPortSignature(apx_port_t *self);
apx_error_t apx_port_updatePackLen(apx_port_t *self);
apx_error_t apx_port_calculateProperInitValue(apx_port_t *self);
dtl_dv_t *apx_port_getProperInitValue(apx_port_t *self);

apx_size_t apx_port_getPackLen(apx_port_t *self);
void apx_port_setPortId(apx_port_t *self, apx_portId_t portId);
apx_portId_t  apx_port_getPortId(apx_port_t *self);
apx_dataElement_t *apx_port_getDerivedDataElement(apx_port_t *self);
const char *apx_port_getName(const apx_port_t *self);

#endif //APX_PORT_H
