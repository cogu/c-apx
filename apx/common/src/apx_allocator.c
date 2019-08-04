//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#ifdef _MSC_VER
#include <process.h>
#endif
#include "apx_allocator.h"
#include "apx_logging.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_allocator_startThread(apx_allocator_t *self);
static THREAD_PROTO(threadTask,arg);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int8_t apx_allocator_create(apx_allocator_t *self, uint16_t maxPendingMessages)
{
   if (self != 0)
   {
      size_t elemSize = sizeof(rbf_data_t);
      adt_buf_err_t bufResult;

      bufResult = adt_rbfh_createEx(&self->messages, (uint8_t) elemSize, ADT_RBFSH_MIN_NUM_ELEMS_DEFAULT, maxPendingMessages);
      if (bufResult != BUF_E_OK)
      {
         return -1;
      }

#ifdef _WIN32
      self->workerThread = INVALID_HANDLE_VALUE;
#else
      self->workerThread = 0;
#endif
      self->workerThreadValid=false;
      SPINLOCK_INIT(self->lock);
      SEMAPHORE_CREATE(self->semaphore);
      self->isRunning = false;
      soa_init(&self->soa);
      return 0;
   }
   return -1;
}

void apx_allocator_destroy(apx_allocator_t *self)
{
   if (self != 0)
   {
      adt_rbfh_destroy(&self->messages);
      soa_destroy(&self->soa);
      SEMAPHORE_DESTROY(self->semaphore);
      SPINLOCK_DESTROY(self->lock);
   }
}

void apx_allocator_start(apx_allocator_t *self)
{
   if( (self != 0) && (self->workerThreadValid == false) )
   {
      apx_allocator_startThread(self);
   }
}

void apx_allocator_stop(apx_allocator_t *self)
{
   if( (self != 0) && (self->workerThreadValid == true) )
   {
#ifdef _MSC_VER
      DWORD result;
#endif
      rbf_data_t data = {0,0}; //sending a null-pointer with size 0 should wake up the workerThread
      //1. enqueue message
      SPINLOCK_ENTER(self->lock);
      self->isRunning = false;
      adt_rbfh_insert(&self->messages,(const uint8_t*) &data);
      SPINLOCK_LEAVE(self->lock);
      //2. wake workerThread
      SEMAPHORE_POST(self->semaphore);
#ifdef _MSC_VER
      result = WaitForSingleObject(self->workerThread, 5000);
      if (result == WAIT_TIMEOUT)
      {
         APX_LOG_ERROR("[APX_ALLOCATOR] timeout while joining workerThread");
      }
      else if (result == WAIT_FAILED)
      {
         DWORD lastError = GetLastError();
         APX_LOG_ERROR("[APX_ALLOCATOR]  joining workerThread failed with %d", (int)lastError);         
      }
      CloseHandle(self->workerThread);
      self->workerThread = INVALID_HANDLE_VALUE;
#else
      if(pthread_equal(pthread_self(),self->workerThread) == 0)
      {
         void *status;
         int s = pthread_join(self->workerThread, &status);
         if (s != 0)
         {
            APX_LOG_ERROR("[APX_ALLOCATOR] pthread_join error %d\n",s);
         }
      }
      else
      {
         APX_LOG_ERROR("[APX_ALLOCATOR] pthread_join attempted on pthread_self()\n");
      }
#endif
   }
}

uint8_t *apx_allocator_alloc(apx_allocator_t *self, size_t size)
{
   uint8_t *data = 0;
   if ( (self != 0) && (size > 0) )
   {
      if (size <= SOA_SMALL_OBJECT_MAX_SIZE)
      {
         //use the small object allocator
         SPINLOCK_ENTER(self->lock);
         data = (uint8_t*) soa_alloc(&self->soa, size);
         SPINLOCK_LEAVE(self->lock);
      }
      else
      {
         //use the default allocator
         data = (uint8_t*) malloc(size);
      }
   }
   return data;
}

void apx_allocator_free(apx_allocator_t *self, uint8_t *ptr, size_t size)
{
   if (self != 0)
   {
      rbf_data_t data;
      data.ptr=ptr;
      data.size=(uint32_t) size;
      //1. enqueue message
      SPINLOCK_ENTER(self->lock);
      adt_rbfh_insert(&self->messages,(const uint8_t*) &data);
      SPINLOCK_LEAVE(self->lock);
      //2. wake worker thread
      SEMAPHORE_POST(self->semaphore);
   }
}

bool apx_allocator_isRunning(apx_allocator_t *self)
{
   if ( self != 0)
   {
      return self->isRunning;
   }
   return false;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_allocator_startThread(apx_allocator_t *self)
{
   if( self != 0){
   self->isRunning = true;
   self->workerThreadValid = true;
#ifdef _WIN32
      THREAD_CREATE(self->workerThread,threadTask,self,self->threadId);
      if(self->workerThread == INVALID_HANDLE_VALUE){
         self->workerThreadValid = false;
         return -1;
      }
#else
      int rc = THREAD_CREATE(self->workerThread,threadTask,self);
      if(rc != 0){
         self->workerThreadValid = false;
         return -1;
      }
#endif
      //from this point forward all access to self must be protected by spin lock
      return 0;
   }
   errno = EINVAL;
   return -1;
}

static THREAD_PROTO(threadTask,arg)
{
   if(arg!=0)
   {
      rbf_data_t data;
      apx_allocator_t *self;
      uint32_t messages_processed=0;
      self = (apx_allocator_t*) arg;
      for(;;)
      {
#ifdef _MSC_VER
         DWORD result = WaitForSingleObject(self->semaphore, INFINITE);
         if (result == WAIT_OBJECT_0)
#else
         int result = sem_wait(&self->semaphore);
         if (result == 0)
#endif
         {
            uint8_t bufResult;
            bool delayedFree = false;
            SPINLOCK_ENTER(self->lock);
            bufResult = adt_rbfh_remove(&self->messages,(uint8_t*) &data);
            if (bufResult == BUF_E_OK)
            {
               if (data.ptr != 0)
               {
                  if (data.size<=SOA_SMALL_OBJECT_MAX_SIZE)
                  {
                     soa_free(&self->soa,data.ptr,data.size);
                  }
                  else
                  {
                     delayedFree = true;
                  }
               }
            }
            SPINLOCK_LEAVE(self->lock);
            messages_processed++;
            if (delayedFree == true)
            {
               free(data.ptr);
            }
            else if ( (bufResult == BUF_E_OK) && (data.ptr == 0) )
            {
               break; //NULL pointer is used to exit the thread
            }
            else
            {
               //Already handled by soa_free or bufResult != E_BUF_OK
               assert(bufResult == BUF_E_OK);
            }
         }
         else
         {
#ifdef _MSC_VER
            DWORD lastError = GetLastError();
            APX_LOG_ERROR("[APX_ALLOCATOR]: failure while waiting for semaphore, lastError=%d", lastError);
#else
            APX_LOG_ERROR("[APX_ALLOCATOR]: failure while waiting for semaphore, errno=%d", errno);
#endif
            break;
         }
      }
      //APX_LOG_DEBUG("[APX_ALLOCATOR]: messages_processed: %u\n", messages_processed);
   }
   THREAD_RETURN(0);
}




