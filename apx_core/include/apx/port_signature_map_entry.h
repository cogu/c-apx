/*****************************************************************************
* \file      port_signature_entry.h
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     An element in an apx_portSignatureMap_t
*
* Copyright (c) 2020-2021 Conny Gustafsson
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
#ifndef APX_PORT_SIGNATURE_MAP_ENTRY_H
#define APX_PORT_SIGNATURE_MAP_ENTRY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_list.h"
#include "apx/types.h"
#include "apx/port_instance.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_nodeData_tag;
struct apx_port_tag;

typedef struct apx_portSignatureMapEntry_tag
{
   apx_portInstance_t *preferred_provider;
   adt_list_t require_ports; //weak references to apx_portInstance_t
   adt_list_t provide_ports; //weak references to apx_portInstance_t
} apx_portSignatureMapEntry_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portSignatureMapEntry_create(apx_portSignatureMapEntry_t *self);
void apx_portSignatureMapEntry_destroy(apx_portSignatureMapEntry_t *self);
apx_portSignatureMapEntry_t *apx_portSignatureMapEntry_new(void);
void apx_portSignatureMapEntry_delete(apx_portSignatureMapEntry_t *self);
void apx_portSignatureMapEntry_vdelete(void *arg);
void apx_portSignatureMapEntry_attach_require_port(apx_portSignatureMapEntry_t *self, apx_portInstance_t* port_instance);
void apx_portSignatureMapEntry_attach_provide_port(apx_portSignatureMapEntry_t *self, apx_portInstance_t* port_instance, bool is_preferred);
void apx_portSignatureMapEntry_detach_require_port(apx_portSignatureMapEntry_t *self, apx_portInstance_t* port_instance);
void apx_portSignatureMapEntry_detach_provide_port(apx_portSignatureMapEntry_t *self, apx_portInstance_t* port_instance);

bool apx_portSignatureMapEntry_is_empty(apx_portSignatureMapEntry_t *self);
int32_t apx_portSignatureMapEntry_get_num_providers(apx_portSignatureMapEntry_t* self);
int32_t apx_portSignatureMapEntry_get_num_requesters(apx_portSignatureMapEntry_t* self);
apx_portInstance_t*apx_portSignatureMapEntry_get_first_provider(apx_portSignatureMapEntry_t *self);
apx_portInstance_t*apx_portSignatureMapEntry_get_last_provider(apx_portSignatureMapEntry_t *self);
apx_portInstance_t* apx_portSignatureMapEntry_get_first_requester(apx_portSignatureMapEntry_t* self);
apx_portInstance_t* apx_portSignatureMapEntry_get_last_requester(apx_portSignatureMapEntry_t* self);
void apx_portSignatureMapEntry_set_preferred_provider(apx_portSignatureMapEntry_t *self, apx_portInstance_t* port_instance);
apx_portInstance_t*apx_portSignatureMapEntry_get_preferred_provider(apx_portSignatureMapEntry_t *self);
apx_error_t apx_portSignatureMapEntry_notify_require_ports_about_provide_port_change(apx_portSignatureMapEntry_t *self, apx_portInstance_t* provide_port, apx_portConnectorEvent_t event_type);
apx_error_t apx_portSignatureMapEntry_notify_provide_ports_about_require_port_change(apx_portSignatureMapEntry_t *self, apx_portInstance_t* require_port, apx_portConnectorEvent_t event_type);

#endif //APX_ROUTING_TABLE_ENTRY_H
