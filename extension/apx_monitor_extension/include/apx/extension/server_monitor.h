/*****************************************************************************
* \file      server_monitor.h
* \author    Conny Gustafsson
* \date      2021-02-28
* \brief     Part of monitor extension
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SERVER_MONITOR_H
#define APX_SERVER_MONITOR_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/server_extension.h"
#include "apx/server_connection.h"
#include "apx/extension/observed_connection.h"
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

typedef struct apx_serverMonitor_tag
{
   struct apx_server_tag* server;
   adt_list_t connection_observers; //strong references to apx_observedConnection_t
   adt_list_t monitor_connections; //weak references to apx_serverConnection_t
   MUTEX_T lock;
} apx_serverMonitor_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_serverMonitor_create(apx_serverMonitor_t* self, struct apx_server_tag* server);
void apx_serverMonitor_destroy(apx_serverMonitor_t* self);
apx_serverMonitor_t* apx_serverMonitor_new(struct apx_server_tag* server);
void apx_serverMonitor_delete(apx_serverMonitor_t* self);
int32_t apx_serverMonitor_num_connections(apx_serverMonitor_t* self);
apx_observedConnection_t* apx_serverMonitor_get_last_observed_connection(apx_serverMonitor_t* self);

//Virtual call points
void apx_serverMonitor_virtual_on_new_connection(void* arg, apx_serverConnection_t* connection);
void apx_serverMonitor_virtual_on_connection_closed(void* arg, apx_serverConnection_t* connection);
void apx_serverMonitor_virtual_on_protocol_header_accepted(void* arg, apx_connectionBase_t* connection);

#endif //APX_SERVER_MONITOR_H
