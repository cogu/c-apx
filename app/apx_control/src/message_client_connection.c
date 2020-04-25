/*****************************************************************************
* \file      message_client_connection.c
* \author    Conny Gustafsson
* \date      2020-03-08
* \brief     msocket connection that sends JSON data to to apx_sernder application
*
* Copyright (c) 2020 Conny Gustafsson
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
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "numheader.h"
#include "message_client_connection.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define UINT32_SIZE 4
#define BUFFER_GROW_SIZE 256
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void message_client_connection_onConnect(void *arg, const char *addr, uint16_t port);
static void message_client_connection_onDisconnect(void *arg);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int32_t message_client_connection_create(message_client_connection_t *self, uint8_t addressFamily)
{
   if (self != 0)
   {
      self->pendingMessage = (adt_bytearray_t*) 0;
      self->msocket = msocket_new(addressFamily);
      if (self->msocket == 0)
      {
         return 1;
      }
      else
      {
         msocket_handler_t handler;
         memset(&handler, 0, sizeof(handler));
         handler.tcp_connected = message_client_connection_onConnect;
         handler.tcp_disconnected = message_client_connection_onDisconnect;
         msocket_sethandler(self->msocket, &handler, (void*) self);
         SEMAPHORE_CREATE(self->messageTransmitted);
      }
   }
   return 0;
}

void message_client_connection_destroy(message_client_connection_t *self)
{
   if (self != 0)
   {
      msocket_delete(self->msocket);
      if (self->pendingMessage != 0)
      {
         adt_bytearray_delete(self->pendingMessage);
         SEMAPHORE_DESTROY(self->messageTransmitted);
      }
   }
}

message_client_connection_t *message_client_connection_new(uint8_t addressFamily)
{
   message_client_connection_t *self = (message_client_connection_t*) malloc(sizeof(message_client_connection_t));
   if (self != 0)
   {
      int32_t result = message_client_connection_create(self, addressFamily);
      if (result != 0)
      {
         free(self);
         self = (message_client_connection_t*) 0;
      }
   }
   return self;
}

void message_client_connection_delete(message_client_connection_t *self)
{
   if (self != 0)
   {
      message_client_connection_destroy(self);
      free(self);
   }
}

adt_error_t message_client_prepare_message(message_client_connection_t *self, adt_str_t *message)
{
   if ( (self != 0) && (message != 0) )
   {
      uint8_t headerData[UINT32_SIZE];
      int32_t messageSize;
      int32_t headerSize;
      adt_bytearray_t *messageBytes = adt_str_bytearray(message);
      if (messageBytes == 0)
      {
         return ADT_MEM_ERROR;
      }
      if (self->pendingMessage == 0)
      {
         self->pendingMessage = adt_bytearray_new(BUFFER_GROW_SIZE);
         if (self->pendingMessage == 0)
         {
            return ADT_MEM_ERROR;
         }
      }
      else
      {
         adt_bytearray_clear(self->pendingMessage);
      }
      assert(self->pendingMessage != 0);
      assert(messageBytes != 0);
      messageSize = adt_bytearray_length(messageBytes);
      headerSize = numheader_encode32(&headerData[0], UINT32_SIZE, messageSize);
      if ( (headerSize > 0) && (headerSize <= UINT32_SIZE) )
      {
         adt_error_t rc = adt_bytearray_append(self->pendingMessage, &headerData[0], headerSize);
         if (rc != ADT_NO_ERROR)
         {
            return rc;
         }
         adt_bytearray_append(self->pendingMessage, adt_bytearray_data(messageBytes), messageSize);
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
   if (self != 0)
   {
      return (int32_t) msocket_connect(self->msocket, address, port);
   }
   return -1;
}

#ifndef _WIN32
int32_t message_client_connect_unix(message_client_connection_t *self, const char *socketPath)
{
   if (self != 0)
   {
      return (int32_t) msocket_unix_connect(self->msocket, socketPath);
   }
   return -1;
}
#endif

int32_t message_client_wait_for_message_transmitted(message_client_connection_t *self)
{
   int32_t retval = 0;
   if (self != 0)
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

static void message_client_connection_onConnect(void *arg, const char *addr, uint16_t port)
{
   message_client_connection_t *self = (message_client_connection_t*) arg;
   if (self != 0 )
   {
      if (self->pendingMessage != 0)
      {
         const char *data = (const char*) adt_bytearray_data(self->pendingMessage);
         uint32_t size = adt_bytearray_length(self->pendingMessage);
         msocket_send(self->msocket, data, size);
         SEMAPHORE_POST(self->messageTransmitted);
      }
   }
}

static void message_client_connection_onDisconnect(void *arg)
{
   printf("[APX_CONTROL] disconnected\n");
}
/*
static int8_t message_client_connection_onData(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{

}
*/
