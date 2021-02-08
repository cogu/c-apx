/*****************************************************************************
* \file      apx_connection.h
* \author    Conny Gustafsson
* \date      2020-04-14
* \brief     APX client connection
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
#ifndef APX_CONNECTION_H
#define APX_CONNECTION_H

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
#endif
#include "apx/client.h"
#include "adt_str.h"
#include "adt_hash.h"
#include "adt_ary.h"
#include "osmacro.h"
#include "adt_hash.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_connection_tag
{
   apx_client_t *client;
   adt_hash_t provide_port_lookup_table; //Key is provide port name, value is port instance (apx_portInstance_t*) (weak references)
   MUTEX_T mutex;
} apx_connection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_connection_create(apx_connection_t *self);
void apx_connection_destroy(apx_connection_t *self);
apx_connection_t *apx_connection_new(void);
void apx_connection_delete(apx_connection_t *self);

void apx_connection_disconnect(apx_connection_t *self);
apx_error_t apx_connection_attachNode(apx_connection_t *self, adt_str_t *apx_definition);
int32_t apx_connection_getLastErrorLine(apx_connection_t *self);
apx_nodeInstance_t *apx_connection_getLastAttachedNode(apx_connection_t *self);
#ifndef _WIN32
apx_error_t apx_connection_connect_unix(apx_connection_t *self, const char *socketPath);
#endif
apx_error_t apx_connection_connect_tcp(apx_connection_t *self, const char *address, uint16_t port);
apx_error_t apx_connection_writeProvidePortData(apx_connection_t *self, const char *providePortName, dtl_dv_t *dv_value);

#endif //APX_CONNECTION_H
