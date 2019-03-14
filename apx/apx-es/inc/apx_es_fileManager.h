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
#include "apx_msg.h"
#include "apx_transmitHandler.h"
#include "apx_es_fileManager_cfg.h"
#include "apx_es_fileMap.h"
#include "ringbuf.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_es_file_write_tag
{
   uint32_t writeAddress;
   uint32_t readOffset;
   apx_file_t *localFile;
   uint32_t remain;
} apx_es_file_write_t;

typedef struct apx_es_fileManager_tag
{
   rbfs_t messageQueue; //internal message queue (contains apx_msg_t object)

   uint8_t *receiveBuf; //receive buffer for large writes
   uint32_t receiveBufLen; //length of receive buffer
   uint32_t receiveBufOffset; //current write position (and length) of receive buffer
   uint32_t receiveStartAddress;

   apx_es_fileMap_t localFileMap;
   apx_es_fileMap_t remoteFileMap;
   apx_file_t *requestedFileList[APX_ES_FILEMANAGER_MAX_NUM_REQUEST_FILES];
   uint16_t numRequestedFiles;

   apx_transmitHandler_t transmitHandler;
   // Buffer used to pack multiple messages prior to transmitHandler.send()
   uint8_t *transmitBuf;
   uint32_t transmitBufLen; // Max size allowed
   uint32_t transmitBufFilledBytes;

   bool pendingWrite; // Used then a sent message is fragmented using more_bit
   bool dropMessage; // Used when received messages are larger than receive buffer
   bool isConnected; // When fileManager is connected to an underlying communication device (like a TCP socket or SPI stream)
   apx_file_t *curFile; // Weak pointer to last accessed file

   apx_msg_t queuedWriteNotify; // Last write notification waiting for more data (until apx_es_fileManager_run() is called)
   apx_msg_t pendingMsg; // Message taken out of the queue and not yet serialized
   apx_es_file_write_t fileWriteInfo;
}apx_es_fileManager_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t apx_es_fileManager_create(apx_es_fileManager_t *self, uint8_t *messageQueueBuf, uint16_t messageQueueLen, uint8_t *receiveBuf, uint16_t receiveBufLen);

// Files added will be kept track of thur connect/disconnect events
// The fileManager will open/close these files as requested by the server
void apx_es_fileManager_attachLocalFile(apx_es_fileManager_t *self, apx_file_t *localFile);
void apx_es_fileManager_requestRemoteFile(apx_es_fileManager_t *self, apx_file_t *requestedFile);

// Ensure the transmitHandler normally can provide a MAX(APX_ES_FILEMANAGER_MAX_CMD_BUF_SIZE,
// APX_ES_FILE_WRITE_MSG_FRAGMENTATION_THRESHOLD) sized send buffer
// The transmitHandler arg is used to set optimal write size (currently not supported by apx-server)
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
DYN_STATIC void apx_es_fileManager_processRemoteFileInfo(apx_es_fileManager_t *self, const rmf_fileInfo_t *fileInfo);

#else
#define DYN_STATIC static
#endif


#endif //APX_ES_FILE_MANAGER_H

