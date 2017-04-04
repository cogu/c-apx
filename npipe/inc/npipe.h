#ifndef NPIPE_H
#define NPIPE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <stdint.h>
#include <stdbool.h>
#include "adt_bytearray.h"
#include "osmacro.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define NPIPE_BUFSIZE 4096
#define NPIPE_MAX_MSG_LEN NPIPE_BUFSIZE

#define NPIPE_STATE_NONE          0
#define NPIPE_STATE_ACCEPTING     1 //server state
#define NPIPE_STATE_PENDING       2 //client state
#define NPIPE_STATE_CONNECTED     3
#define NPIPE_STATE_CLOSING       4
#define NPIPE_STATE_CLOSED        5


//forward declarations
struct npipe_server_tag;

typedef struct npipe_handlerTable_tag
{
   void(*onAccept)(void *arg, struct npipe_server_tag *srv, struct npipe_tag *npipe);
   void(*onConnected)(void *arg);
   void(*onDisconnected)(void *arg);
   int8_t(*onData)(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen); //return 0 on success, -1 on failure (this will force the pipe to close)
   void *arg;
}npipe_handlerTable_t;

typedef struct npipe_tag
{
   HANDLE hPipe;
   OVERLAPPED overlappedRead;
   OVERLAPPED overlappedWrite;
   HANDLE readEvent; //overlapped read event 
   HANDLE writeEvent; //overlapped write event 
   HANDLE quitEvent;
   char *path;
   adt_bytearray_t receiveBuf;
   adt_bytearray_t sendBuf;
   npipe_handlerTable_t handlerTable;   
   uint8_t state;
   MUTEX_T mutex;
   THREAD_T ioThread;
   unsigned int ioThreadId;
   bool isThreadRunning;
   bool isServerPipe;
   bool hasPendingWrite;
   bool hasPendingRead;

}npipe_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//client and server functions
void npipe_create(npipe_t *self);
void npipe_destroy(npipe_t *self);
npipe_t *npipe_new(void);
void npipe_delete(npipe_t *self);
void npipe_vdelete(void *arg);
void npipe_close(npipe_t *self);
void npipe_sethandlerTable(npipe_t *self, const npipe_handlerTable_t *handlerTable);
int8_t npipe_send(npipe_t *self, const uint8_t *msgData, uint32_t msgLen);

//client-only functions
int8_t npipe_connect(npipe_t *self, const char *path);

//server-only functions
int8_t npipe_start_io(npipe_t *self);


#endif //NPIPE_H

