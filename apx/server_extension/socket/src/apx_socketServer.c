/*****************************************************************************
* \file      apx_socketServer.c
* \author    Conny Gustafsson
* \date      2019-09-07
* \brief     Description
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "apx_types.h"
#include "apx_socketServer.h"
#include "apx_server.h"
#include "apx_serverSocketConnection.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_serverInfo_tag
{
   uint8_t addressFamily;
}apx_serverInfo_t;

struct msocket_server_tag;

#ifdef UNIT_TEST
#define SOCKET_TYPE testsocket_t
#define SOCKET_DELETE testsocket_delete
#define SOCKET_START_IO(x)
#define SOCKET_SET_HANDLER testsocket_setServerHandler
#else
#define SOCKET_DELETE msocket_delete
#define SOCKET_TYPE msocket_t
#define SOCKET_START_IO(x) msocket_start_io(x)
#define SOCKET_SET_HANDLER msocket_sethandler
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_socketServer_tcpAccept(void *arg, struct msocket_server_tag *srv, SOCKET_TYPE *sock);
#ifndef UNIT_TEST
static void apx_socketServer_unixAccept(void *arg, struct msocket_server_tag *srv, SOCKET_TYPE *sock);
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_socketServer_create(apx_socketServer_t *self, struct apx_server_tag *apx_server)
{
   if (self != 0)
   {
      self->parent = apx_server;
      self->tcpPort = 0u;
      self->unixServerFile = (char*) 0;
      self->isTcpServerStarted = false;
      self->isUnixServerStarted = false;
      self->tcpConnectionTag = (char*) 0;
      self->unixConnectionTag = (char*) 0;
   }
}

void apx_socketServer_destroy(apx_socketServer_t *self)
{
   if (self != 0)
   {
      if (self->unixServerFile != 0)
      {
         free(self->unixServerFile);
      }
      if (self->tcpConnectionTag != 0)
      {
         free(self->tcpConnectionTag);
      }
      if (self->unixConnectionTag != 0)
      {
         free(self->unixConnectionTag);
      }
   }
}

apx_socketServer_t* apx_socketServer_new(struct apx_server_tag *apx_server)
{
   apx_socketServer_t *self = (apx_socketServer_t*) malloc(sizeof(apx_socketServer_t));
   if (self != 0)
   {
      apx_socketServer_create(self, apx_server);
   }
   return self;
}

void apx_socketServer_delete(apx_socketServer_t *self)
{
   if (self != 0)
   {
      apx_socketServer_destroy(self);
      free(self);
   }
}

void apx_socketServer_startTcpServer(apx_socketServer_t *self, uint16_t tcpPort, const char *tag)
{
   if (self != 0)
   {
      //char msg[80];
      msocket_handler_t serverHandler;
      self->tcpPort = tcpPort;
      if (tag != 0)
      {
         self->tcpConnectionTag = STRDUP(tag);
      }
      memset(&serverHandler,0,sizeof(serverHandler));
#ifndef UNIT_TEST
      serverHandler.tcp_accept = apx_socketServer_tcpAccept;
#endif
      msocket_server_create(&self->tcpServer, AF_INET, NULL);
      msocket_server_disable_cleanup(&self->tcpServer); //we will use our own garbage collector
      msocket_server_sethandler(&self->tcpServer, &serverHandler, self);
      msocket_server_start(&self->tcpServer, NULL, 0, self->tcpPort);
      self->isTcpServerStarted = true;
      printf("Listening on TCP port %d\n", (int) self->tcpPort);
      //sprintf(msg, "Listening on TCP port %d", (int) self->tcpPort);
      //apx_server_logEvent(self->parent, APX_LOG_LEVEL_INFO, APX_SOCKET_SERVER_LABEL, &msg[0]);

   }
}

void apx_socketServer_startUnixServer(apx_socketServer_t *self, const char *filePath, const char *tag)
{
   if ( (self != 0) && (filePath != 0))
   {
      //char msg[APX_MAX_LOG_LEN];
      msocket_handler_t serverHandler;
      self->unixServerFile = STRDUP(filePath);
      if (tag != 0)
      {
         self->unixConnectionTag = STRDUP(tag);
      }
      memset(&serverHandler,0,sizeof(serverHandler));
#ifndef UNIT_TEST
      serverHandler.tcp_accept = apx_socketServer_unixAccept;
#endif
      msocket_server_create(&self->unixServer, AF_LOCAL, NULL);
      msocket_server_disable_cleanup(&self->unixServer); //we will use our own garbage collector
      msocket_server_sethandler(&self->unixServer, &serverHandler, self);
      msocket_server_unix_start(&self->unixServer, self->unixServerFile);
      self->isUnixServerStarted = true;
      printf("Listening on UNIX socket %s\n", self->unixServerFile);
//      sprintf(msg, "Listening on UNIX socket %s", self->unixServerFile);
//      apx_server_logEvent(self->parent, APX_LOG_LEVEL_INFO, APX_SOCKET_SERVER_LABEL, &msg[0]);
   }
}

void apx_socketServer_stopAll(apx_socketServer_t *self)
{
   if (self != 0)
   {
      apx_socketServer_stopTcpServer(self);
      apx_socketServer_stopUnixServer(self);
   }
}

void apx_socketServer_stopTcpServer(apx_socketServer_t *self)
{
   if ( (self != 0) && (self->isTcpServerStarted) )
   {
      msocket_server_destroy(&self->tcpServer);
      self->isTcpServerStarted = false;
   }
}

void apx_socketServer_stopUnixServer(apx_socketServer_t *self)
{
   if ( (self != 0) && (self->isUnixServerStarted) )
   {
#ifndef _MSC_VER
      msocket_server_destroy(&self->unixServer);
#endif
      self->isUnixServerStarted = false;
   }
}

#ifdef UNIT_TEST
void apx_socketServer_acceptTestSocket(apx_socketServer_t *self, testsocket_t *sock)
{
   apx_socketServer_tcpAccept((void*) self, (struct msocket_server_tag*) 0, sock);
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_socketServer_tcpAccept(void *arg, struct msocket_server_tag *srv, SOCKET_TYPE *sock)
{
   apx_socketServer_t *self = (apx_socketServer_t*) arg;
#if APX_DEBUG_ENABLE
   printf("[SOCKET-SERVER] New TCP connection\n");
#endif
   if (self != 0)
   {
      apx_serverSocketConnection_t *newConnection = apx_serverSocketConnection_new(sock);
      ///TODO: Add support for connection tag
      if (newConnection != 0)
      {
         apx_server_acceptConnection(self->parent, (apx_serverConnectionBase_t*) newConnection);
      }
      else
      {
         ///TODO: cleanup socket object
         assert(0);
      }
   }
}

#ifndef UNIT_TEST
static void apx_socketServer_unixAccept(void *arg, struct msocket_server_tag *srv, SOCKET_TYPE *sock)
{
   apx_socketServer_t *self = (apx_socketServer_t*) arg;
#if APX_DEBUG_ENABLE
   printf("[SOCKET-SERVER] New UNIX connection\n");
#endif
   if (self != 0)
   {
      apx_serverSocketConnection_t *newConnection = apx_serverSocketConnection_new(sock);
      ///TODO: Add support for connection tag
      if (newConnection != 0)
      {
         apx_server_acceptConnection(self->parent, (apx_serverConnectionBase_t*) newConnection);
      }
      else
      {
         ///TODO: cleanup socket object
         assert(0);
      }
   }
}
#endif

