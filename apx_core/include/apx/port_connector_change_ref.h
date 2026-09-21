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
typedef struct apx_portConnectorChangeRef_tag
{
   bool is_connector_changes_weak_ref;
   apx_nodeInstance_t *node_instance;
   apx_portConnectorChangeTable_t *connector_changes;
} apx_portConnectorChangeRef_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_portConnectorChangeRef_create(apx_portConnectorChangeRef_t *self, apx_nodeInstance_t * node_instance, apx_portConnectorChangeTable_t * connector_changes);
void apx_portConnectorChangeRef_destroy(apx_portConnectorChangeRef_t *self);
apx_portConnectorChangeRef_t *apx_portConnectorChangeRef_new(apx_nodeInstance_t * node_instance, apx_portConnectorChangeTable_t * connector_changes);
void apx_portConnectorChangeRef_delete(apx_portConnectorChangeRef_t *self);
void apx_portConnectorChangeRef_vdelete(void *arg);

#endif //APX_PORT_CONNECTOR_CHANGE_REF_H
