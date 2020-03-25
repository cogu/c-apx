/*****************************************************************************
* \file      apx_connectionManager.c
* \author    Conny Gustafsson
* \date      2018-12-28
* \brief     APX server connection manager
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
#include "apx_connectionManager.h"
#include <stdio.h>
#include <errno.h>
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
static uint32_t apx_connectionManager_generateConnectionId(apx_connectionManager_t *self);
THREAD_PROTO(cleanupTask, arg);
static void apx_connectionManager_cleanupTask_run(apx_connectionManager_t *self, int32_t numInactiveConnections);

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
      adt_list_create(&self->activeConnections, apx_connectionBase_vdelete); //the base class has the actual destructor using vtable
      adt_list_create(&self->inactiveConnections, apx_connectionBase_vdelete);
      adt_u32Set_create(&self->connectionIdSet);
      self->nextConnectionId = 0u;
      self->numConnections = 0u;
      self->cleanupThreadRunning = false;
      self->cleanupThreadValid = false;
   }
}

void apx_connectionManager_destroy(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      apx_connectionManager_stop(self);
      adt_list_destroy(&self->activeConnections);
      adt_list_destroy(&self->inactiveConnections);
      adt_u32Set_destroy(&self->connectionIdSet);
      SPINLOCK_DESTROY(self->lock);
   }
}

void apx_connectionManager_start(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      self->cleanupThreadRunning = true;
      self->cleanupThreadValid = true;
#ifdef _WIN32
      THREAD_CREATE(self->cleanupThread, cleanupTask, (void*) self, self->cleanupThreadId);
#else
      THREAD_CREATE(self->cleanupThread, cleanupTask, (void*) self);
#endif
   }
}

void apx_connectionManager_stop(apx_connectionManager_t *self)
{
   if ( (self != 0) && (self->cleanupThreadValid == true) )
   {
#ifndef _WIN32
   void *result;
#endif
      SPINLOCK_ENTER(self->lock);
      self->cleanupThreadRunning = 0;
      SPINLOCK_LEAVE(self->lock);
#ifdef _WIN32
      WaitForSingleObject( self->cleanupThread, INFINITE );
      CloseHandle( self->cleanupThread );
#else
      pthread_join(self->cleanupThread, &result);
#endif
      self->cleanupThreadValid = false;
   }
}

void apx_connectionManager_attach(apx_connectionManager_t *self, apx_serverConnectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0) )
   {
      uint32_t connectionId;
      SPINLOCK_ENTER(self->lock);
      connectionId = apx_connectionManager_generateConnectionId(self);
      apx_serverConnectionBase_connectNotify(connection, connectionId);
      adt_list_insert_unique(&self->activeConnections, connection);

      SPINLOCK_LEAVE(self->lock);
#if (APX_DEBUG_ENABLE)
      printf("[CONNECTION-MANAGER] New connection %d\n", (int) connectionId );
#endif
   }
}

void apx_connectionManager_detach(apx_connectionManager_t *self, apx_serverConnectionBase_t *connection)
{
   if ( (self != 0) && (connection != 0))
   {
      adt_list_elem_t *iter;
      SPINLOCK_ENTER(self->lock);
      iter = adt_list_find(&self->activeConnections, connection);
      if (iter != 0)
      {
         adt_list_erase(&self->activeConnections, iter);
         adt_list_insert(&self->inactiveConnections, connection);
      }
      SPINLOCK_LEAVE(self->lock);
   }
}

apx_serverConnectionBase_t* apx_connectionManager_getLastConnection(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      if (adt_list_is_empty(&self->activeConnections) == false)
      {
         return (apx_serverConnectionBase_t*) adt_list_last(&self->activeConnections);
      }
   }
   return (apx_serverConnectionBase_t*) 0;
}

uint32_t apx_connectionManager_getNumConnections(apx_connectionManager_t *self)
{
   if (self != 0)
   {
      return self->numConnections;
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
         int32_t numInactiveConnections;
         adt_list_elem_t *it = adt_list_iter_first(&self->activeConnections);
         //run the event loop of each active connection
         while(it != 0)
         {
            apx_serverConnectionBase_t *baseConnection = (apx_serverConnectionBase_t*) it->pItem;
            apx_serverConnectionBase_run(baseConnection);
            it = adt_list_iter_next(it);
         }
         it = adt_list_iter_first(&self->inactiveConnections);
         //run the event loop of each inactive connection
         while(it != 0)
         {
            apx_serverConnectionBase_t *baseConnection = (apx_serverConnectionBase_t*) it->pItem;
            apx_serverConnectionBase_run(baseConnection);
            it = adt_list_iter_next(it);
         }
         //run the cleanup task
         numInactiveConnections = adt_list_length(&self->inactiveConnections);
         apx_connectionManager_cleanupTask_run(self, numInactiveConnections);
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
static uint32_t apx_connectionManager_generateConnectionId(apx_connectionManager_t *self)
{
   for(;;)
   {
      bool result = adt_u32Set_contains(&self->connectionIdSet, self->nextConnectionId);
      if (result == false)
      {
         adt_u32Set_insert(&self->connectionIdSet, self->nextConnectionId);
         break;
      }
      self->nextConnectionId++;
      if (self->nextConnectionId == APX_INVALID_CONNECTION_ID)
      {
         self->nextConnectionId++;
      }
   }
   return self->nextConnectionId;
}




THREAD_PROTO(cleanupTask,arg)
{
   apx_connectionManager_t *self = (apx_connectionManager_t*) arg;
   if(self != 0)
   {
      while(1)
      {
         bool isRunning;
         int32_t numInactiveConnections;
         SLEEP(CLEANUP_WAIT_TIME);
         SPINLOCK_ENTER(self->lock);
         isRunning = self->cleanupThreadRunning;
         numInactiveConnections = adt_list_length(&self->inactiveConnections);
         SPINLOCK_LEAVE(self->lock);
         if (isRunning == false)
         {
            break;
         }
#if (APX_DEBUG_ENABLE)
         //printf("[CONNECTION-MANAGER] Running cleanupTask\n");
#endif
         apx_connectionManager_cleanupTask_run(self, numInactiveConnections);
#if (APX_DEBUG_ENABLE)
         //printf("[CONNECTION-MANAGER] Done running cleanupTask\n");
#endif
      }
   }
   THREAD_RETURN(0);
}

/**
 * Called by cleanupTask thread (or from internal run function during unit test)
 */
static void apx_connectionManager_cleanupTask_run(apx_connectionManager_t *self, int32_t numInactiveConnections)
{
   if (numInactiveConnections > 0)
   {
      SPINLOCK_ENTER(self->lock);
      adt_list_elem_t *iter = adt_list_iter_first(&self->inactiveConnections);
      apx_serverConnectionBase_t *serverConnection = (apx_serverConnectionBase_t*) iter->pItem;
      if ( (apx_connectionBase_getNumPendingWorkerMessages(&serverConnection->base) == 0u) && (apx_connectionBase_getNumPendingEvents(&serverConnection->base) == 0u))
      {
#if (APX_DEBUG_ENABLE)
         printf("[CONNECTION-MANAGER] Cleaning up %d\n", (int) serverConnection->base.connectionId);
#endif
         adt_list_erase(&self->inactiveConnections, iter);
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



