/*****************************************************************************
* \file      byte_port_map.h
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Byte offset to port id map
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_BYTE_PORT_MAP_H
#define APX_BYTE_PORT_MAP_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/port_instance.h"
#include "apx/error.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_bytePortMap_tag
{
   apx_portId_t *map_data;
   uint32_t map_len;
}apx_bytePortMap_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_bytePortMap_create(apx_bytePortMap_t *self, apx_size_t total_size, apx_portInstance_t const* port_instance_list, apx_size_t num_ports);
void apx_bytePortMap_destroy(apx_bytePortMap_t *self);
apx_bytePortMap_t *apx_bytePortMap_new(apx_size_t total_size, apx_portInstance_t  const* port_instance_list, apx_size_t num_ports, apx_error_t *errorCode);
void apx_bytePortMap_delete(apx_bytePortMap_t *self);

apx_portId_t apx_bytePortMap_lookup(const apx_bytePortMap_t *self, uint32_t offset);
apx_size_t apx_bytePortMap_length(const apx_bytePortMap_t *self);

#endif //APX_BYTE_PORT_MAP_H
