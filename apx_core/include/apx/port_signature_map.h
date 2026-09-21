/*****************************************************************************
* \file      port_signature_map.h
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     Port signature map
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
struct apx_node_instance_tag;

typedef struct apx_port_signature_map_tag
{
   adt_hash_t internal_map; //strong references to apx_port_signature_map_entry_t. The hash key is the portSignature string.
} apx_port_signature_map_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portSignatureMap_create(apx_port_signature_map_t *self);
void apx_portSignatureMap_destroy(apx_port_signature_map_t *self);
apx_port_signature_map_t *apx_portSignatureMap_new(void);
void apx_portSignatureMap_delete(apx_port_signature_map_t *self);

apx_port_signature_map_entry_t *apx_portSignatureMap_find(apx_port_signature_map_t *self, const char *portSignature);
int32_t apx_portSignatureMap_length(apx_port_signature_map_t *self);
apx_error_t apx_portSignatureMap_connect_provide_ports(apx_port_signature_map_t *self, struct apx_node_instance_tag *node_instance);
apx_error_t apx_portSignatureMap_connect_require_ports(apx_port_signature_map_t *self, struct apx_node_instance_tag *node_instance);
apx_error_t apx_portSignatureMap_disconnect_provide_ports(apx_port_signature_map_t *self, struct apx_node_instance_tag *node_instance);
apx_error_t apx_portSignatureMap_disconnect_require_ports(apx_port_signature_map_t *self, struct apx_node_instance_tag *node_instance);


#endif //APX_PORT_SIGNATURE_MAP_H
