/*****************************************************************************
* \file      port_connector_list.h
* \author    Conny Gustafsson
* \date      2018-12-07
* \brief     Container for port connectors from P-Port perspective
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PORT_CONNECTOR_LIST_H
#define APX_PORT_CONNECTOR_LIST_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "apx/error.h"
#include "apx/port_instance.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

/**
 * Keeps a list of port connectors from one p-port to zero or more r-ports.
 */
typedef struct apx_port_connector_list_tag
{
   adt_ary_t require_ports; //weak references to apx_port_instance_t
} apx_port_connector_list_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//dataTriggerTable
void apx_port_connector_list_create(apx_port_connector_list_t *self);
void apx_port_connector_list_destroy(apx_port_connector_list_t *self);
apx_port_connector_list_t* apx_port_connector_list_new(void);
void apx_port_connector_list_delete(apx_port_connector_list_t *self);

apx_error_t apx_port_connector_list_insert(apx_port_connector_list_t *self, apx_port_instance_t* port_instance);
void apx_port_connector_list_remove(apx_port_connector_list_t *self, apx_port_instance_t* port_instance);
void apx_port_connector_list_clear(apx_port_connector_list_t *self);
int32_t apx_port_connector_list_length(apx_port_connector_list_t *self);
apx_port_instance_t*apx_port_connector_list_get(apx_port_connector_list_t *self, int32_t index);

#endif //APX_PORT_CONNECTOR_LIST_H
