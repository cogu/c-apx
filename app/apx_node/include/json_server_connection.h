/*****************************************************************************
* \file      json_server_connection.h
* \author    Conny Gustafsson
* \date      2020-03-07
* \brief     Connection in the json server
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
//forward declarations
struct apx_connection_tag;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct json_server_connection_tag
{
   msocket_t *msocket; //Strong reference
   struct apx_connection_tag *apx_connection; //Weak reference
} json_server_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void json_server_connection_create(json_server_connection_t *self, msocket_t *msocket, struct apx_connection_tag *apx_connection);
void json_server_connection_destroy(json_server_connection_t *self);
json_server_connection_t *json_server_connection_new(msocket_t *msocket, struct apx_connection_tag *apx_connection);
void json_server_connection_delete(json_server_connection_t *self);
void json_server_connection_vdelete(void *arg);
void json_server_connection_start(json_server_connection_t *self);

#endif //MESSAGE_SERVER_CONNECTION_H
