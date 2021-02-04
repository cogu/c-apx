/*****************************************************************************
* \file      port_signature_map.h
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     Port signature map
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
#ifndef APX_PORT_SIGNATURE_MAP_H
#define APX_PORT_SIGNATURE_MAP_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_hash.h"
#include "apx/types.h"
#include "apx/error.h"
#include "apx/port_signature_map_entry.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//Forward declaration
struct apx_nodeInstance_tag;

typedef struct apx_portSignatureMap_tag
{
   adt_hash_t internal_map; //strong references to apx_portSignatureMapEntry_t. The hash key is the portSignature string.
} apx_portSignatureMap_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portSignatureMap_create(apx_portSignatureMap_t *self);
void apx_portSignatureMap_destroy(apx_portSignatureMap_t *self);
apx_portSignatureMap_t *apx_portSignatureMap_new(void);
void apx_portSignatureMap_delete(apx_portSignatureMap_t *self);

apx_portSignatureMapEntry_t *apx_portSignatureMap_find(apx_portSignatureMap_t *self, const char *portSignature);
int32_t apx_portSignatureMap_length(apx_portSignatureMap_t *self);
apx_error_t apx_portSignatureMap_connect_provide_ports(apx_portSignatureMap_t *self, struct apx_nodeInstance_tag *node_instance);
apx_error_t apx_portSignatureMap_connect_require_ports(apx_portSignatureMap_t *self, struct apx_nodeInstance_tag *node_instance);
apx_error_t apx_portSignatureMap_disconnect_provide_ports(apx_portSignatureMap_t *self, struct apx_nodeInstance_tag *node_instance);
apx_error_t apx_portSignatureMap_disconnect_require_ports(apx_portSignatureMap_t *self, struct apx_nodeInstance_tag *node_instance);


#endif //APX_PORT_SIGNATURE_MAP_H
