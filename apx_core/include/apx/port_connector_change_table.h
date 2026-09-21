/*****************************************************************************
* \file      port_connector_change_table.h
* \author    Conny Gustafsson
* \date      2019-01-31
* \brief     A list of apx_portConnectionChangeEntry_t.
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
typedef struct apx_portConnectorChangeTable_tag
{
   apx_portConnectorChangeEntry_t *entries; //Array of apx_portConnectionChangeEntry_t (created using single malloc)
   apx_size_t num_ports;                    //This must match node_instance->num_require_ports when this is used for requirePorts
                                            //or node_instance->num_provide_ports when used for providePorts
} apx_portConnectorChangeTable_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_portConnectorChangeTable_create(apx_portConnectorChangeTable_t *self, apx_size_t num_ports);
void apx_portConnectorChangeTable_destroy(apx_portConnectorChangeTable_t *self);
apx_portConnectorChangeTable_t *apx_portConnectorChangeTable_new(int32_t num_ports);
void apx_portConnectorChangeTable_delete(apx_portConnectorChangeTable_t *self);

apx_error_t apx_portConnectorChangeTable_connect(apx_portConnectorChangeTable_t *self, apx_portInstance_t* local_port, apx_portInstance_t* remote_port);
apx_error_t apx_portConnectorChangeTable_disconnect(apx_portConnectorChangeTable_t *self, apx_portInstance_t* local_port, apx_portInstance_t* remote_port);
apx_portConnectorChangeEntry_t *apx_portConnectorChangeTable_get_entry(apx_portConnectorChangeTable_t *self, apx_portId_t port_id);
apx_portInstance_t*apx_portConnectorChangeTable_get_port(apx_portConnectorChangeTable_t *self, apx_portId_t port_id, int32_t index);
int32_t apx_portConnectorChangeTable_count(apx_portConnectorChangeTable_t *self, apx_portId_t port_id);


#endif //APX_PORT_CONNECTION_CHANGE_TABLE_H
