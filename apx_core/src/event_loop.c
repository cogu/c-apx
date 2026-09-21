/*****************************************************************************
* \file      event_loop.c
* \author    Conny Gustafsson
* \date      2018-10-15
* \brief     APX event loop
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <assert.h>
#include "apx/event_loop.h"
#include "apx/event.h"
#include "apx/logging.h"
#include "apx/file_manager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_eventLoop_processEvent(apx_eventLoop_t *self, apx_event_t *event, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_eventLoop_create(apx_eventLoop_t *self)
{
   if (self != NULL)
   {
      adt_buf_err_t result = adt_rbfh_create(&self->pendingEvents, (uint8_t) APX_EVENT_SIZE);
      if (result != BUF_E_OK)
      {
         return APX_MEM_ERROR;
      }
      self->exitFlag = false;
      (void)SPINLOCK_INIT(self->lock);
      SEMAPHORE_CREATE(self->semaphore);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_eventLoop_destroy(apx_eventLoop_t *self, void (*destructor)(void*, apx_event_t*), void *destructor_arg)
{
   if (self != NULL)
   {
      if (destructor != NULL)
      {
         apx_event_t event;
         adt_buf_err_t rc = BUF_E_OK;
         SPINLOCK_ENTER(self->lock);
         while (rc == BUF_E_OK)
         {
            rc = adt_rbfh_remove(&self->pendingEvents, (uint8_t*)&event);
            if (rc == BUF_E_OK)
            {
               destructor(destructor_arg, &event);
            }
         }         
         SPINLOCK_LEAVE(self->lock);
      }
      SPINLOCK_DESTROY(self->lock);
      adt_rbfh_destroy(&self->pendingEvents);
   }
}

apx_eventLoop_t *apx_eventLoop_new(void)
{
   apx_eventLoop_t *self = (apx_eventLoop_t*) malloc(sizeof(apx_eventLoop_t));
   if (self != NULL)
   {
      apx_error_t errorType = apx_eventLoop_create(self);
      if (errorType != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_eventLoop_delete(apx_eventLoop_t* self, void (*destructor)(void*, apx_event_t*), void* destructor_arg)
{
   if (self != NULL)
   {
      apx_eventLoop_destroy(self, destructor, destructor_arg);
      free(self);
   }
}

void apx_eventLoop_append(apx_eventLoop_t *self, apx_event_t *event)
{
   SPINLOCK_ENTER(self->lock);
   adt_rbfh_insert(&self->pendingEvents, (const uint8_t*) event);
   SPINLOCK_LEAVE(self->lock);
#ifndef UNIT_TEST
   SEMAPHORE_POST(self->semaphore);
#endif
}

void apx_eventLoop_exit(apx_eventLoop_t *self)
{
   if (self != NULL)
   {
      SPINLOCK_ENTER(self->lock);
      self->exitFlag = true;
      SPINLOCK_LEAVE(self->lock);
      SEMAPHORE_POST(self->semaphore);
   }
}

/**
 * Executes events in an infinite loop. This function will only return when self->exitFlag is set to true
 */
void apx_eventLoop_run(apx_eventLoop_t *self, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg)
{
   bool exitFlag = false;
   while(exitFlag == false)
   {
      apx_event_t event;
#ifdef _MSC_VER
      DWORD result = WaitForSingleObject(self->semaphore, INFINITE);
      if (result == WAIT_OBJECT_0)
#else
      int result = sem_wait(&self->semaphore);
      if (result == 0)
#endif
      {
         SPINLOCK_ENTER(self->lock);
         exitFlag = self->exitFlag;
         if (exitFlag == false)
         {
            adt_rbfh_remove(&self->pendingEvents,(uint8_t*) &event);
         }
         SPINLOCK_LEAVE(self->lock);
         if (exitFlag == false)
         {
            apx_eventLoop_processEvent(self, &event, eventHandler, eventHandlerArg);
         }
      }
   }
}

uint16_t apx_eventLoop_numPendingEvents(apx_eventLoop_t *self)
{
   if (self != NULL)
   {
      return adt_rbfh_length(&self->pendingEvents);
   }
   return 0;
}



#ifdef UNIT_TEST
/**
 * Special version of apx_eventLoop_run that is suitable for unit tests (where no threads are used)
 */
void apx_eventLoop_runAll(apx_eventLoop_t *self, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg)
{
   while(true)
   {
      apx_event_t event;
      uint8_t rc = adt_rbfh_remove(&self->pendingEvents,(uint8_t*) &event);
      if (rc == BUF_E_OK)
      {
         apx_eventLoop_processEvent(self, &event, eventHandler, eventHandlerArg);
      }
      else
      {
         break;
      }
   }
}
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_eventLoop_processEvent(apx_eventLoop_t *self, apx_event_t *event, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg)
{
   (void)self;
   if(eventHandler != 0)
   {
      eventHandler(eventHandlerArg, event);
   }
}
