/*****************************************************************************
* \file      server_monitor_state.h
* \author    Conny Gustafsson
* \date      2021-02-28
* \brief     Server Monitor State
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
#ifndef APX_SERVER_MONITOR_STATE_H
#define APX_SERVER_MONITOR_STATE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/server_extension.h"
#include "apx/server_connection.h"
#include "apx/file_info.h"
#include "adt_list.h"
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_MONITOR_EXTENSION_CFG_KEY "monitor"

struct apx_server_tag;

typedef struct apx_serverMonitorState_tag
{
   struct apx_server_tag* server;
   adt_list_t client_connections; //weak references to apx_serverConnection_t
   MUTEX_T lock;
} apx_serverMonitorState_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_serverMonitorState_create(apx_serverMonitorState_t* self, struct apx_server_tag* server);
void apx_serverMonitorState_destroy(apx_serverMonitorState_t* self);
apx_serverMonitorState_t* apx_serverMonitorState_new(struct apx_server_tag* server);
void apx_serverMonitorState_delete(apx_serverMonitorState_t* self);

//Virtual call points
void apx_serverMonitorState_virtual_on_new_connection(void* arg, apx_serverConnection_t* connection);
void apx_serverMonitorState_virtual_on_connection_closed(void* arg, apx_serverConnection_t* connection);
void apx_serverMonitorState_virtual_on_protocol_header_accepted(void* arg, struct apx_connectionBase_tag* connection);

#endif //APX_SERVER_MONITOR_STATE_H
