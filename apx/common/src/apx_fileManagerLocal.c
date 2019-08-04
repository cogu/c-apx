/*****************************************************************************
* \file      apx_fileManagerLocal.c
* \author    Conny Gustafsson
* \date      2018-08-02
* \brief     APX Filemanager local representation
*
* Copyright (c) 2018 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "apx_fileManagerLocal.h"
#include "rmf.h"
#include "apx_file2.h"
#include "numheader.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//temporary include
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
#if 0
static int8_t apx_fileManagerWorker_startThread(apx_fileManager_t *self);
static THREAD_PROTO(threadTask, arg);

static void apx_fileManagerWorker_connectHandler(apx_fileManager_t *self);
static void apx_fileManagerWorker_fileWriteNotifyHandler(apx_fileManager_t *self, apx_file2_t *file, apx_offset_t offset, apx_size_t len);
static void apx_fileManagerWorker_fileWriteCmdHandler(apx_fileManager_t *self, apx_file2_t *file, const uint8_t *data, apx_offset_t offset, apx_size_t len);

static void apx_fileManagerWorker_sendFileInfo(apx_fileManager_t *self, rmf_fileInfo_t *fileInfo);
static void apx_fileManagerWorker_sendAck(apx_fileManager_t *self);
#endif

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_fileManagerLocal_create(apx_fileManagerLocal_t *self, apx_fileManagerShared_t *shared)
{
   if (self != 0)
   {
      MUTEX_INIT(self->mutex);
      apx_fileMap_create(&self->localFileMap);
      self->shared = shared;
   }
}

void apx_fileManagerLocal_destroy(apx_fileManagerLocal_t *self)
{
   if (self != 0)
   {
      apx_fileMap_destroy(&self->localFileMap);
      MUTEX_DESTROY(self->mutex);
   }
}

void apx_fileManagerLocal_attachFile(apx_fileManagerLocal_t *self, struct apx_file2_tag *localFile, void *caller)
{
   if ((self != 0) && (localFile != 0))
   {
      apx_fileMap_insertFile(&self->localFileMap, localFile);
      if (self->shared->fileCreated != 0)
      {
         self->shared->fileCreated(self->shared->arg, localFile, caller);
      }
      if ( (self->shared->isConnected) && (self->shared->sendFileInfo != 0) )
      {
         self->shared->sendFileInfo(self->shared->arg, localFile);
      }
   }
}

int32_t apx_fileManagerLocal_getNumFiles(apx_fileManagerLocal_t *self)
{
   if (self != 0)
   {
      return apx_fileMap_length(&self->localFileMap);
   }
   errno = EINVAL;
   return -1;
}

void apx_fileManagerLocal_sendFileInfo(apx_fileManagerLocal_t *self)
{
   if ( (self != 0) && (self->shared->sendFileInfo != 0) )
   {
      MUTEX_LOCK(self->mutex);
      adt_list_t *files = apx_fileMap_getList(&self->localFileMap);
      MUTEX_UNLOCK(self->mutex);
      if (files != 0)
      {
         adt_list_elem_t *iter = adt_list_iter_first(files);
         while (iter != 0)
         {
            apx_file2_t *file = (apx_file2_t*)iter->pItem;
            printf("Sending FileInfo for %s\n", file->fileInfo.name);
            self->shared->sendFileInfo(self->shared->arg, file);
            iter = adt_list_iter_next(iter);
         }
      }
   }
}

struct apx_file2_tag *apx_fileManagerLocal_find(apx_fileManagerLocal_t *self, uint32_t address)
{
   apx_file2_t *localFile = (apx_file2_t *) 0;
   if (self != 0)
   {
      MUTEX_LOCK(self->mutex);
      localFile = apx_fileMap_findByAddress(&self->localFileMap, address);
      MUTEX_UNLOCK(self->mutex);
   }
   return localFile;
}

struct apx_file2_tag *apx_fileManagerLocal_findByName(apx_fileManagerLocal_t *self, const char *name)
{
   apx_file2_t *localFile = (apx_file2_t *) 0;
   if (self != 0)
   {
      MUTEX_LOCK(self->mutex);
      localFile = apx_fileMap_findByName(&self->localFileMap, name);
      MUTEX_UNLOCK(self->mutex);
   }
   return localFile;
}


#if 0
static void apx_fileManager_processOpenFile(apx_fileManager_t *self, const rmf_cmdOpenFile_t *cmdOpenFile)
{
   if ( (self != 0) && (cmdOpenFile != 0) )
   {
      apx_file2_t *localFile;
      SPINLOCK_ENTER(self->lock);
      localFile = apx_fileMap_findByAddress(&self->localFileMap, cmdOpenFile->address);
      SPINLOCK_LEAVE(self->lock);
      if (localFile != 0)
      {
         int32_t bytesToSend = localFile->fileInfo.length;
         if (self->debugInfo != (void*) 0)
         {
            APX_LOG_DEBUG("[APX_FILE_MANAGER] (%p) Client opened %s", self->debugInfo, localFile->fileInfo.name);
         }
         apx_fileManager_triggerFileUpdatedEvent(self, localFile, 0, bytesToSend);
         if (localFile->nodeData != 0)
         {
            apx_file_open(localFile);
            if ( localFile->fileType == APX_OUTDATA_FILE )
            {
               apx_nodeData_setOutPortDataFile(localFile->nodeData, localFile);
               apx_nodeData_setFileManager(localFile->nodeData, self);
            }
            else if ( localFile->fileType == APX_INDATA_FILE)
            {
               apx_nodeData_setInPortDataFile(localFile->nodeData, localFile);
               apx_nodeData_setFileManager(localFile->nodeData, self);
            }
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_fileManager_startThread(apx_fileManager_t *self)
{
   if( (self != 0) && (self->workerThreadValid == false) ){
      self->workerThreadValid = true;
#ifdef _WIN32
      THREAD_CREATE(self->workerThread,threadTask,self,self->threadId);
      if(self->workerThread == INVALID_HANDLE_VALUE){
         self->workerThreadValid = false;
         return -1;
      }
#else
      int rc = THREAD_CREATE(self->workerThread,threadTask,self);
      if(rc != 0){
         self->workerThreadValid = false;
         return -1;
      }
#endif
      //from this point forward all access to self must be protected by self->lock
      return 0;
   }
   errno = EINVAL;
   return -1;
}

/**
* Internal event Handler
*/
static THREAD_PROTO(threadTask,arg)
{
   if(arg!=0)
   {
      apx_msg_t msg;
      apx_fileManager_t *self;
      uint32_t messages_processed=0;
      bool isRunning=true;
      self = (apx_fileManager_t*) arg;
      while(isRunning == true)
      {

#ifdef _MSC_VER
         DWORD result = WaitForSingleObject(self->semaphore, INFINITE);
         if (result == WAIT_OBJECT_0)
#else
         int result = sem_wait(&self->semaphore);
         if (result == 0)
#endif
         {
            SPINLOCK_ENTER(self->lock);
            rbfs_remove(&self->ringbuffer,(uint8_t*) &msg);
            SPINLOCK_LEAVE(self->lock);
            messages_processed++;
            switch(msg.msgType)
            {
            case RMF_MSG_EXIT:
               isRunning=false;
               break;
            case RMF_MSG_CONNECT:
               apx_fileManager_connectHandler(self);
               break;
            case RMF_MSG_WRITE_NOTIFY:
               apx_fileManager_fileWriteNotifyHandler(self, (apx_file2_t*) msg.msgData3, (apx_offset_t) msg.msgData1, (apx_size_t) msg.msgData2);
               break;
            case RMF_MSG_FILE_WRITE:
               apx_fileManager_fileWriteCmdHandler(self, (apx_file2_t*) msg.msgData3, (const uint8_t*) msg.msgData4, (apx_offset_t) msg.msgData1, (apx_size_t) msg.msgData2);
               apx_allocator_free(&self->allocator, (uint8_t*) msg.msgData4, (uint32_t) msg.msgData2);
               break;
            default:
               APX_LOG_ERROR("[APX_FILE_MANAGER]: unknown message type: %u", msg.msgType);
               isRunning=false;
               break;
            }
         }
         else
         {
            APX_LOG_ERROR("[APX_FILE_MANAGER]: failure while waiting for semaphore, errno=%d",errno);
            break;
         }
      }
      APX_LOG_ERROR("[APX_FILE_MANAGER]: messages_processed: %u",messages_processed);
   }
   THREAD_RETURN(0);
}

/**
 * Handlers are run by our worker thread
 */
static void apx_fileManager_connectHandler(apx_fileManager_t *self)
{
   if ( (self != 0) )
   {
      SPINLOCK_ENTER(self->lock);
      self->isConnected = true;
      SPINLOCK_LEAVE(self->lock);
      if (self->transmitHandler.send != 0)
      {
         adt_list_elem_t *iter;
         if (self->mode == APX_FILEMANAGER_SERVER_MODE)
         {
            apx_fileManager_sendAck(self);
         }
         SPINLOCK_ENTER(self->lock);
         adt_list_iter_init(&self->localFileMap.fileList);
         SPINLOCK_LEAVE(self->lock);
         do
         {
            SPINLOCK_ENTER(self->lock);
            iter = adt_list_iter_next(&self->localFileMap.fileList);
            SPINLOCK_LEAVE(self->lock);
            if (iter != 0)
            {
               apx_file2_t *file = (apx_file2_t*)iter->pItem;
               assert(file != 0);
               apx_fileManager_sendFileInfo(self, &file->fileInfo);
            }
         } while (iter != 0);
      }
   }
}

/**
 * called by worker thread when it needs to send data from local files to remote connections
 */
static void apx_fileManager_fileWriteNotifyHandler(apx_fileManager_t *self, apx_file2_t *file, apx_offset_t offset, apx_size_t len)
{
   if ( (self != 0) && (file != 0) && (len > 0) )
   {
      //in addition to the data itself we need to send a 2 byte or 4 byte header in addition to the actual data
      //to achieve this we increase the len variable with 4 bytes and then adjust for the header length later
      uint8_t *buf=0;
      buf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, len+RMF_MAX_HEADER_SIZE);
      if (buf != 0)
      {
         uint8_t *dataBuf = &buf[RMF_MAX_HEADER_SIZE]; //the dataBuf starts RMF_MAX_HEADER_SIZE (4 bytes) into buf
         int32_t dataLen = len;
         int32_t address = file->fileInfo.address + offset;
         int8_t result=-1;
         switch(file->fileType)
         {
            case APX_UNKNOWN_FILE:
               break;
            case APX_OUTDATA_FILE:
               result = apx_nodeData_readOutPortData(file->nodeData, dataBuf, offset, dataLen);
               if (result != 0)
               {
                  APX_LOG_ERROR("[APX_FILE_MANAGER] apx_nodeData_readOutPortData failed");
               }
               break;
            case APX_INDATA_FILE:
               result = apx_nodeData_readInPortData(file->nodeData, dataBuf, offset, dataLen);
               if (result != 0)
               {
                  APX_LOG_ERROR("[APX_FILE_MANAGER] apx_nodeData_writeInData failed");
               }
               break;
            case APX_DEFINITION_FILE:
               result = apx_nodeData_readDefinitionData(file->nodeData, dataBuf, offset, dataLen);
               if (result != 0)
               {
                  APX_LOG_ERROR("[APX_FILE_MANAGER] apx_nodeData_readDefinitionData failed");
               }
               break;
            default:
               //TODO: check fpr user data files here
               break;
         }
         if (result == 0)
         {
            int32_t headerLen = rmf_packHeaderBeforeData(dataBuf, RMF_MAX_HEADER_SIZE, address, false);
            if (headerLen > 0)
            {
               int32_t msgLen = (headerLen+dataLen);
               self->transmitHandler.send(self->transmitHandler.arg, RMF_MAX_HEADER_SIZE-headerLen, msgLen);
            }
         }
      }
   }
}

/**
 * called by worker thread when data in a remote file needs to be updated
 */
static void apx_fileManager_fileWriteCmdHandler(apx_fileManager_t *self, apx_file2_t *file, const uint8_t *data, apx_offset_t offset, apx_size_t len)
{
   if ( (self != 0) && (file != 0) && (data != 0) )
   {
      if ( (file->fileType == APX_INDATA_FILE) && (file->nodeData != 0) && (file->nodeData->inPortDataBuf != 0) )
      {
         uint32_t startOffset=offset;
         uint32_t endOffset = startOffset+len;
         if ( (startOffset >= file->fileInfo.length) || (endOffset > file->fileInfo.length) )
         {
            APX_LOG_ERROR("[APX_FILE_MANAGER(%s)] attempted write outside bounds, file=%s, offset=%d, len=%d", apx_fileManager_modeString(self), file->fileInfo.name, (int) offset, (int) len);
         }
         else
         {
            int8_t result;
            result = apx_nodeData_writeInPortData(file->nodeData, data, offset, len);
            if (result != 0)
            {
               APX_LOG_ERROR("[APX_FILE_MANAGER(%s)] apx_nodeData_writeInPortData(%d,%d) failed, file=%s", apx_fileManager_modeString(self), offset, len, file->fileInfo.name);
            }
            else
            {
               if ( (self->isConnected == true) && (file->isOpen == true) )
               {
                  uint8_t *sendBuf=0;
                  if (self->debugInfo != 0)
                  {
                     APX_LOG_DEBUG("[APX_FILE_MANAGER] (%p) Server Write %s[%d,%d]", self->debugInfo, file->fileInfo.name, (int) offset, (int) len );
                  }
                  sendBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, len+RMF_MAX_HEADER_SIZE);
                  if (sendBuf != 0)
                  {
                     uint8_t *dataBuf = &sendBuf[RMF_MAX_HEADER_SIZE]; //the dataBuf starts RMF_MAX_HEADER_SIZE (4 bytes) into sendBuf, this gives us enough room for a header
                     int32_t dataLen = len;
                     int32_t headerLen;
                     int32_t address = file->fileInfo.address + offset;
                     memcpy(dataBuf,data,len);
                     headerLen = rmf_packHeaderBeforeData(dataBuf, RMF_MAX_HEADER_SIZE, address, false);
                     if (headerLen > 0)
                     {
                        int32_t msgLen = (headerLen+dataLen);
                        self->transmitHandler.send(self->transmitHandler.arg, RMF_MAX_HEADER_SIZE-headerLen, msgLen);
                     }
                  }
               }
               else if (file->isOpen == false)
               {
                  APX_LOG_WARNING("[APX_FILE_MANAGER] Attempted Write on closed file %s", file->fileInfo.name);
               }
            }
         }
      }
   }
}

static void apx_fileManagerWorker_sendFileInfo(apx_fileManager_t *self, rmf_fileInfo_t *fileInfo)
{
   if (self != 0)
   {
      uint8_t *buf;
      buf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, RMF_MAX_CMD_BUF_SIZE+RMF_MAX_HEADER_SIZE);
      if (buf != 0)
      {
         int32_t bufLen = RMF_MAX_CMD_BUF_SIZE;
         uint8_t *dataBuf = &buf[RMF_MAX_HEADER_SIZE]; //the dataBuf starts RMF_MAX_HEADER_SIZE (4 bytes) into buf
         int32_t dataLen;
         rmf_fileInfo_t cmd;
         strcpy( (char*)&cmd.name, (char*)fileInfo->name);
         cmd.address = fileInfo->address;
         memcpy(cmd.digestData, fileInfo->digestData, RMF_DIGEST_SIZE);
         cmd.digestType = fileInfo->digestType;
         cmd.fileType = fileInfo->fileType;
         cmd.length = fileInfo->length;
         dataLen = rmf_serialize_cmdFileInfo(dataBuf,bufLen,&cmd);
         if (dataLen > 0)
         {
            int32_t headerLen = rmf_packHeaderBeforeData(dataBuf, RMF_MAX_HEADER_SIZE, RMF_CMD_START_ADDR, false);
            if (headerLen > 0)
            {
               int32_t msgLen = (headerLen+dataLen);
               self->transmitHandler.send(self->transmitHandler.arg, RMF_MAX_HEADER_SIZE-headerLen, msgLen);
            }
         }
      }
   }
}

/*
* send an acknowledge message
*/
static void apx_fileManagerWorker_sendAck(apx_fileManager_t *self)
{
   if (self != 0)
   {
      uint8_t *sendBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, RMF_MAX_CMD_BUF_SIZE + RMF_MAX_HEADER_SIZE);
      if (sendBuf != 0)
      {
         uint8_t *dataBuf = &sendBuf[RMF_MAX_HEADER_SIZE]; //the dataBuf starts RMF_MAX_HEADER_SIZE (4 bytes) into buf
         int32_t dataLen;
         dataLen = rmf_serialize_acknowledge(dataBuf, RMF_MAX_CMD_BUF_SIZE);
         if (dataLen > 0)
         {
            int32_t headerLen = rmf_packHeaderBeforeData(dataBuf, RMF_MAX_HEADER_SIZE, RMF_CMD_START_ADDR, false);
            if ( (headerLen > 0) && (headerLen<= (int32_t) RMF_MAX_HEADER_SIZE) )
            {
               int32_t msgLen = (headerLen + dataLen);
               self->transmitHandler.send(self->transmitHandler.arg, RMF_MAX_HEADER_SIZE - headerLen, msgLen);
            }
         }
      }
   }
}

/**
 * sends a file open request
 */
static void apx_fileManagerWorker_sendFileOpen(apx_fileManager_t *self, uint32_t remoteAddress)
{
   uint8_t *buf;
   assert(self->transmitHandler.getSendBuffer != 0);
   buf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, RMF_MAX_CMD_BUF_SIZE+RMF_MAX_HEADER_SIZE);
   if (buf != 0)
   {
      int32_t bufLen = RMF_MAX_CMD_BUF_SIZE;
      uint8_t *dataBuf = &buf[RMF_MAX_HEADER_SIZE]; //the dataBuf starts RMF_MAX_HEADER_SIZE (4 bytes) into buf
      int32_t dataLen;
      rmf_cmdOpenFile_t cmdOpenFile;
      cmdOpenFile.address = remoteAddress;
      dataLen = rmf_serialize_cmdOpenFile(dataBuf, bufLen, &cmdOpenFile);
      if (dataLen > 0)
      {
         int32_t headerLen = rmf_packHeaderBeforeData(dataBuf, RMF_MAX_HEADER_SIZE, RMF_CMD_START_ADDR, false);
         if (headerLen > 0)
         {
            int32_t msgLen = (headerLen+dataLen);
            self->transmitHandler.send(self->transmitHandler.arg, RMF_MAX_HEADER_SIZE-headerLen, msgLen);
         }
      }
   }
}
#endif
