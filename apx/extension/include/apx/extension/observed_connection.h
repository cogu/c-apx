/*****************************************************************************
* \file      monitor_connection_state.h
* \author    Conny Gustafsson
* \date      2021-04-10
* \brief     Current connection state seen by a server monitor
*
* Copyright (c) 2021 Conny Gustafsson
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
#ifndef APX_OBSERVED_CONNECTION_H
#define APX_OBSERVED_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "adt_str.h"
#include "adt_list.h"
#include "apx/server_connection.h"
#include "apx/extension/observed_file.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_observedConnection_tag
{
   apx_serverConnection_t* server_connection;
   apx_connectionId_t connection_id;
   apx_connectionType_t connection_type;
   apx_connectionState_t connection_state;
   adt_str_t* tag;
   adt_list_t file_list; //strong references to apx_observed_file_t
} apx_observedConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


void apx_observedConnection_create(apx_observedConnection_t* self, apx_serverConnection_t* server_connection);
void apx_observedConnection_destroy(apx_observedConnection_t* self);
apx_observedConnection_t* apx_observedConnection_new(apx_serverConnection_t* server_connection);
void apx_observedConnection_delete(apx_observedConnection_t* self);
void apx_observedConnection_vdelete(void* arg);
char const* apx_observedConnection_tag(apx_observedConnection_t* const self);
apx_connectionId_t apx_observedConnection_connection_id(apx_observedConnection_t* const self);
apx_connectionType_t apx_observedConnection_get_connection_type(apx_observedConnection_t* const self);
void apx_observedConnection_set_connection_type(apx_observedConnection_t* self, apx_connectionType_t connection_type);
apx_connectionState_t apx_observedConnection_get_connection_state(apx_observedConnection_t* const self);
void apx_observedConnection_set_connection_state(apx_observedConnection_t* self, apx_connectionState_t connection_state);

#endif //APX_OBSERVED_CONNECTION_H
