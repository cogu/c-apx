//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_clientSession.h"
#include <malloc.h>
#include <errno.h>
#include <stdio.h>
#include "apx_logging.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

#define STATE_IDLE             0
#define STATE_WAITING          1

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_clientSession_startThread(apx_clientSession_t *self);
static void apx_clientSession_stopThread(apx_clientSession_t *self);
static THREAD_PROTO(threadTask,arg);
static int8_t apx_clientSession_getNextCmd(apx_clientSession_t *self);
static bool apx_clientSession_startNextCmd(apx_clientSession_t *self);
static bool apx_clientSession_abortCurrentCmd(apx_clientSession_t *self);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int8_t apx_clientSession_create(apx_clientSession_t *self, apx_clientSessionHandler_t *sessionHandler)
{
#ifdef _MSC_VER
   self->workerThread = INVALID_HANDLE_VALUE;
#else
   self->workerThread = 0;
#endif
   self->workerThreadValid=false;
   SPINLOCK_INIT(self->lock);
   SEMAPHORE_CREATE(self->semaphore);
   self->ringbufferLen = APX_CLIENT_SESSION_MAX_MESSAGES;
   self->ringbufferData = (uint8_t*) malloc(APX_CLIENT_SESSION_MAX_MESSAGES*APX_CLIENT_SESSION_MESSAGE_SIZE);
   if (self->ringbufferData == 0)
   {
      return -1;
   }
   rbfs_create(&self->messages, self->ringbufferData,(uint16_t) APX_CLIENT_SESSION_MAX_MESSAGES,(uint8_t) APX_CLIENT_SESSION_MESSAGE_SIZE);
   apx_cmd_create(&self->nextCmd);
   apx_cmd_create(&self->currentCmd);
   apx_clientSession_setSessionHandler(self, sessionHandler);
   return 0;
}

void apx_clientSession_destroy(apx_clientSession_t *self)
{
   if (self != 0)
   {
      if (self->ringbufferData != 0)
      {
         free(self->ringbufferData);
      }
      SEMAPHORE_DESTROY(self->semaphore);
      SPINLOCK_DESTROY(self->lock);
      apx_cmd_destroy(&self->nextCmd);
      apx_cmd_create(&self->currentCmd);
   }
}

apx_clientSession_t* apx_clientSession_new(apx_clientSessionHandler_t *sessionHandler)
{
   apx_clientSession_t *self = (apx_clientSession_t*) malloc(sizeof(apx_clientSession_t));
   if(self != 0)
   {
      apx_clientSession_create(self, sessionHandler);
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_clientSession_delete(apx_clientSession_t *self)
{
   if(self  != 0)
   {
      apx_clientSession_destroy(self);
      free(self);
   }
}

void apx_clientSession_vdelete(void *arg)
{
   apx_clientSession_delete((apx_clientSession_t*) arg);
}

void apx_clientSession_start(apx_clientSession_t *self)
{
   if( (self != 0) && (self->workerThreadValid == false) )
   {
      apx_clientSession_startThread(self);
   }
}

void apx_clientSession_stop(apx_clientSession_t *self)
{
   if( (self != 0) && (self->workerThreadValid == true) )
   {
      apx_clientSession_stopThread(self);
   }
}

void apx_clientSession_setSessionHandler(apx_clientSession_t *self, apx_clientSessionHandler_t *sessionHandler)
{
   if (self != 0)
   {
      SPINLOCK_ENTER(self->lock);
      if (sessionHandler == 0)
      {
         memset(&self->handler, 0, sizeof(apx_clientSessionHandler_t));
      }
      else
      {
         memcpy(&self->handler, sessionHandler, sizeof(apx_clientSessionHandler_t));
      }
      SPINLOCK_LEAVE(self->lock);
   }
}

void apx_clientSession_connectCmd(apx_clientSession_t *self)
{

}

void apx_clientSession_disconnectCmd(apx_clientSession_t *self)
{

}

void apx_clientSession_completedCmd(apx_clientSession_t *self, int32_t userCode)
{

}

void apx_clientSession_pingNodeByNameCmd(apx_clientSession_t *self, const char *nodeName)
{

}



//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void apx_clientSession_startThread(apx_clientSession_t *self)
{
   if( (self != 0) && (self->workerThreadValid == false) ){
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
      }
#endif
      //from this point forward all access to self must be protected by self->lock
   }
}

void apx_clientSession_stopThread(apx_clientSession_t *self)
{
   if ( (self != 0) && (self->workerThreadValid == true) )
   {
#ifdef _MSC_VER
      DWORD result;
#endif
      apx_cmd_t msg = {RMF_MSG_EXIT,0,0}; //{msgType, msgData, msgDestructor}
      SPINLOCK_ENTER(self->lock);
      rbfs_insert(&self->messages,(const uint8_t*) &msg);
      SPINLOCK_LEAVE(self->lock);
      SEMAPHORE_POST(self->semaphore);
#ifdef _MSC_VER
      result = WaitForSingleObject(self->workerThread, 5000);
      if (result == WAIT_TIMEOUT)
      {
         fprintf(stderr, "[APX_CLIENT_SESSION] timeout while joining workerThread\n");
      }
      else if (result == WAIT_FAILED)
      {
         DWORD lastError = GetLastError();
         fprintf(stderr, "[APX_CLIENT_SESSION]  joining workerThread failed with %d\n", (int)lastError);
      }
      CloseHandle(self->workerThread);
      self->workerThread = INVALID_HANDLE_VALUE;
      self->workerThreadValid = false;
#else
      if (pthread_equal(pthread_self(), self->workerThread) == 0)
      {
         void *status;
         int s = pthread_join(self->workerThread, &status);
         if (s != 0)
         {
            APX_LOG_ERROR("[APX_CLIENT_SESSION] pthread_join error %d", s);
         }
         else
         {
            self->workerThreadValid = false;
         }
      }
      else
      {
         APX_LOG_ERROR("[APX_CLIENT_SESSION] pthread_join attempted on pthread_self()\n");
      }
#endif
   }
}

static THREAD_PROTO(threadTask,arg)
      {
   if(arg!=0)
   {
      apx_clientSession_t *self;
      bool isRunning=true;
      self = (apx_clientSession_t*) arg;

      while(isRunning == true)
      {
#ifdef _MSC_VER
         DWORD result = WaitForSingleObject(self->semaphore, 0);
         if (result == WAIT_OBJECT_0)
#else
         int result = sem_trywait(&self->semaphore);
         if (result == 0)
#endif
         {
            int8_t i8Result = apx_clientSession_getNextCmd(self);
            if (i8Result == 0)
            {
               if(self->nextCmd.cmdType == APX_CMD_EXIT)
               {
                  isRunning=apx_clientSession_abortCurrentCmd(self);
                  APX_LOG_DEBUG("[APX_CLIENT_SESSION]: Exit command");
               }
            }
            else
            {
               isRunning = false;
               APX_LOG_ERROR("[APX_CLIENT_SESSION]: Failed to call apx_clientSession_getNextCmd");
            }
         }
         if (isRunning == true)
         {
            if (self->currentCmd.cmdType == APX_CMD_NONE)
            {
               APX_LOG_DEBUG("[APX_CLIENT_SESSION]: Idle");
               if (self->nextCmd.cmdType != APX_CMD_NONE)
               {
                  isRunning = apx_clientSession_startNextCmd(self);
               }
            }
            else
            {
               APX_LOG_DEBUG("[APX_CLIENT_SESSION]: Waiting for command to finish");
            }
            SLEEP(100);
         }
      }
   }
   THREAD_RETURN(0);
}





/**
 * returns -1 on error, 0 success
 */
static int8_t apx_clientSession_getNextCmd(apx_clientSession_t *self)
{
   if (self != 0)
   {
      int8_t retval = 0;
      {
         uint8_t result;
         SPINLOCK_ENTER(self->lock);
         result = rbfs_remove(&self->messages,(uint8_t*) &self->nextCmd);
         SPINLOCK_LEAVE(self->lock);
         if (result != 0)
         {
            retval = -1;
         }
      }
      return retval;
   }
   return -1;
}

/**
 * returns true if the processing thread should continue or false if it should abort
 */
static bool apx_clientSession_startNextCmd(apx_clientSession_t *self)
{
   if(self != 0)
   {
      bool retval = true;
      apx_cmd_destroy(&self->currentCmd);
      self->currentCmd = self->nextCmd;
      switch(self->currentCmd.cmdType)
      {
      case APX_CMD_CONNECT:
         APX_LOG_DEBUG("[APX_CLIENT_SESSION]: APX_CMD_CONNECT");
         break;
      default:
         APX_LOG_ERROR("[APX_CLIENT_SESSION]: Unknown command type: %d", self->currentCmd.cmdType);
      }
      return retval;
   }
   return false;
}

static bool apx_clientSession_abortCurrentCmd(apx_clientSession_t *self)
{
   if(self != 0)
   {
      //TODO: Placeholder logic, implement later
      bool retval = false;
      return retval;
   }
   return false;
}
