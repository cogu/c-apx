/*****************************************************************************
* \file      port_connector_change_ref.h
* \author    Conny Gustafsson
* \date      2020-03-03
* \brief     Simple struct containing two pointers
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PORT_CONNECTOR_CHANGE_REF_H
#define APX_PORT_CONNECTOR_CHANGE_REF_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/node_instance.h"
#include "apx/port_connector_change_table.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_port_connector_change_ref_tag
{
   bool is_connector_changes_weak_ref;
   apx_node_instance_t *node_instance;
   apx_port_connector_change_table_t *connector_changes;
} apx_port_connector_change_ref_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_port_connector_change_ref_create(apx_port_connector_change_ref_t *self, apx_node_instance_t * node_instance, apx_port_connector_change_table_t * connector_changes);
void apx_port_connector_change_ref_destroy(apx_port_connector_change_ref_t *self);
apx_port_connector_change_ref_t *apx_port_connector_change_ref_new(apx_node_instance_t * node_instance, apx_port_connector_change_table_t * connector_changes);
void apx_port_connector_change_ref_delete(apx_port_connector_change_ref_t *self);
void apx_port_connector_change_ref_vdelete(void *arg);

#endif //APX_PORT_CONNECTOR_CHANGE_REF_H
