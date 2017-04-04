//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_pipeConnection.h"
#include <malloc.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <process.h>


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void on_tcp_connected(void *arg, const char *addr, uint16_t port);
static void on_tcp_disconnected(void *arg);
static int8_t on_tcp_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen); 
static void on_pipe_disconnected(void *arg);
static int8_t on_pipe_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);

static THREAD_PROTO(threadTask, arg);


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

///TODO: perhaps put instances of this class into a pool? These are quite costly to create.
int8_t apx_pipeConnection_create(apx_pipeConnection_t *self, npipe_t *pipe, npipe_server_t *server, const char *address, uint16_t port)
{
   if (self != 0)
   {
      int8_t result;
      msocket_handler_t msocketHandler;
      npipe_handlerTable_t npipeHandler;
      self->pipe = pipe;
      self->server = server;      
      SEMAPHORE_CREATE(self->semaphore);
      SPINLOCK_INIT(self->apxPendingLock);
      SPINLOCK_INIT(self->pipePendingLock);
      SPINLOCK_INIT(self->connectionStatusLock);
      self->quitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (self->quitEvent == INVALID_HANDLE_VALUE)
      {
         fprintf(stderr, "CreateEvent() failed. GLE=%d\n", GetLastError());
         return -1;
      }
      self->socket = msocket_new(AF_INET);
      if (self->socket == 0)
      {
         fprintf(stderr, "msocket_new failed\n");
         return -1;
      }
      adt_bytearray_create(&self->apxPending, 0);
      adt_bytearray_create(&self->pipePending, 0);
      self->isPipeConnected = true;
      self->isTcpConnected = false;
      self->isThreadRunning = false;
      self->workerThread = INVALID_HANDLE_VALUE;
      //setup msocket handler
      memset(&msocketHandler, 0, sizeof(msocketHandler));
      msocketHandler.tcp_connected = on_tcp_connected;
      msocketHandler.tcp_disconnected = on_tcp_disconnected;
      msocketHandler.tcp_data = on_tcp_data;
      msocket_sethandler(self->socket, &msocketHandler, self);

      //setup npipe handler
      memset(&npipeHandler, 0, sizeof(npipeHandler));
      npipeHandler.onData = on_pipe_data;
      npipeHandler.onDisconnected = on_pipe_disconnected;
      npipeHandler.arg = self;
      npipe_sethandlerTable(self->pipe, &npipeHandler);

      //start npipe worker thread
      npipe_start_io(self->pipe);

      result = msocket_connect(self->socket, address, port); //msocket worker thread starts automatically if connection is successful.
      if (result != 0)
      {
         fprintf(stderr, "msocket_connect(%s, %d) failed\n", address, port);
         msocket_delete(self->socket);
         self->socket = 0;
         return -1;
      }
      apx_pipeConnection_start(self);
      return 0;
   }
   return -1;
}

void apx_pipeConnection_destroy(apx_pipeConnection_t *self)
{
   if (self != 0)
   {
      //stop worker thread if not already stopped
      apx_pipeConnection_stop(self);
      if (self->pipe != 0)
      {
         npipe_delete(self->pipe);
      }
      if (self->socket != 0)
      {
         msocket_delete(self->socket);
      }      
      adt_bytearray_destroy(&self->apxPending);
      adt_bytearray_destroy(&self->pipePending);
      CloseHandle(self->quitEvent);
      SEMAPHORE_DESTROY(self->semaphore);
      SPINLOCK_DESTROY(self->apxPendingLock);
      SPINLOCK_DESTROY(self->pipePendingLock);
      SPINLOCK_DESTROY(self->connectionStatusLock);
   }
}

apx_pipeConnection_t *apx_pipeConnection_new(npipe_t *pipe, npipe_server_t *server, const char *address, uint16_t port)
{
   if ( (pipe != 0) && (server != 0) )
   {
      apx_pipeConnection_t *self = (apx_pipeConnection_t*)malloc(sizeof(apx_pipeConnection_t));
      if (self != 0) 
      {
         int8_t result = apx_pipeConnection_create(self, pipe, server, address, port);
         if (result != 0)
         {
            free(self);
            self = 0;
         }
      }
      else 
      {
         errno = ENOMEM;
      }
      return self;
   }
   return (apx_pipeConnection_t*)0;
}

void apx_pipeConnection_delete(apx_pipeConnection_t *self)
{
   if (self != 0)
   {
      apx_pipeConnection_destroy(self);
      free(self);
   }
}

void apx_pipeConnection_vdelete(void *arg)
{
   apx_pipeConnection_delete((apx_pipeConnection_t*)arg);
}

/**
* Starts worker thread
*/
int8_t apx_pipeConnection_start(apx_pipeConnection_t *self)
{
   if ( (self != 0) && (self->isThreadRunning == false) )
   {
      self->isThreadRunning = true;
      THREAD_CREATE(self->workerThread, threadTask, self, self->workerThreadId);
      if (self->workerThread == INVALID_HANDLE_VALUE)   
      {
         fprintf(stderr, "Failed to start worker thread, GLE=%d\n", GetLastError());
         return -1;
      }
      self->isThreadRunning = true;
      return 0;
   }   
   return -1;
}

/**
* Stops worker thread
*/
int8_t apx_pipeConnection_stop(apx_pipeConnection_t *self)
{
   if ( (self != 0) && (self->isThreadRunning == true) )
   {
      SetEvent(self->quitEvent);
      WaitForSingleObject(self->workerThread, 2000);
      CloseHandle(self->workerThread);
      self->workerThread = INVALID_HANDLE_VALUE;
      self->isThreadRunning = false;
      return 0;
   }
   return -1;
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void on_tcp_connected(void *arg, const char *addr, uint16_t port)
{
   apx_pipeConnection_t *self = (apx_pipeConnection_t*)arg;
   if (self != 0)
   {
      printf("on_tcp_connected\n");
      SPINLOCK_ENTER(self->connectionStatusLock);
      self->isTcpConnected = true;
      SPINLOCK_LEAVE(self->connectionStatusLock);
      SEMAPHORE_POST(self->semaphore);
   }
}

static void on_tcp_disconnected(void *arg)
{
   apx_pipeConnection_t *self = (apx_pipeConnection_t*)arg;
   if (self != 0)
   {
      printf("on_tcp_disconnected\n");
      SPINLOCK_ENTER(self->connectionStatusLock);
      self->isTcpConnected = false;
      SPINLOCK_LEAVE(self->connectionStatusLock);
      npipe_server_cleanup_connection(self->server, (void*)self);
   }
}

static void on_pipe_disconnected(void *arg)
{
   apx_pipeConnection_t *self = (apx_pipeConnection_t*)arg;
   if (self != 0)
   {
      printf("on_pipe_disconnected\n");
      SPINLOCK_ENTER(self->connectionStatusLock);
      self->isPipeConnected = false;
      SPINLOCK_LEAVE(self->connectionStatusLock);
      npipe_server_cleanup_connection(self->server, (void*)self);
   }
}

/*
* all data received on apx/tcp connection is forwared to named pipe connection
*/
static int8_t on_tcp_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   *parseLen = dataLen;

   apx_pipeConnection_t *self = (apx_pipeConnection_t*)arg;
   if (self != 0)
   {
      printf("on_tcp_data\n");
      SPINLOCK_ENTER(self->pipePendingLock);
      adt_bytearray_append(&self->pipePending, dataBuf, dataLen);
      SPINLOCK_LEAVE(self->pipePendingLock);
      SEMAPHORE_POST(self->semaphore);
   }
   return 0;
}

/*
* all data received on pipe connection is forwared to apx (tcp) connection
*/
static int8_t on_pipe_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   *parseLen = dataLen;

   apx_pipeConnection_t *self = (apx_pipeConnection_t*)arg;
   if (self != 0)
   {
      printf("on_pipe_data\n");
      SPINLOCK_ENTER(self->apxPendingLock);
      adt_bytearray_append(&self->apxPending, dataBuf, dataLen);
      SPINLOCK_LEAVE(self->apxPendingLock);
      SEMAPHORE_POST(self->semaphore);
   }
   return 0;
}



THREAD_PROTO(threadTask, arg)
{
   if (arg != 0)
   {      
      HANDLE waitObjects[2];
      const DWORD numObjects = sizeof(waitObjects) / sizeof(waitObjects[0]);
      bool isRunning = true;

      apx_pipeConnection_t *self = (apx_pipeConnection_t*)arg;
      waitObjects[0] = self->quitEvent;
      waitObjects[1] = self->semaphore;

      while(isRunning == true)
      {
         DWORD result;         
         bool isTcpConnected;
         bool isPipeConnected;
         result = WaitForMultipleObjects(numObjects, &waitObjects[0], FALSE, INFINITE);
         switch (result)
         {
         case WAIT_OBJECT_0: //quitEvent
            isRunning = false;
            break;
         case (WAIT_OBJECT_0 + 1): //semaphore
            SPINLOCK_ENTER(self->connectionStatusLock);
            isTcpConnected = self->isTcpConnected;
            isPipeConnected = self->isPipeConnected;
            SPINLOCK_LEAVE(self->connectionStatusLock);

            //check first queue
            if (isTcpConnected == true)
            {
               uint32_t pendingLen;
               SPINLOCK_ENTER(self->apxPendingLock);
               pendingLen = (uint32_t)adt_bytearray_length(&self->apxPending);
               if (pendingLen > 0)
               {
                  int8_t rc;
                  rc = msocket_send(self->socket, adt_bytearray_data(&self->apxPending), pendingLen);
                  if (rc != 0)
                  {
                     fprintf(stderr, "msocket_send failed with %d\n", rc);
                     isRunning = false;
                     SPINLOCK_LEAVE(self->apxPendingLock);
                     break;
                  }
                  adt_bytearray_clear(&self->apxPending);
               }
               SPINLOCK_LEAVE(self->apxPendingLock);
            }
            //check second queue
            if (isPipeConnected == true)
            {
               uint32_t pendingLen;
               SPINLOCK_ENTER(self->pipePendingLock);
               pendingLen = (uint32_t)adt_bytearray_length(&self->pipePending);
               if (pendingLen > 0)
               {
                  int8_t rc;
                  rc = npipe_send(self->pipe, adt_bytearray_data(&self->pipePending), pendingLen);
                  if (rc != 0)
                  {
                     fprintf(stderr, "npipe_send failed with %d\n", rc);
                     isRunning = false;
                     SPINLOCK_LEAVE(self->pipePendingLock);
                     break;
                  }
                  adt_bytearray_clear(&self->apxPending);
               }
               SPINLOCK_LEAVE(self->pipePendingLock);
            }
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
   printf("[PIPE_GATEWAY] threadTask(%p): exit\n", arg);
   THREAD_RETURN(0);
}
