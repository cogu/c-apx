/*****************************************************************************
* \file      socket_server.c
* \author    Conny Gustafsson
* \date      2019-09-07
* \brief     Socket server for apx_server
*
* Copyright (c) 2019-2021 Conny Gustafsson
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
#include "apx/types.h"
#include "apx/extension/socket_server.h"
#include "apx/server.h"
#include "apx/extension/socket_server_connection.h"
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
static void apx_socketServer_tcp_accept(void *arg, struct msocket_server_tag *srv, SOCKET_TYPE *sock);
#if !defined(UNIT_TEST) && !defined(_WIN32)
static void apx_socketServer_unix_accept(void *arg, struct msocket_server_tag *srv, SOCKET_TYPE *sock);
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
      self->tcp_port = 0u;
      self->unix_server_file = (char*) 0;
      self->is_tcp_server_started = false;
      self->is_unix_server_started = false;
      self->tcp_connection_tag = (char*) 0;
      self->unix_connection_tag = (char*) 0;
   }
}

void apx_socketServer_destroy(apx_socketServer_t *self)
{
   if (self != 0)
   {
      if (self->unix_server_file != 0)
      {
         free(self->unix_server_file);
      }
      if (self->tcp_connection_tag != 0)
      {
         free(self->tcp_connection_tag);
      }
      if (self->unix_connection_tag != 0)
      {
         free(self->unix_connection_tag);
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

void apx_socketServer_start_tcp_server(apx_socketServer_t *self, uint16_t tcp_port, const char *tag)
{
   if (self != 0)
   {
      //char msg[80];
      msocket_handler_t serverHandler;
      self->tcp_port = tcp_port;
      if (tag != 0)
      {
         self->tcp_connection_tag = STRDUP(tag);
      }
      memset(&serverHandler,0,sizeof(serverHandler));
#ifndef UNIT_TEST
      serverHandler.tcp_accept = apx_socketServer_tcp_accept;
#endif
      msocket_server_create(&self->tcp_server, AF_INET, NULL);
      msocket_server_disable_cleanup(&self->tcp_server); //we will use our own garbage collector
      msocket_server_sethandler(&self->tcp_server, &serverHandler, self);
      msocket_server_start(&self->tcp_server, NULL, 0, self->tcp_port);
      self->is_tcp_server_started = true;
      printf("Listening on TCP port %d\n", (int) self->tcp_port);
      //sprintf(msg, "Listening on TCP port %d", (int) self->tcpPort);
      //apx_server_logEvent(self->parent, APX_LOG_LEVEL_INFO, APX_SOCKET_SERVER_LABEL, &msg[0]);

   }
}

#if !defined(UNIT_TEST) && !defined(_WIN32)
void apx_socketServer_start_unix_server(apx_socketServer_t *self, const char *file_path, const char *tag)
{
   if ( (self != 0) && (file_path != 0))
   {
      //char msg[APX_MAX_LOG_LEN];
      msocket_handler_t server_handler;
      self->unix_server_file = STRDUP(file_path);
      if (tag != 0)
      {
         self->unix_connection_tag = STRDUP(tag);
      }
      memset(&server_handler,0,sizeof(server_handler));
#ifndef UNIT_TEST
      serverHandler.tcp_accept = apx_socketServer_unix_accept;
#endif
      msocket_server_create(&self->unix_server, AF_LOCAL, NULL);
      msocket_server_disable_cleanup(&self->unix_server); //we will use our own garbage collector
      msocket_server_sethandler(&self->unix_server, &server_handler, self);
      msocket_server_unix_start(&self->unix_server, self->unix_server_file);
      self->is_unix_server_started = true;
      printf("Listening on UNIX socket %s\n", self->unix_server_file);
//      sprintf(msg, "Listening on UNIX socket %s", self->unixServerFile);
//      apx_server_logEvent(self->parent, APX_LOG_LEVEL_INFO, APX_SOCKET_SERVER_LABEL, &msg[0]);
   }
}
#endif

void apx_socketServer_stop_all(apx_socketServer_t *self)
{
   if (self != 0)
   {
      apx_socketServer_stop_tcp_server(self);
#ifndef _WIN32
      apx_socketServer_stop_unix_server(self);
#endif
   }
}

void apx_socketServer_stop_tcp_server(apx_socketServer_t *self)
{
   if ( (self != 0) && (self->is_tcp_server_started) )
   {
      msocket_server_destroy(&self->tcp_server);
      self->is_tcp_server_started = false;
   }
}

#if !defined(UNIT_TEST) && !defined(_WIN32)
void apx_socketServer_stop_unix_server(apx_socketServer_t *self)
{
   if ( (self != 0) && (self->is_unix_server_started) )
   {
#ifndef _MSC_VER
      msocket_server_destroy(&self->unix_server);
#endif
      self->is_unix_server_started = false;
   }
}
#endif

#ifdef UNIT_TEST
void apx_socketServer_accept_testsocket(apx_socketServer_t *self, testsocket_t *sock)
{
   apx_socketServer_tcp_accept((void*) self, (struct msocket_server_tag*) 0, sock);
}
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_socketServer_tcp_accept(void *arg, struct msocket_server_tag *srv, SOCKET_TYPE *sock)
{
   apx_socketServer_t *self = (apx_socketServer_t*) arg;
   (void)srv;
#if APX_DEBUG_ENABLE
   printf("[SOCKET-SERVER] New TCP connection\n");
#endif
   if (self != NULL)
   {
      apx_socketServerConnection_t *new_connection = apx_socketServerConnection_new(sock);
      ///TODO: Add support for connection tag
      if (new_connection != NULL)
      {
         apx_server_accept_connection(self->parent, (apx_serverConnection_t*)new_connection);
      }
      else
      {
         ///TODO: cleanup socket object
         assert(0);
      }
   }
}

#if !defined(UNIT_TEST) && !defined(_WIN32)
static void apx_socketServer_unix_accept(void *arg, struct msocket_server_tag *srv, SOCKET_TYPE *sock)
{
   apx_socketServer_t *self = (apx_socketServer_t*) arg;
#if APX_DEBUG_ENABLE
   printf("[SOCKET-SERVER] New UNIX connection\n");
#endif
   if (self != 0)
   {
      apx_socketServerConnection_t * new_connection = apx_socketServerConnection_new(sock);
      ///TODO: Add support for connection tag
      if (new_connection != 0)
      {
         apx_server_accept_connection(self->parent, (apx_serverConnection_t*)new_connection);
      }
      else
      {
         ///TODO: cleanup socket object
         assert(0);
      }
   }
}
#endif

