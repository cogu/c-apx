/*****************************************************************************
* \file      port_signature_entry.h
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     An element in an apx_portSignatureMap_t
*
* Copyright (c) 2020 Conny Gustafsson
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
#include "apx/port_data_ref.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_nodeData_tag;
struct apx_port_tag;

typedef struct apx_portSignatureMapEntry_tag
{
   apx_portRef_t *preferredProvider;
   adt_list_t requirePortRef; //weak references to apx_portRef_t
   adt_list_t providePortRef; //weak references to apx_portRef_t
} apx_portSignatureMapEntry_t;

typedef uint8_t apx_portConnectorEvent_t;
#define APX_PORT_CONNECTED_EVENT       ((apx_portConnectorEvent_t) 0u)
#define APX_PORT_DISCONNECTED_EVENT    ((apx_portConnectorEvent_t) 1u)

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portSignatureMapEntry_create(apx_portSignatureMapEntry_t *self);
void apx_portSignatureMapEntry_destroy(apx_portSignatureMapEntry_t *self);
apx_portSignatureMapEntry_t *apx_portSignatureMapEntry_new(void);
void apx_portSignatureMapEntry_delete(apx_portSignatureMapEntry_t *self);
void apx_portSignatureMapEntry_vdelete(void *arg);
void apx_portSignatureMapEntry_attachRequirePort(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef);
void apx_portSignatureMapEntry_attachProvidePort(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef, bool isPreferred);
void apx_portSignatureMapEntry_detachRequirePort(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef);
void apx_portSignatureMapEntry_detachProvidePort(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef);

bool apx_portSignatureMapEntry_isEmpty(apx_portSignatureMapEntry_t *self);
apx_portRef_t *apx_portSignatureMapEntry_getFirstProvider(apx_portSignatureMapEntry_t *self);
apx_portRef_t *apx_portSignatureMapEntry_getLastProvider(apx_portSignatureMapEntry_t *self);
void apx_portSignatureMapEntry_setPreferredProvider(apx_portSignatureMapEntry_t *self, apx_portRef_t *portRef);
apx_portRef_t *apx_portSignatureMapEntry_getPreferredProvider(apx_portSignatureMapEntry_t *self);
void apx_portSignatureMapEntry_notifyRequirePortsAboutProvidePortChange(apx_portSignatureMapEntry_t *self, apx_portRef_t *providePortRef, apx_portConnectorEvent_t eventType);
void apx_portSignatureMapEntry_notifyProvidePortsAboutRequirePortChange(apx_portSignatureMapEntry_t *self, apx_portRef_t *requirePortRef, apx_portConnectorEvent_t eventType);

#endif //APX_ROUTING_TABLE_ENTRY_H
