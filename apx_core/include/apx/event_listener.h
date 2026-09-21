/*****************************************************************************
* \file      event_listener.h
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Event listener API
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_EVENT_LISTENER_H
#define APX_EVENT_LISTENER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"

//forward declarations
struct apx_server_connection_tag;
struct apx_client_connection_tag;
struct apx_port_connection_table_tag;
struct rmf_file_info_tag;
struct apx_file_info_tag;
struct apx_file_tag;
struct apx_connection_base_tag;
struct apx_node_instance_tag;
struct apx_port_instance_tag;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//Client/Server typedefs
typedef void (apx_client_connection_event_func_t)(void* arg, struct apx_client_connection_tag* connection);
typedef void (apx_server_connection_event_func_t)(void* arg, struct apx_server_connection_tag* connection);
typedef void (apx_server_log_write_event_func_t)(void* arg, apx_log_level_t level, const char* label, const char* msg);

//Connection typedefs
typedef void (apx_protocol_header_accepted_func_t)(void* arg, struct apx_connection_base_tag* connection);
typedef void (apx_port_data_write_func_t)(void* arg, struct apx_port_instance_tag* port_instance, uint8_t const* data, apx_size_t size);
typedef void (apx_file_event_func_t)(void* arg, struct apx_connection_base_tag* connection, const struct rmf_file_info_tag* file_info);

typedef struct apx_client_event_listener_tag
{
   void *arg;
   apx_client_connection_event_func_t* connected;
   apx_client_connection_event_func_t* disconnected;
   apx_port_data_write_func_t* require_port_write;
} apx_client_event_listener_t;

typedef struct apx_server_event_listener_tag
{
   void *arg;
   apx_server_connection_event_func_t* new_connection;
   apx_server_connection_event_func_t* connection_closed;
   apx_server_log_write_event_func_t* server_write_log;
} apx_server_event_listener_t;

typedef struct apx_server_connection_event_listener_tag
{
   void *arg;
   apx_protocol_header_accepted_func_t* protocol_header_accepted;
   apx_file_event_func_t* file_published;
   apx_file_event_func_t* file_revoked;
} apx_server_connection_event_listener_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_client_event_listener_t *apx_client_event_listener_clone(apx_client_event_listener_t *other);
void apx_client_event_listener_delete(apx_client_event_listener_t *self);
void apx_client_event_listener_vdelete(void *arg);

apx_server_event_listener_t *apx_server_event_listener_clone(apx_server_event_listener_t *other);
void apx_server_event_listener_delete(apx_server_event_listener_t *self);
void apx_server_event_listener_vdelete(void *arg);

apx_server_connection_event_listener_t *apx_connection_event_listener_clone(apx_server_connection_event_listener_t *other);
void apx_connection_event_listener_delete(apx_server_connection_event_listener_t *self);
void apx_connection_event_listener_vdelete(void *arg);



#endif //APX_EVENT_LISTENER_H
