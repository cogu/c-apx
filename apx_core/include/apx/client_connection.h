/*****************************************************************************
* \file      client_connection.h
* \author    Conny Gustafsson
* \date      2018-12-31
* \brief     Abstract base class for client connections.
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_CLIENT_CONNECTION_BASE_H
#define APX_CLIENT_CONNECTION_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/connection_base.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_client_tag;
struct apx_nodeInstance_tag;

typedef struct apx_clientConnection_tag
{
   apx_connectionBase_t base;
   struct apx_client_tag *client;
   bool is_greeting_accepted;
   apx_error_t last_error;
} apx_clientConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientConnection_create(apx_clientConnection_t *self, apx_connectionBaseVTable_t* base_connection_vtable, apx_connectionInterface_t* connection_interface);
void apx_clientConnection_destroy(apx_clientConnection_t *self);
//This class has no delete functions as it is an abstract base class
void apx_clientConnection_greeting_header_accepted_notification(apx_clientConnection_t* self);
void apx_clientConnection_connected_notification(apx_clientConnection_t* self);
void apx_clientConnection_disconnected_notification(apx_clientConnection_t* self);
void apx_clientConnection_attach_node_manager(apx_clientConnection_t* self, apx_nodeManager_t* node_manager);
apx_nodeManager_t* apx_clientConnection_get_node_manager(apx_clientConnection_t* self);
void apx_clientConnection_require_port_data_written(apx_clientConnection_t* self, apx_nodeInstance_t* node_instance, apx_size_t offset, apx_size_t size);
void apx_clientConnection_set_client(apx_clientConnection_t* self, struct apx_client_tag* client);
void apx_clientConnection_set_connection_type(apx_clientConnection_t* self, apx_connectionType_t connection_type);
apx_connectionType_t apx_clientConnection_get_connection_type(apx_clientConnection_t const* self);
int apx_clientConnection_on_data_received(apx_clientConnection_t* self, uint8_t const* data, apx_size_t data_size, apx_size_t* parse_len, apx_size_t* msg_size_hint);
apx_error_t apx_clientConnection_attach_node_instance(apx_clientConnection_t* self, apx_nodeInstance_t* node_instance);
void apx_clientConnection_set_rmf_proto_id(apx_clientConnection_t* self, rmf_versionId_t version_id);
rmf_versionId_t apx_clientConnection_get_rmf_proto_id(apx_clientConnection_t* self);

// ClientConnection API
apx_fileManager_t *apx_clientConnection_get_file_manager(apx_clientConnection_t *self);
void apx_clientConnection_start(apx_clientConnection_t *self);
void apx_clientConnection_close(apx_clientConnection_t *self);
uint32_t apx_clientConnection_get_total_bytes_received(apx_clientConnection_t *self);
uint32_t apx_clientConnection_get_total_bytes_sent(apx_clientConnection_t *self);
void apx_clientConnection_vrequire_port_write_notification(void* arg, apx_portInstance_t* port_instance, uint8_t const* data, apx_size_t size);

//ConnectionInterface API
apx_error_t apx_clientConnection_vremote_file_published_notification(void* arg, apx_file_t* file);
apx_error_t apx_clientConnection_vremote_file_write_notification(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);


// Unit Test API
#ifdef UNIT_TEST
void apx_clientConnection_run(apx_clientConnection_t *self);
#endif


#endif //APX_CLIENT_CONNECTION_BASE_H
