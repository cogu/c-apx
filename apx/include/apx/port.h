/*****************************************************************************
* \file      port.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Parse tree APX port
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
#include "apx/data_signature.h"
#include "apx/port_attribute.h"
#include "apx/type_attribute.h"
#include "apx/data_element.h"
#include "apx/error.h"
#include "apx/types.h"
#include "adt_ary.h"
#include "adt_hash.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
typedef struct apx_port_tag
{
   apx_portType_t port_type;
   apx_portId_t port_id;
   int32_t line_number;
   char *name;
   apx_dataSignature_t data_signature;
   apx_portAttributes_t* attributes; //Strong reference
   dtl_dv_t* proper_init_value;
} apx_port_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_port_create(apx_port_t* self, apx_portType_t port_type, const char* name, int32_t line_number);
void apx_port_destroy(apx_port_t *self);
apx_port_t* apx_port_new(apx_portType_t port_type, const char* name, int32_t line_number);
void apx_port_delete(apx_port_t *self);
void apx_port_vdelete(void *arg);

apx_dataElement_t* apx_port_get_data_element(apx_port_t const* self);
apx_dataElement_t* apx_port_get_effective_data_element(apx_port_t const* self);
apx_portAttributes_t* apx_port_get_attributes(apx_port_t* self);
bool apx_port_has_attributes(apx_port_t* self);
apx_error_t apx_port_init_attributes(apx_port_t* self);
apx_typeAttributes_t* apx_port_get_referenced_type_attributes(apx_port_t const* self); //Will return NULL if this port isn't referencing a data type
apx_error_t apx_port_derive_types(apx_port_t* self, adt_ary_t const* type_list, adt_hash_t const* type_map);
apx_error_t apx_port_derive_proper_init_value(apx_port_t* self);
bool apx_port_is_queued(apx_port_t* self);
bool apx_port_is_parameter(apx_port_t* self);
uint32_t apx_port_get_queue_length(apx_port_t* self);
apx_error_t apx_port_flatten_data_element(apx_port_t* self);
const char* apx_port_get_name(apx_port_t const* self);
apx_portType_t apx_port_get_port_type(apx_port_t const* self);
void apx_port_set_id(apx_port_t* self, apx_portId_t port_id);
apx_portId_t apx_port_get_id(apx_port_t* self);
dtl_dv_t* apx_port_get_proper_init_value(apx_port_t* self);

#endif //APX_PORT_H
