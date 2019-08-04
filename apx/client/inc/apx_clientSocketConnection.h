/*****************************************************************************
* \file      apx_clientSocketConnection.h
* \author    Conny Gustafsson
* \date      2018-12-31
* \brief     Client socket connection. Inherits from apx_clientConnectionBase_t (which in turn inherits from base class apx_connectionBase_t)
*
* Copyright (c) 2018-2019 Conny Gustafsson
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
#ifndef APX_CLIENT_SOCKET_CONNECTION_H
#define APX_CLIENT_SOCKET_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "apx_clientConnectionBase.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
#define SOCKET_TYPE struct testsocket_tag
#else
#define SOCKET_TYPE struct msocket_t
#endif
SOCKET_TYPE; //this is a forward declaration of the declared type just above

typedef struct apx_clientSocketConnection_tag
{
   apx_clientConnectionBase_t base;
   adt_bytearray_t sendBuffer;
   SOCKET_TYPE *socketObject;
}apx_clientSocketConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientSocketConnection_create(apx_clientSocketConnection_t *self, SOCKET_TYPE *socketObject, struct apx_client_tag *client);
void apx_clientSocketConnection_destroy(apx_clientSocketConnection_t *self);
void apx_clientSocketConnection_vdestroy(void *arg);
apx_clientSocketConnection_t *apx_clientSocketConnection_new(SOCKET_TYPE *socketObject, struct apx_client_tag *client);

#ifdef UNIT_TEST
apx_error_t apx_clientSocketConnection_connect(apx_clientSocketConnection_t *self);
#else
apx_error_t apx_clientConnection_tcp_connect(apx_clientSocketConnection_t *self, const char *address, uint16_t port);
apx_error_t apx_clientConnection_unix_connect(apx_clientSocketConnection_t *self, const char *socketPath);
#endif

#undef SOCKET_TYPE
#endif //APX_CLIENT_SOCKET_CONNECTION_H
