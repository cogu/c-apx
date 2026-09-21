/*****************************************************************************
* \file      socket_server.h
* \author    Conny Gustafsson
* \date      2019-09-07
* \brief     Socket server for apx_server
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SOCKET_SERVER_H
#define APX_SOCKET_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx/error.h"
#include "msocket.h"
#include "msocket_server.h"
#include "testsocket.h"
#include "dtl_type.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//Forward declarations
struct apx_server_tag;

typedef struct apx_socket_server_tag
{
   uint16_t tcp_port; //TCP port for tcpServer
   char *unix_server_file; //path to socket file for unix domain sockets (used for localServer)
   msocket_server_t tcp_server; //tcp server
   msocket_server_t unix_server; //unix domain socket server
   struct apx_server_tag *parent; //parent server
   char *tcp_connection_tag; //Optional tag to set on new TCP connections
   char *unix_connection_tag; //Optional tag to set on new Unix socket connections
   bool is_tcp_server_started;
   bool is_unix_server_started;
} apx_socket_server_t;

#define APX_SOCKET_SERVER_LABEL "SOCKET"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_socket_server_create(apx_socket_server_t *self, struct apx_server_tag *apx_server);
void apx_socket_server_destroy(apx_socket_server_t *self);
apx_socket_server_t* apx_socket_server_new(struct apx_server_tag *apx_server);
void apx_socket_server_delete(apx_socket_server_t *self);

void apx_socket_server_start_tcp_server(apx_socket_server_t *self, uint16_t tcp_port, const char *tag);
#if !defined(UNIT_TEST) && !defined(_WIN32)
void apx_socket_server_start_unix_server(apx_socket_server_t *self, const char *file_path, const char *tag);
void apx_socket_server_stop_unix_server(apx_socket_server_t *self);
#endif
void apx_socket_server_stop_all(apx_socket_server_t *self);
void apx_socket_server_stop_tcp_server(apx_socket_server_t *self);
#ifdef UNIT_TEST
void apx_socket_server_accept_testsocket(apx_socket_server_t *self, testsocket_t *sock);
#endif
#endif //APX_SOCKET_SERVER_H
