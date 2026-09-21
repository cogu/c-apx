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
static void apx_event_loop_process_event(apx_event_loop_t *self, apx_event_t *event, apx_event_handler_func_t *event_handler, void *event_handler_arg);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_event_loop_create(apx_event_loop_t *self)
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

void apx_event_loop_destroy(apx_event_loop_t *self, void (*destructor)(void*, apx_event_t*), void *destructor_arg)
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

apx_event_loop_t *apx_event_loop_new(void)
{
   apx_event_loop_t *self = (apx_event_loop_t*) malloc(sizeof(apx_event_loop_t));
   if (self != NULL)
   {
      apx_error_t errorType = apx_event_loop_create(self);
      if (errorType != APX_NO_ERROR)
      {
         free(self);
         self = NULL;
      }
   }
   return self;
}

void apx_event_loop_delete(apx_event_loop_t* self, void (*destructor)(void*, apx_event_t*), void* destructor_arg)
{
   if (self != NULL)
   {
      apx_event_loop_destroy(self, destructor, destructor_arg);
      free(self);
   }
}

void apx_event_loop_append(apx_event_loop_t *self, apx_event_t *event)
{
   SPINLOCK_ENTER(self->lock);
   adt_rbfh_insert(&self->pendingEvents, (const uint8_t*) event);
   SPINLOCK_LEAVE(self->lock);
#ifndef UNIT_TEST
   SEMAPHORE_POST(self->semaphore);
#endif
}

void apx_event_loop_exit(apx_event_loop_t *self)
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
void apx_event_loop_run(apx_event_loop_t *self, apx_event_handler_func_t *event_handler, void *event_handler_arg)
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
            apx_event_loop_process_event(self, &event, event_handler, event_handler_arg);
         }
      }
   }
}

uint16_t apx_event_loop_num_pending_events(apx_event_loop_t *self)
{
   if (self != NULL)
   {
      return adt_rbfh_length(&self->pendingEvents);
   }
   return 0;
}



#ifdef UNIT_TEST
/**
 * Special version of apx_event_loop_run that is suitable for unit tests (where no threads are used)
 */
void apx_event_loop_run_all(apx_event_loop_t *self, apx_event_handler_func_t *event_handler, void *event_handler_arg)
{
   while(true)
   {
      apx_event_t event;
      uint8_t rc = adt_rbfh_remove(&self->pendingEvents,(uint8_t*) &event);
      if (rc == BUF_E_OK)
      {
         apx_event_loop_process_event(self, &event, event_handler, event_handler_arg);
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

static void apx_event_loop_process_event(apx_event_loop_t *self, apx_event_t *event, apx_event_handler_func_t *event_handler, void *event_handler_arg)
{
   (void)self;
   if(event_handler != 0)
   {
      event_handler(event_handler_arg, event);
   }
}
