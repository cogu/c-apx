/*****************************************************************************
* \file      port_signature_map_entry.h
* \author    Conny Gustafsson
* \date      2020-02-18
* \brief     An element in an apx_port_signature_map_t
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_PORT_SIGNATURE_MAP_ENTRY_H
#define APX_PORT_SIGNATURE_MAP_ENTRY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_list.h"
#include "apx/types.h"
#include "apx/port_instance.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_node_data_tag;
struct apx_port_tag;

typedef struct apx_port_signature_map_entry_tag
{
   apx_port_instance_t *preferred_provider;
   adt_list_t require_ports; //weak references to apx_port_instance_t
   adt_list_t provide_ports; //weak references to apx_port_instance_t
} apx_port_signature_map_entry_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_port_signature_map_entry_create(apx_port_signature_map_entry_t *self);
void apx_port_signature_map_entry_destroy(apx_port_signature_map_entry_t *self);
apx_port_signature_map_entry_t *apx_port_signature_map_entry_new(void);
void apx_port_signature_map_entry_delete(apx_port_signature_map_entry_t *self);
void apx_port_signature_map_entry_vdelete(void *arg);
void apx_port_signature_map_entry_attach_require_port(apx_port_signature_map_entry_t *self, apx_port_instance_t* port_instance);
void apx_port_signature_map_entry_attach_provide_port(apx_port_signature_map_entry_t *self, apx_port_instance_t* port_instance, bool is_preferred);
void apx_port_signature_map_entry_detach_require_port(apx_port_signature_map_entry_t *self, apx_port_instance_t* port_instance);
void apx_port_signature_map_entry_detach_provide_port(apx_port_signature_map_entry_t *self, apx_port_instance_t* port_instance);

bool apx_port_signature_map_entry_is_empty(apx_port_signature_map_entry_t *self);
int32_t apx_port_signature_map_entry_get_num_providers(apx_port_signature_map_entry_t* self);
int32_t apx_port_signature_map_entry_get_num_requesters(apx_port_signature_map_entry_t* self);
apx_port_instance_t*apx_port_signature_map_entry_get_first_provider(apx_port_signature_map_entry_t *self);
apx_port_instance_t*apx_port_signature_map_entry_get_last_provider(apx_port_signature_map_entry_t *self);
apx_port_instance_t* apx_port_signature_map_entry_get_first_requester(apx_port_signature_map_entry_t* self);
apx_port_instance_t* apx_port_signature_map_entry_get_last_requester(apx_port_signature_map_entry_t* self);
void apx_port_signature_map_entry_set_preferred_provider(apx_port_signature_map_entry_t *self, apx_port_instance_t* port_instance);
apx_port_instance_t*apx_port_signature_map_entry_get_preferred_provider(apx_port_signature_map_entry_t *self);
apx_error_t apx_port_signature_map_entry_notify_require_ports_about_provide_port_change(apx_port_signature_map_entry_t *self, apx_port_instance_t* provide_port, apx_port_connector_event_t event_type);
apx_error_t apx_port_signature_map_entry_notify_provide_ports_about_require_port_change(apx_port_signature_map_entry_t *self, apx_port_instance_t* require_port, apx_port_connector_event_t event_type);

#endif //APX_ROUTING_TABLE_ENTRY_H
