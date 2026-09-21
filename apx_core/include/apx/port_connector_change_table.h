/*****************************************************************************
* \file      port_connector_change_table.h
* \author    Conny Gustafsson
* \date      2019-01-31
* \brief     A list of apx_port_connection_change_entry_t.
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PORT_CONNECTION_CHANGE_TABLE_H
#define APX_PORT_CONNECTION_CHANGE_TABLE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include "apx/port_connector_change_entry.h"
#include "adt_ary.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_port_connector_change_table_tag
{
   apx_port_connector_change_entry_t *entries; //Array of apx_port_connection_change_entry_t (created using single malloc)
   apx_size_t num_ports;                    //This must match node_instance->num_require_ports when this is used for requirePorts
                                            //or node_instance->num_provide_ports when used for providePorts
} apx_port_connector_change_table_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_port_connector_change_table_create(apx_port_connector_change_table_t *self, apx_size_t num_ports);
void apx_port_connector_change_table_destroy(apx_port_connector_change_table_t *self);
apx_port_connector_change_table_t *apx_port_connector_change_table_new(int32_t num_ports);
void apx_port_connector_change_table_delete(apx_port_connector_change_table_t *self);

apx_error_t apx_port_connector_change_table_connect(apx_port_connector_change_table_t *self, apx_port_instance_t* local_port, apx_port_instance_t* remote_port);
apx_error_t apx_port_connector_change_table_disconnect(apx_port_connector_change_table_t *self, apx_port_instance_t* local_port, apx_port_instance_t* remote_port);
apx_port_connector_change_entry_t *apx_port_connector_change_table_get_entry(apx_port_connector_change_table_t *self, apx_port_id_t port_id);
apx_port_instance_t*apx_port_connector_change_table_get_port(apx_port_connector_change_table_t *self, apx_port_id_t port_id, int32_t index);
int32_t apx_port_connector_change_table_count(apx_port_connector_change_table_t *self, apx_port_id_t port_id);


#endif //APX_PORT_CONNECTION_CHANGE_TABLE_H
