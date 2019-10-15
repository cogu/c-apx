//////////////////////////////////////////////////////////////////////////////
// DEFINES
//////////////////////////////////////////////////////////////////////////////
#ifndef APX_DEBUG_ENABLE
#define APX_DEBUG_ENABLE 0
#endif

#if APX_DEBUG_ENABLE
#ifndef APX_MSQ_QUEUE_WARN_THRESHOLD
#define APX_MSQ_QUEUE_WARN_THRESHOLD 8
#endif
#endif

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <assert.h>
#if APX_DEBUG_ENABLE || defined(UNIT_TEST)
#include <stdio.h>
#endif
#include <limits.h>
#include "apx_es_fileManager.h"
#include "apx_error.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_es_resetConnectionState(apx_es_fileManager_t *self);
static void apx_es_initTransmitBuf(apx_es_fileManager_t *self);
static apx_error_t apx_es_transmitMsg(apx_es_fileManager_t *self, uint32_t msgSize);
static int32_t apx_es_processPendingWrite(apx_es_fileManager_t *self);
static int32_t apx_es_fileManager_runEventLoop(apx_es_fileManager_t *self);
static void apx_es_initFragmentedFileWrite(apx_es_fileManager_t *self,
                                           apx_file_t *file,
                                           uint32_t readOffset,
                                           uint32_t writeAddress,
                                           int32_t dataLen);
static int32_t apx_es_createFileWriteMsg(apx_es_fileManager_t* self, int32_t sendAvail);
static void apx_es_transmitSuccess(apx_es_fileManager_t* self);
static void apx_es_processQueuedWriteNotify(apx_es_fileManager_t *self);
static int32_t apx_es_fileManager_calcSendAvail(apx_es_fileManager_t *self);
static int32_t apx_es_fileManager_processPendingMessage(apx_es_fileManager_t *self);
static int32_t apx_es_fileManager_preparePendingMessage(apx_es_fileManager_t *self);
static int32_t apx_es_fileManager_transmitPendingMessage(apx_es_fileManager_t *self, int32_t msgLen);
static void apx_es_fileManager_parseCmdMsg(apx_es_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen);
static void apx_es_fileManager_parseDataMsg(apx_es_fileManager_t *self, uint32_t address, const uint8_t *dataBuf, int32_t dataLen, bool more_bit);
static void apx_es_fileManager_processOpenFile(apx_es_fileManager_t *self, const rmf_cmdOpenFile_t *cmdOpenFile);
static void apx_es_fileManager_setError(apx_es_fileManager_t *self, apx_error_t errorCode);
#ifndef UNIT_TEST
DYN_STATIC int8_t apx_es_fileManager_removeRequestedAt(apx_es_fileManager_t *self, int32_t removeIndex);
DYN_STATIC void apx_es_fileManager_processRemoteFileInfo(apx_es_fileManager_t *self, const rmf_fileInfo_t *fileInfo);
#endif

//local inline functions
static inline uint8_t apx_es_calcHeaderLenForAddress(uint32_t address);
static inline bool apx_es_isPendingMessage(const apx_msg_t* msg);


//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_es_fileManager_create(apx_es_fileManager_t *self, uint8_t *messageQueueBuf, uint16_t messageQueueSize, uint8_t *receiveBuf, uint16_t receiveBufLen)
{
   int8_t retval = APX_INVALID_ARGUMENT_ERROR;
   if ( (self != 0) && (messageQueueBuf != 0) && (messageQueueSize != 0))
   {
      rbfs_create(&self->messageQueue, messageQueueBuf, messageQueueSize, (uint8_t) sizeof(apx_msg_t));
      self->receiveBuf = receiveBuf;
      self->receiveBufLen = receiveBufLen;
      apx_es_fileMap_create(&self->localFileMap);
      apx_es_fileMap_create(&self->remoteFileMap);
      apx_es_fileManager_setTransmitHandler(self, 0);
      self->numRequestedFiles = 0;

      apx_es_resetConnectionState(self);
      retval = APX_NO_ERROR;
   }
   return retval;
}

void apx_es_fileManager_attachLocalFile(apx_es_fileManager_t *self, apx_file_t *localFile)
{
   if ( (self != 0) && (localFile != 0) )
   {
      apx_es_fileMap_autoInsert(&self->localFileMap, localFile);
   }
}

void apx_es_fileManager_requestRemoteFile(apx_es_fileManager_t *self, apx_file_t *requestedFile)
{
   if ( (self != 0) && (requestedFile != 0) )
   {
      int32_t i;
      if ( self->numRequestedFiles >= APX_ES_FILEMANAGER_MAX_NUM_REQUEST_FILES)
      {
         return;
      }
      //prevent duplicates
      for(i=0;i<self->numRequestedFiles;i++)
      {
         apx_file_t *file = self->requestedFileList[i];
         if (strcmp(requestedFile->fileInfo.name, file->fileInfo.name)==0)
         {
            return;
         }
      }
      self->requestedFileList[self->numRequestedFiles++] = requestedFile;
   }
}

void apx_es_fileManager_setTransmitHandler(apx_es_fileManager_t *self, apx_transmitHandler_t *handler)
{
   if (self != 0)
   {
      if (handler == 0)
      {
         memset(&self->transmitHandler, 0, sizeof(apx_transmitHandler_t));
      }
      else
      {
         memcpy(&self->transmitHandler, handler, sizeof(apx_transmitHandler_t));
      }
   }
}

void apx_es_fileManager_onConnected(apx_es_fileManager_t *self)
{
   if (self != 0)
   {
      int32_t i;
      int32_t localFileCount = apx_es_fileMap_length(&self->localFileMap);
      self->isConnected = true;
      for(i=0;i<localFileCount;i++)
      {
         apx_file_t *file = apx_es_fileMap_get(&self->localFileMap,i);
         if (file != 0)
         {
            uint8_t result;
            apx_msg_t msg = {RMF_MSG_FILEINFO,0,0, {0}};
            msg.msgData3.ptr = file;
#if APX_DEBUG_ENABLE
            if (rbfs_free(&self->messageQueue) <= APX_MSQ_QUEUE_WARN_THRESHOLD)
            {
               fprintf(stderr, "messageQueue fill warning for RMF_MSG_FILEINFO. Free before add: %d\n", rbfs_free(&self->messageQueue));
            }
#endif
            result = rbfs_insert(&self->messageQueue,(uint8_t*) &msg);
            if (result != E_BUF_OK)
            {
               apx_es_fileManager_setError(self, APX_QUEUE_FULL_ERROR);
            }
         }
      }
   }
}

void apx_es_fileManager_onDisconnected(apx_es_fileManager_t *self)
{
   if (self != 0)
   {
      int32_t i;
      int32_t fileCount = apx_es_fileMap_length(&self->remoteFileMap);

      apx_es_resetConnectionState(self);

      // Move opened remote files back to request list
      for(i=0; i < fileCount; ++i)
      {
         apx_file_t *file = apx_es_fileMap_get(&self->remoteFileMap, i);
         apx_file_close(file);
         apx_es_fileManager_requestRemoteFile(self, file);
      }
      apx_es_fileMap_clear(&self->remoteFileMap);

      fileCount = apx_es_fileMap_length(&self->localFileMap);
      for(i=0; i < fileCount; ++i)
      {
         apx_file_t *file = apx_es_fileMap_get(&self->localFileMap, i);
         apx_file_close(file);
      }
   }
}

/**
 * triggered by lower layer when a message has been received
 */
void apx_es_fileManager_onMsgReceived(apx_es_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen)
{
   rmf_msg_t msg;
   int32_t result = rmf_unpackMsg(msgBuf, msgLen, &msg);
   if ( (result > 0) && (self != 0) )
   {
#if APX_DEBUG_ENABLE
      printf("[APX_ES_FILEMANAGER] address: %08X\n", msg.address);
      printf("[APX_ES_FILEMANAGER] length: %d\n", msg.dataLen);
      printf("[APX_ES_FILEMANAGER] more_bit: %d\n", (int) msg.more_bit);
#endif
      if (msg.address == RMF_CMD_START_ADDR)
      {
         apx_es_fileManager_parseCmdMsg(self, msg.data, msg.dataLen);
      }
      else if (msg.address < RMF_CMD_START_ADDR)
      {
         apx_es_fileManager_parseDataMsg(self, msg.address, msg.data, msg.dataLen, msg.more_bit);
      }
      else
      {
         result = -2;
      }
   }
   if (result <= 0)
   {
#if APX_DEBUG_ENABLE
      fprintf(stderr, "Discarding: rmf_unpackMsg failed or bad address %d\n", result);
#endif
      // result=0: Not enough data to parse header will get out of sync
      // result<0: bad arguments to rmf_unpackMsg or too high address
      assert(false);
   }
   else
   {
      // Data parsed
   }
}

/**
 * triggered when the local file is written to
 * Returns 0 on success, negative value on error.
 */
int8_t apx_es_fileManager_triggerFileUpdate(apx_es_fileManager_t *self, apx_file_t *file, uint32_t offset, uint32_t length)
{
   int8_t retval = APX_NO_ERROR;
   if ( (self != 0) && (file != 0) && (length > 0) )
   {
      apx_msg_t msg = {RMF_MSG_WRITE_NOTIFY, 0, 0, {0} }; //{msgType,msgData1,msgData2,msgData3.ptr}
      msg.msgData1 = offset;
      msg.msgData2 = length;
      msg.msgData3.ptr = (void*)file;
      if (self->queuedWriteNotify.msgType == RMF_MSG_WRITE_NOTIFY)
      {
         // Check if sequential write to the same file as last received fileUpdate
         const uint32_t lengthOfQueuedFileUpdate     = self->queuedWriteNotify.msgData2;
         const uint32_t potentialSequentialWriteSize = lengthOfQueuedFileUpdate + length;
         const uint32_t offsetOfQueuedFileUpdate     = self->queuedWriteNotify.msgData1;
         const uint32_t oneBytePastQueuedFileUpdate  = offsetOfQueuedFileUpdate +
                                                       lengthOfQueuedFileUpdate;
         const bool smallEnoughToAllowAlign = (potentialSequentialWriteSize <= (APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD-RMF_HIGH_ADDRESS_SIZE));
         const bool sameFileAsQueued = (void*)file == self->queuedWriteNotify.msgData3.ptr;
         if ( sameFileAsQueued &&
              ( oneBytePastQueuedFileUpdate == offset) &&
              smallEnoughToAllowAlign )
         {
            // The write aligns. Just update the size
            self->queuedWriteNotify.msgData2 = potentialSequentialWriteSize;
         }
         else
         {
            uint8_t result = rbfs_insert(&self->messageQueue,(const uint8_t*) &self->queuedWriteNotify);
            if (result == E_BUF_OK)
            {
               self->queuedWriteNotify = msg;
            }
            else
            {
               retval = APX_QUEUE_FULL_ERROR;
               apx_es_fileManager_setError(self, (apx_error_t) retval);
            }
         }
      }
      else
      {
         // First Write notify after run()
         self->queuedWriteNotify = msg;
      }
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

/**
 * Returns 0 on success, negative value on error.
 */
int8_t apx_es_fileManager_triggerDirectWrite(apx_es_fileManager_t *self, uint8_t *data, uint32_t address, uint32_t length)
{
   int8_t retval = APX_NO_ERROR;
   if ( (self != 0) && (data != 0) && (length > 0) )
   {
      uint8_t result;
      apx_msg_t msg = {RMF_MSG_DIRECT_WRITE, 0, 0, {0} }; //{msgType,msgData1,msgData2,msgData3.data}
      msg.msgData1 = address;
      msg.msgData2 = length;
      memcpy(&msg.msgData3.data[0], data, length);
      result = rbfs_insert(&self->messageQueue, (const uint8_t*) &msg);
      if (result != E_BUF_OK)
      {
         retval = APX_QUEUE_FULL_ERROR;
         apx_es_fileManager_setError(self, (apx_error_t) retval);
      }
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

/**
 * runs the message handler until either the sendbuffer is full or the are no more messages to process
 */
void apx_es_fileManager_run(apx_es_fileManager_t *self)
{
   int32_t result;
   if ( (self == 0) || (!self->isConnected) || (self->transmitHandler.getMsgBuffer == 0) || (self->transmitHandler.sendMsg == 0) )
   {
      return;
   }

   if (self->lastErrorCode != APX_NO_ERROR)
   {
      return; //stay in error mode
   }

   apx_es_initTransmitBuf(self);
   if (self->hasPendingWrite)
   {
      apx_es_processPendingWrite(self);
   }
   if (apx_es_isPendingMessage(&self->pendingMsg))
   {
      assert(!self->hasPendingWrite);
      apx_es_fileManager_processPendingMessage(self);
   }
   if (self->queuedWriteNotify.msgType == RMF_MSG_WRITE_NOTIFY)
   {
      // Add any pending write notifications to the msg queue
      apx_es_processQueuedWriteNotify(self);
   }

   result = 1;
   while ( (result > 0) && (!self->hasPendingWrite) && (!apx_es_isPendingMessage(&self->pendingMsg) ) )
   {
      result = apx_es_fileManager_runEventLoop(self);
   }
}

bool apx_es_fileManager_hasPendingMsg(apx_es_fileManager_t *self)
{
   if (self != 0)
   {
      return apx_es_isPendingMessage(&self->pendingMsg);
   }
   return false;
}

apx_error_t apx_es_fileManager_getLastError(apx_es_fileManager_t *self)
{
   apx_error_t retval;
   if (self != 0)
   {
      retval = self->lastErrorCode;
   }
   else
   {
      retval = APX_INVALID_ARGUMENT_ERROR;
   }
   return retval;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_es_resetConnectionState(apx_es_fileManager_t *self)
{
   rbfs_clear(&self->messageQueue);
   self->receiveBufOffset = 0;
   self->receiveStartAddress = RMF_INVALID_ADDRESS;
   self->queuedWriteNotify.msgType = RMF_CMD_INVALID_MSG;
   self->pendingMsg.msgType = RMF_CMD_INVALID_MSG;
   self->transmitBuf.avail = 0;
   self->transmitBuf.data = (uint8_t*) 0;
   self->transmitBuf.maxMsgLen = 0;
   self->hasPendingWrite = false;
   self->dropMessage = false;
   self->isConnected = false;
   self->curFile = 0;
   self->lastErrorCode = APX_NO_ERROR;
   memset(&self->fileWriteInfo, 0, sizeof(apx_es_file_write_t));
}

static void apx_es_initTransmitBuf(apx_es_fileManager_t *self)
{
   if (self != 0)
   {
      self->transmitBuf.data = self->transmitHandler.getMsgBuffer(self->transmitHandler.arg, &self->transmitBuf.maxMsgLen, &self->transmitBuf.avail);
      if (self->transmitBuf.data == (uint8_t*) 0)
      {
         self->transmitBuf.avail = 0;
      }
   }
}

static apx_error_t apx_es_transmitMsg(apx_es_fileManager_t *self, uint32_t msgSize)
{
   apx_error_t retval = APX_NO_ERROR;
   if (msgSize > 0)
   {
      int32_t bytesConsumed = self->transmitHandler.sendMsg(self->transmitHandler.arg, 0, msgSize);
      if(bytesConsumed < 0)
      {
         if (bytesConsumed == APX_TRANSMIT_HANDLER_INVALID_ARGUMENT_ERROR)
         {
            retval = APX_INVALID_ARGUMENT_ERROR;
         }
         else if (bytesConsumed == APX_TRANSMIT_HANDLER_BUFFER_OVERFLOW_ERROR)
         {
            retval = APX_BUFFER_FULL_ERROR;
         }
         else
         {
            retval = APX_TRANSMIT_ERROR;
         }
      }
      else
      {
         if (bytesConsumed > self->transmitBuf.avail)
         {
            self->transmitBuf.avail = 0;
         }
         else
         {
            self->transmitBuf.avail -= bytesConsumed;
         }
      }
   }
   return retval;
}

/**
 * Returns 0 when it can no longer write any more data to underlying message buffer.
 * Returns 1 on completion.
 * Returns -1 on error. The error code can be read using apx_getLastError.
 */
static int32_t apx_es_processPendingWrite(apx_es_fileManager_t *self)
{
   int32_t retval = 1;
   while( (self->hasPendingWrite) && (retval > 0) )
   {
      int32_t sendAvail = apx_es_fileManager_calcSendAvail(self);
      retval = 0;
      if (sendAvail >= APX_ES_FILEMANAGER_MIN_BUFFER_TRESHOLD)
      {
         int32_t msgLen = apx_es_createFileWriteMsg(self, sendAvail);
         if (msgLen >= 0)
         {
            int32_t result = apx_es_fileManager_transmitPendingMessage(self, msgLen);
            retval = (result > 0)? 1 : result;
         }
         else
         {
            retval = -1;
            break;
         }
      }
   }
   return retval;
}

/**
 * runs internal event loop
 * returns 0 when no more messages can be processed, -1 on error and 1 on success
 */
static int32_t apx_es_fileManager_runEventLoop(apx_es_fileManager_t *self)
{
   int32_t retval = 0;
   int8_t rc = rbfs_remove(&self->messageQueue, (uint8_t*) &self->pendingMsg);
   if (rc == E_BUF_OK)
   {
      retval = apx_es_fileManager_processPendingMessage(self);
   }
   return retval;
}

/**
 * This is the apx_es_fileManager worker (by the main event loop)
 * Prior to call the transmitBuf needs to be prepared.
 * Returns 1 on success.
 * In case of error it returns -1.
 * In case of no available buffer it returns 0.
 */
static int32_t apx_es_fileManager_processPendingMessage(apx_es_fileManager_t *self)
{
   int32_t retval = 0;
   int32_t msgLen = apx_es_fileManager_preparePendingMessage(self);
   if (msgLen >= 0)
   {
      int32_t result = apx_es_fileManager_transmitPendingMessage(self, msgLen);
      retval = (result > 0)? 1 : result;
   }
   if ( (retval > 0) && (self->hasPendingWrite) )
   {
      retval = apx_es_processPendingWrite(self);
   }
   return retval;
}

/**
 * Returns next message length (in bytes).
 * Returns -1 on error.
 * Returns 0 when no buffer is available
 */
static int32_t apx_es_fileManager_preparePendingMessage(apx_es_fileManager_t *self)
{
   int32_t retval = 0;
   int32_t sendAvail = apx_es_fileManager_calcSendAvail(self);
   if (sendAvail > 0)
   {
      uint32_t address;
      int32_t headerLen = 0;
      int32_t dataLen   = 0;
      uint8_t *msgBuf   = self->transmitBuf.data;
      int32_t msgLen = 0;
      switch(self->pendingMsg.msgType)
      {
      case RMF_MSG_FILEINFO:
         {
            int32_t nameLen;
            apx_file_t *file = (apx_file_t*) self->pendingMsg.msgData3.ptr;
            nameLen = strlen(file->fileInfo.name);
            headerLen = RMF_HIGH_ADDRESS_SIZE;
            dataLen = CMD_FILE_INFO_BASE_SIZE + nameLen + 1; //+1 for null terminator
            msgLen = headerLen+dataLen;
            if (msgLen <= sendAvail)
            {
               if (dataLen != rmf_serialize_cmdFileInfo(&msgBuf[headerLen], dataLen, &file->fileInfo))
               {
                  apx_es_fileManager_setError(self, APX_PACK_ERROR);
                  msgLen = -1;
               }
               if (headerLen != rmf_packHeaderBeforeData(&msgBuf[headerLen], headerLen, RMF_CMD_START_ADDR, false))
               {
                  apx_es_fileManager_setError(self, APX_PACK_ERROR);
                  msgLen = -1;
               }
            }
            else
            {
               msgLen = 0;
            }
         }
         break;
      case RMF_MSG_FILE_OPEN: //sends file open command
         {
            rmf_cmdOpenFile_t cmdOpenFile;
            headerLen = (int32_t) RMF_HIGH_ADDRESS_SIZE;
            dataLen = (int32_t) RMF_FILE_OPEN_CMD_LEN;
            msgLen = headerLen+dataLen;
            cmdOpenFile.address = self->pendingMsg.msgData1;
            if (msgLen <= sendAvail)
            {
               // Mark the remote file as open
               apx_file_open((apx_file_t*) self->pendingMsg.msgData3.ptr);
               retval = msgLen;
               if (dataLen != rmf_serialize_cmdOpenFile(&msgBuf[headerLen], dataLen, &cmdOpenFile))
               {
                  apx_es_fileManager_setError(self, APX_PACK_ERROR);
                  msgLen = -1;
               }
               if (headerLen != rmf_packHeaderBeforeData(&msgBuf[headerLen], headerLen, RMF_CMD_START_ADDR, false))
               {
                  apx_es_fileManager_setError(self, APX_PACK_ERROR);
                  msgLen = -1;
               }
            }
            else
            {
               msgLen = 0;
            }
         }
         break;
      case RMF_MSG_WRITE_NOTIFY:
         {
            uint32_t offset;
            apx_file_t *file;
            dataLen = self->pendingMsg.msgData2;
            offset = self->pendingMsg.msgData1;
            file = (apx_file_t*) self->pendingMsg.msgData3.ptr;
            address = file->fileInfo.address + offset;
            headerLen = apx_es_calcHeaderLenForAddress(address);
            msgLen = headerLen + dataLen;
            if (msgLen <= sendAvail)
            {
               if (headerLen != rmf_packHeader(&msgBuf[0], headerLen, address, false))
               {
                  apx_es_fileManager_setError(self, APX_PACK_ERROR);
                  msgLen = -1;
               }
               else if (apx_file_read(file, &msgBuf[headerLen], offset, dataLen) != 0)
               {
                  apx_es_fileManager_setError(self, APX_READ_ERROR);
                  msgLen = -1;
               }
               else
               {
                  //MISRA
               }
            }
            else if ( (msgLen >= APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD) && (APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD <= sendAvail) )
            {
               assert(sendAvail >= (int32_t) RMF_MIN_MSG_LEN);
               apx_es_initFragmentedFileWrite(self, file, 0, address, dataLen);
               msgLen = apx_es_createFileWriteMsg(self, sendAvail);
            }
            else
            {
               msgLen = 0;
            }
         }
         break;
      case RMF_MSG_FILE_SEND: //sends local file to remote side
         {
            apx_file_t *file = (apx_file_t*) self->pendingMsg.msgData3.ptr;
            address = file->fileInfo.address;
            headerLen = apx_es_calcHeaderLenForAddress(address);
            dataLen = file->fileInfo.length;
            msgLen = dataLen + headerLen;
            apx_file_open(file);
            if (msgLen <= sendAvail)
            {
               if ( (headerLen == rmf_packHeader(&msgBuf[0], headerLen, address, false)) &&
                    (0 == apx_file_read(file, &msgBuf[headerLen], 0, dataLen)) )
               {
                  retval = msgLen;
               }
               else
               {
                  retval = -1;
               }
            }
            else if ( (msgLen >= APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD) && (APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD <= sendAvail) )
            {
               assert(sendAvail >= (int32_t) RMF_MIN_MSG_LEN);
               apx_es_initFragmentedFileWrite(self, file, 0, address, dataLen);
               msgLen = apx_es_createFileWriteMsg(self, sendAvail);
               retval = msgLen;
            }
            else
            {
               msgLen = 0;
            }
         }
         break;
      case RMF_MSG_DIRECT_WRITE:
         {
            uint8_t *dataPtr;
            address = self->pendingMsg.msgData1;
            dataLen = self->pendingMsg.msgData2;
            dataPtr = &self->pendingMsg.msgData3.data[0];
            headerLen = apx_es_calcHeaderLenForAddress(address);
            msgLen = headerLen + dataLen;

            if (dataPtr == (uint8_t*) 0)
            {
               apx_es_fileManager_setError(self, APX_INVALID_ARGUMENT_ERROR);
               msgLen = -1;
            }
            else
            {
               if ( msgLen <= sendAvail )
               {
                  int32_t packedHeaderLen = rmf_packHeader(&msgBuf[0], headerLen, address, false);
                  assert(headerLen == packedHeaderLen);
                  memcpy(&msgBuf[headerLen], dataPtr, dataLen);
               }
               else
               {
                  msgLen = 0;
               }
            }
         }
         break;
      case RMF_MSG_CONNECT: // Intentional fall thru
      case RMF_MSG_FILE_WRITE:
      default:
         {
            apx_es_fileManager_setError(self, APX_INVALID_MSG_ERROR);
            msgLen = -1;
         }
         break;
      }
      retval = msgLen;
   }
   return retval;
}

/**
 * Transmits prepared message
 */
static int32_t apx_es_fileManager_transmitPendingMessage(apx_es_fileManager_t *self, int32_t msgLen)
{
   int32_t retval = msgLen;
   if (msgLen > 0)
   {
      apx_error_t errorCode = apx_es_transmitMsg(self, msgLen);
      if (errorCode == APX_NO_ERROR)
      {
         apx_es_transmitSuccess(self);
      }
      else if (errorCode == APX_BUFFER_FULL_ERROR)
      {
         retval = 0; //try again later
      }
      else
      {
         apx_es_fileManager_setError(self, errorCode);
         retval = -1;
      }
   }
   if (retval == 0)
   {
      //Verify that we at some point in the future will be able to transmit the message
      if (msgLen > self->transmitBuf.maxMsgLen)
      {
         apx_es_fileManager_setError(self, APX_MSG_TOO_LARGE_ERROR);
         retval = -1;
      }
   }
   else
   {
      //MISRA
   }
   return retval;
}

static void apx_es_initFragmentedFileWrite(apx_es_fileManager_t *self,
                                           apx_file_t *file,
                                           uint32_t readOffset,
                                           uint32_t writeAddress,
                                           int32_t dataLen)
{
   self->pendingMsg.msgType = RMF_CMD_INVALID_MSG;
   self->hasPendingWrite = true;
   self->fileWriteInfo.localFile = file;
   self->fileWriteInfo.readOffset = readOffset;
   self->fileWriteInfo.writeAddress = writeAddress;
   self->fileWriteInfo.remain = dataLen;
}

/**
 * Serializes a file write message from fileWriteInfo structure into transmit buffer.
 * Returns number of bytes written or -1 on error
 */
static int32_t apx_es_createFileWriteMsg(apx_es_fileManager_t* self, int32_t sendAvail)
{

   int32_t retval = 0;
   int32_t dataAvail;
   self->fileWriteInfo.headerLen = (int32_t) apx_es_calcHeaderLenForAddress(self->fileWriteInfo.writeAddress);

   dataAvail = sendAvail-self->fileWriteInfo.headerLen;
   if (dataAvail > 0)
   {
      bool hasMoreData = false;
      int32_t headerLen;
      int8_t readResult;
      if (self->fileWriteInfo.remain > dataAvail)
      {
         hasMoreData = true;
         self->fileWriteInfo.dataLen = dataAvail;
      }
      else
      {
         self->fileWriteInfo.dataLen = self->fileWriteInfo.remain;
      }
      assert (self->fileWriteInfo.dataLen > 0);
      headerLen = rmf_packHeader(&self->transmitBuf.data[0], sendAvail, self->fileWriteInfo.writeAddress, hasMoreData);
      readResult = apx_file_read(self->fileWriteInfo.localFile, &self->transmitBuf.data[self->fileWriteInfo.headerLen], self->fileWriteInfo.readOffset, self->fileWriteInfo.dataLen);

      if (headerLen != self->fileWriteInfo.headerLen)
      {
         apx_es_fileManager_setError(self, APX_PACK_ERROR);
         retval = -1;
      }
      else if (readResult != 0)
      {
         apx_es_fileManager_setError(self, APX_READ_ERROR);
         retval = -1;
      }
      else
      {
         retval = self->fileWriteInfo.headerLen + self->fileWriteInfo.dataLen;
      }
   }
   return retval;
}

static void apx_es_transmitSuccess(apx_es_fileManager_t* self)
{
   if (self->hasPendingWrite)
   {
      self->fileWriteInfo.remain -= self->fileWriteInfo.dataLen;
      if (self->fileWriteInfo.remain == 0)
      {
         self->hasPendingWrite = false;
      }
      else
      {
         self->fileWriteInfo.readOffset += self->fileWriteInfo.dataLen;
         self->fileWriteInfo.writeAddress += self->fileWriteInfo.dataLen;
      }
   }
   else
   {
      self->pendingMsg.msgType = RMF_CMD_INVALID_MSG;
   }
}

static void apx_es_processQueuedWriteNotify(apx_es_fileManager_t *self)
{
   uint8_t result = rbfs_insert(&self->messageQueue, (const uint8_t*) &self->queuedWriteNotify);
   if (result == E_BUF_OK)
   {
      self->queuedWriteNotify.msgType = RMF_CMD_INVALID_MSG;
   }
   else
   {
      apx_es_fileManager_setError(self, APX_QUEUE_FULL_ERROR);
   }
}

static int32_t apx_es_fileManager_calcSendAvail(apx_es_fileManager_t *self)
{
   int32_t retval = self->transmitBuf.avail;
   if (retval > self->transmitBuf.maxMsgLen)
   {
      retval = self->transmitBuf.maxMsgLen;
   }
   return retval;
}

static void apx_es_fileManager_parseCmdMsg(apx_es_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen)
{
   if (msgBuf != 0)
   {
      uint32_t cmdType;
      int32_t result;
      result = rmf_deserialize_cmdType(msgBuf, msgLen, &cmdType);
      if (result > 0)
      {
         switch(cmdType)
         {
            case RMF_CMD_FILE_INFO:
               {
                  rmf_fileInfo_t fileInfo;
                  result = rmf_deserialize_cmdFileInfo(msgBuf, msgLen, &fileInfo);
                  if (result > 0)
                  {
                     apx_es_fileManager_processRemoteFileInfo(self, &fileInfo);
                  }
                  else if (result < 0)
                  {
#if APX_DEBUG_ENABLE
                     fprintf(stderr, "rmf_deserialize_cmdFileInfo failed with %d\n", result);
#endif
                  }
                  else
                  {
#if APX_DEBUG_ENABLE
                     fprintf(stderr, "rmf_deserialize_cmdFileInfo returned 0\n");
#endif
                  }
               }
               break;
            case RMF_CMD_FILE_OPEN:
               {
                  rmf_cmdOpenFile_t cmdOpenFile;
                  result = rmf_deserialize_cmdOpenFile(msgBuf, msgLen, &cmdOpenFile);
                  if (result > 0)
                  {
                     apx_es_fileManager_processOpenFile(self, &cmdOpenFile);
                  }
                  else if (result < 0)
                  {
#if APX_DEBUG_ENABLE
                     fprintf(stderr, "rmf_deserialize_cmdOpenFile failed with %d\n", result);
#endif
                  }
                  else
                  {
#if APX_DEBUG_ENABLE
                     fprintf(stderr, "rmf_deserialize_cmdOpenFile returned 0\n");
#endif
                  }
               }
               break;
            default:
#if APX_DEBUG_ENABLE
               fprintf(stderr, "not implemented cmdType: %u\n", cmdType);
#endif
               break;

         }
      }
   }
}

/**
 * called when a data message has been received.
 */
static void apx_es_fileManager_parseDataMsg(apx_es_fileManager_t *self, uint32_t address, const uint8_t *dataBuf, int32_t dataLen, bool more_bit)
{
   if ( (dataBuf != 0) && (dataLen>=0) )
   {
      uint32_t offset;
      if ( (self->receiveStartAddress == RMF_INVALID_ADDRESS) )
      {
         //new reception
         apx_file_t *remoteFile = apx_es_fileMap_findByAddress(&self->remoteFileMap, address);
         if ( (remoteFile != 0) && remoteFile->isOpen)
         {
            offset=address-remoteFile->fileInfo.address;
            if (!more_bit)
            {
               apx_file_write(remoteFile, dataBuf, offset, dataLen);
            }
            else if(((uint32_t)dataLen) <= self->receiveBufLen)
            {
               //start fragmented message reception
               self->curFile = remoteFile;
               self->receiveStartAddress = address;
               memcpy(self->receiveBuf, dataBuf, dataLen);
               self->receiveBufOffset = dataLen;
            }
            else
            {
               //drop message
               self->receiveStartAddress = address;
               self->dropMessage = true; //message too long
#if APX_DEBUG_ENABLE
               fprintf(stderr, "[APX_ES_FILEMANAGER] message too long (%d bytes), message dropped\n",dataLen);
#endif
            }
         }
      }
      else
      {
         if (self->dropMessage)
         {
            // Avoid curFile deref - keep dropping message
            offset = self->receiveBufLen;
         }
         else
         {
            offset = address-self->curFile->fileInfo.address;
         }
         if (offset != self->receiveBufOffset)
         {
            self->dropMessage = true; //drop message since offsets don't match
#if APX_DEBUG_ENABLE
            fprintf(stderr, "[APX_ES_FILEMANAGER] invalid offset (%u), message dropped\n",offset);
#endif
         }
         else if((offset+dataLen) <= self->receiveBufLen)
         {
            //copy data
            memcpy(&self->receiveBuf[offset], dataBuf, dataLen);
            self->receiveBufOffset = offset + dataLen;
         }
         else
         {
            self->dropMessage = true; //message too long
#if APX_DEBUG_ENABLE
            fprintf(stderr, "[APX_ES_FILEMANAGER] message too long (%d bytes), message dropped\n",dataLen);
#endif
         }

         if (!more_bit)
         {
            if (!self->dropMessage)
            {
               //send message to upper layer
               uint32_t startOffset=self->receiveStartAddress-self->curFile->fileInfo.address;
               apx_file_write(self->curFile, self->receiveBuf, startOffset, self->receiveBufOffset);
            }
            //reset variables for next reception
            self->dropMessage=false;
            self->curFile=0;
            self->receiveStartAddress=RMF_INVALID_ADDRESS;
            self->receiveBufOffset=0;
         }

      }
      //printf("apx_es_fileManager_parseDataMsg %08X, %d, %d\n",address, (int) dataLen, (int) more_bit);
   }
}

/**
 * called when we see a new rmf_cmdFileInfo_t in the input/parse stream
 */
DYN_STATIC void apx_es_fileManager_processRemoteFileInfo(apx_es_fileManager_t *self, const rmf_fileInfo_t *fileInfo)
{
   if (fileInfo != 0)
   {
      int32_t i;
      apx_file_t *file=0;
      int32_t removeIndex=-1;
      uint8_t result;
      for(i=0;i<self->numRequestedFiles;i++)
      {
         file = self->requestedFileList[i];
         if (file != 0)
         {
            if (strcmp(file->fileInfo.name, fileInfo->name)==0)
            {
               if (file->fileInfo.length == fileInfo->length)
               {
                  removeIndex=i;
                  break;
               }
               else
               {
#if APX_DEBUG_ENABLE
                  fprintf(stderr, "[APX_ES_FILEMANAGER] unexpected file size of file %s. Expected %d, got %d\n",file->fileInfo.name,
                        file->fileInfo.length, fileInfo->length);
#endif
               }
            }
         }
      }
      if (removeIndex>=0)
      {
         apx_msg_t msg = {RMF_MSG_FILE_OPEN, 0, 0, {0} };
#if APX_DEBUG_ENABLE
         printf("Opening requested file: %s\n", fileInfo->name);
#endif
         //remove file from requestedFileList
         int8_t rc = apx_es_fileManager_removeRequestedAt(self, removeIndex);
         assert(rc == 0);
         assert(file != 0);
         //copy fileInfo data into and file->fileInfo
         file->fileInfo.address = fileInfo->address;
         file->fileInfo.fileType = fileInfo->fileType;
         file->fileInfo.digestType = fileInfo->digestType;
         memcpy(&file->fileInfo.digestData, fileInfo->digestData, RMF_DIGEST_SIZE);
         msg.msgData1 = file->fileInfo.address;
         apx_es_fileMap_insert(&self->remoteFileMap, file);
         // Delay open indication until it reaches the top of messageQueue
         msg.msgData3.ptr = (void*) file;
#if APX_DEBUG_ENABLE
         if (rbfs_free(&self->messageQueue) <= APX_MSQ_QUEUE_WARN_THRESHOLD)
         {
            fprintf(stderr, "messageQueue fill warning for RMF_MSG_FILE_OPEN. Free before add: %d\n", rbfs_free(&self->messageQueue));
         }
#endif
         result = rbfs_insert(&self->messageQueue, (const uint8_t*) &msg);
         if (result != E_BUF_OK)
         {
            apx_es_fileManager_setError(self, APX_QUEUE_FULL_ERROR);
         }
      }
   }
}

static void apx_es_fileManager_processOpenFile(apx_es_fileManager_t *self, const rmf_cmdOpenFile_t *cmdOpenFile)
{
   if (cmdOpenFile != 0)
   {

      apx_file_t *localFile = apx_es_fileMap_findByAddress(&self->localFileMap, cmdOpenFile->address);
      if (localFile != 0)
      {
         uint8_t result;
         apx_msg_t msg = {RMF_MSG_FILE_SEND, 0, 0, {0} };
         // Delay file open indication until we start to transmit
         msg.msgData3.ptr = localFile;
#if APX_DEBUG_ENABLE
         if (rbfs_free(&self->messageQueue) <= APX_MSQ_QUEUE_WARN_THRESHOLD)
         {
            fprintf(stderr, "messageQueue fill warning for RMF_MSG_FILE_SEND. Free before add: %d\n", rbfs_free(&self->messageQueue));
         }
#endif
         result = rbfs_insert(&self->messageQueue,(uint8_t*) &msg);
         if (result != E_BUF_OK)
         {
            apx_es_fileManager_setError(self, APX_QUEUE_FULL_ERROR);
         }
      }
   }
}

/**
 * returns -1 on failure, 0 on success
 */
DYN_STATIC int8_t apx_es_fileManager_removeRequestedAt(apx_es_fileManager_t *self, int32_t removeIndex)
{
   if ( (self != 0) && (removeIndex>=0) && (removeIndex < self->numRequestedFiles) )
   {
      int32_t i;
      for(i=removeIndex+1; i<self->numRequestedFiles;i++)
      {
         //move item from removeIndex to removeIndex-1
         self->requestedFileList[i-1] = self->requestedFileList[i];
      }
      self->numRequestedFiles--; //remove last item
      return 0;
   }
   return -1;
}

static void apx_es_fileManager_setError(apx_es_fileManager_t *self, apx_error_t errorCode)
{
   self->lastErrorCode = errorCode;
}

#ifdef UNIT_TEST
int32_t apx_es_fileManager_getNumMessagesInQueue(apx_es_fileManager_t *self)
{
   return (int32_t) rbfs_size(&self->messageQueue);
}
#endif

//////////////////////////////////////////////////////////////////////////////
// INLINE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static inline uint8_t apx_es_calcHeaderLenForAddress(uint32_t address)
{
   return (address <= RMF_DATA_LOW_MAX_ADDR) ? RMF_LOW_ADDRESS_SIZE : RMF_HIGH_ADDRESS_SIZE;
}

static inline bool apx_es_isPendingMessage(const apx_msg_t* msg)
{
   return msg->msgType != RMF_CMD_INVALID_MSG;
}
