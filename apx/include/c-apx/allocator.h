/*****************************************************************************
* \file      allocator.h
* \author    Conny Gustafsson
* \date      2017-02-20
* \brief     Small object allocator for usage within c-apx
*
* Copyright (c) 2017-2020 Conny Gustafsson
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
#ifndef APX_ALLOCATOR_H
#define APX_ALLOCATOR_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#include "osmacro.h"
#include "adt_ringbuf.h"
#include "soa.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

/**
 * this is a simple small object allocator with built-in garbage collector thread
 */
typedef struct apx_allocator_tag
{
   THREAD_T workerThread; //local worker thread
   SPINLOCK_T lock;  //variable lock
   SEMAPHORE_T semaphore; //thread semaphore

   //data object, all read/write accesses to these must be protected by the lock variable above
   adt_rbfh_t messages; //pending cleanup messages (ringbuffer)
   bool isRunning; //when false it's time do shut down
   bool workerThreadValid; //true if workerThread is a valid variable
   soa_t soa;

#ifdef _MSC_VER
   unsigned int threadId;
#endif
}apx_allocator_t;

//this data structure is used as elements in the ring buffer
typedef struct rbf_data_tag
{
   uint8_t *ptr;
   uint32_t size;
}rbf_data_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_allocator_create(apx_allocator_t *self, uint16_t maxPendingMessages);
void apx_allocator_destroy(apx_allocator_t *self);

void apx_allocator_start(apx_allocator_t *self);
void apx_allocator_stop(apx_allocator_t *self);
uint8_t *apx_allocator_alloc(apx_allocator_t *self, size_t size);
void apx_allocator_free(apx_allocator_t *self, uint8_t *ptr, size_t size);
bool apx_allocator_isRunning(apx_allocator_t *self);
#ifdef UNIT_TEST
void apx_allocator_processAll(apx_allocator_t *self);
int32_t apx_allocator_numPendingMessages(apx_allocator_t *self);
#endif

#endif //APX_ALLOCATOR_H
