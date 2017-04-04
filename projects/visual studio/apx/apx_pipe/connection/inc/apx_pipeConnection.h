#ifndef APX_PIPE_CONNECTION_H
#define APX_PIPE_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "npipe.h"
#include "npipe_server.h"
#include "msocket.h"
#include "osmacro.h"
#include "adt_bytearray.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


typedef struct apx_pipeConnection_tag
{
   npipe_t *pipe; //named pipe server connection to Excel or other named pipe application
   npipe_server_t *server; //the npipe server the pipe belongs to
   msocket_t *socket; //socket client connection to apx_server   
   HANDLE quitEvent; //worker thread quit event
   SEMAPHORE_T semaphore; //worker thread wait semaphore   
   SPINLOCK_T apxPendingLock; //lock for apxPending
   SPINLOCK_T pipePendingLock; //lock for pipePending
   SPINLOCK_T connectionStatusLock; //lock for protecting apxConnected and pipeConnected
   adt_bytearray_t apxPending; //pending transmit buffer on apx connection
   adt_bytearray_t pipePending; //pending transmit buffer on pipe connection   
   bool isTcpConnected;
   bool isPipeConnected;
   bool isThreadRunning;
   THREAD_T workerThread; //internal worker thread
   unsigned int workerThreadId;
    
}apx_pipeConnection_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t apx_pipeConnection_create(apx_pipeConnection_t *self, npipe_t *pipe, npipe_server_t *server, const char *address, uint16_t port);
void apx_pipeConnection_destroy(apx_pipeConnection_t *self);
apx_pipeConnection_t *apx_pipeConnection_new(npipe_t *pipe, npipe_server_t *server, const char *address, uint16_t port);
void apx_pipeConnection_delete(apx_pipeConnection_t *self);
void apx_pipeConnection_vdelete(void *arg);
int8_t apx_pipeConnection_start(apx_pipeConnection_t *self);
int8_t apx_pipeConnection_stop(apx_pipeConnection_t *self);


#endif //APX_PIPE_CONNECTION_H

