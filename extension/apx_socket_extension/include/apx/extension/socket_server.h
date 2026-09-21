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

typedef struct apx_socketServer_tag
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
} apx_socketServer_t;

#define APX_SOCKET_SERVER_LABEL "SOCKET"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_socketServer_create(apx_socketServer_t *self, struct apx_server_tag *apx_server);
void apx_socketServer_destroy(apx_socketServer_t *self);
apx_socketServer_t* apx_socketServer_new(struct apx_server_tag *apx_server);
void apx_socketServer_delete(apx_socketServer_t *self);

void apx_socketServer_start_tcp_server(apx_socketServer_t *self, uint16_t tcp_port, const char *tag);
#if !defined(UNIT_TEST) && !defined(_WIN32)
void apx_socketServer_start_unix_server(apx_socketServer_t *self, const char *file_path, const char *tag);
void apx_socketServer_stop_unix_server(apx_socketServer_t *self);
#endif
void apx_socketServer_stop_all(apx_socketServer_t *self);
void apx_socketServer_stop_tcp_server(apx_socketServer_t *self);
#ifdef UNIT_TEST
void apx_socketServer_accept_testsocket(apx_socketServer_t *self, testsocket_t *sock);
#endif
#endif //APX_SOCKET_SERVER_H
