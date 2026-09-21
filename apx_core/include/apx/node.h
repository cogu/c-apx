/*****************************************************************************
* \file      node.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     APX (parse tree) node
*
* Copyright (c) 2017-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_NODE_H
#define APX_NODE_H
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "adt_hash.h"
#include "apx/data_type.h"
#include "apx/port.h"
#include "apx/error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_node_tag {
   adt_ary_t data_types; //strong reference to apx_dataType_t
   adt_ary_t require_ports; //strong reference to apx_port_t
   adt_ary_t provide_ports; //strong reference to apx_port_t
   adt_hash_t type_map; //weak reference to apx_dataType_t
   adt_hash_t port_map; //weak reference to apx_port_t
   char* name;
   bool is_finalized;
   int32_t last_error_line;
} apx_node_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_node_create(apx_node_t* self, const char* name);
void apx_node_destroy(apx_node_t* self);
apx_node_t* apx_node_new(const char* name);
void apx_node_delete(apx_node_t* self);
void apx_node_vdelete(void* arg);
apx_error_t apx_node_append_data_type(apx_node_t* self, apx_dataType_t* data_type);
apx_error_t apx_node_append_port(apx_node_t* self, apx_port_t* port);
void apx_node_set_name(apx_node_t* self, const char* name);
const char* apx_node_get_name(const apx_node_t* self);
int32_t apx_node_num_data_types(const apx_node_t* self);
int32_t apx_node_num_require_ports(const apx_node_t* self);
int32_t apx_node_num_provide_ports(const apx_node_t* self);
apx_dataType_t* apx_node_get_data_type(const apx_node_t* self, apx_typeId_t type_id);
apx_port_t* apx_node_get_require_port(const apx_node_t* self, apx_portId_t port_id);
apx_port_t* apx_node_get_provide_port(const apx_node_t* self, apx_portId_t port_id);

apx_dataType_t* apx_node_get_last_data_type(const apx_node_t* self);
apx_port_t* apx_node_get_last_require_port(const apx_node_t* self);
apx_port_t* apx_node_get_last_provide_port(const apx_node_t* self);
apx_error_t apx_node_finalize(apx_node_t* self);
int32_t apx_node_get_last_error_line(const apx_node_t* self);




#endif //APX_NODE_H