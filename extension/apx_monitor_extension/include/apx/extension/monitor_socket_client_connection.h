/*****************************************************************************
* \file      monitor_socket_client_connection.h
* \author    Conny Gustafsson
* \date      2021-03-01
* \brief     Custom class for client connections using monitor connection type
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_MONITOR_SOCKET_CLIENT_CONNECTION_H
#define APX_MONITOR_SOCKET_CLIENT_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "apx/client_connection.h"
#ifdef UNIT_TEST
#include "testsocket.h"
#else
#include "msocket.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
#define SOCKET_TYPE struct testsocket_tag
#else
#define SOCKET_TYPE struct msocket_tag
#endif
SOCKET_TYPE; //this is a forward declaration of the declared type just above

typedef struct apx_monitor_socket_client_connection_tag
{
   apx_client_connection_t base;
   adt_bytearray_t send_buffer;
   apx_size_t default_buffer_size;
   apx_size_t pending_bytes;
   SOCKET_TYPE* socket_object;
   MUTEX_T lock;
} apx_monitor_socket_client_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_monitor_socket_client_connection_create(apx_monitor_socket_client_connection_t* self, SOCKET_TYPE* socket_object);
void apx_monitor_socket_client_connection_destroy(apx_monitor_socket_client_connection_t* self);
void apx_monitor_socket_client_connection_vdestroy(void* arg);
apx_monitor_socket_client_connection_t* apx_monitor_socket_client_connection_new(SOCKET_TYPE* socket_object);
apx_connection_type_t apx_monitor_socket_client_connection_get_connection_type(apx_monitor_socket_client_connection_t const* self);

#ifndef UNIT_TEST
apx_error_t apx_monitor_socket_client_connection_connect_tcp(apx_monitor_socket_client_connection_t* self, const char* address, uint16_t port);
# ifndef _WIN32
apx_error_t apx_monitor_socket_client_connection_connect_unix(apx_monitor_socket_client_connection_t* self, const char* socket_path);
# endif
#endif

// ConnectionInterface API
int32_t apx_monitor_socket_client_connection_vtransmit_max_bytes_avaiable(void* arg);
int32_t apx_monitor_socket_client_connection_vtransmit_current_bytes_avaiable(void* arg);
void apx_monitor_socket_client_connection_vtransmit_begin(void* arg);
void apx_monitor_socket_client_connection_vtransmit_end(void* arg);
apx_error_t apx_monitor_socket_client_connection_vtransmit_data_message(void* arg, uint32_t write_address, bool more_bit, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);
apx_error_t apx_monitor_socket_client_connection_vtransmit_direct_message(void* arg, uint8_t const* msg_data, int32_t msg_size, int32_t* bytes_available);

#ifdef UNIT_TEST
void apx_monitor_socket_client_connection_run(apx_monitor_socket_client_connection_t* self);
#endif

#undef SOCKET_TYPE
#endif //APX_MONITOR_SOCKET_CLIENT_CONNECTION_H

