/*****************************************************************************
* \file      connection_manager.h
* \author    Conny Gustafsson
* \date      2018-12-28
* \brief     Server connection manager
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
typedef struct apx_connection_manager_tag
{
   SPINLOCK_T lock; //thread lock
   adt_u32Set_t connection_id_set; //used to keep track of which connection IDs are in use
   adt_list_t active_connections; //Strong references to apx_server_connection_t
   adt_list_t inactive_connections; //Strong references to apx_server_connection_t
   uint32_t next_connection_id;
   uint32_t num_connections;
   THREAD_T cleanup_thread; //garbage collector thread
   bool cleanup_thread_running; //when false it's time do shut down
   bool cleanup_thread_valid; //true if cleanup_thread is a valid variable
#ifdef _MSC_VER
   unsigned int cleanup_thread_id;
#endif
} apx_connection_manager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connection_manager_create(apx_connection_manager_t *self);
void apx_connection_manager_destroy(apx_connection_manager_t *self);
void apx_connection_manager_start(apx_connection_manager_t *self);
void apx_connection_manager_stop(apx_connection_manager_t *self);
void apx_connection_manager_attach(apx_connection_manager_t *self, apx_server_connection_t *connection);
void apx_connection_manager_detach(apx_connection_manager_t *self, apx_server_connection_t *connection);
apx_server_connection_t* apx_connection_manager_get_last_connection(apx_connection_manager_t const* self);
uint32_t apx_connection_manager_get_num_connections(apx_connection_manager_t *self);
#ifdef UNIT_TEST
void apx_connection_manager_run(apx_connection_manager_t *self);
#endif


#endif //APX_CONNECTION_MANAGER_H
