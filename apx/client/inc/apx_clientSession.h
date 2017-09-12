#ifndef APX_CLIENT_SESSION_H
#define APX_CLIENT_SESSION_H
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#endif
#include "osmacro.h"
#include "apx_clientConnection.h"
#include "apx_cmd.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef struct apx_clientSessionHandler_tag
{
   void *arg; //user argument
   void (*completed)(void *arg, int32_t userCode);
   void (*error)(void *arg, const apx_cmd_t *cmd, int32_t errorCode);
} apx_clientSessionHandler_t;


/**
 * APX clientSession - A class used for clients to programmatically control a connection to an APX server
 */
typedef struct apx_clientSession_tag
{
   THREAD_T workerThread; //local worker thread
   SPINLOCK_T lock;  //variable lock
   SEMAPHORE_T semaphore; //thread semaphore

   rbfs_t messages; //pending messages (ringbuffer)
   size_t ringbufferLen;
   uint8_t *ringbufferData;
   bool isRunning; //when false it's time to shut down
   bool workerThreadValid; //true if workerThread is a valid variable

   apx_cmd_t nextCmd;
   apx_cmd_t currentCmd;

   apx_clientConnection_t *clientConnection;
   apx_clientSessionHandler_t handler;
#ifdef _WIN32
   unsigned int threadId;
#endif
} apx_clientSession_t;


#define APX_CLIENT_SESSION_MAX_MESSAGES 10
#define APX_CLIENT_SESSION_MESSAGE_SIZE sizeof(apx_cmd_t)

//////////////////////////////////////////////////////////////////////////////
// FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t apx_clientSession_create(apx_clientSession_t *self, apx_clientSessionHandler_t *sessionHandler);
void apx_clientSession_destroy(apx_clientSession_t *self);
apx_clientSession_t* apx_clientSession_new(apx_clientSessionHandler_t *sessionHandler);
void apx_clientSession_delete(apx_clientSession_t *self);
void apx_clientSession_vdelete(void *arg);
void apx_clientSession_start(apx_clientSession_t *self);
void apx_clientSession_stop(apx_clientSession_t *self);
void apx_clientSession_setSessionHandler(apx_clientSession_t *self, apx_clientSessionHandler_t *sessionHandler);
void apx_clientSession_connectCmd(apx_clientSession_t *self);
void apx_clientSession_disconnectCmd(apx_clientSession_t *self);
void apx_clientSession_completedCmd(apx_clientSession_t *self, int32_t userCode);
void apx_clientSession_pingNodeByNameCmd(apx_clientSession_t *self, const char *nodeName);


#endif //APX_CLIENT_SESSION_H
