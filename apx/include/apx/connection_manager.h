/*****************************************************************************
* \file      connection_manager.h
* \author    Conny Gustafsson
* \date      2018-12-28
* \brief     Server connection manager
*
* Copyright (c) 2018-2021 Conny Gustafsson
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
#ifndef APX_CONNECTION_MANAGER_H
#define APX_CONNECTION_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/server_connection.h"
#include "adt_list.h"
#include "adt_set.h"
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <pthread.h>
#include <unistd.h> //needed for SLEEP macro
#endif
#include "osmacro.h"



//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_connectionManager_tag
{
   SPINLOCK_T lock; //thread lock
   adt_u32Set_t connection_id_set; //used to keep track of which connection IDs are in use
   adt_list_t active_connections; //Strong references to apx_serverConnection_t
   adt_list_t inactive_connections; //Strong references to apx_serverConnection_t
   uint32_t next_connection_id;
   uint32_t num_connections;
   THREAD_T cleanup_thread; //garbage collector thread
   bool cleanup_thread_running; //when false it's time do shut down
   bool cleanup_thread_valid; //true if cleanup_thread is a valid variable
#ifdef _MSC_VER
   unsigned int cleanup_thread_id;
#endif
} apx_connectionManager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connectionManager_create(apx_connectionManager_t *self);
void apx_connectionManager_destroy(apx_connectionManager_t *self);
void apx_connectionManager_start(apx_connectionManager_t *self);
void apx_connectionManager_stop(apx_connectionManager_t *self);
void apx_connectionManager_attach(apx_connectionManager_t *self, apx_serverConnection_t *connection);
void apx_connectionManager_detach(apx_connectionManager_t *self, apx_serverConnection_t *connection);
apx_serverConnection_t* apx_connectionManager_get_last_connection(apx_connectionManager_t const* self);
uint32_t apx_connectionManager_get_num_connections(apx_connectionManager_t *self);
#ifdef UNIT_TEST
void apx_connectionManager_run(apx_connectionManager_t *self);
#endif


#endif //APX_CONNECTION_MANAGER_H
