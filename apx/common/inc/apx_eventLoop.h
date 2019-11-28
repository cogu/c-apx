/*****************************************************************************
* \file      apx_eventLoop.h
* \author    Conny Gustafsson
* \date      2018-10-15
* \brief     APX event loop
*
* Copyright (c) 2018 Conny Gustafsson
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
#ifndef APX_EVENT_LOOP_H
#define APX_EVENT_LOOP_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "apx_eventListener2.h"
#include "apx_event.h"
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
# include <semaphore.h>
#endif
#include "osmacro.h"
#include "adt_ringbuf.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations

typedef struct apx_eventLoop_tag
{
   SPINLOCK_T lock;
   SEMAPHORE_T semaphore;
   adt_rbfh_t pendingEvents;
   bool exitFlag;
} apx_eventLoop_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_eventLoop_create(apx_eventLoop_t *self);
void apx_eventLoop_destroy(apx_eventLoop_t *self);
apx_eventLoop_t *apx_eventLoop_new(void);
void apx_eventLoop_delete(apx_eventLoop_t *self);
void apx_eventLoop_setEventHandler(apx_eventLoop_t *self, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg);
//External events (handler implemented in this class)
void apx_eventLoop_append(apx_eventLoop_t *self, apx_event_t *event);
void apx_eventLoop_run(apx_eventLoop_t *self, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg);
void apx_eventLoop_exit(apx_eventLoop_t *self);
uint16_t apx_eventLoop_numPendingEvents(apx_eventLoop_t *self);
#ifdef UNIT_TEST
void apx_eventLoop_runAll(apx_eventLoop_t *self, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg);
#endif

#endif //APX_EVENT_LOOP_H
