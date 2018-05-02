#ifndef APX_FILE_MANAGER_H
#define APX_FILE_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif

#include "apx_fileManager_cfg.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif
#include "osmacro.h"
#include "ringbuf.h"
#include "apx_allocator.h"
#include "apx_msg.h"
#include "apx_types.h"
#include "apx_nodeData.h"
#include "apx_fileMap.h"
#include "adt_bytearray.h"
#include "apx_transmitHandler.h"
#include "apx_serverEventRecorder.h"
#include "apx_serverEventPlayer.h"
#include "apx_clientEventRecorder.h"
#include "apx_clientEventPlayer.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//forward declaration
struct apx_nodeData_tag;
struct apx_nodeManager_tag;

#define APX_FILEMANAGER_CLIENT_MODE 0
#define APX_FILEMANAGER_SERVER_MODE 1


typedef struct apx_serverEventContainer_tag
{
   apx_serverEventRecorder_t *recorder;
   apx_serverEventPlayer_t *player;
}apx_serverEventContainer_t;

typedef struct apx_clientEventContainer_tag
{
   apx_clientEventRecorder_t *recorder;
   apx_clientEventPlayer_t *player;
}apx_clientEventContainer_t;

typedef struct apx_fileManager_tag
{
   //OS interaction variables
   THREAD_T workerThread; //local worker thread
   SPINLOCK_T lock;  //variable lock
   SEMAPHORE_T semaphore; //thread semaphore

   //data object, all read/write accesses to these must be protected by the lock variable above
   rbfs_t ringbuffer; //pending messages
   bool workerThreadValid;
   void *debugInfo;
   uint8_t *ringbufferData; //strong pointer to raw data used by our ringbuffer
   uint32_t ringbufferLen; //number of items in ringbuffer
   apx_allocator_t allocator;

   apx_fileMap_t localFileMap;
   apx_fileMap_t remoteFileMap;
   uint8_t mode; //this could be either APX_FILEMANAGER_CLIENT_MODE or APX_FILEMANAGER_SERVER_MODE
   apx_transmitHandler_t transmitHandler;

   uint32_t curFileStartAddress; //cached start address of last accessed file
   uint32_t curFileEndAddress; //cached end address of of last accessed file
   apx_file_t *curFile; //weak pointer to last accessed file

   struct apx_nodeManager_tag *nodeManager; //weak pointer to attached nodeManager
   bool isConnected;
   union events_tag
   {
      //A file manager cannot be both server and client at the same time. Determine which union value is valid by testing self->mode variable.
      apx_serverEventContainer_t server;
      apx_clientEventContainer_t client;
   } event;
#ifdef _WIN32
   unsigned int threadId;
#endif
}apx_fileManager_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t apx_fileManager_create(apx_fileManager_t *self, uint8_t mode);
void apx_fileManager_destroy(apx_fileManager_t *self);
apx_fileManager_t *apx_fileManager_new(uint8_t mod);
void apx_fileManager_delete(apx_fileManager_t *self);
void apx_fileManager_vdelete(void *arg);

void apx_fileManager_start(apx_fileManager_t *self);
void apx_fileManager_stop(apx_fileManager_t *self);

void apx_fileManager_setNodeManager(apx_fileManager_t *self, struct apx_nodeManager_tag *nodeManager); //used to create remote nodes
void apx_fileManager_setTransmitHandler(apx_fileManager_t *self, apx_transmitHandler_t *handler);
int32_t apx_fileManager_parseMessage(apx_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen);
void apx_fileManager_sendFileOpen(apx_fileManager_t *self, uint32_t remoteAddress);
apx_file_t *apx_fileManager_findRemoteFile(apx_fileManager_t *self, const char *name);
void apx_fileManager_attachLocalDefinitionFile(apx_fileManager_t *self, apx_file_t *localFile);
void apx_fileManager_attachLocalPortDataFile(apx_fileManager_t *self, apx_file_t *localFile);
void apx_fileManager_attachLocalDataFile(apx_fileManager_t *self, apx_file_t *localFile);
const char *apx_fileManager_modeString(apx_fileManager_t *self);
void apx_fileManager_setDebugInfo(apx_fileManager_t *self, void *debugInfo);

//these messages can be sent to the fileManager to be processed by its internal worker thread
void apx_fileManager_onConnected(apx_fileManager_t *self);
void apx_fileManager_onDisconnected(apx_fileManager_t *self);
void apx_fileManager_triggerFileUpdatedEvent(apx_fileManager_t *self, apx_file_t *file, uint32_t offset, uint32_t length);
void apx_fileManager_triggerFileWriteCmdEvent(apx_fileManager_t *self, apx_file_t *file, const uint8_t *data, apx_offset_t offset, apx_size_t length);

#endif //APX_FILE_MANAGER_H
