/*****************************************************************************
* \file      server_connection.h
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Base class for all APX server connections
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SERVER_CONNECTION_BASE_H
#define APX_SERVER_CONNECTION_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include "osmacro.h"
#include "apx/event_listener.h"
#include "apx/connection_base.h"
#include "adt_list.h"
#include "adt_str.h"
#include "apx/event_listener.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_server_tag;

typedef struct apx_server_connection_tag
{
   apx_connection_base_t base;
   struct apx_server_tag *parent;
   adt_str_t *tag; //optional tag
   apx_connection_state_t connection_state;
   apx_error_t last_error;
   MUTEX_T event_listener_lock;
   adt_list_t event_listeners;  //strong references to apx_connection_event_listener_t
} apx_server_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_server_connection_create(apx_server_connection_t* self, apx_connection_base_vtable_t* base_connection_vtable, apx_connection_interface_t* connection_interface);
void apx_server_connection_destroy(apx_server_connection_t* self);
//This class has no delete functions as it is an abstract base class
void apx_server_connection_greeting_header_accepted_notification(apx_server_connection_t* self);
void apx_server_connection_connected_notification(apx_server_connection_t* self);
apx_error_t apx_server_connection_disconnected_notification(apx_server_connection_t* self);
void apx_server_connection_attach_node_manager(apx_server_connection_t* self, apx_node_manager_t* node_manager);
apx_node_manager_t* apx_server_connection_get_node_manager(apx_server_connection_t* self);
void apx_server_connection_require_port_data_written(apx_server_connection_t* self, apx_node_instance_t* node_instance, apx_size_t offset, apx_size_t size);
apx_error_t apx_server_connection_attach_node_instance(apx_server_connection_t* self, apx_node_instance_t* node_instance);
int apx_server_connection_on_data_received(apx_server_connection_t* self, uint8_t const* data, apx_size_t data_size, apx_size_t* parse_len, apx_size_t* msg_size_hint);
void apx_server_connection_vnode_created_notification(void* arg, apx_node_instance_t* node_instance);
void apx_server_connection_set_connection_id(apx_server_connection_t* self, uint32_t connection_id);
uint32_t apx_server_connection_get_connection_id(apx_server_connection_t* self);
void apx_server_connection_set_server(apx_server_connection_t* self, struct apx_server_tag* server);
struct apx_server_tag* apx_server_connection_get_server(apx_server_connection_t* self);
void* apx_server_connection_register_event_listener(apx_server_connection_t* self, apx_server_connection_event_listener_t* event_listener);
void apx_server_connection_unregister_event_listener(apx_server_connection_t* self, void* handle);
void apx_server_connection_set_connection_type(apx_server_connection_t* self, apx_connection_type_t connection_type);
apx_connection_type_t apx_server_connection_get_connection_type(apx_server_connection_t const* self);
void apx_server_connection_set_num_header_size(apx_server_connection_t* self, apx_size_t size);
apx_size_t apx_server_connection_get_num_header_size(apx_server_connection_t const* self);
void apx_server_connection_set_rmf_proto_id(apx_server_connection_t* self, rmf_version_id_t version_id);
rmf_version_id_t apx_server_connection_get_rmf_proto_id(apx_server_connection_t const* self);
apx_connection_state_t apx_server_connection_get_connection_state(apx_server_connection_t const* self);
void apx_server_connection_set_tag(apx_server_connection_t* self, char const* tag);
adt_str_t* apx_server_connection_get_tag(apx_server_connection_t const* self); //It's the callers responsibility to dispose of the returned string object


// ServerConnection API
apx_file_manager_t* apx_server_connection_get_file_manager(apx_server_connection_t* self);
void apx_server_connection_start(apx_server_connection_t* self);
void apx_server_connection_close(apx_server_connection_t* self);
uint32_t apx_server_connection_get_total_bytes_received(apx_server_connection_t* self);
uint32_t apx_server_connection_get_total_bytes_sent(apx_server_connection_t* self);


//ConnectionInterface API
apx_error_t apx_server_connection_vremote_file_published_notification(void* arg, apx_file_t* file);
apx_error_t apx_server_connection_vremote_file_write_notification(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);

//Internal Event API (Called asynchronously from server event thread)
void apx_server_connection_process_protocol_header_accepted_event(apx_server_connection_t* self);
void apx_server_connection_process_remote_file_published_event(apx_server_connection_t* self, rmf_file_info_t *file_info);

/*** UNIT TEST API ***/
#ifdef UNIT_TEST
void apx_server_connection_run(apx_server_connection_t *self);


#endif

#endif //APX_SERVER_CONNECTION_BASE_H
