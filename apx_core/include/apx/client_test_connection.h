/*****************************************************************************
* \file      client_test_connection.h
* \author    Conny Gustafsson
* \date      2018-01-15
* \brief     Unit Test connection for APX clients
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_CLIENT_TEST_CONNECTION_H
#define APX_CLIENT_TEST_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdbool.h>
#include "apx/error.h"
#include "apx/client_connection.h"
#include "apx/file_info.h"
#include "adt_bytearray.h"
#include "adt_ary.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations


typedef struct apx_client_test_connection_tag
{
   apx_client_connection_t base;
   adt_ary_t * transmit_log; //strong references to adt_bytearray_t
   adt_bytearray_t transmit_buffer;
   apx_size_t default_buffer_size;
   apx_size_t pending_bytes;
   apx_node_manager_t node_manager;
} apx_client_test_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
apx_error_t apx_client_test_connection_create(apx_client_test_connection_t *self);
void apx_client_test_connection_destroy(apx_client_test_connection_t *self);
void apx_client_test_connection_vdestroy(void *arg);
apx_client_test_connection_t *apx_client_test_connection_new(void);
void apx_client_test_connection_delete(apx_client_test_connection_t *self);

// BaseConnection API
void apx_client_test_connection_start(apx_client_test_connection_t *self);
void apx_client_test_connection_vstart(void *arg);
void apx_client_test_connection_close(apx_client_test_connection_t *self);
void apx_client_test_connection_vclose(void *arg);

// ConnectionInterface API
int32_t apx_client_test_connection_vtransmit_max_bytes_avaiable(void* arg);
int32_t apx_client_test_connection_vtransmit_current_bytes_avaiable(void* arg);
void apx_client_test_connection_vtransmit_begin(void* arg);
void apx_client_test_connection_vtransmit_end(void* arg);
apx_error_t apx_client_test_connection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t*bytes_available);
apx_error_t apx_client_test_connection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
apx_error_t apx_client_test_connection_remote_file_published_notification(apx_client_test_connection_t* self, apx_file_t* file);
apx_error_t apx_client_test_connection_remote_file_write_notification(apx_client_test_connection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);

//Log API
int32_t apx_client_test_connection_log_length(apx_client_test_connection_t* self);
adt_bytearray_t* apx_client_test_connection_get_log_packet(apx_client_test_connection_t* self, int32_t index);
void apx_client_test_connection_clear_log(apx_client_test_connection_t* self);

//Test-case API
apx_node_manager_t* apx_client_test_connection_get_node_manager(apx_client_test_connection_t* self);
void apx_client_test_connection_greeting_header_accepted_notification(apx_client_test_connection_t* self);
apx_file_manager_t* apx_client_test_connection_get_file_manager(apx_client_test_connection_t* self);
apx_error_t apx_client_test_connection_request_open_local_file(apx_client_test_connection_t* self, char const* file_name);
apx_error_t apx_client_test_connection_publish_remote_file(apx_client_test_connection_t* self, uint32_t address, char const* file_name, apx_size_t file_size);
apx_error_t apx_client_test_connection_write_remote_data(apx_client_test_connection_t* self, uint32_t address, uint8_t const* payload_data, apx_size_t payload_size);
apx_node_instance_t* apx_client_test_connection_find_node(apx_client_test_connection_t* self, char const* name);
apx_error_t apx_client_test_connection_build_node(apx_client_test_connection_t* self, char const* definition_text);
void apx_client_test_connection_run(apx_client_test_connection_t* self);

#endif //APX_CLIENT_TEST_CONNECTION_H
