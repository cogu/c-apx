/*****************************************************************************
* \file      message_server_connection.h
* \author    Conny Gustafsson
* \date      2020-03-07
* \brief     Connection in the message server
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
#ifndef MESSAGE_SERVER_CONNECTION_H
#define MESSAGE_SERVER_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "msocket.h"
#include "apx_send_connection.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct message_server_connection_tag
{
   msocket_t *msocket; //Strong reference
   apx_send_connection_t *apx_connection; //Weak reference
} message_server_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void message_server_connection_create(message_server_connection_t *self, msocket_t *msocket, apx_send_connection_t *apx_connection);
void message_server_connection_destroy(message_server_connection_t *self);
message_server_connection_t *message_server_connection_new(msocket_t *msocket, apx_send_connection_t *apx_connection);
void message_server_connection_delete(message_server_connection_t *self);
void message_server_connection_vdelete(void *arg);
void message_server_connection_start(message_server_connection_t *self);

#endif //MESSAGE_SERVER_CONNECTION_H
