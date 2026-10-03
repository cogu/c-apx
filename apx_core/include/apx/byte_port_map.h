/*****************************************************************************
* \file      byte_port_map.h
* \author    Conny Gustafsson
* \date      2018-10-09
* \brief     Byte offset to port id map using contiguous partition indexing
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
#include "adt_partition.h"
#include "apx/types.h"
#include "apx/port_instance.h"
#include "apx/error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_PARTITION_U16 0u
#define APX_PARTITION_U32 1u

typedef struct apx_byte_port_map_tag
{
   uint8_t partition_type;
   union {
      adt_u16_partition_t u16_part;
      adt_u32_partition_t u32_part;
   } partition;
} apx_byte_port_map_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_byte_port_map_create(apx_byte_port_map_t *self, apx_size_t total_size, apx_port_instance_t const* port_instance_list, apx_size_t num_ports);
void apx_byte_port_map_destroy(apx_byte_port_map_t *self);
apx_byte_port_map_t *apx_byte_port_map_new(apx_size_t total_size, apx_port_instance_t const* port_instance_list, apx_size_t num_ports, apx_error_t *error_code);
void apx_byte_port_map_delete(apx_byte_port_map_t *self);

apx_port_id_t apx_byte_port_map_lookup(const apx_byte_port_map_t *self, uint32_t offset);
apx_size_t apx_byte_port_map_length(const apx_byte_port_map_t *self);
uint8_t apx_byte_port_map_type(const apx_byte_port_map_t *self);

#endif //APX_BYTE_PORT_MAP_H
