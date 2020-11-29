/*****************************************************************************
* \file      apx_serverSocketConnection.h
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Server socket connection. Inherits from apx_serverConnectionBase_t (which in turn uses base class apx_connectionBase_t)
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#ifndef APX_SERVER_SOCKET_CONNECTION_H
#define APX_SERVER_SOCKET_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_bytearray.h"
#include "apx_serverConnectionBase.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
#define SOCKET_TYPE struct testsocket_tag
#else
#define SOCKET_TYPE struct msocket_t
#endif
SOCKET_TYPE; //this is a forward declaration of the declared type just above

typedef struct apx_serverSocketConnection_tag
{
   apx_serverConnectionBase_t base;
   adt_bytearray_t sendBuffer;
   SOCKET_TYPE *socketObject;
}apx_serverSocketConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverSocketConnection_create(apx_serverSocketConnection_t *self, SOCKET_TYPE *socketObject);
void apx_serverSocketConnection_destroy(apx_serverSocketConnection_t *self);
void apx_serverSocketConnection_vdestroy(void *arg);
apx_serverSocketConnection_t *apx_serverSocketConnection_new(SOCKET_TYPE *socketObject);
void apx_serverSocketConnection_delete(apx_serverSocketConnection_t *self);
void apx_serverSocketConnection_vdelete(void *arg);
void apx_serverSocketConnection_start(apx_serverSocketConnection_t *self);
void apx_serverSocketConnection_vstart(void *arg);
void apx_serverSocketConnection_close(apx_serverSocketConnection_t *self);
void apx_serverSocketConnection_vclose(void *arg);


#undef SOCKET_TYPE
#endif //APX_SERVER_SOCKET_CONNECTION_H
