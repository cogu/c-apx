/*****************************************************************************
* \file      port_connector_change_entry.h
* \author    Conny Gustafsson
* \date      2019-01-23
* \brief     APX port connection change information (for one port)
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PORT_CONNECTION_ENTRY_H
#define APX_PORT_CONNECTION_ENTRY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "apx/types.h"
#include "apx/port_instance.h"
#include "apx/error.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

/**
 * Tracks new connectors attached/detached to a port. The count variable is used to track number of changes made to a port.
 * When count is 0 it means no changes was made.
 * When count is negative it means one or more ports connectors was removed
 * When count is positive it means one or more port connectors has been added
 */
typedef struct apx_port_connector_change_entry_tag
{
   int32_t count; //initial value is 0. When in negative range it holds port disconnect info. When in positive range it holds port connect info.
   union portref_union_tag
   {
      apx_port_instance_t* port_instance; //Applies when -1 <= count <= 1
      adt_ary_t *array; //Applies when count<-1 or when count > 1
   } data;
   //All references to apx_port_ref_t are weak references
} apx_port_connector_change_entry_t;

typedef apx_error_t (apx_port_connector_change_entry_action_func_t)(apx_port_connector_change_entry_t *self, apx_port_instance_t* port_instance);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_port_connector_change_entry_create(apx_port_connector_change_entry_t *self);
void apx_port_connector_change_entry_destroy(apx_port_connector_change_entry_t *self);
apx_port_connector_change_entry_t *apx_port_connector_change_entry_new(void);
void apx_port_connector_change_entry_delete(apx_port_connector_change_entry_t *self);
apx_error_t apx_port_connector_change_entry_add_connection(apx_port_connector_change_entry_t *self, apx_port_instance_t* port_instance);
apx_error_t apx_port_connector_change_entry_remove_connection(apx_port_connector_change_entry_t *self, apx_port_instance_t* port_instance);
apx_port_instance_t* apx_port_connector_change_entry_get(apx_port_connector_change_entry_t *self, int32_t index);
int32_t apx_port_connector_change_entry_count(apx_port_connector_change_entry_t *self);

#endif //APX_PORT_CONNECTION_ENTRY_H
