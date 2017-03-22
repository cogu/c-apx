/**
 * description: embedded version of the APX fileManager. The intention is to:
 * 1. apply to MISRA rules
 * 2. attempt not to use dynamic memory (malloc/free) as far as possible.
 */
#ifndef APX_ES_FILE_MANAGER_H
#define APX_ES_FILE_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx_nodeData.h"
#include "apx_file.h"
#include "apx_transmitHandler.h"
#include "apx_es_FileManager_cfg.h"
#include "apx_es_fileMap.h"
#include "ringbuf.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declaration
struct apx_nodeData_tag;
struct apx_es_nodeManager_tag;

#define APX_FILEMANAGER_CLIENT_MODE 0
#define APX_FILEMANAGER_SERVER_MODE 1

typedef struct apx_es_file_write_tag
{
   uint32_t writeAddress;
   uint32_t readOffset;
   apx_file_t *localFile;
   uint32_t remain;
} apx_es_file_write_t;

typedef struct apx_es_command_tag
{
   uint8_t buf[RMF_MAX_CMD_BUF_SIZE];
   uint32_t length;
}apx_es_command_t;

typedef struct apx_es_fileManager_tag
{
    //data object, all read/write accesses to these must be protected by the lock variable above
   rbfs_t messageQueue; //internal message queue (contains apx_msg_t object)
   uint8_t *messageQueueBuf; //strong reference to byte buffer
   uint16_t messageQueueLen; //number of items in messageQueue (length of each message is sizeof(apx_msg_t))
   uint8_t *receiveBuf; //receive buffer for large writes
   uint32_t receiveBufLen; //length of receive buffer
   uint32_t receiveBufOffset; //current write position (and length) of receive buffer
   uint32_t receiveStartAddress;

   apx_es_fileMap_t localFileMap;
   apx_es_fileMap_t remoteFileMap;
   apx_file_t requestedFileList[APX_ES_FILEMANAGER_MAX_NUM_REQUEST_FILES];
   uint16_t numRequestedFiles;

   apx_transmitHandler_t transmitHandler;

   apx_file_t *curFile; //weak pointer to last accessed file

   bool pendingWrite;
   bool pendingCmd;
   bool dropMessage;

   apx_es_command_t cmdInfo;
   apx_es_file_write_t fileWriteInfo;

   struct apx_es_nodeManager_tag *nodeManager; //weak pointer to attached nodeManager
   bool isConnected; //true if this fileManager is connected to an underlying communication device (like a TCP socket or SPI stream)
}apx_es_fileManager_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//int8_t apx_es_fileManager_create(apx_es_fileManager_t *self, uint8_t *messageQueueBuf, uint16_t messageQueueLen, uint8_t *messageDataBuf, uint16_t messageDataLen);
int8_t apx_es_fileManager_create(apx_es_fileManager_t *self, uint8_t *messageQueueBuf, uint16_t messageQueueLen, uint8_t *receiveBuf, uint16_t receiveBufLen);

void apx_es_fileManager_attachLocalFile(apx_es_fileManager_t *self, apx_file_t *localFile);
void apx_es_fileManager_requestRemoteFile(apx_es_fileManager_t *self, apx_file_t *requestedFile);

void apx_es_fileManager_setTransmitHandler(apx_es_fileManager_t *self, apx_transmitHandler_t *handler);

//these messages can be sent to the fileManager to be processed by its internal worker thread
void apx_es_fileManager_onConnected(apx_es_fileManager_t *self);
void apx_es_fileManager_onDisconnected(apx_es_fileManager_t *self);
void apx_es_fileManager_onFileUpdate(apx_es_fileManager_t *self, apx_file_t *file, uint32_t offset, uint32_t length);
void apx_es_fileManager_onMsgReceived(apx_es_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen);

void apx_es_fileManager_run(apx_es_fileManager_t *self);
#ifdef UNIT_TEST
#define DYN_STATIC

DYN_STATIC int8_t apx_es_fileManager_removeRequestedAt(apx_es_fileManager_t *self, int32_t removeIndex);

#else
#define DYN_STATIC static
#endif


#endif //APX_ES_FILE_MANAGER_H

