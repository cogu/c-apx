/*****************************************************************************
* \file      apx_socketServer.h
* \author    Conny Gustafsson
* \date      2019-09-07
* \brief     Socket server for apx_server
*
* Copyright (c) 2019 Conny Gustafsson
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
#ifndef APX_SOCKET_SERVER_H
#define APX_SOCKET_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx_error.h"
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
   uint16_t tcpPort; //TCP port for tcpServer
   char *localServerFile; //path to socket file for unix domain sockets (used for localServer)
   msocket_server_t tcpServer; //tcp server
   msocket_server_t unixServer; //unix domain socket server
   struct apx_server_tag *parent; //parent server
   char *tcpConnectionTag; //Optional tag to set on new TCP connections
   char *unixConnectionTag; //Optional tag to set on new Unix socket connections
   bool isTcpServerStarted;
   bool isUnixServerStarted;
} apx_socketServer_t;

#define APX_SOCKET_SERVER_LABEL "SOCKET"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_socketServer_create(apx_socketServer_t *self, struct apx_server_tag *apx_server);
void apx_socketServer_destroy(apx_socketServer_t *self);
apx_socketServer_t* apx_socketServer_new(struct apx_server_tag *apx_server);
void apx_socketServer_delete(apx_socketServer_t *self);

void apx_socketServer_startTcpServer(apx_socketServer_t *self, uint16_t tcpPort, const char *tag);
void apx_socketServer_startUnixServer(apx_socketServer_t *self, const char *filePath, const char *tag);
void apx_socketServer_stopAll(apx_socketServer_t *self);
void apx_socketServer_stopTcpServer(apx_socketServer_t *self);
void apx_socketServer_stopUnixServer(apx_socketServer_t *self);

void apx_socketServer_acceptTestSocket(apx_socketServer_t *self, testsocket_t *sock);

#endif //APX_SOCKET_SERVER_H
