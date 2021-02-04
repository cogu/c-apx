/*****************************************************************************
* \file      server_test_connection.h
* \author    Conny Gustafsson
* \date      2018-12-09
* \brief     Server unit test connection
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
#ifndef APX_SERVER_TEST_CONNECTION_H
#define APX_SERVER_TEST_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdbool.h>
#include "apx/error.h"
#include "apx/server_connection.h"
#include "apx/file_info.h"
#include "adt_bytearray.h"
#include "adt_ary.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_serverTestConnection_tag
{
   apx_serverConnection_t base;
   adt_ary_t* transmit_log; //strong references to adt_bytearray_t
   adt_bytearray_t transmit_buffer;
   apx_size_t default_buffer_size;
   apx_size_t pending_bytes;
   apx_nodeManager_t node_manager;
}apx_serverTestConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverTestConnection_create(apx_serverTestConnection_t *self);
void apx_serverTestConnection_destroy(apx_serverTestConnection_t *self);
void apx_serverTestConnection_vdestroy(void *arg);
apx_serverTestConnection_t *apx_serverTestConnection_new(void);
void apx_serverTestConnection_delete(apx_serverTestConnection_t *self);

// BaseConnection API
void apx_serverTestConnection_start(apx_serverTestConnection_t* self);
void apx_serverTestConnection_vstart(void* arg);
void apx_serverTestConnection_close(apx_serverTestConnection_t* self);
void apx_serverTestConnection_vclose(void* arg);

// ConnectionInterface API
int32_t apx_serverTestConnection_vtransmit_max_bytes_avaiable(void* arg);
int32_t apx_serverTestConnection_vtransmit_current_bytes_avaiable(void* arg);
void apx_serverTestConnection_vtransmit_begin(void* arg);
void apx_serverTestConnection_vtransmit_end(void* arg);
apx_error_t apx_serverTestConnection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
apx_error_t apx_serverTestConnection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
apx_error_t apx_serverTestConnection_remote_file_published_notification(apx_serverTestConnection_t* self, apx_file_t* file);
apx_error_t apx_serverTestConnection_remote_file_write_notification(apx_serverTestConnection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);

//Log API
int32_t apx_serverTestConnection_log_length(apx_serverTestConnection_t* self);
adt_bytearray_t* apx_serverTestConnection_get_log_packet(apx_serverTestConnection_t* self, int32_t index);
void apx_serverTestConnection_clear_log(apx_serverTestConnection_t* self);

//Test-case API
apx_fileManager_t* apx_serverTestConnection_get_file_manager(apx_serverTestConnection_t* self);
apx_nodeManager_t* apx_serverTestConnection_get_node_manager(apx_serverTestConnection_t* self);
apx_error_t apx_serverTestConnection_send_greeting_header(apx_serverTestConnection_t* self);
apx_error_t apx_serverTestConnection_request_open_local_file(apx_serverTestConnection_t* self, char const* file_name);
apx_error_t apx_serverTestConnection_publish_remote_file(apx_serverTestConnection_t* self, uint32_t address, char const* file_name, apx_size_t file_size);
apx_error_t apx_serverTestConnection_write_remote_data(apx_serverTestConnection_t* self, uint32_t address, uint8_t const* payload_data, apx_size_t payload_size);
apx_nodeInstance_t* apx_serverTestConnection_find_node(apx_serverTestConnection_t* self, char const* name);
apx_error_t apx_serverTestConnection_build_node(apx_serverTestConnection_t* self, char const* definition_text);
void apx_serverTestConnection_run(apx_serverTestConnection_t* self);

#endif //APX_SERVER_TEST_CONNECTION_H
