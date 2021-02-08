/*****************************************************************************
* \file      apx_client_socket_connection.h
* \author    Conny Gustafsson
* \date      2018-12-31
* \brief     Client socket connection class
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
#ifndef APX_CLIENT_SOCKET_CONNECTION_H
#define APX_CLIENT_SOCKET_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "apx/client_connection.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
#define SOCKET_TYPE struct testsocket_tag
#else
#define SOCKET_TYPE struct msocket_t
#endif
SOCKET_TYPE; //this is a forward declaration of the declared type just above

typedef struct apx_clientSocketConnection_tag
{
   apx_clientConnection_t base;
   adt_bytearray_t send_buffer;
   apx_size_t default_buffer_size;
   apx_size_t pending_bytes;
   SOCKET_TYPE *socket_object;
   MUTEX_T lock;
}apx_clientSocketConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientSocketConnection_create(apx_clientSocketConnection_t *self, SOCKET_TYPE * socket_object);
void apx_clientSocketConnection_destroy(apx_clientSocketConnection_t *self);
void apx_clientSocketConnection_vdestroy(void *arg);
apx_clientSocketConnection_t *apx_clientSocketConnection_new(SOCKET_TYPE * socket_object);

#ifndef UNIT_TEST
apx_error_t apx_clientConnection_tcp_connect(apx_clientSocketConnection_t *self, const char *address, uint16_t port);
# ifndef _WIN32
apx_error_t apx_clientConnection_unix_connect(apx_clientSocketConnection_t *self, const char *socket_path);
# endif
#endif

// ConnectionInterface API
int32_t apx_clientSocketConnection_vtransmit_max_bytes_avaiable(void* arg);
int32_t apx_clientSocketConnection_vtransmit_current_bytes_avaiable(void* arg);
void apx_clientSocketConnection_vtransmit_begin(void* arg);
void apx_clientSocketConnection_vtransmit_end(void* arg);
apx_error_t apx_clientSocketConnection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
apx_error_t apx_clientSocketConnection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
#ifdef UNIT_TEST
void apx_clientSocketConnection_attach_node_manager(apx_clientSocketConnection_t* self, apx_nodeManager_t* node_manager);
apx_error_t apx_clientSocketConnection_build_node(apx_clientSocketConnection_t* self, char const* definition_text);
void apx_clientSocketConnection_run(apx_clientSocketConnection_t* self);
#endif

#undef SOCKET_TYPE
#endif //APX_CLIENT_SOCKET_CONNECTION_H
