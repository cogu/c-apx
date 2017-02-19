#ifndef APX_ALLOCATOR_H
#define APX_ALLOCATOR_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=100)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#include "osmacro.h"
#include "ringbuf.h"
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
   rbfs_t messages; //pending messages (ringbuffer)
   bool isRunning; //when false it's time do shut down
   bool workerThreadValid; //true if workerThread is a valid variable
   uint8_t *ringBufferData; //memory for ringbuffer
   uint32_t ringBufferLen; //number of items in ringbuffer
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
int8_t apx_allocator_create(apx_allocator_t *self, uint16_t maxPendingMessages);
void apx_allocator_destroy(apx_allocator_t *self);

void apx_allocator_start(apx_allocator_t *self);
void apx_allocator_stop(apx_allocator_t *self);
uint8_t *apx_allocator_alloc(apx_allocator_t *self, size_t size);
void apx_allocator_free(apx_allocator_t *self, uint8_t *ptr, uint32_t size);

#endif //APX_ALLOCATOR_H
