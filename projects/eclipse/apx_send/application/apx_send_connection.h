/*****************************************************************************
* \file      apx_send_connection.h
* \author    Conny Gustafsson
* \date      2020-03-09
* \brief     APX client connection for sending dynamic values to APX server
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
#ifndef APX_SEND_CONNECTION_H
#define APX_SEND_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_client.h"
#include "adt_bytearray.h"
#include <pthread.h>
#include "osmacro.h"
#include "adt_hash.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_send_connection_tag
{
   apx_client_t *client;
   adt_bytearray_t *apx_definition;
   adt_hash_t portLookupTable; //Key is provide port name, value is port handle (void*) (weak references)
   MUTEX_T mutex;
} apx_send_connection_t;
//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_send_connection_create(apx_send_connection_t *self);
void apx_send_connection_destroy(apx_send_connection_t *self);
apx_send_connection_t *apx_send_connection_new(void);
void apx_send_connection_delete(apx_send_connection_t *self);

apx_error_t apx_send_connection_attachNode(apx_send_connection_t *self, adt_str_t *apx_definition);
int32_t apx_send_connection_getLastErrorLine(apx_send_connection_t *self);
apx_nodeInstance_t *apx_send_connection_getLastAttachedNode(apx_send_connection_t *self);
apx_error_t apx_send_connection_prepareProvidePorts(apx_send_connection_t *self);
apx_error_t apx_send_connection_connect_unix(apx_send_connection_t *self, const char *socketPath);
apx_error_t apx_send_connection_writeProvidePortData(apx_send_connection_t *self, const char *providePortName, dtl_dv_t *dv_value);


#endif //APX_SENDER_CONNECTION_H
