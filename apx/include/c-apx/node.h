/*****************************************************************************
* \file      node.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX (parse tree) node 
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#ifndef APX_NODE_H
#define APX_NODE_H
#include <stdint.h>
#include "adt_ary.h"
#include "adt_bytearray.h"
#include "apx/data_type.h"
#include "apx/port.h"
#include "apx/attribute_parser.h"
#include "apx/error.h"
#include <stdbool.h>

typedef struct apx_node_tag {
   adt_ary_t datatypeList;
   adt_ary_t requirePortList;
   adt_ary_t providePortList;
   char *name;
   bool isFinalized;
   apx_attributeParser_t attributeParser;
   apx_error_t lastError;
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
const char *apx_node_getName(const apx_node_t *self);
void apx_node_setVersion(apx_node_t *self, int16_t majorVersion, int16_t minorVersion);

//datatype functions
apx_datatype_t *apx_node_createDataType(apx_node_t *self, const char* name, const char *dsg, const char *attr, int32_t lineNumber);
//port functions
apx_port_t *apx_node_createRequirePort(apx_node_t *self, const char* name, const char *dsg, const char *attr, int32_t lineNumber);
apx_port_t *apx_node_createProvidePort(apx_node_t *self, const char* name, const char *dsg, const char *attr, int32_t lineNumber);
apx_error_t apx_node_finalize(apx_node_t *self, int32_t *errorLine);
apx_port_t *apx_node_getRequirePort(const apx_node_t *self, apx_portId_t portIndex);
apx_port_t *apx_node_getProvidePort(const apx_node_t *self, apx_portId_t portIndex);
int32_t apx_node_getNumRequirePorts(apx_node_t *self);
int32_t apx_node_getNumProvidePorts(apx_node_t *self);
int32_t apx_node_calcOutPortDataLen(apx_node_t *self);
int32_t apx_node_calcInPortDataLen(apx_node_t *self);
adt_bytearray_t *apx_node_createPortInitData(apx_node_t *self, apx_port_t *port);
apx_error_t apx_node_fillPortInitData(apx_node_t *self, apx_port_t *port, adt_bytearray_t *output);
apx_error_t apx_node_getLastError(apx_node_t *self);
adt_ary_t *apx_node_getRequirePortList(const apx_node_t *self);
adt_ary_t *apx_node_getProvidePortList(const apx_node_t *self);


#endif //APX_NODE_H
