/*****************************************************************************
* \file      node_instance.h
* \author    Conny Gustafsson
* \date      2019-12-02
* \brief     Parent container for all things node-related.
*
* Copyright (c) 2021 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
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
struct apx_nodeManager_tag;
struct apx_fileManager_tag;
struct apx_server_tag;

typedef struct apx_nodeInstance_tag
{
   char* name;
   apx_size_t num_provide_ports;
   apx_size_t num_require_ports;
   apx_size_t num_data_elements;
   apx_size_t num_computation_lists;
   apx_size_t provide_port_init_data_size;
   apx_size_t require_port_init_data_size;
   apx_portInstance_t* provide_ports; //Length: num_provide_ports
   apx_portInstance_t* require_ports; //Length: num_require_ports
   apx_dataElement_t** data_elements; //Length: num_data_elements
   apx_computationList_t** computation_lists; //Length: num_computation_lists
   uint8_t* require_port_init_data; //Calculated init data for requirePorts
   uint8_t* provide_port_init_data; //Calculated init data for providePorts
   apx_nodeData_t *node_data; //All dynamic data in a node, things that change during runtime (strong reference)
   apx_portConnectorList_t *connector_table; //Array of apx_portConnectorList_t; Length of array: info->numProvidePorts. Created using a single malloc. Only used in server mode.
   apx_bytePortMap_t* byte_port_map; //context of this is mode dependent.
   apx_portConnectorChangeTable_t *require_port_changes; //temporary data structure used for tracking port connector changes to requirePorts
   apx_portConnectorChangeTable_t *provide_port_changes; //temporary data structure used for tracking port connector changes to providePorts
   apx_mode_t mode;
   apx_dataState_t definition_data_state;
   apx_dataState_t require_port_data_state;
   apx_dataState_t provide_port_data_state;
   struct apx_nodeManager_tag* parent;
   struct apx_server_tag *server; //Only used in APX_SERVER_MODE
   apx_file_t* definition_file; //Weak reference
   apx_file_t* provide_port_data_file; //Weak reference
   apx_file_t* require_port_data_file; //Weak reference
   MUTEX_T lock;
} apx_nodeInstance_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeInstance_create(apx_nodeInstance_t *self, apx_mode_t mode, char const* name);
void apx_nodeInstance_destroy(apx_nodeInstance_t *self);
apx_nodeInstance_t *apx_nodeInstance_new(apx_mode_t mode, char const* name);
void apx_nodeInstance_delete(apx_nodeInstance_t *self);
void apx_nodeInstance_vdelete(void *arg);

char const* apx_nodeInstance_get_name(apx_nodeInstance_t const* self);
apx_size_t apx_nodeInstance_get_num_data_elements(apx_nodeInstance_t const* self);
apx_size_t apx_nodeInstance_get_num_computation_lists(apx_nodeInstance_t const* self);
apx_size_t apx_nodeInstance_get_num_provide_ports(apx_nodeInstance_t const* self);
apx_size_t apx_nodeInstance_get_num_require_ports(apx_nodeInstance_t const* self);
apx_size_t apx_nodeInstance_get_provide_port_init_data_size(apx_nodeInstance_t const* self);
apx_size_t apx_nodeInstance_get_require_port_init_data_size(apx_nodeInstance_t const* self);
uint8_t const* apx_nodeInstance_get_provide_port_init_data(apx_nodeInstance_t const* self);
uint8_t const* apx_nodeInstance_get_require_port_init_data(apx_nodeInstance_t const* self);
apx_portInstance_t* apx_nodeInstance_get_provide_port(apx_nodeInstance_t const* self, apx_portId_t port_id);
apx_portInstance_t* apx_nodeInstance_get_require_port(apx_nodeInstance_t const* self, apx_portId_t port_id);
apx_dataElement_t const* apx_nodeInstance_get_data_element(apx_nodeInstance_t const* self, apx_elementId_t id);
apx_computationList_t const* apx_nodeInstance_get_computation_list(apx_nodeInstance_t* self, apx_computationListId_t id);
apx_error_t apx_nodeInstance_alloc_port_instance_memory(apx_nodeInstance_t* self, apx_size_t num_provide_ports,apx_size_t num_require_ports);
apx_error_t apx_nodeInstance_create_provide_port(apx_nodeInstance_t* self, apx_portId_t port_id, char const* name,
   apx_program_t const* pack_program, uint32_t data_offset, uint32_t* data_size);
apx_error_t apx_nodeInstance_create_require_port(apx_nodeInstance_t* self, apx_portId_t port_id, char const* name,
   apx_program_t const* pack_program, apx_program_t const* unpack_program, uint32_t data_offset, uint32_t* data_size);
apx_error_t apx_nodeInstance_alloc_init_data_memory(apx_nodeInstance_t* self, uint8_t** provide_port_data,
   apx_size_t* provide_port_data_size, uint8_t** require_port_data, apx_size_t* require_port_data_size);
apx_error_t apx_nodeInstance_init_node_data(apx_nodeInstance_t* self, uint8_t const* definition_data, apx_size_t definition_size);
apx_error_t apx_nodeInstance_finalize_node_data(apx_nodeInstance_t* self);
bool apx_nodeInstance_has_provide_port_data(apx_nodeInstance_t const* self);
bool apx_nodeInstance_has_require_port_data(apx_nodeInstance_t const* self);
apx_nodeData_t const* apx_nodeInstance_get_const_node_data(apx_nodeInstance_t const* self);
apx_nodeData_t* apx_nodeInstance_get_node_data(apx_nodeInstance_t const* self);
bool apx_nodeInstance_has_node_data(apx_nodeInstance_t const* self);
apx_size_t apx_nodeInstance_get_definition_size(apx_nodeInstance_t const* self);
uint8_t const* apx_nodeInstance_get_definition_data(apx_nodeInstance_t const* self);
apx_error_t apx_nodeInstance_create_data_element_list(apx_nodeInstance_t* self, adt_ary_t* data_element_list);
apx_error_t apx_nodeInstance_create_computation_lists(apx_nodeInstance_t* self, adt_ary_t* computation_lists);
apx_error_t apx_nodeInstance_create_byte_port_map(apx_nodeInstance_t* self);
apx_bytePortMap_t const* apx_nodeInstance_get_byte_port_map(apx_nodeInstance_t const* self);
apx_dataState_t apx_nodeInstance_get_definition_data_state(apx_nodeInstance_t const* self);
apx_dataState_t apx_nodeInstance_get_require_port_data_state(apx_nodeInstance_t const* self);
apx_dataState_t apx_nodeInstance_get_provide_port_data_state(apx_nodeInstance_t const* self);
void apx_nodeInstance_set_definition_data_state(apx_nodeInstance_t* self, apx_dataState_t state);
void apx_nodeInstance_set_require_port_data_state(apx_nodeInstance_t* self, apx_dataState_t state);
void apx_nodeInstance_set_provide_port_data_state(apx_nodeInstance_t* self, apx_dataState_t state);
void apx_nodeInstance_set_parent(apx_nodeInstance_t* self, struct apx_nodeManager_tag* parent);
struct apx_nodeManager_tag* apx_nodeInstance_get_parent(apx_nodeInstance_t const* self);
apx_portId_t apx_nodeInstance_lookup_require_port_id(apx_nodeInstance_t const* self, apx_size_t byte_offset);
apx_portId_t apx_nodeInstance_lookup_provide_port_id(apx_nodeInstance_t const* self, apx_size_t byte_offset);
apx_portInstance_t* apx_nodeInstance_find(apx_nodeInstance_t const* self, char const* name);
apx_error_t apx_nodeInstance_attach_to_file_manager(apx_nodeInstance_t* self, struct apx_fileManager_tag* file_manager);
apx_error_t apx_nodeInstance_remote_file_published_notification(apx_nodeInstance_t* self, apx_file_t* file);
void apx_nodeInstance_set_server(apx_nodeInstance_t* self, struct apx_server_tag* server);
apx_portInstance_t* apx_nodeInstance_find_port_by_name(apx_nodeInstance_t const* self, char const* name);
apx_error_t apx_nodeInstance_write_provide_port_data(apx_nodeInstance_t* self, apx_size_t offset, uint8_t* data, apx_size_t size);

// FileNotificationHandler API
apx_error_t apx_nodeInstance_vfile_open_notify(void* arg, apx_file_t* file);
apx_error_t apx_nodeInstance_vfile_close_notify(void* arg, apx_file_t* file);
apx_error_t apx_nodeInstance_vfile_write_notify(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);

// Port Connector Change API
apx_portConnectorChangeTable_t* apx_nodeInstance_get_require_port_connector_changes(apx_nodeInstance_t* self, bool auto_create);
apx_portConnectorChangeTable_t* apx_nodeInstance_get_provide_port_connector_changes(apx_nodeInstance_t* self, bool auto_create);
void apx_nodeInstance_clear_require_port_connector_changes(apx_nodeInstance_t* self, bool release_memory);
void apx_nodeInstance_clear_provide_port_connector_changes(apx_nodeInstance_t* self, bool release_memory);
apx_error_t apx_nodeInstance_handle_require_ports_disconnected(apx_nodeInstance_t* self, apx_portConnectorChangeTable_t* connector_changes);

// Data Routing API
apx_error_t apx_nodeInstance_handle_require_port_connected_to_provide_port(apx_portInstance_t* require_port, apx_portInstance_t* provide_port);
apx_error_t apx_nodeInstance_handle_provide_port_connected_to_require_port(apx_portInstance_t* provide_port, apx_portInstance_t* require_port);
apx_error_t apx_nodeInstance_handle_require_port_disconnected_from_provide_port(apx_portInstance_t* require_port, apx_portInstance_t* provide_port);
//apx_error_t apx_nodeInstance_send_require_port_data_to_file_manager(apx_nodeInstance_t* self);

//apx_error_t apx_nodeInstance_route_provide_port_data_change_to_receivers(apx_nodeInstance_t* self, const uint8_t* src, uint32_t offset, apx_size_t len);

// ConnectorTable API
apx_error_t apx_nodeInstance_build_connector_table(apx_nodeInstance_t* self);
void apx_nodeInstance_lock_port_connector_table(apx_nodeInstance_t* self);
void apx_nodeInstance_unlock_port_connector_table(apx_nodeInstance_t* self);
apx_portConnectorList_t* apx_nodeInstance_get_connectors_on_provide_port(apx_nodeInstance_t* self, apx_portId_t port_id);
//apx_error_t apx_nodeInstance_insert_provide_port_connector(apx_nodeInstance_t* self, apx_portId_t provide_port_id, apx_portInstance_t* require_port);
//apx_error_t apx_nodeInstance_remove_provide_port_connector(apx_nodeInstance_t* self, apx_portId_t provide_port_id, apx_portInstance_t* require_port);
void apx_nodeInstance_clear_connector_table(apx_nodeInstance_t* self);

#endif //APX_NODE_INSTANCE_H
