/*****************************************************************************
* \file      event_loop.c
* \author    Conny Gustafsson
* \date      2018-10-15
* \brief     APX event loop
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
   if (self != 0)
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

void apx_eventLoop_destroy(apx_eventLoop_t *self)
{
   if (self != 0)
   {
      SPINLOCK_DESTROY(self->lock);
      adt_rbfh_destroy(&self->pendingEvents);
   }
}

apx_eventLoop_t *apx_eventLoop_new(void)
{
   apx_eventLoop_t *self = (apx_eventLoop_t*) malloc(sizeof(apx_eventLoop_t));
   if (self != 0)
   {
      apx_error_t errorType = apx_eventLoop_create(self);
      if (errorType != APX_NO_ERROR)
      {
         free(self);
         self = (apx_eventLoop_t*) 0;
      }
   }
   return self;
}

void apx_eventLoop_delete(apx_eventLoop_t *self)
{
   if (self != 0)
   {
      apx_eventLoop_destroy(self);
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
   if (self != 0)
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
   if (self != 0)
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
