//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "os_task.h"
#ifdef _MSC_VER
#include <process.h>
#endif
#include "osmacro.h"
#include <malloc.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define THREAD_STACK_SIZE 65536

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * return 0 on success, non-zero on error
 */
int8_t os_task_create(os_task_t *self, THREAD_PROTO_PTR(thread_func, arg), uint16_t u16MaxNumEvents)
{
   if (self != 0)
   {
      self->eventQueueBufIsWeakRef = false;
      self->workerThreadValid = false;
      self->eventQueueBuf = (uint16_t*) malloc(sizeof(uint16_t)*((size_t)u16MaxNumEvents));
      if (self->eventQueueBuf != 0)
      {
         self->thread_func = thread_func;
         rbfu16_create(&self->eventQueue, self->eventQueueBuf, u16MaxNumEvents);
         SEMAPHORE_CREATE(self->semaphore);
         SPINLOCK_INIT(self->lock);
         return 0;
      }
   }
   return 1;
}

void os_task_destroy(os_task_t *self)
{
   if (self != 0)
   {
      SPINLOCK_DESTROY(self->lock);
      if ( (self->eventQueueBuf != 0) && (self->eventQueueBufIsWeakRef == false) )
      {
         free(self->eventQueueBuf);
      }
   }
}

void os_task_start(os_task_t *self)
{
   if ( (self != 0) && (self->workerThreadValid == false) )
   {
#ifndef _MSC_VER
      pthread_attr_t attr;
#endif

#ifdef _MSC_VER
      THREAD_CREATE(self->workerThread, self->thread_func, self, self->threadId);
      if (self->workerThread == INVALID_HANDLE_VALUE) 
      {
         self->workerThreadValid = false;
         return;
      }
#else
      pthread_attr_init(&attr);
      pthread_attr_setstacksize(&attr, THREAD_STACK_SIZE);
      THREAD_CREATE_ATTR(self->workerThread,attr,self->thread_func,self);
#endif
      self->workerThreadValid = true;
   }   
}

void os_task_stop(os_task_t *self)
{
   if (self != 0)
   {
      os_task_setEvent(self, OS_SHUTDOWN_EVENT_ID);      
      THREAD_JOIN(self->workerThread);
      THREAD_DESTROY(self->workerThread);
      self->workerThreadValid = false;
   }
}

void os_task_setEvent(os_task_t *self, uint16_t eventId)
{
   if ((self != 0) && (self->workerThreadValid != false))
   {
      SPINLOCK_ENTER(self->lock);
      rbfu16_insert(&self->eventQueue, eventId);
      SPINLOCK_LEAVE(self->lock);
      SEMAPHORE_POST(self->semaphore);      
   }
}

uint16_t os_task_waitEvent(os_task_t *self)
{
   if (self != 0)
   {
   #ifdef _MSC_VER
      DWORD result = WaitForSingleObject(self->semaphore, INFINITE);
      if (result == WAIT_OBJECT_0)
   #else
      int result = sem_wait(&self->semaphore);
      if (result == 0)
   #endif
      {
         uint16_t eventId;
         uint8_t rc;
         SPINLOCK_ENTER(self->lock);
         rc = rbfu16_remove(&self->eventQueue, &eventId);
         SPINLOCK_LEAVE(self->lock);
         if (rc == E_BUF_OK)
         {
            return eventId;
         }
         else
         {
            return OS_INVALID_EVENT_ID;
         }
      }
   }
   return OS_INVALID_EVENT_ID;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
