/*****************************************************************************
* \file      connection_base.h
* \author    Conny Gustafsson
* \date      2018-12-09
* \brief     Base class for all connections (client and server)
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_CONNECTION_BASE_H
#define APX_CONNECTION_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
//# include <semaphore.h>
#endif
#include "apx/types.h"
#include "apx/error.h"
#include "apx/file_manager.h"
#include "apx/node_manager.h"
//#include "apx/event_loop.h"
#include "apx/allocator.h"
#include "apx/connection_interface.h"
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_node_data_tag;
struct apx_port_connection_table_tag;
struct apx_file_tag;
struct rmf_file_info_tag;

typedef void (apx_file_info_notify_func_t)(void *arg, const struct rmf_file_info_tag *file_info);
typedef void (apx_node_file_write_notify_func_t)(void *arg, apx_node_instance_t * node_instance, apx_file_type_t file_type, uint32_t offset, const uint8_t *data, uint32_t len);
typedef void (apx_node_file_open_notify_func_t)(void *arg, apx_node_instance_t * node_instance, apx_file_type_t file_type);
typedef void (apx_port_connector_change_create_notify_func_t)(void *arg, apx_node_instance_t * node_instance, apx_port_type_t port_type);
typedef void (apx_node_created_func_t)(void* arg, apx_node_instance_t* node_instance);
typedef void (apx_require_port_write_notification_func_t)(void* arg, apx_port_instance_t* port_instance, uint8_t const* data, apx_size_t size);

typedef struct apx_connection_base_vtable_tag
{
   apx_void_ptr_func_t* destructor;
   apx_void_ptr_func_t* start;
   apx_void_ptr_func_t* close;
   apx_node_created_func_t* node_created_notification;
   apx_port_connector_change_create_notify_func_t* port_connector_change_notify;
   apx_require_port_write_notification_func_t* require_port_write_notification;
} apx_connection_base_vtable_t;

typedef struct apx_connection_base_tag
{
   apx_file_manager_t file_manager;
   apx_node_manager_t* node_manager;
   apx_allocator_t allocator;
   apx_connection_base_vtable_t vtable;
   apx_connection_interface_t connection_interface;
   uint32_t total_bytes_received;
   uint32_t total_bytes_sent;
   uint32_t connection_id;
   apx_size_t num_header_size; //UINT16_SIZE or UINT32_SIZE
   apx_mode_t mode;
   rmf_version_id_t rmf_version_id; //Remotefile protocol version ID
   apx_connection_type_t connection_type;

#ifdef _WIN32
   unsigned int thread_id;
#endif
} apx_connection_base_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connection_base_vtable_create(apx_connection_base_vtable_t *self, apx_void_ptr_func_t *destructor, apx_void_ptr_func_t *start, apx_void_ptr_func_t *close);
apx_error_t apx_connection_base_create(apx_connection_base_t *self, apx_mode_t mode, apx_connection_base_vtable_t* base_connection_vtable, apx_connection_interface_t* connection_interface);
void apx_connection_base_destroy(apx_connection_base_t *self);
void apx_connection_base_delete(apx_connection_base_t *self);
void apx_connection_base_vdelete(void *arg);
apx_file_manager_t *apx_connection_base_get_file_manager(apx_connection_base_t const* self);
void apx_connection_base_start(apx_connection_base_t *self);
void apx_connection_base_stop(apx_connection_base_t *self);
void apx_connection_base_close(apx_connection_base_t *self);
void apx_connection_base_attach_node_manager(apx_connection_base_t* self, apx_node_manager_t* node_manager);
apx_node_manager_t* apx_connection_base_get_node_manager(apx_connection_base_t const* self);
apx_connection_interface_t const* apx_connection_base_get_connection(apx_connection_base_t const* self);
apx_error_t apx_connection_base_message_received(apx_connection_base_t *self, const uint8_t *data, apx_size_t size);
uint16_t apx_connection_base_get_num_pending_worker_commands(apx_connection_base_t *self);
void apx_connection_base_set_connection_id(apx_connection_base_t* self, uint32_t connection_id);
uint32_t apx_connection_base_get_connection_id(apx_connection_base_t const* self);
void apx_connection_base_set_connection_type(apx_connection_base_t* self, apx_connection_type_t connection_type);
apx_connection_type_t apx_connection_base_get_connection_type(apx_connection_base_t const* self);
void apx_connection_base_set_num_header_size(apx_connection_base_t* self, apx_size_t size);
apx_size_t apx_connection_base_get_num_header_size(apx_connection_base_t const* self);
void apx_connection_base_set_rmf_proto_id(apx_connection_base_t* self, rmf_version_id_t version_id);
rmf_version_id_t apx_connection_base_get_rmf_proto_id(apx_connection_base_t const* self);

//uint8_t *apx_connection_base_alloc(apx_connection_base_t *self, size_t size);
//void apx_connection_base_free(apx_connection_base_t *self, uint8_t *ptr, size_t size);


//Virtual function call-points
void apx_connection_base_node_created_notification(apx_connection_base_t const* self, apx_node_instance_t* node_instance);
void apx_connection_base_require_port_write_notification(apx_connection_base_t const* self, apx_port_instance_t* port_instance, uint8_t const* raw_data, apx_size_t data_size);
uint32_t apx_connection_base_vget_connection_id(void* arg);
rmf_version_id_t apx_connection_base_vget_remotefile_protocol_version_id(void* arg);
apx_connection_type_t apx_connection_base_vget_connection_type(void* arg);

/*** Internal Callback API ***/

//Callbacks triggered due to events happening locally
void apx_connection_base_disconnect_notification(apx_connection_base_t *self);


#ifdef UNIT_TEST
void apx_connection_base_run(apx_connection_base_t *self);
#endif


#endif //APX_CONNECTION_BASE_H
