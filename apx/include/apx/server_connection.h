/*****************************************************************************
* \file      server_connection_base.h
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Base class for all APX server connections
*
* Copyright (c) 2018-2021 Conny Gustafsson
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
#include "apx/event_listener.h"
#include "apx/connection_base.h"
#include "adt_list.h"
#include "adt_str.h"
#include "apx/event_listener.h"
#include "osmacro.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_server_tag;

typedef struct apx_serverConnection_tag
{
   apx_connectionBase_t base;
   struct apx_server_tag *parent;
   adt_str_t *tag; //optional tag
   bool is_greeting_accepted;
   apx_error_t last_error;
   MUTEX_T event_listener_lock;
   adt_list_t event_listeners;  //strong references to apx_connectionEventListener_t
} apx_serverConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverConnection_create(apx_serverConnection_t* self, apx_connectionBaseVTable_t* base_connection_vtable, apx_connectionInterface_t* connection_interface);
void apx_serverConnection_destroy(apx_serverConnection_t* self);
//This class has no delete functions as it is an abstract base class
void apx_serverConnection_greeting_header_accepted_notification(apx_serverConnection_t* self);
void apx_serverConnection_connected_notification(apx_serverConnection_t* self);
apx_error_t apx_serverConnection_disconnected_notification(apx_serverConnection_t* self);
void apx_serverConnection_attach_node_manager(apx_serverConnection_t* self, apx_nodeManager_t* node_manager);
apx_nodeManager_t* apx_serverConnection_get_node_manager(apx_serverConnection_t* self);
void apx_serverConnection_require_port_data_written(apx_serverConnection_t* self, apx_nodeInstance_t* node_instance, apx_size_t offset, apx_size_t size);
apx_error_t apx_serverConnection_attach_node_instance(apx_serverConnection_t* self, apx_nodeInstance_t* node_instance);
int apx_serverConnection_on_data_received(apx_serverConnection_t* self, uint8_t const* data, apx_size_t data_size, apx_size_t* parse_len);
void apx_serverConnection_vnode_created_notification(void* arg, apx_nodeInstance_t* node_instance);
void apx_serverConnection_set_connection_id(apx_serverConnection_t* self, uint32_t connection_id);
uint32_t apx_serverConnection_get_connection_id(apx_serverConnection_t* self);
void apx_serverConnection_set_server(apx_serverConnection_t* self, struct apx_server_tag* server);
struct apx_server_tag* apx_serverConnection_get_server(apx_serverConnection_t* self);
void* apx_serverConnection_register_event_listener(apx_serverConnection_t* self, apx_connectionEventListener_t* event_listener);
void apx_serverConnection_unregister_event_listener(apx_serverConnection_t* self, void* handle);
void apx_serverConnection_set_connection_type(apx_serverConnection_t* self, apx_connectionType_t connection_type);
apx_connectionType_t apx_serverConnection_get_connection_type(apx_serverConnection_t const* self);
void apx_serverConnection_set_num_header_size(apx_serverConnection_t* self, apx_size_t size);
apx_size_t apx_serverConnection_get_num_header_size(apx_serverConnection_t const* self);
void apx_serverConnection_set_rmf_proto_id(apx_serverConnection_t* self, rmf_versionId_t version_id);
rmf_versionId_t apx_serverConnection_get_rmf_proto_id(apx_serverConnection_t const* self);


// ClientConnection API
apx_fileManager_t* apx_serverConnection_get_file_manager(apx_serverConnection_t* self);
void apx_serverConnection_start(apx_serverConnection_t* self);
void apx_serverConnection_close(apx_serverConnection_t* self);
uint32_t apx_serverConnection_get_total_bytes_received(apx_serverConnection_t* self);
uint32_t apx_serverConnection_get_total_bytes_sent(apx_serverConnection_t* self);


//ConnectionInterface API
apx_error_t apx_serverConnection_vremote_file_published_notification(void* arg, apx_file_t* file);
apx_error_t apx_serverConnection_vremote_file_write_notification(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);

//Internal Event API (Called asynchronously from server event thread)
void apx_server_connection_trigger_protocol_header_accepted(apx_serverConnection_t* self);
void apx_server_connection_trigger_remote_file_published(apx_serverConnection_t* self, rmf_fileInfo_t *file_info);

/*** UNIT TEST API ***/
#ifdef UNIT_TEST
void apx_serverConnection_run(apx_serverConnection_t *self);
apx_error_t apx_serverConnection_on_remote_file_published(apx_serverConnection_t* self, const rmf_fileInfo_t* file_info);

#endif

#endif //APX_SERVER_CONNECTION_BASE_H
