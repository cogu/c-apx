/**
* This file is heavily influenced by the msocket_server implementation
*/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include "npipe_server.h"
#include "osmacro.h"
#include "osutil.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define strdup _strdup

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static THREAD_PROTO(acceptTask, arg);
static THREAD_PROTO(cleanupTask, arg);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void npipe_server_create(npipe_server_t *self, void(*pDestructor)(void*)) 
{
   if (self != 0)
   {
      self->pipePath = 0;
      self->acceptThread = INVALID_HANDLE_VALUE;
      self->cleanupThread = INVALID_HANDLE_VALUE;
      self->acceptPipe = INVALID_HANDLE_VALUE;
      self->isThreadRunning = false;
      memset(&self->handlerTable, 0, sizeof(self->handlerTable));
      if (pDestructor != 0)
      {
         self->pDestructor = pDestructor;
      }
      else
      {
         self->pDestructor = npipe_vdelete;
      }
      adt_ary_create(&self->cleanupItems, 0);
      memset(&self->overlappedConnect, 0, sizeof(OVERLAPPED));
      self->connectEvent = CreateEvent(
         NULL,    // default security attribute 
         TRUE,    // manual-reset event 
         TRUE,    // initial state = signaled 
         NULL);   // unnamed event object 
      self->quitEvent = CreateEvent(
         NULL,    // default security attribute 
         TRUE,    // manual-reset event 
         FALSE,    // initial state = signaled 
         NULL);   // unnamed event object 
      self->overlappedConnect.hEvent = self->connectEvent;
      MUTEX_INIT(self->mutex);
      SEMAPHORE_CREATE(self->sem);
   }
}

void npipe_server_destroy(npipe_server_t *self)
{
   if (self != 0) 
   {
      if (self->isThreadRunning == true)
      {
         SetEvent(self->quitEvent);
         WaitForSingleObject(self->acceptThread, INFINITE);
         WaitForSingleObject(self->cleanupThread, INFINITE);
         CloseHandle(self->cleanupThread);
         CloseHandle(self->acceptThread);
      }
      adt_ary_destroy(&self->cleanupItems);
      SEMAPHORE_DESTROY(self->sem);
      MUTEX_DESTROY(self->mutex);
      if (self->pipePath != 0) 
      {
         free(self->pipePath);
      }
   }
}

npipe_server_t *npipe_server_new(void(*pDestructor)(void*))
{
   npipe_server_t *self = (npipe_server_t*)malloc(sizeof(npipe_server_t));
   if (self != 0) 
   {
      npipe_server_create(self, pDestructor);
   }
   return self;
}

void npipe_server_delete(npipe_server_t *self)
{
   if (self != 0)
   {
      npipe_server_destroy(self);
      free(self);
   }
}

void npipe_server_sethandlerTable(npipe_server_t *self, npipe_handlerTable_t *handler)
{
   if ( self != 0 )
   {
      if (handler != 0)
      {
         memcpy(&self->handlerTable, handler, sizeof(npipe_handlerTable_t));
      }
      else
      {
         memset(&self->handlerTable, 0, sizeof(npipe_handlerTable_t));
      }
   }
}

void npipe_server_start(npipe_server_t *self, const char *pipePath)
{
   if ( (self != 0) && (pipePath != 0 )  )
   {
      self->pipePath = strdup(pipePath);
      self->isThreadRunning = true;
      THREAD_CREATE(self->acceptThread, acceptTask, (void*)self, self->acceptThreadId);
      THREAD_CREATE(self->cleanupThread, cleanupTask, (void*)self, self->cleanupThreadId);
   }
}

void npipe_server_cleanup_connection(npipe_server_t *self, void *arg)
{
   MUTEX_LOCK(self->mutex);
   adt_ary_push(&self->cleanupItems, arg);
   MUTEX_UNLOCK(self->mutex);
   SEMAPHORE_POST(self->sem);
}



//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

THREAD_PROTO(acceptTask, arg) {
   npipe_server_t *self = (npipe_server_t *)arg;
   if (self != 0)
   {
      HANDLE waitObjects[2];
      DWORD numObjects = 2;
      bool isRunning = true;
      waitObjects[0] = self->quitEvent;
      waitObjects[1] = self->connectEvent;
      while(isRunning == true)
      {
         int isConnected = 0;
         HANDLE hPipe;
         npipe_t *npipe = npipe_new();
         if (npipe == 0)         
         {
            fprintf(stderr, "npipe_new returned NULL");
            break;
         }

         hPipe = CreateNamedPipe(
            self->pipePath,           // pipe name 
            PIPE_ACCESS_DUPLEX |       // read/write access
            FILE_FLAG_OVERLAPPED,      //use overlapped
            PIPE_TYPE_MESSAGE |       // message type pipe 
            PIPE_READMODE_MESSAGE |   // message-read mode 
            PIPE_WAIT,                // blocking mode 
            PIPE_UNLIMITED_INSTANCES, // max. instances  
            NPIPE_BUFSIZE,          // output buffer size 
            NPIPE_BUFSIZE,          // input buffer size 
            0,                        // client time-out 
            NULL);                    // default security attribute 

         if (hPipe == INVALID_HANDLE_VALUE)
         {
            fprintf(stderr, "CreateNamedPipe failed, GLE=%d.\n", GetLastError());
            THREAD_RETURN(-1);
         }

         // Wait for the client to connect; if it succeeds, 
         // the function returns a nonzero value. If the function
         // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
         npipe->state = NPIPE_STATE_ACCEPTING;
         npipe->isServerPipe = true;
         printf("waiting for connection\n");
         isConnected = ConnectNamedPipe(hPipe, &self->overlappedConnect);
         if (isConnected == 0)
         {
            DWORD result;
            switch (GetLastError())
            {
               case ERROR_IO_PENDING:                  
                  break;
               case ERROR_PIPE_CONNECTED:
                  SetEvent(self->connectEvent);
                  break;
            }
            result = WaitForMultipleObjects(numObjects, waitObjects, FALSE, INFINITE);
            switch (result)
            {
            case WAIT_OBJECT_0: //self->quitEvent
               isRunning = false;
               break;
            case (WAIT_OBJECT_0 + 1):
               npipe->state = NPIPE_STATE_CONNECTED;
               npipe->hPipe = hPipe;
               if (self->handlerTable.onAccept != 0)
               {                                    
                  self->handlerTable.onAccept(self->handlerTable.arg, self, npipe);
               }
               break;
            case WAIT_FAILED:
               fprintf(stderr, "WaitForMultipleObjects failed, GLE=%d\n", GetLastError());
               break;
            }
         }
      }
   }
   printf("acceptTask exit\n");
   THREAD_RETURN(0);
}

THREAD_PROTO(cleanupTask, arg)
{
   npipe_server_t *self = (npipe_server_t *)arg;
   if (self != 0)
   {
      HANDLE waitObjects[2];
      DWORD numObjects = 2;
      bool isRunning = true;
      waitObjects[0] = self->quitEvent;
      waitObjects[1] = self->sem;
      if (self->pDestructor == 0)
      {
         THREAD_RETURN(0);
      }
      while (isRunning == true)
      {
         DWORD result;         
         void *item;
         result = WaitForMultipleObjects(numObjects, &waitObjects[0], FALSE, INFINITE);
         switch (result)
         {
         case WAIT_OBJECT_0: //self->quitEvent
            isRunning = false;
            break;
         case (WAIT_OBJECT_0 + 1):
            MUTEX_LOCK(self->mutex);
            assert(adt_ary_length(&self->cleanupItems)>0);
            item = adt_ary_shift(&self->cleanupItems);
            self->pDestructor(item);
            MUTEX_UNLOCK(self->mutex);
            break;
         default:
            isRunning = false;
         }         
      }
   }
   THREAD_RETURN(0);
}
