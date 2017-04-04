//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "npipe.h"
#include <process.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define strdup _strdup
#define TIMEOUT_MS 100

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static THREAD_PROTO(ioTask, arg);
static int8_t npipe_startIoThread(npipe_t *self);
static int8_t npipe_clientConnected(npipe_t *self);
static int npipe_rxHandler(npipe_t *self, DWORD result, uint8_t *recvBuf, uint32_t len);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void npipe_create(npipe_t *self)
{
   if (self != 0)
   {
      memset(&self->overlappedRead, 0, sizeof(OVERLAPPED));
      memset(&self->overlappedWrite, 0, sizeof(OVERLAPPED));
      self->state = NPIPE_STATE_NONE;
      npipe_sethandlerTable(self, 0);
      self->hPipe = INVALID_HANDLE_VALUE;
      self->ioThread = INVALID_HANDLE_VALUE;
      self->ioThreadId = 0;
      self->path = 0;      
      adt_bytearray_create(&self->receiveBuf, 0);
      adt_bytearray_create(&self->sendBuf, 0);
      MUTEX_INIT(self->mutex);
      self->isServerPipe = false;
      self->isThreadRunning = false;
      self->hasPendingWrite = false;
      self->hasPendingRead = false;

      self->readEvent = CreateEvent(
         NULL,    // default security attribute 
         TRUE,    // manual-reset event 
         TRUE,    // initial state = signaled 
         NULL);   // unnamed event object 
      self->writeEvent = CreateEvent(
         NULL,    // default security attribute 
         TRUE,    // manual-reset event 
         TRUE,    // initial state = signaled 
         NULL);   // unnamed event object 
      self->quitEvent = CreateEvent(
         NULL,    // default security attribute 
         TRUE,    // manual-reset event 
         FALSE,    // initial state = non-signaled 
         NULL);   // unnamed event object 
      assert(self->readEvent != INVALID_HANDLE_VALUE);
      assert(self->writeEvent != INVALID_HANDLE_VALUE);
      self->overlappedRead.hEvent = self->readEvent;
      self->overlappedWrite.hEvent = self->writeEvent;
   }
}

void npipe_destroy(npipe_t *self)
{
   if (self != 0)
   {
      npipe_close(self);
      if (self->path != 0)
      {
         free(self->path);
      }
      adt_bytearray_destroy(&self->receiveBuf);
      adt_bytearray_destroy(&self->sendBuf);
      MUTEX_DESTROY(self->mutex);
      CloseHandle(self->quitEvent);
      CloseHandle(self->readEvent);
      CloseHandle(self->writeEvent);
   }
}

npipe_t *npipe_new(void)
{
   npipe_t *self = (npipe_t *) malloc(sizeof(npipe_t));
   if (self != 0) 
   {
      npipe_create(self);
   }
   return self;
}

void npipe_delete(npipe_t *self)
{
   if (self != 0) 
   {
      npipe_destroy(self);
      free(self);
   }
}

void npipe_vdelete(void *arg)
{
   npipe_delete((npipe_t*)arg);
}

void npipe_close(npipe_t *self)
{
   if (self != 0)
   {
      bool isThreadRunning;
      MUTEX_LOCK(self->mutex);
      isThreadRunning = self->isThreadRunning;
      if ( (self->state == NPIPE_STATE_ACCEPTING) || (self->state == NPIPE_STATE_PENDING) ||  (self->state == NPIPE_STATE_CONNECTED) )
      {
         self->state = NPIPE_STATE_CLOSING;
      }
      MUTEX_UNLOCK(self->mutex);
      if (isThreadRunning == true)
      {
         SetEvent(self->quitEvent);
         WaitForSingleObject(self->ioThread, 2000);
         CloseHandle(self->ioThread);
         self->ioThread = INVALID_HANDLE_VALUE;
      }
      if (self->hPipe != INVALID_HANDLE_VALUE) 
      {
         CloseHandle(self->hPipe);
         self->hPipe = INVALID_HANDLE_VALUE;
      }
      self->state = NPIPE_STATE_CLOSED;
   }
}

void npipe_sethandlerTable(npipe_t *self, const npipe_handlerTable_t *handlerTable)
{
   if (self != 0)
   {
      if (handlerTable != 0)
      {
         memcpy(&self->handlerTable, handlerTable, sizeof(npipe_handlerTable_t));
      }
      else
      {
         memset(&self->handlerTable, 0, sizeof(npipe_handlerTable_t));
      }
   }
}

/**
*  connects a client to a named pipe server
*/
int8_t npipe_connect(npipe_t *self, const char *path)
{
   if ( (self != 0) && (path != 0) )
   {
      HANDLE hPipe;
      self->path = strdup(path);
      if (self->path == 0)
      {
         errno = ENOMEM;
         return -1;
      }
      self->state = NPIPE_STATE_PENDING;
      hPipe = CreateFile( self->path, GENERIC_READ |  GENERIC_WRITE, 0,  NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
      if (hPipe != INVALID_HANDLE_VALUE)
      {
         int8_t rc;
         self->hPipe = hPipe;
         self->state = NPIPE_STATE_CONNECTED;
         rc = npipe_clientConnected(self);
         if (rc == 0)
         {
            npipe_startIoThread(self);
         }  
         else
         {
            return rc;
         }
      }
      else
      {
         if (GetLastError() == ERROR_PIPE_BUSY)
         {
            //TODO: implement call to WaitNamedPipe inside ioTask. self->state stays in pending
         }
      }
      return 0;
   }
   errno = EINVAL;
   return -1;
}

int8_t npipe_start_io(npipe_t *self) 
{
   if (self != 0) 
   {
      return (int8_t)npipe_startIoThread(self);
   }
   errno = EINVAL;
   return -1;
}

int8_t npipe_send(npipe_t *self, const uint8_t *msgData, uint32_t msgLen)
{
   if ( (self != 0) && (msgData != 0) && (msgLen <= NPIPE_MAX_MSG_LEN) )
   {
      if (self->state == NPIPE_STATE_CONNECTED)
      {
         bool hasPendingWrite;
         MUTEX_LOCK(self->mutex);
         hasPendingWrite = self->hasPendingWrite;
         MUTEX_UNLOCK(self->mutex);
         
         if (hasPendingWrite == true)
         {
            adt_bytearray_append(&self->sendBuf, msgData, msgLen);
         }
         else
         {
            DWORD result = WriteFile(self->hPipe, (void*)msgData, (DWORD)msgLen, NULL, &self->overlappedWrite);
            if (result == 0)
            {
               DWORD error = GetLastError();
               if (error = ERROR_IO_PENDING)
               {
                  MUTEX_LOCK(self->mutex);
                  self->hasPendingWrite = true;
                  MUTEX_UNLOCK(self->mutex);
               }
               else
               {
                  fprintf(stderr, "WriteFile failed with %d\n", error);
                  return -1;
               }
            }
         }
         return 0;
      }
      else
      {
         errno = ENOTCONN;
         return -1;
      }
   }

   errno = EINVAL;
   return -1;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static int8_t npipe_startIoThread(npipe_t *self) 
{
   if ((self != 0) && (self->isThreadRunning == false)) 
   {
      THREAD_CREATE(self->ioThread, ioTask, self, self->ioThreadId);
      if (self->ioThread == INVALID_HANDLE_VALUE) 
      {
         return -1;
      }
      self->isThreadRunning = true;
      return 0;
   }
   errno = EINVAL;
   return -1;
}

THREAD_PROTO(ioTask, arg)
{
   if (arg != 0)
   {
      DWORD result;
      HANDLE waitObjects[3];      
      npipe_t *self = (npipe_t*)arg;
      uint8_t *recvBuf;
      bool isServerPipe;
      uint8_t state;
      DWORD numberOfBytes;
      recvBuf = (uint8_t*)malloc(NPIPE_BUFSIZE);
      bool isRunning = true;
      
      waitObjects[0] = self->quitEvent;
      waitObjects[1] = self->readEvent;
      waitObjects[2] = self->writeEvent;

      if (recvBuf == 0) 
      {
         THREAD_RETURN(1);
      }
      //wait for parent thread to release lock before starting main event loop
      MUTEX_LOCK(self->mutex);
      isServerPipe = self->isServerPipe;
      state = self->state;
      MUTEX_UNLOCK(self->mutex);
      //TODO: implement something here that waits for pipe to connect in case state == NPIPE_STATE_PENDING      
      if (state == NPIPE_STATE_CONNECTED)
      {
         while (isRunning == true)
         {
            int8_t rc;
            DWORD numObjects = 1; //include self->quitEvent only
            numberOfBytes = 0;
            if (self->hasPendingRead == false)
            {
               BOOL rc;
               rc = ReadFile(self->hPipe, recvBuf, NPIPE_BUFSIZE, NULL, &self->overlappedRead);
               if (rc == 0)
               {
                  DWORD error = GetLastError();
                  if (error != ERROR_IO_PENDING)
                  {
                     fprintf(stderr, "ReadFile failed, GLE=%d.\n", error);
                     break;
                  }
                  MUTEX_LOCK(self->mutex);
                  self->hasPendingRead = true;
                  MUTEX_UNLOCK(self->mutex);
                  numObjects = 2;
               }
               else
               {
                  result = GetOverlappedResult(self->hPipe, &self->overlappedRead, &numberOfBytes, FALSE);
                  rc = npipe_rxHandler(self, result, recvBuf, (uint32_t)numberOfBytes);
                  if (rc < 0)
                  {
                     isRunning = false;
                  }
               }
            }
            else
            {
               numObjects = 2;
            }
            if (numObjects >= 2)
            {
               if (self->hasPendingWrite == true)
               {
                  numObjects = 3; //include self->quitEvent, self->readEvent, self->writeEvent
               }
//               printf("ioTask(%p): waiting, numObjects=%d\n", arg, numObjects);
               result = WaitForMultipleObjects(numObjects, &waitObjects[0], FALSE, INFINITE);
               printf("ioTask(%p): woke up from wait\n", arg);
               switch (result)
               {
               case WAIT_OBJECT_0: //quitEvent
                  isRunning = false;
                  break;
               case (WAIT_OBJECT_0 + 1): //overlapped read event
                  self->hasPendingRead = false;
                  result = GetOverlappedResult(self->hPipe, &self->overlappedRead, &numberOfBytes, FALSE);
                  rc = npipe_rxHandler(self, result, recvBuf, (uint32_t)numberOfBytes);
                  if (rc < 0)
                  {
                     isRunning = false;
                  }
                  break;
               case (WAIT_OBJECT_0 + 2): //overlapped write event
                  self->hasPendingWrite = false;
                  result = GetOverlappedResult(self->hPipe, &self->overlappedRead, &numberOfBytes, FALSE);
                  break;
               case WAIT_FAILED:
                  fprintf(stderr, "WaitForMultipleObjects failed, GLE=%d.\n", GetLastError());
                  isRunning = false;
                  break;
               default:
                  break;
               }
            }           
         }
      }
      else
      {
         fprintf(stderr, "npipe is in an unexpected state, %d\n", state);
      }
      free(recvBuf);
   }
   printf("ioTask(%p): exit\n", arg);
   THREAD_RETURN(0);
}

/*
* sets a client pipe into message mode
*/
static int8_t npipe_clientConnected(npipe_t *self)
{
   if (self != 0)
   {
      if (self->state == NPIPE_STATE_CONNECTED)
      {
         BOOL rc;
         DWORD mode = PIPE_READMODE_MESSAGE;
         rc = SetNamedPipeHandleState(self->hPipe, &mode, NULL, NULL);
         if (rc == 0)
         {
            fprintf(stderr, "SetNamedPipeHandleState failed. GLE=%d\n", GetLastError() );
            return -1;
         }
         if (self->handlerTable.onConnected != 0)
         {
            self->handlerTable.onConnected(self->handlerTable.arg);
         }
         return 0;
      }
      //pipe not connected
      return -1;      
   }
   errno = EINVAL;
   return -1;
}

static int npipe_rxHandler(npipe_t *self, DWORD result, uint8_t *recvBuf, uint32_t len) 
{
   if (result== 0) 
   {          
      fprintf(stderr, "[NPIPE] ReadFile failed, GLE=%d\n", GetLastError());
      return -1;
   }
   if (self != 0)
   {
      if (self->handlerTable.onData != 0)
      {
         uint32_t u32Len;
         const uint8_t *pBegin;
         if (adt_bytearray_append(&self->receiveBuf, recvBuf, (uint32_t)len) != 0) 
         {
            fprintf(stderr, "adt_bytearray_append failed\n");
            return -1;
         }
         while (1)
         {
            //message parse loop
            int8_t rc;
            uint32_t parseLen = 0;
            pBegin = (const uint8_t*)self->receiveBuf.pData;
            u32Len = self->receiveBuf.u32CurLen;
            if (u32Len == 0) 
            {
               break; //no more data
            }
            rc = self->handlerTable.onData(self->handlerTable.arg, pBegin, u32Len, &parseLen);
            if (rc != 0) 
            {
               MUTEX_LOCK(self->mutex);
               self->state = NPIPE_STATE_CLOSING;
               MUTEX_UNLOCK(self->mutex);
               break;
            }
            if (parseLen == 0) 
            {
               break;
            }
            else
            {
               assert(parseLen <= u32Len);
               adt_bytearray_trimLeft(&self->receiveBuf, pBegin + parseLen);
            }
         }
      }      
   }
   return 0;
}
