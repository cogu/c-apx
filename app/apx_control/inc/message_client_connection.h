/*****************************************************************************
* \file      message_client_connection.h
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
#ifndef MESSAGE_CLIENT_CONNECTION_H
#define MESSAGE_CLIENT_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#include "msocket.h"
#include "adt_str.h"
#include "adt_bytearray.h"
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct message_client_connection_tag
{
   msocket_t *msocket;
   adt_bytearray_t *pendingMessage;
   SEMAPHORE_T messageTransmitted;
} message_client_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int32_t message_client_connection_create(message_client_connection_t *self, uint8_t addressFamily);
void message_client_connection_destroy(message_client_connection_t *self);
message_client_connection_t *message_client_connection_new(uint8_t addressFamily);
void message_client_connection_delete(message_client_connection_t *self);

adt_error_t message_client_prepare_message(message_client_connection_t *self, adt_str_t *message);
int32_t message_client_connect_tcp(message_client_connection_t *self, const char *address, uint16_t port);
#ifndef _WIN32
int32_t message_client_connect_unix(message_client_connection_t *self, const char *socketPath);
#endif
int32_t message_client_wait_for_message_transmitted(message_client_connection_t *self);

#endif //MESSAGE_CLIENT_CONNECTION_H
