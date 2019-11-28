/*****************************************************************************
* \file      apx_routingTableEntry.h
* \author    Conny Gustafsson
* \date      2018-10-08
* \brief     An element in the globalPortMap_t
*
* Copyright (c) 2018 Conny Gustafsson
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
#ifndef APX_ROUTING_TABLE_ENTRY_H
#define APX_ROUTING_TABLE_ENTRY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_list.h"
#include "apx_types.h"
#include "apx_portDataRef2.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_nodeData_tag;
struct apx_port_tag;

typedef struct apx_routingTableEntry_tag
{
   const char *portSignature; //weak reference to string
   int32_t currentProviderId; //which provider is currently selected
   adt_list_t requirePortRef; //weak references to apx_portDataRef_t
   adt_list_t providePortRef; //weak references to apx_portDataRef_t
}apx_routingTableEntry_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_routingTableEntry_create(apx_routingTableEntry_t *self, const char *portSignature);
void apx_routingTableEntry_destroy(apx_routingTableEntry_t *self);
apx_routingTableEntry_t *apx_routingTableEntry_new(const char *portSignature);
void apx_routingTableEntry_delete(apx_routingTableEntry_t *self);
void apx_routingTableEntry_vdelete(void *arg);

void apx_routingTableEntry_attachPortDataRef(apx_routingTableEntry_t *self, apx_portDataRef2_t *portDataRef);
void apx_routingTableEntry_detachPortDataRef(apx_routingTableEntry_t *self, apx_portDataRef2_t *portDataRef);
bool apx_routingTableEntry_isEmpty(apx_routingTableEntry_t *self);
apx_portDataRef2_t *apx_routingTableEntry_getFirstProvider(apx_routingTableEntry_t *self);
apx_portDataRef2_t *apx_routingTableEntry_getLastProvider(apx_routingTableEntry_t *self);

#endif //APX_ROUTING_TABLE_ENTRY_H
