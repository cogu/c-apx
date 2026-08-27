/*****************************************************************************
* \file      client_test_connection.h
* \author    Conny Gustafsson
* \date      2018-01-15
* \brief     Unit Test connection for APX clients
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


typedef struct apx_clientTestConnection_tag
{
   apx_clientConnection_t base;
   adt_ary_t * transmit_log; //strong references to adt_bytearray_t
   adt_bytearray_t transmit_buffer;
   apx_size_t default_buffer_size;
   apx_size_t pending_bytes;
   apx_nodeManager_t node_manager;
} apx_clientTestConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
// Constructor/Destructor
apx_error_t apx_clientTestConnection_create(apx_clientTestConnection_t *self);
void apx_clientTestConnection_destroy(apx_clientTestConnection_t *self);
void apx_clientTestConnection_vdestroy(void *arg);
apx_clientTestConnection_t *apx_clientTestConnection_new(void);
void apx_clientTestConnection_delete(apx_clientTestConnection_t *self);

// BaseConnection API
void apx_clientTestConnection_start(apx_clientTestConnection_t *self);
void apx_clientTestConnection_vstart(void *arg);
void apx_clientTestConnection_close(apx_clientTestConnection_t *self);
void apx_clientTestConnection_vclose(void *arg);

// ConnectionInterface API
int32_t apx_clientTestConnection_vtransmit_max_bytes_avaiable(void* arg);
int32_t apx_clientTestConnection_vtransmit_current_bytes_avaiable(void* arg);
void apx_clientTestConnection_vtransmit_begin(void* arg);
void apx_clientTestConnection_vtransmit_end(void* arg);
apx_error_t apx_clientTestConnection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t*bytes_available);
apx_error_t apx_clientTestConnection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
apx_error_t apx_clientTestConnection_remote_file_published_notification(apx_clientTestConnection_t* self, apx_file_t* file);
apx_error_t apx_clientTestConnection_remote_file_write_notification(apx_clientTestConnection_t* self, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);

//Log API
int32_t apx_clientTestConnection_log_length(apx_clientTestConnection_t* self);
adt_bytearray_t* apx_clientTestConnection_get_log_packet(apx_clientTestConnection_t* self, int32_t index);
void apx_clientTestConnection_clear_log(apx_clientTestConnection_t* self);

//Test-case API
apx_nodeManager_t* apx_clientTestConnection_get_node_manager(apx_clientTestConnection_t* self);
void apx_clientTestConnection_greeting_header_accepted_notification(apx_clientTestConnection_t* self);
apx_fileManager_t* apx_clientTestConnection_get_file_manager(apx_clientTestConnection_t* self);
apx_error_t apx_clientTestConnection_request_open_local_file(apx_clientTestConnection_t* self, char const* file_name);
apx_error_t apx_clientTestConnection_publish_remote_file(apx_clientTestConnection_t* self, uint32_t address, char const* file_name, apx_size_t file_size);
apx_error_t apx_clientTestConnection_write_remote_data(apx_clientTestConnection_t* self, uint32_t address, uint8_t const* payload_data, apx_size_t payload_size);
apx_nodeInstance_t* apx_clientTestConnection_find_node(apx_clientTestConnection_t* self, char const* name);
apx_error_t apx_clientTestConnection_build_node(apx_clientTestConnection_t* self, char const* definition_text);
void apx_clientTestConnection_run(apx_clientTestConnection_t* self);

#endif //APX_CLIENT_TEST_CONNECTION_H
