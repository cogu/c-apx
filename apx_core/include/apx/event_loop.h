/*****************************************************************************
* \file      event_loop.h
* \author    Conny Gustafsson
* \date      2018-10-15
* \brief     APX event loop
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_EVENT_LOOP_H
#define APX_EVENT_LOOP_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/event_listener.h"
#include "apx/event.h"
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

typedef struct apx_event_loop_tag
{
   SPINLOCK_T lock;
   SEMAPHORE_T semaphore;
   adt_rbfh_t pendingEvents;
   bool exitFlag;
} apx_event_loop_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_event_loop_create(apx_event_loop_t *self);
void apx_event_loop_destroy(apx_event_loop_t* self, void (*destructor)(void*, apx_event_t*), void* destructor_arg);
apx_event_loop_t *apx_event_loop_new(void);
void apx_event_loop_delete(apx_event_loop_t *self, void (*destructor)(void*, apx_event_t*), void* destructor_arg);
//void apx_event_loop_set_event_handler(apx_event_loop_t *self, apx_event_handler_func_t *event_handler, void *event_handler_arg);
//External events (handler implemented in this class)
void apx_event_loop_append(apx_event_loop_t *self, apx_event_t *event);
void apx_event_loop_run(apx_event_loop_t *self, apx_event_handler_func_t *event_handler, void *event_handler_arg);
void apx_event_loop_exit(apx_event_loop_t *self);
uint16_t apx_event_loop_num_pending_events(apx_event_loop_t *self);
#ifdef UNIT_TEST
void apx_event_loop_run_all(apx_event_loop_t *self, apx_event_handler_func_t *event_handler, void *event_handler_arg);
#endif

#endif //APX_EVENT_LOOP_H
