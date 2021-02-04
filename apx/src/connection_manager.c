/*****************************************************************************
* \file      connection_manager.c
* \author    Conny Gustafsson
* \date      2018-12-28
* \brief     Server connection manager
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <errno.h>
#include "apx/connection_manager.h"
#ifdef _WIN32
#include <process.h>
#endif
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define CLEANUP_WAIT_TIME 500

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static uint32_t apx_connectionManager_generate_connection_id(apx_connectionManager_t *self);
THREAD_PROTO(cleanup_task, arg);
static void apx_connectionManager_cleanup_task_main(apx_connectionManager_t *self, int32_t num_inactive_connections);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_connectionManager_create(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      SPINLOCK_INIT(self->lock);
      adt_list_create(&self->active_connections, apx_connectionBase_vdelete); //the base class has the actual destructor using vtable
      adt_list_create(&self->inactive_connections, apx_connectionBase_vdelete);
      adt_u32Set_create(&self->connection_id_set);
      self->next_connection_id = 0u;
      self->num_connections = 0u;
      self->cleanup_thread_running = false;
      self->cleanup_thread_valid = false;
   }
}

void apx_connectionManager_destroy(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      apx_connectionManager_stop(self);
      adt_list_destroy(&self->active_connections);
      adt_list_destroy(&self->inactive_connections);
      adt_u32Set_destroy(&self->connection_id_set);
      SPINLOCK_DESTROY(self->lock);
   }
}

void apx_connectionManager_start(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      self->cleanup_thread_running = true;
      self->cleanup_thread_valid = true;
#ifdef _WIN32
      THREAD_CREATE(self->cleanup_thread, cleanup_task, (void*) self, self->cleanup_thread_id);
#else
      THREAD_CREATE(self->cleanupThread, cleanupTask, (void*) self);
#endif
   }
}

void apx_connectionManager_stop(apx_connectionManager_t *self)
{
   if ( (self != 0) && (self->cleanup_thread_valid == true) )
   {
#ifndef _WIN32
   void *result;
#endif
      SPINLOCK_ENTER(self->lock);
      self->cleanup_thread_running = false;
      SPINLOCK_LEAVE(self->lock);
#ifdef _WIN32
      WaitForSingleObject( self->cleanup_thread, INFINITE );
      CloseHandle( self->cleanup_thread );
#else
      pthread_join(self->cleanupThread, &result);
#endif
      self->cleanup_thread_valid = false;
   }
}

void apx_connectionManager_attach(apx_connectionManager_t *self, apx_serverConnection_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      uint32_t connection_id;
      SPINLOCK_ENTER(self->lock);
      connection_id = apx_connectionManager_generate_connection_id(self);
      adt_list_insert_unique(&self->active_connections, connection);
      SPINLOCK_LEAVE(self->lock);
      apx_serverConnection_set_connection_id(connection, connection_id);
      apx_serverConnection_connected_notification(connection);
#if (APX_DEBUG_ENABLE)
      printf("[CONNECTION-MANAGER] New connection %d\n", (int)connection_id);
#endif
   }
}

void apx_connectionManager_detach(apx_connectionManager_t *self, apx_serverConnection_t *connection)
{
   if ( (self != 0) && (connection != 0))
   {
      adt_list_elem_t *iter;
      SPINLOCK_ENTER(self->lock);
      iter = adt_list_find(&self->active_connections, (void*)connection);
      if (iter != 0)
      {
         adt_list_erase(&self->active_connections, iter);
         adt_list_insert(&self->inactive_connections, connection);
      }
      SPINLOCK_LEAVE(self->lock);
   }
}

apx_serverConnection_t* apx_connectionManager_get_last_connection(apx_connectionManager_t const*self)
{
   if (self != 0)
   {
      if (adt_list_is_empty(&self->active_connections) == false)
      {
         return (apx_serverConnection_t*) adt_list_last(&self->active_connections);
      }
   }
   return (apx_serverConnection_t*) 0;
}

uint32_t apx_connectionManager_get_num_connections(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      return self->num_connections;
   }
   return 0;
}


#ifdef UNIT_TEST
#define APX_SERVER_RUN_CYCLES 10

void apx_connectionManager_run(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      int32_t i;
      for(i=0;i<APX_SERVER_RUN_CYCLES;i++)
      {
         int32_t num_inactive_connections;
         adt_list_elem_t *it = adt_list_iter_first(&self->active_connections);
         //run the event loop of each active connection
         while(it != 0)
         {
            apx_serverConnection_t * server_connection = (apx_serverConnection_t*) it->pItem;
            apx_serverConnection_run(server_connection);
            it = adt_list_iter_next(it);
         }
         it = adt_list_iter_first(&self->inactive_connections);
         //run the event loop of each inactive connection
         while(it != 0)
         {
            apx_serverConnection_t * server_connection = (apx_serverConnection_t*) it->pItem;
            apx_serverConnection_run(server_connection);
            it = adt_list_iter_next(it);
         }
         //run the cleanup task
         num_inactive_connections = adt_list_length(&self->inactive_connections);
         apx_connectionManager_cleanup_task_main(self, num_inactive_connections);
      }
   }
}


#endif



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * Generates a unique connection ID by comparing ID candidates against its internal set data structure
 * This function assumes that APX_SERVER_MAX_CONCURRENT_CONNECTIONS is much less than 2^32-1 and that the caller has previously checked that
 * self->numConnections < APX_SERVER_MAX_CONCURRENT_CONNECTIONS
 */
static uint32_t apx_connectionManager_generate_connection_id(apx_connectionManager_t *self)
{
   for(;;)
   {
      bool result = adt_u32Set_contains(&self->connection_id_set, self->next_connection_id);
      if (result == false)
      {
         adt_u32Set_insert(&self->connection_id_set, self->next_connection_id);
         break;
      }
      self->next_connection_id++;
      if (self->next_connection_id == APX_INVALID_CONNECTION_ID)
      {
         self->next_connection_id++;
      }
   }
   return self->next_connection_id;
}


THREAD_PROTO(cleanup_task,arg)
{
   apx_connectionManager_t *self = (apx_connectionManager_t*) arg;
   if(self != 0)
   {
      while(1)
      {
         bool is_running;
         int32_t num_inactive_connections;
         SLEEP(CLEANUP_WAIT_TIME);
         SPINLOCK_ENTER(self->lock);
         is_running = self->cleanup_thread_running;
         num_inactive_connections = adt_list_length(&self->inactive_connections);
         SPINLOCK_LEAVE(self->lock);
         if (is_running == false)
         {
            break;
         }
#if (APX_DEBUG_ENABLE)
         //printf("[CONNECTION-MANAGER] Running cleanupTask\n");
#endif
         apx_connectionManager_cleanup_task_main(self, num_inactive_connections);
#if (APX_DEBUG_ENABLE)
         //printf("[CONNECTION-MANAGER] Done running cleanupTask\n");
#endif
      }
   }
   THREAD_RETURN(0);
}

/**
 * Called by cleanup_task thread (or from internal run function during unit test)
 */
static void apx_connectionManager_cleanup_task_main(apx_connectionManager_t *self, int32_t num_inactive_connections)
{
   if (num_inactive_connections > 0)
   {
      SPINLOCK_ENTER(self->lock);
      adt_list_elem_t *iter = adt_list_iter_first(&self->inactive_connections);
      apx_serverConnection_t *serverConnection = (apx_serverConnection_t*) iter->pItem;
      if ( (apx_connectionBase_get_num_pending_worker_commands(&serverConnection->base) == 0u) && (apx_connectionBase_get_num_pending_events(&serverConnection->base) == 0u))
      {
#if (APX_DEBUG_ENABLE)
         printf("[CONNECTION-MANAGER] Cleaning up %d\n", (int) serverConnection->base.connectionId);
#endif
         adt_list_erase(&self->inactive_connections, iter);
         apx_connectionBase_stop(&serverConnection->base);
         apx_connectionBase_close(&serverConnection->base);
         apx_connectionBase_delete(&serverConnection->base);
#if (APX_DEBUG_ENABLE)
         printf("[CONNECTION-MANAGER] Cleanup complete\n");
#endif
      }
      SPINLOCK_LEAVE(self->lock);
   }
}



