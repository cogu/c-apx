#ifndef NPIPE_SERVER_H
#define NPIPE_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <Windows.h>
#include <stdint.h>
#include <stdbool.h>
#include "osmacro.h"
#include "npipe.h"
#include "adt_ary.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct  npipe_server_tag
{
   char *pipePath;
   OVERLAPPED overlappedConnect;
   HANDLE connectEvent;
   HANDLE quitEvent;
   THREAD_T acceptThread;
   THREAD_T cleanupThread;
   SEMAPHORE_T sem;
   MUTEX_T mutex;
   bool isThreadRunning;
   void(*pDestructor)(void *arg);
   unsigned int acceptThreadId;
   unsigned int cleanupThreadId;
   npipe_handlerTable_t handlerTable;
   adt_ary_t cleanupItems;
   HANDLE acceptPipe;
} npipe_server_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void npipe_server_create(npipe_server_t *self, void(*pDestructor)(void*));
void npipe_server_destroy(npipe_server_t *self);
npipe_server_t *npipe_server_new(void(*pDestructor)(void*));
void npipe_server_delete(npipe_server_t *self);
void npipe_server_sethandlerTable(npipe_server_t *self, npipe_handlerTable_t *handler);
void npipe_server_start(npipe_server_t *self, const char *pipePath);
void npipe_server_cleanup_connection(npipe_server_t *self, void *arg);


#endif //NPIPE_SERVER_H

