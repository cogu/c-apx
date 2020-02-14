/*****************************************************************************
* \file      apx_fileManagerWorker.c
* \author    Conny Gustafsson
* \date      2020-01-23
* \brief     APX Filemanager worker
*
* Copyright (c) 2020 Conny Gustafsson
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
#include <assert.h>
#include <string.h>
#include "apx_types.h"
//BEGIN TEMPORARY INCLUDES
#include <stdio.h>
//END TEMPORARY INCLUDES
#include "apx_fileManagerWorker.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
#define DYN_STATIC
#else
#define DYN_STATIC static
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int32_t apx_fileManagerWorker_calcFileInfoMsgSize(const struct apx_fileInfo_tag *fileInfo);
static int32_t apx_fileManagerWorker_serializeFileInfo(uint8_t *bufData, int32_t bufLen, const apx_fileInfo_t *fileInfo);
#ifndef UNIT_TEST
static apx_error_t apx_fileManager_starThread(apx_fileManager_t *self);
static void apx_fileManager_stopThread(apx_fileManager_t *self);
static THREAD_PROTO(workerThread,arg);
#endif
static bool workerThread_processMessage(apx_fileManagerWorker_t *self, apx_msg_t *msg);
static void workerThread_sendFileInfo(apx_fileManagerWorker_t *self, apx_msg_t *msg);
static void workerThread_sendFileOpen(apx_fileManagerWorker_t *self, apx_msg_t *msg);
static void workerThread_sendAcknowledge(apx_fileManagerWorker_t *self);
static apx_error_t workerThread_sendFileConstData(apx_fileManagerWorker_t *self, apx_msg_t *msg);
static apx_error_t apx_fileManagerWorker_processRingBufErrorCode(adt_buf_err_t errorCode);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManagerWorker_create(apx_fileManagerWorker_t *self, apx_fileManagerShared_t *shared, apx_mode_t mode)
{
   if (self != 0)
   {
      adt_buf_err_t bufResult;

      bufResult = adt_rbfh_create(&self->messages, (uint8_t) RMF_MSG_SIZE);

      if (bufResult != BUF_E_OK)
      {
         return APX_MEM_ERROR;
      }

      self->mode = mode;
      self->shared = shared;
      MUTEX_INIT(self->mutex);
      SPINLOCK_INIT(self->lock);
      SEMAPHORE_CREATE(self->semaphore);
#ifdef _WIN32
      self->workerThread = INVALID_HANDLE_VALUE;
#else
      self->workerThread = 0;
#endif
      self->workerThreadValid=false;
      self->numHeaderSize = 0u;

      apx_fileManagerWorker_setTransmitHandler(self, 0);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManagerWorker_destroy(apx_fileManagerWorker_t *self)
{
   if (self != 0)
   {
      if (self->workerThreadValid == true)
      {
         //apx_fileManagerWorker_stop(self);
      }
      MUTEX_DESTROY(self->mutex);
      SPINLOCK_DESTROY(self->lock);
      SEMAPHORE_DESTROY(self->semaphore);
      adt_rbfh_destroy(&self->messages);
   }
}



//Direct API

void apx_fileManagerWorker_setTransmitHandler(apx_fileManagerWorker_t *self, apx_transmitHandler_t *handler)
{
   if (self != 0)
   {
      SPINLOCK_ENTER(self->lock);
      if (handler == 0)
      {
         memset(&self->transmitHandler, 0, sizeof(apx_transmitHandler_t));
      }
      else
      {
         memcpy(&self->transmitHandler, handler, sizeof(apx_transmitHandler_t));
      }
      SPINLOCK_LEAVE(self->lock);
   }
}

void apx_fileManagerWorker_copyTransmitHandler(apx_fileManagerWorker_t *self, apx_transmitHandler_t *handler)
{
   if ( (self != 0) && (handler != 0) )
   {
      memcpy(handler, &self->transmitHandler, sizeof(apx_transmitHandler_t));
   }
}


void apx_fileManagerWorker_setNumHeaderSize(apx_fileManagerWorker_t *self, uint8_t bits)
{
   if (self != 0 && ( (bits == 16u) || (bits == 32u) ) )
   {
      self->numHeaderSize = bits;
   }
}


//Message API




void apx_fileManagerWorker_sendFileInfoMsg(apx_fileManagerWorker_t *self, apx_fileInfo_t *fileInfo)
{
   if (self != 0)
   {
      apx_msg_t msg = {APX_MSG_SEND_FILEINFO, 0, 0, {0}, 0};
      msg.msgData3.ptr = (void*) fileInfo;
      SPINLOCK_ENTER(self->lock);
      adt_rbfh_insert(&self->messages, (const uint8_t*) &msg);
      SPINLOCK_LEAVE(self->lock);
   #ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
   #endif
   }
}

void apx_fileManagerWorker_sendFileOpenMsg(apx_fileManagerWorker_t *self, uint32_t address)
{
   if (self != 0)
   {
      apx_msg_t msg = {APX_MSG_SEND_FILE_OPEN, 0, 0, {0}, 0};
      msg.msgData1 = address;
      SPINLOCK_ENTER(self->lock);
      adt_rbfh_insert(&self->messages, (const uint8_t*) &msg);
      SPINLOCK_LEAVE(self->lock);
   #ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
   #endif
   }
}

apx_error_t apx_fileManagerWorker_sendConstData(apx_fileManagerWorker_t *self, uint32_t address, uint32_t len, apx_file_read_const_data_func *readFunc, void *arg)
{
   if ( (self != 0) && (readFunc != 0) )
   {
      apx_msg_t msg = {APX_MSG_SEND_FILE_CONST_DATA, 0, 0, {0}, 0};
      msg.msgData1 = address;
      msg.msgData2 = len;
      msg.msgData3.ptr = readFunc;
      msg.msgData4 = arg;
      SPINLOCK_ENTER(self->lock);
      adt_rbfh_insert(&self->messages, (const uint8_t*) &msg);
      SPINLOCK_LEAVE(self->lock);
   #ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
   #endif
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManagerWorker_sendHeaderAckMsg(apx_fileManagerWorker_t *self)
{
   if ( (self != 0) )
   {
      adt_buf_err_t result;
      apx_msg_t msg = {APX_MSG_SEND_ACKNOWLEDGE, 0, 0, {0}, 0};
      SPINLOCK_ENTER(self->lock);
      result = adt_rbfh_insert(&self->messages, (const uint8_t*) &msg);
      SPINLOCK_LEAVE(self->lock);
      if (result == BUF_E_OK)
      {
#ifndef UNIT_TEST
         SEMAPHORE_POST(self->semaphore);
#endif
      }
      else
      {
         return apx_fileManagerWorker_processRingBufErrorCode(result);
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//UNIT TEST API

#ifdef UNIT_TEST
bool apx_fileManagerWorker_run(apx_fileManagerWorker_t *self)
{
   if (self != 0)
   {
      while (adt_rbfh_length(&self->messages) > 0)
      {
         apx_msg_t msg;
         adt_rbfh_remove(&self->messages,(uint8_t*) &msg);
         return workerThread_processMessage(self, &msg);
      }
   }
   return false;
}

int32_t apx_fileManagerWorker_numPendingMessages(apx_fileManagerWorker_t *self)
{
   if (self != 0)
   {
      return (int32_t) adt_rbfh_length(&self->messages);
   }
   return -1;
}
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static int32_t apx_fileManagerWorker_calcFileInfoMsgSize(const struct apx_fileInfo_tag *fileInfo)
{
   int32_t msgLen = RMF_CMD_ADDRESS_LEN+RMF_CMD_FILE_INFO_BASE_SIZE+1; //add 1 to fit string null terminator
   msgLen+=strlen(fileInfo->name);
   return msgLen;
}

static int32_t apx_fileManagerWorker_serializeFileInfo(uint8_t *bufData, int32_t bufLen, const apx_fileInfo_t *fileInfo)
{
   if ((bufData != 0) && (bufLen > 0) && (fileInfo != 0))
   {
      int32_t msgLen = apx_fileManagerWorker_calcFileInfoMsgSize(fileInfo);
      if (msgLen<=bufLen)
      {
         int32_t result;
         result = rmf_packHeader(bufData, bufLen, RMF_CMD_START_ADDR, false);
         if (result > 0)
         {
            rmf_fileInfo_t rmfInfo;
            apx_fileInfo_fillRmfInfo(fileInfo, &rmfInfo);
            bufData+=result;
            bufLen-=result;
            result = rmf_serialize_cmdFileInfo(bufData, bufLen, &rmfInfo);
            if (result > 0)
            {
               result = msgLen;
            }
         }
         return result;
      }
      else
      {
         return -1;
      }
   }
   return -1;
}



#ifndef UNIT_TEST
static int8_t apx_fileManagerWorker_startThread(apx_fileManagerWorker_t *self)
{
   if( self->workerThreadValid == false ){
      self->workerThreadValid = true;
#ifdef _WIN32
      THREAD_CREATE(self->workerThread, workerThread, self, self->threadId);
      if(self->workerThread == INVALID_HANDLE_VALUE){
         self->workerThreadValid = false;
         return -1;
      }
#else
      int rc = THREAD_CREATE(self->workerThread, workerThread, self);
      if(rc != 0){
         self->workerThreadValid = false;
         return -1;
      }
#endif
      return 0;
   }
   errno = EINVAL;
   return -1;
}

static void apx_fileManagerWorker_stopThread(apx_fileManagerWorker_t *self)
{
   if ( self->workerThreadValid == true )
   {
   #ifdef _MSC_VER
         DWORD result;
   #endif
         apx_msg_t msg = {APX_MSG_EXIT,0,0,{0}}; //{msgType, sender, msgData1, msgData2, msgData3}
         SPINLOCK_ENTER(self->lock);
         adt_rbfh_insert(&self->messages,(const uint8_t*) &msg);
         SPINLOCK_LEAVE(self->lock);
         SEMAPHORE_POST(self->semaphore);
   #ifdef _MSC_VER
         result = WaitForSingleObject(self->workerThread, 5000);
         if (result == WAIT_TIMEOUT)
         {
            APX_LOG_ERROR("[APX_FILE_MANAGER] timeout while joining workerThread");
         }
         else if (result == WAIT_FAILED)
         {
            DWORD lastError = GetLastError();
            APX_LOG_ERROR("[APX_FILE_MANAGER]  joining workerThread failed with %d", (int)lastError);
         }
         CloseHandle(self->workerThread);
         self->workerThread = INVALID_HANDLE_VALUE;
   #else
         if (pthread_equal(pthread_self(), self->workerThread) == 0)
         {
            void *status;
            int s = pthread_join(self->workerThread, &status);
            if (s != 0)
            {
               APX_LOG_ERROR("[APX_FILE_MANAGER] pthread_join error %d\n", s);
            }
         }
         else
         {
            APX_LOG_ERROR("[APX_FILE_MANAGER] pthread_join attempted on pthread_self()\n");
         }
   #endif
   self->workerThreadValid = false;
   }
}

static THREAD_PROTO(workerThread,arg)
{

}
#endif //UNIT_TEST

static bool workerThread_processMessage(apx_fileManagerWorker_t *self, apx_msg_t *msg)
{
   bool retval = true;
   uint32_t connectionId = apx_fileManagerShared_getConnectionId(self->shared);
   if (self->transmitHandler.send != 0)
   {
      apx_error_t rc;
      switch(msg->msgType)
      {
      case APX_MSG_EXIT:
         retval = false;
         break;
      case APX_MSG_SEND_FILEINFO:
         workerThread_sendFileInfo(self, msg);
         break;
      case APX_MSG_SEND_ACKNOWLEDGE:
         workerThread_sendAcknowledge(self);
         break;
      case APX_MSG_SEND_FILE_OPEN:
         workerThread_sendFileOpen(self, msg);
         break;
      case APX_MSG_SEND_FILE_CLOSE:
         break;
      case APX_MSG_SEND_FILE_CONST_DATA:
         rc = workerThread_sendFileConstData(self, msg);
         if (rc != APX_NO_ERROR)
         {
            printf("[WORKER] ERROR %d\n", (int) rc);
         }
         break;
      case APX_MSG_SEND_FILE_DATA:
         break;
      case APX_MSG_SEND_FILE_DATA_DIRECT:
         break;
      case APX_MSG_SEND_ERROR_CODE:
         break;
      default:
         printf("[APX_FILE_MANAGER_WORKER(%u)]: Unknown message type: %u\n", connectionId, msg->msgType);
         assert(0);
      }
   }
   return retval;
}

static void workerThread_sendFileInfo(apx_fileManagerWorker_t *self, apx_msg_t *msg)
{
   int32_t msgSize;
   uint8_t *msgBuf;
   apx_fileInfo_t *fileInfo = (apx_fileInfo_t*) msg->msgData3.ptr;
   assert(fileInfo != 0);
   msgSize = apx_fileManagerWorker_calcFileInfoMsgSize(fileInfo);
   assert(self->transmitHandler.getSendBuffer != 0);
   assert(self->transmitHandler.send != 0);
   msgBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgSize);
   if (msgBuf != 0)
   {
      int32_t result = apx_fileManagerWorker_serializeFileInfo(msgBuf, msgSize, fileInfo);
      if (result > 0)
      {
         self->transmitHandler.send(self->transmitHandler.arg, 0, result);
      }
   }
   apx_fileInfo_delete(fileInfo);
}

static void workerThread_sendFileOpen(apx_fileManagerWorker_t *self, apx_msg_t *msg)
{
   const int32_t msgSize = RMF_CMD_ADDRESS_LEN+RMF_CMD_FILE_OPEN_LEN;
   uint8_t *msgBuf;
   apx_file_t *file;
   uint32_t address = msg->msgData1;
   file = apx_fileManagerShared_findFileByAddress(self->shared, address);
   if (file != 0)
   {
      apx_file_open(file);
      assert(self->transmitHandler.getSendBuffer != 0);
      assert(self->transmitHandler.send != 0);
      msgBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgSize);
      if (msgBuf != 0)
      {
         int32_t result = rmf_packHeader(msgBuf, msgSize, RMF_CMD_START_ADDR, false);
         if (result == RMF_CMD_ADDRESS_LEN)
         {
            rmf_cmdOpenFile_t cmd;
            msgBuf+=RMF_CMD_ADDRESS_LEN;
            cmd.address = address & RMF_ADDRESS_MASK_INTERNAL;
            result = rmf_serialize_cmdOpenFile(msgBuf, RMF_CMD_FILE_OPEN_LEN, &cmd);
            if (result == RMF_CMD_FILE_OPEN_LEN)
            {
               self->transmitHandler.send(self->transmitHandler.arg, 0, msgSize);
            }
         }
      }
   }
}

static apx_error_t workerThread_sendFileConstData(apx_fileManagerWorker_t *self, apx_msg_t *msg)
{
   if ( (self != 0) && (msg != 0) )
   {
      int32_t headerSize;
      int32_t msgSize;
      uint8_t *msgBuf;
      apx_file_t *file;
      uint32_t startAddress;
      uint32_t address = msg->msgData1;
      uint32_t dataSize = msg->msgData2;
      void *arg = msg->msgData4;
      apx_file_read_const_data_func *readFunc = msg->msgData3.ptr;
      uint32_t offset;
      file = apx_fileManagerShared_findFileByAddress(self->shared, address & RMF_ADDRESS_MASK_INTERNAL);
      if (file == 0)
      {
         return APX_FILE_NOT_FOUND_ERROR;
      }
      if (apx_file_isOpen(file) == false)
      {
         return APX_FILE_NOT_OPEN_ERROR;
      }
      startAddress =  apx_file_getStartAddress(file);
      assert(address >= startAddress);
      offset = address - startAddress;
      assert(offset+dataSize <= apx_file_getFileSize(file));
      assert(self->transmitHandler.getSendBuffer != 0);
      assert(self->transmitHandler.send != 0);
      assert(readFunc != 0);
      headerSize = (address <= RMF_DATA_LOW_MAX_ADDR)? RMF_LOW_ADDRESS_SIZE : RMF_HIGH_ADDRESS_SIZE;
      msgSize = headerSize + dataSize;
      msgBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgSize);
      if (msgBuf != 0)
      {
         int32_t result = rmf_packHeader(msgBuf, msgSize, address, false);
         if (result == headerSize)
         {

            apx_error_t rc = readFunc(arg, file, offset, &msgBuf[headerSize], dataSize);
            if (rc == APX_NO_ERROR)
            {
               result = self->transmitHandler.send(self->transmitHandler.arg, 0, msgSize);
               if (result != msgSize)
               {
                  return APX_TRANSMIT_ERROR;
               }
            }
            else
            {
               return rc;
            }
         }
      }
      else
      {
         return APX_MISSING_BUFFER_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static void workerThread_sendAcknowledge(apx_fileManagerWorker_t *self)
{
   const int32_t msgSize = RMF_CMD_ADDRESS_LEN+RMF_CMD_ACK_LEN;
   uint8_t *msgBuf;
   assert(self->transmitHandler.getSendBuffer != 0);
   assert(self->transmitHandler.send != 0);
   msgBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgSize);

   if (msgBuf != 0)
   {
      int32_t result = rmf_packHeader(msgBuf, msgSize, RMF_CMD_START_ADDR, false);
      if (result == RMF_CMD_ADDRESS_LEN)
      {
         result = rmf_serialize_acknowledge(msgBuf+RMF_CMD_ADDRESS_LEN, RMF_CMD_ACK_LEN);
         if (result == RMF_CMD_ACK_LEN)
         {
            self->transmitHandler.send(self->transmitHandler.arg, 0, msgSize);
         }
      }
   }
}

static apx_error_t apx_fileManagerWorker_processRingBufErrorCode(adt_buf_err_t errorCode)
{
   if (errorCode == BUF_E_OVERFLOW)
   {
      return APX_BUFFER_FULL_ERROR;
   }
   return APX_MEM_ERROR;
}
