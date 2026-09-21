/*****************************************************************************
* \file      message_client_connection.c
* \author    Conny Gustafsson
* \date      2020-03-08
* \brief     msocket connection that sends JSON data to to apx_sernder application
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "apx/numheader.h"
#include "message_client_connection.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define UINT32_SIZE 4
#define BUFFER_GROW_SIZE 256
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void message_client_connection_on_connect(void *arg, void *socket, const char *addr, uint16_t port);
static void message_client_connection_on_disconnect(void *arg, void *socket);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int32_t message_client_connection_create(message_client_connection_t *self, uint8_t address_family)
{
   if (self != NULL)
   {
      self->pendingMessage = NULL;
      self->msocket = msocket_new(address_family);
      if (self->msocket == NULL)
      {
         return 1;
      }
      else
      {
         msocket_handler_t handler;
         memset(&handler, 0, sizeof(handler));
         handler.tcp_connected = message_client_connection_on_connect;
         handler.tcp_disconnected = message_client_connection_on_disconnect;
         msocket_sethandler(self->msocket, &handler, (void*) self);
         SEMAPHORE_CREATE(self->messageTransmitted);
      }
   }
   return 0;
}

void message_client_connection_destroy(message_client_connection_t *self)
{
   if (self != NULL)
   {
      msocket_delete(self->msocket);
      if (self->pendingMessage != NULL)
      {
         adt_bytearray_delete(self->pendingMessage);
         SEMAPHORE_DESTROY(self->messageTransmitted);
      }
   }
}

message_client_connection_t *message_client_connection_new(uint8_t address_family)
{
   message_client_connection_t *self = (message_client_connection_t*) malloc(sizeof(message_client_connection_t));
   if (self != NULL)
   {
      int32_t result = message_client_connection_create(self, address_family);
      if (result != 0)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void message_client_connection_delete(message_client_connection_t *self)
{
   if (self != NULL)
   {
      message_client_connection_destroy(self);
      free(self);
   }
}

adt_error_t message_client_prepare_message(message_client_connection_t *self, adt_str_t *message)
{
   if ( (self != NULL) && (message != NULL) )
   {
      uint8_t headerData[UINT32_SIZE];
      uint32_t messageSize;
      int32_t headerSize;
      adt_bytearray_t *messageBytes = adt_str_bytearray(message);
      if (messageBytes == NULL)
      {
         return ADT_MEM_ERROR;
      }
      if (self->pendingMessage == NULL)
      {
         self->pendingMessage = adt_bytearray_new();
         if (self->pendingMessage == NULL)
         {
            return ADT_MEM_ERROR;
         }
      }
      else
      {
         adt_bytearray_clear(self->pendingMessage);
      }
      assert(self->pendingMessage != NULL);
      assert(messageBytes != NULL);
      messageSize = adt_bytearray_length(messageBytes);
      headerSize = numheader_encode32(&headerData[0], UINT32_SIZE, messageSize);
      if ( (headerSize > 0) && (headerSize <= UINT32_SIZE) )
      {
         adt_error_t rc = adt_bytearray_append(self->pendingMessage, &headerData[0], (uint32_t)headerSize);
         if (rc != ADT_NO_ERROR)
         {
            return rc;
         }
         rc = adt_bytearray_append(self->pendingMessage, adt_bytearray_data(messageBytes), messageSize);
         if (rc != ADT_NO_ERROR)
         {
            return rc;
         }
      }
      adt_bytearray_delete(messageBytes);
      //printf("Prepared %d+%d bytes of data\n", (int) headerSize, (int) messageSize);
      return ADT_NO_ERROR;
   }
   return ADT_INVALID_ARGUMENT_ERROR;
}

int32_t message_client_connect_tcp(message_client_connection_t *self, const char *address, uint16_t port)
{
   if (self != NULL)
   {
      return (int32_t) msocket_connect(self->msocket, address, port);
   }
   return -1;
}

#ifndef _WIN32
int32_t message_client_connect_unix(message_client_connection_t *self, const char *socket_path)
{
   if (self != NULL)
   {
      return (int32_t) msocket_unix_connect(self->msocket, socket_path);
   }
   return -1;
}
#endif

int32_t message_client_wait_for_message_transmitted(message_client_connection_t *self)
{
   int32_t retval = 0;
   if (self != NULL)
   {
#ifdef _WIN32
      DWORD result = WaitForSingleObject(self->messageTransmitted, INFINITE);
      if (result != WAIT_OBJECT_0)
      {
         DWORD lastError = GetLastError();
         fprintf(stderr, "Semaphore wait failed with error %d\n", (int) lastError);
         retval = -1;
      }
#else
      retval = (int32_t) sem_wait(&self->messageTransmitted);
#endif
   }
   else
   {
      retval = -1;
   }
   return retval;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void message_client_connection_on_connect(void *arg, void *socket, const char *addr, uint16_t port)
{
   message_client_connection_t *self = (message_client_connection_t*) arg;
   (void)socket;
   (void)port;
   (void)addr;
   if (self != NULL )
   {
      if (self->pendingMessage != NULL)
      {
         const char *data = (const char*) adt_bytearray_data(self->pendingMessage);
         uint32_t size = adt_bytearray_length(self->pendingMessage);
#if APX_DEBUG_ENABLE
         printf("[APX_CONTROL] Message transmitted\n");
#endif
         msocket_send(self->msocket, data, size);
         SEMAPHORE_POST(self->messageTransmitted);

      }
   }
}

static void message_client_connection_on_disconnect(void *arg, void *socket)
{
   (void)arg;
   (void)socket;
   printf("[APX_CONTROL] disconnected\n");
}
