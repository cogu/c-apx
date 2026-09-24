/*****************************************************************************
* \file      node_instance.h
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Parent container for all things node-related.
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_NODE_INSTANCE_H
#define APX_NODE_INSTANCE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_ary.h"
#include "apx/byte_port_map.h"
#include "apx/types.h"
#include "apx/types.h"
#include "apx/port_instance.h"
#include "apx/error.h"
#include "apx/program.h"
#include "apx/node_data.h"
#include "apx/data_element.h"
#include "apx/computation.h"
#include "apx/byte_port_map.h"
#include "apx/port_connector_list.h"
#include "apx/file.h"
#include "apx/port_connector_change_table.h"
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_node_manager_tag;
struct apx_file_manager_tag;
struct apx_server_tag;

typedef struct apx_node_instance_tag
{
   char* name;
   apx_size_t num_provide_ports;
   apx_size_t num_require_ports;
   apx_size_t num_data_elements;
   apx_size_t num_computation_lists;
   apx_size_t provide_port_init_data_size;
   apx_size_t require_port_init_data_size;
   apx_port_instance_t* provide_ports; //Length: num_provide_ports
   apx_port_instance_t* require_ports; //Length: num_require_ports
   apx_data_element_t** data_elements; //Length: num_data_elements
   apx_computation_list_t** computation_lists; //Length: num_computation_lists
   uint8_t* require_port_init_data; //Calculated init data for requirePorts
   uint8_t* provide_port_init_data; //Calculated init data for providePorts
   apx_node_data_t *node_data; //All dynamic data in a node, things that change during runtime (strong reference)
   apx_port_connector_list_t *connector_table; //Array of apx_port_connector_list_t; Length of array: info->numProvidePorts. Created using a single malloc. Only used in server mode.
   apx_byte_port_map_t* provide_byte_port_map; //Used in APX_SERVER_MODE, APX_MONITOR_MODE
   apx_byte_port_map_t* require_byte_port_map; //Used in APX_CLIENT_MODE, APX_MONITOR_MODE
   apx_port_connector_change_table_t *require_port_changes; //temporary data structure used for tracking port connector changes to requirePorts
   apx_port_connector_change_table_t *provide_port_changes; //temporary data structure used for tracking port connector changes to providePorts
   apx_mode_t mode;
   apx_data_state_t definition_data_state;
   apx_data_state_t require_port_data_state;
   apx_data_state_t provide_port_data_state;
   struct apx_node_manager_tag* parent;
   struct apx_server_tag *server; //Only used in APX_SERVER_MODE
   apx_file_t* definition_file; //Weak reference
   apx_file_t* provide_port_data_file; //Weak reference
   apx_file_t* require_port_data_file; //Weak reference
   apx_file_t* provide_port_count_file; //Weak reference
   apx_file_t* require_port_count_file; //Weak reference
   int32_t major_version; // APX version of the associated node (major)
   int32_t minor_version; // APX version of the associated node (minor)
   MUTEX_T lock;
} apx_node_instance_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_node_instance_create(apx_node_instance_t *self, apx_mode_t mode, char const* name);
void apx_node_instance_destroy(apx_node_instance_t *self);
apx_node_instance_t *apx_node_instance_new(apx_mode_t mode, char const* name);
void apx_node_instance_delete(apx_node_instance_t *self);
void apx_node_instance_vdelete(void *arg);

char const* apx_node_instance_get_name(apx_node_instance_t const* self);
apx_size_t apx_node_instance_get_num_data_elements(apx_node_instance_t const* self);
apx_size_t apx_node_instance_get_num_computation_lists(apx_node_instance_t const* self);
apx_size_t apx_node_instance_get_num_provide_ports(apx_node_instance_t const* self);
apx_size_t apx_node_instance_get_num_require_ports(apx_node_instance_t const* self);
apx_size_t apx_node_instance_get_provide_port_init_data_size(apx_node_instance_t const* self);
apx_size_t apx_node_instance_get_require_port_init_data_size(apx_node_instance_t const* self);
uint8_t const* apx_node_instance_get_provide_port_init_data(apx_node_instance_t const* self);
uint8_t const* apx_node_instance_get_require_port_init_data(apx_node_instance_t const* self);
apx_port_instance_t* apx_node_instance_get_provide_port(apx_node_instance_t const* self, apx_port_id_t port_id);
apx_port_instance_t* apx_node_instance_get_require_port(apx_node_instance_t const* self, apx_port_id_t port_id);
apx_data_element_t const* apx_node_instance_get_data_element(apx_node_instance_t const* self, apx_element_id_t id);
apx_computation_list_t const* apx_node_instance_get_computation_list(apx_node_instance_t* self, apx_computation_list_id_t id);
apx_error_t apx_node_instance_alloc_port_instance_memory(apx_node_instance_t* self, apx_size_t num_provide_ports,apx_size_t num_require_ports);
apx_error_t apx_node_instance_create_provide_port(apx_node_instance_t* self, apx_port_id_t port_id, char const* name,
   apx_program_t const* pack_program, uint32_t data_offset, uint32_t* data_size);
apx_error_t apx_node_instance_create_require_port(apx_node_instance_t* self, apx_port_id_t port_id, char const* name,
   apx_program_t const* pack_program, apx_program_t const* unpack_program, uint32_t data_offset, uint32_t* data_size);
apx_error_t apx_node_instance_alloc_init_data_memory(apx_node_instance_t* self, uint8_t** provide_port_data,
   apx_size_t* provide_port_data_size, uint8_t** require_port_data, apx_size_t* require_port_data_size);
apx_error_t apx_node_instance_init_node_data(apx_node_instance_t* self, uint8_t const* definition_data, apx_size_t definition_size);
apx_error_t apx_node_instance_finalize_node_data(apx_node_instance_t* self);
bool apx_node_instance_has_provide_port_data(apx_node_instance_t const* self);
bool apx_node_instance_has_require_port_data(apx_node_instance_t const* self);
bool apx_node_instance_has_provide_port_count_data(apx_node_instance_t const* self);
bool apx_node_instance_has_require_port_count_data(apx_node_instance_t const* self);
apx_node_data_t const* apx_node_instance_get_const_node_data(apx_node_instance_t const* self);
apx_node_data_t* apx_node_instance_get_node_data(apx_node_instance_t const* self);
bool apx_node_instance_has_node_data(apx_node_instance_t const* self);
apx_size_t apx_node_instance_get_definition_size(apx_node_instance_t const* self);
uint8_t const* apx_node_instance_get_definition_data(apx_node_instance_t const* self);
apx_error_t apx_node_instance_create_data_element_list(apx_node_instance_t* self, adt_ary_t* data_element_list);
apx_error_t apx_node_instance_create_computation_lists(apx_node_instance_t* self, adt_ary_t* computation_lists);
apx_error_t apx_node_instance_create_byte_port_map(apx_node_instance_t* self);
apx_byte_port_map_t const* apx_node_instance_get_provide_byte_port_map(apx_node_instance_t const* self);
apx_byte_port_map_t const* apx_node_instance_get_require_byte_port_map(apx_node_instance_t const* self);
apx_data_state_t apx_node_instance_get_definition_data_state(apx_node_instance_t const* self);
apx_data_state_t apx_node_instance_get_require_port_data_state(apx_node_instance_t const* self);
apx_data_state_t apx_node_instance_get_provide_port_data_state(apx_node_instance_t const* self);
void apx_node_instance_set_definition_data_state(apx_node_instance_t* self, apx_data_state_t state);
void apx_node_instance_set_require_port_data_state(apx_node_instance_t* self, apx_data_state_t state);
void apx_node_instance_set_provide_port_data_state(apx_node_instance_t* self, apx_data_state_t state);
void apx_node_instance_set_parent(apx_node_instance_t* self, struct apx_node_manager_tag* parent);
struct apx_node_manager_tag* apx_node_instance_get_parent(apx_node_instance_t const* self);
apx_port_id_t apx_node_instance_lookup_require_port_id(apx_node_instance_t const* self, apx_size_t byte_offset);
apx_port_id_t apx_node_instance_lookup_provide_port_id(apx_node_instance_t const* self, apx_size_t byte_offset);
apx_port_instance_t* apx_node_instance_find(apx_node_instance_t const* self, char const* name);
apx_error_t apx_node_instance_attach_to_file_manager(apx_node_instance_t* self, struct apx_file_manager_tag* file_manager);
apx_error_t apx_node_instance_remote_file_published_notification(apx_node_instance_t* self, apx_file_t* file);
void apx_node_instance_set_server(apx_node_instance_t* self, struct apx_server_tag* server);
apx_port_instance_t* apx_node_instance_find_port_by_name(apx_node_instance_t const* self, char const* name);
apx_error_t apx_node_instance_write_provide_port_data(apx_node_instance_t* self, apx_size_t offset, uint8_t* data, apx_size_t size);

// FileNotificationHandler API
apx_error_t apx_node_instance_vfile_open_notify(void* arg, apx_file_t* file);
apx_error_t apx_node_instance_vfile_close_notify(void* arg, apx_file_t* file);
apx_error_t apx_node_instance_vfile_write_notify(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);

// Port Connector Change API
apx_port_connector_change_table_t* apx_node_instance_get_require_port_connector_changes(apx_node_instance_t* self, bool auto_create);
apx_port_connector_change_table_t* apx_node_instance_get_provide_port_connector_changes(apx_node_instance_t* self, bool auto_create);
void apx_node_instance_clear_require_port_connector_changes(apx_node_instance_t* self, bool release_memory);
void apx_node_instance_clear_provide_port_connector_changes(apx_node_instance_t* self, bool release_memory);
apx_error_t apx_node_instance_handle_require_ports_disconnected(apx_node_instance_t* self, apx_port_connector_change_table_t* connector_changes);

// Data Routing API
apx_error_t apx_node_instance_handle_require_port_connected_to_provide_port(apx_port_instance_t* require_port, apx_port_instance_t* provide_port);
apx_error_t apx_node_instance_handle_provide_port_connected_to_require_port(apx_port_instance_t* provide_port, apx_port_instance_t* require_port);
apx_error_t apx_node_instance_handle_require_port_disconnected_from_provide_port(apx_port_instance_t* require_port, apx_port_instance_t* provide_port);
//apx_error_t apx_node_instance_send_require_port_data_to_file_manager(apx_node_instance_t* self);

//apx_error_t apx_node_instance_route_provide_port_data_change_to_receivers(apx_node_instance_t* self, const uint8_t* src, uint32_t offset, apx_size_t len);

// ConnectorTable API
apx_error_t apx_node_instance_build_connector_table(apx_node_instance_t* self);
void apx_node_instance_lock_port_connector_table(apx_node_instance_t* self);
void apx_node_instance_unlock_port_connector_table(apx_node_instance_t* self);
apx_port_connector_list_t* apx_node_instance_get_provide_port_connectors(apx_node_instance_t* self, apx_port_id_t port_id);
//apx_error_t apx_node_instance_insert_provide_port_connector(apx_node_instance_t* self, apx_port_id_t provide_port_id, apx_port_instance_t* require_port);
//apx_error_t apx_node_instance_remove_provide_port_connector(apx_node_instance_t* self, apx_port_id_t provide_port_id, apx_port_instance_t* require_port);
void apx_node_instance_clear_connector_table(apx_node_instance_t* self);

// Version API
void apx_node_instance_set_version(apx_node_instance_t* self, int32_t major_version, int32_t minor_version);
int32_t apx_node_instance_get_major_version(const apx_node_instance_t* self);
int32_t apx_node_instance_get_minor_version(const apx_node_instance_t* self);

// Port Count File API
apx_file_t* apx_node_instance_get_provide_port_count_file(const apx_node_instance_t* self);
apx_file_t* apx_node_instance_get_require_port_count_file(const apx_node_instance_t* self);
void apx_node_instance_set_provide_port_count_file(apx_node_instance_t* self, apx_file_t* file);
void apx_node_instance_set_require_port_count_file(apx_node_instance_t* self, apx_file_t* file);
apx_error_t apx_node_instance_send_provide_port_count_data(apx_node_instance_t* self, apx_port_id_t port_id, apx_port_count_t count);
apx_error_t apx_node_instance_send_require_port_count_data(apx_node_instance_t* self, apx_port_id_t port_id, apx_port_count_t count);
apx_port_count_t apx_node_instance_get_provide_port_connection_count(apx_node_instance_t const* self, apx_port_id_t port_id);
apx_port_count_t apx_node_instance_get_require_port_connection_count(apx_node_instance_t const* self, apx_port_id_t port_id);
apx_port_count_t apx_node_instance_get_port_connection_count(apx_node_instance_t const* self, apx_port_type_t port_type, apx_port_id_t port_id);

#endif //APX_NODE_INSTANCE_H
