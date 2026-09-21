/*****************************************************************************
* \file      message_client_connection.h
* \author    Conny Gustafsson
* \date      2020-03-08
* \brief     msocket connection that sends JSON data to to apx_sernder application
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
int32_t message_client_connection_create(message_client_connection_t *self, uint8_t address_family);
void message_client_connection_destroy(message_client_connection_t *self);
message_client_connection_t *message_client_connection_new(uint8_t address_family);
void message_client_connection_delete(message_client_connection_t *self);

adt_error_t message_client_prepare_message(message_client_connection_t *self, adt_str_t *message);
int32_t message_client_connect_tcp(message_client_connection_t *self, const char *address, uint16_t port);
#ifndef _WIN32
int32_t message_client_connect_unix(message_client_connection_t *self, const char *socket_path);
#endif
int32_t message_client_wait_for_message_transmitted(message_client_connection_t *self);

#endif //MESSAGE_CLIENT_CONNECTION_H
