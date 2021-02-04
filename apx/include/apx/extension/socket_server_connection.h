/*****************************************************************************
* \file      socket_server_connection.h
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     socket server connection class
*            Inherits from apx_serverConnection_t
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
#ifndef APX_SOCKET_SERVER_CONNECTION_H
#define APX_SOCKET_SERVER_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "apx/server_connection.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
#define SOCKET_TYPE struct testsocket_tag
#else
#define SOCKET_TYPE struct msocket_t
#endif
SOCKET_TYPE; //this is a forward declaration of the declared type just above

typedef struct apx_socketServerConnection_tag
{
   apx_serverConnection_t base;
   apx_nodeManager_t node_manager;
   adt_bytearray_t send_buffer;
   apx_size_t default_buffer_size;
   apx_size_t pending_bytes;
   SOCKET_TYPE *socket_object;
   MUTEX_T lock;
}apx_socketServerConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_socketServerConnection_create(apx_socketServerConnection_t *self, SOCKET_TYPE *socketObject);
void apx_socketServerConnection_destroy(apx_socketServerConnection_t *self);
void apx_socketServerConnection_vdestroy(void *arg);
apx_socketServerConnection_t *apx_socketServerConnection_new(SOCKET_TYPE *socketObject);
void apx_socketServerConnection_delete(apx_socketServerConnection_t *self);
void apx_socketServerConnection_vdelete(void *arg);
void apx_socketServerConnection_vstart(void *arg);
void apx_socketServerConnection_vclose(void *arg);

// ConnectionInterface API
int32_t apx_socketServerConnection_vtransmit_max_bytes_avaiable(void* arg);
int32_t apx_socketServerConnection_vtransmit_current_bytes_avaiable(void* arg);
void apx_socketServerConnection_vtransmit_begin(void* arg);
void apx_socketServerConnection_vtransmit_end(void* arg);
apx_error_t apx_socketServerConnection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
apx_error_t apx_socketServerConnection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
#ifdef UNIT_TEST
void apx_socketServerConnection_run(apx_socketServerConnection_t* self);
#endif

#undef SOCKET_TYPE
#endif //APX_SOCKET_SERVER_CONNECTION_H
