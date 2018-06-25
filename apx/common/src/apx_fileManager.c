//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "rmf.h"

#include <stdio.h>
#ifdef _MSC_VER
#include <process.h>
#endif
#include "apx_fileManager.h"
#include "apx_nodeManager.h"
#include "apx_logging.h"
#include "apx_event.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#undef SEMAPHORE_MAX_COUNT
#define SEMAPHORE_MAX_COUNT APX_CONTEXT_NUM_MESSAGES //redefine here on Win32 platforms
#endif

#ifndef APX_FILEMANAGER_DEBUG_ENABLE
#define APX_FILEMANAGER_DEBUG_ENABLE 0
#endif
//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_fileManager_startThread(apx_fileManager_t *self);
static THREAD_PROTO(threadTask,arg);


//handlers are run by internal thread
static void apx_fileManager_connectHandler(apx_fileManager_t *self);
static void apx_fileManager_fileWriteNotifyHandler(apx_fileManager_t *self, apx_file_t *file, apx_offset_t offset, apx_size_t len);
static void apx_fileManager_fileWriteCmdHandler(apx_fileManager_t *self, apx_file_t *file, const uint8_t *data, apx_offset_t offset, apx_size_t len);

//process functions are called from inside apx_fileManager_parseMessage)
static void apx_fileManager_parseCmdMsg(apx_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen);
static void apx_fileManager_parseDataMsg(apx_fileManager_t *self, uint32_t address, const uint8_t *msgBuf, int32_t msgLen, bool more_bit);
static void apx_fileManager_processRemoteFileInfo(apx_fileManager_t *self, const rmf_fileInfo_t *cmdFileInfo);
static void apx_fileManager_processOpenFile(apx_fileManager_t *self, const rmf_cmdOpenFile_t *cmdOpenFile);

//other internal functions
static void apx_fileManager_sendFileInfo(apx_fileManager_t *self, rmf_fileInfo_t *fileInfo);
static void apx_fileManager_sendAck(apx_fileManager_t *self);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int8_t apx_fileManager_create(apx_fileManager_t *self, uint8_t mode)
{
   if (self != 0 && ( (mode == APX_FILEMANAGER_CLIENT_MODE) || (mode == APX_FILEMANAGER_SERVER_MODE) ) )
   {
      size_t numItems = APX_CONTEXT_NUM_MESSAGES;
      size_t elemSize = RMF_MSG_SIZE;
      int8_t result = apx_allocator_create(&self->allocator, APX_CONTEXT_NUM_MESSAGES);

      if (result == 0)
      {
#ifdef _WIN32
         self->workerThread = INVALID_HANDLE_VALUE;
#else
         self->workerThread = 0;
#endif
         self->mode = mode;
         self->debugInfo = (void*) 0;
         self->workerThreadValid=false;
         SPINLOCK_INIT(self->lock);
         SEMAPHORE_CREATE(self->semaphore);
         self->ringbufferLen = numItems;
         self->ringbufferData = (uint8_t*) malloc(numItems*elemSize);
         if (self->ringbufferData == 0)
         {
            apx_allocator_destroy(&self->allocator);
            return -1;
         }
         rbfs_create(&self->ringbuffer, self->ringbufferData,(uint16_t) numItems,(uint8_t) elemSize);
         apx_fileMap_create(&self->localFileMap);
         apx_fileMap_create(&self->remoteFileMap);
         apx_fileManager_setTransmitHandler(self, 0);
         apx_allocator_start(&self->allocator);

         self->curFileStartAddress = 0;
         self->curFileEndAddress = 0;
         self->curFile = 0;
         self->nodeManager = (apx_nodeManager_t*) 0;
         self->isConnected = false;
         if (self->mode == APX_FILEMANAGER_SERVER_MODE)
         {
            self->event.server.recorder = apx_serverEventRecorder_new(self);
            self->event.server.player = apx_serverEventPlayer_new();
         }
         else
         {
            self->event.client.recorder = apx_clientEventRecorder_new();
            self->event.client.player = apx_clientEventPlayer_new();
         }
         return 0;
      }
   }
   errno = EINVAL;
   return -1;
}

void apx_fileManager_destroy(apx_fileManager_t *self)
{
   if (self != 0)
   {
      if (self->ringbufferData != 0)
      {
         free(self->ringbufferData);
      }
      if (self->mode == APX_FILEMANAGER_SERVER_MODE)
      {
         apx_serverEventContainer_t *event = &self->event.server;
         if (event->recorder != (apx_serverEventRecorder_t*) 0)
         {
            apx_serverEventRecorder_delete(event->recorder);
         }
         if (event->player != (apx_serverEventPlayer_t*) 0)
         {
            apx_serverEventPlayer_delete(event->player);
         }
      }
      else
      {
         apx_clientEventContainer_t *event = &self->event.client;
         if (event->recorder != (apx_clientEventRecorder_t*) 0)
         {
            apx_clientEventRecorder_delete(event->recorder);
         }
         if (event->player != (apx_clientEventPlayer_t*) 0)
         {
            apx_clientEventPlayer_delete(event->player);
         }
      }
      apx_allocator_stop(&self->allocator);
      apx_allocator_destroy(&self->allocator);
      SEMAPHORE_DESTROY(self->semaphore);
      SPINLOCK_DESTROY(self->lock);
      apx_fileMap_destroy(&self->localFileMap);
      apx_fileMap_destroy(&self->remoteFileMap);
   }
}

apx_fileManager_t *apx_fileManager_new(uint8_t  mode)
{
   apx_fileManager_t *self;
   if ( (mode != APX_FILEMANAGER_CLIENT_MODE) && (mode != APX_FILEMANAGER_SERVER_MODE) )
   {
      errno = EINVAL;
      return 0;
   }
   self = (apx_fileManager_t*) malloc(sizeof(apx_fileManager_t));
   if(self != 0)
   {
      int8_t result = apx_fileManager_create(self, mode);
      if (result < 0)
      {
         free(self);
         return 0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_fileManager_delete(apx_fileManager_t *self)
{
   if (self != 0)
   {
      apx_fileManager_destroy(self);
      free(self);
   }
}

void apx_fileManager_vdelete(void *arg)
{
   apx_fileManager_delete((apx_fileManager_t*) arg);
}

void apx_fileManager_start(apx_fileManager_t *self)
{
   if( (self != 0) && (self->workerThreadValid == false) )
   {
      apx_fileManager_startThread(self);
   }
}

void apx_fileManager_stop(apx_fileManager_t *self)
{
   if ( (self != 0) && (self->workerThreadValid == true) )
   {
#ifdef _MSC_VER
      DWORD result;
#endif
      apx_msg_t msg = {RMF_MSG_EXIT,0,0,0,0}; //{msgType, sender, msgData1, msgData2, msgData3}
      SPINLOCK_ENTER(self->lock);
      rbfs_insert(&self->ringbuffer,(const uint8_t*) &msg);
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
   }
}


/**
 * used to attach a node manager to allow fileManager to create remote nodes
 */
void apx_fileManager_setNodeManager(apx_fileManager_t *self, struct apx_nodeManager_tag *nodeManager)
{
   if(self != 0 )
   {
      self->nodeManager = nodeManager;
   }
}

void apx_fileManager_setTransmitHandler(apx_fileManager_t *self, apx_transmitHandler_t *handler)
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

void apx_fileManager_setDebugInfo(apx_fileManager_t *self, void *debugInfo)
{
   if (self != 0)
   {
      self->debugInfo = debugInfo;
   }
}


/**
 * returns number of bytes parsed from msgBuf. returns -1 on error or 0 if msgBuf is too short (wait for more data to arrive)
 *
 */
int32_t apx_fileManager_parseMessage(apx_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen)
{
   rmf_msg_t msg;
   int32_t result = rmf_unpackMsg(msgBuf, msgLen, &msg);
   if (result > 0)
   {
#if APX_FILEMANAGER_DEBUG_ENABLE
      APX_LOG_DEBUG("[APX_FILE_MANAGER] address: %08X", msg.address);
      APX_LOG_DEBUG("[APX_FILE_MANAGER] length: %d", msg.dataLen);
      APX_LOG_DEBUG("[APX_FILE_MANAGER] more_bit: %d", (int) msg.more_bit);
#endif
      if (msg.address == RMF_CMD_START_ADDR)
      {
         apx_fileManager_parseCmdMsg(self, msg.data, msg.dataLen);
      }
      else if (msg.address < RMF_CMD_START_ADDR)
      {
         apx_fileManager_parseDataMsg(self, msg.address, msg.data, msg.dataLen, msg.more_bit);
      }
      else
      {
         //discard
      }
   }
   else if (result < 0)
   {
      APX_LOG_ERROR("[APX_FILE_MANAGER] rmf_unpackMsg failed with %d", (int)result);      
   }
   else
   {
      //MISRA
   }
   return result;
}

/**
 * sends a file open request
 */
void apx_fileManager_sendFileOpen(apx_fileManager_t *self, uint32_t remoteAddress)
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

/**
 * searches among the remote files for a file with specific name
 */
apx_file_t *apx_fileManager_findRemoteFile(apx_fileManager_t *self, const char *name)
{
   if (self != 0)
   {
      return apx_fileMap_findByName(&self->remoteFileMap, name);
   }
   errno=EINVAL;
   return (apx_file_t*) 0;
}

/**
 * attaches a new local file to file manager, if transmit function is enabled, send a new file info to remote side
 * fileManager takes ownership of the pointer to localFile (will be deleted when fileManager is destroyed)
 */
void apx_fileManager_attachLocalDefinitionFile(apx_fileManager_t *self, apx_file_t *localFile)
{
   if ( (self != 0) && (localFile != 0) )
   {
      bool isConnected;
      SPINLOCK_ENTER(self->lock);
      isConnected = self->isConnected;
      apx_fileMap_autoInsertDefinitionFile(&self->localFileMap, localFile);
      SPINLOCK_LEAVE(self->lock);
      if ( (isConnected == true) && (self->transmitHandler.send != 0) )
      {
         apx_fileManager_sendFileInfo(self, &localFile->fileInfo);
      }
   }
}

/**
 * attaches a new local file to file manager, if transmit function is enabled, send a new file info to remote side
 * fileManager takes ownership of the pointer to localFile (will be deleted when fileManager is destroyed)
 */
void apx_fileManager_attachLocalPortDataFile(apx_fileManager_t *self, apx_file_t *localFile)
{
   if ( (self != 0) && (localFile != 0) )
   {
      bool isConnected;
      SPINLOCK_ENTER(self->lock);
      isConnected = self->isConnected;
      apx_fileMap_autoInsertPortDataFile(&self->localFileMap, localFile);
      SPINLOCK_LEAVE(self->lock);
      if ( (isConnected == true) && (self->transmitHandler.send != 0) )
      {
         apx_fileManager_sendFileInfo(self, &localFile->fileInfo);
      }

   }
}

/**
 * attaches a new local file to file manager, if transmit function is enabled, send a new file info to remote side
 * fileManager takes ownership of the pointer to localFile (will be deleted when fileManager is destroyed)
 */
void apx_fileManager_attachLocalDataFile(apx_fileManager_t *self, apx_file_t *localFile)
{
   if ( (self != 0) && (localFile != 0) )
   {
      bool isConnected;
      SPINLOCK_ENTER(self->lock);
      isConnected = self->isConnected;
      if (localFile->fileInfo.address == RMF_INVALID_ADDRESS)
      {
         apx_fileMap_autoInsertDataFile(&self->localFileMap, localFile);
      }
      else
      {
         apx_fileMap_insertFile(&self->localFileMap, localFile);
      }
      SPINLOCK_LEAVE(self->lock);
      if ( (isConnected == true) && (self->transmitHandler.send != 0) )
      {
         apx_fileManager_sendFileInfo(self, &localFile->fileInfo);
      }
   }
}

/**
 *
 * It is assumed that the detachedFiles array is empty when this function is called. This function will populate it.
 * \param self - the object
 * \param detachedFiles - an array with nodeData pointers to be detached and deleted by caller.
 *
 */
void apx_fileManager_detachFiles(apx_fileManager_t *self, adt_ary_t *detachedFiles)
{
   if ( (self != 0) && (detachedFiles != 0) )
   {
      adt_list_elem_t *iter;
      adt_list_iter_init(&self->remoteFileMap.fileList);
      do
      {
         iter = adt_list_iter_next(&self->remoteFileMap.fileList);
         if (iter != 0)
         {
            apx_file_t *file = (apx_file_t*)iter->pItem;
            if ( file != 0 )
            {
               apx_file_close(file);
               adt_ary_push(detachedFiles, file);
            }
         }
      }while(iter != 0);
      adt_list_iter_init(&self->localFileMap.fileList);
      do
      {
         iter = adt_list_iter_next(&self->localFileMap.fileList);
         if (iter != 0)
         {
            apx_file_t *file = (apx_file_t*)iter->pItem;
            if ( file != 0 )
            {
               apx_file_close(file);
               adt_ary_push(detachedFiles, file);
            }
         }
      }while(iter != 0);
   }
   apx_fileMap_clear_weak(&self->remoteFileMap);
   apx_fileMap_clear_weak(&self->localFileMap);
}

/**
 * returns string CLI or SRV depending on its mode (used for debug print messages)
 */
const char *apx_fileManager_modeString(apx_fileManager_t *self)
{
   if (self != 0)
   {
      switch(self->mode)
      {
         case APX_FILEMANAGER_CLIENT_MODE:
            return "CLI";
         case APX_FILEMANAGER_SERVER_MODE:
            return "SRV";
         default:
            break;
      }
   }
   return (const char*) 0;
}


void apx_fileManager_onConnected(apx_fileManager_t *self)
{
   if (self != 0)
   {
      apx_msg_t msg = {RMF_MSG_CONNECT,0,0,0,0}; //{msgType,  msgData1, msgData2, msgData3, msgData4}
      SPINLOCK_ENTER(self->lock);
      rbfs_insert(&self->ringbuffer,(const uint8_t*) &msg);
      SPINLOCK_LEAVE(self->lock);
      SEMAPHORE_POST(self->semaphore);
   }
}

void apx_fileManager_onDisconnected(apx_fileManager_t *self)
{
   if (self != 0)
   {
      apx_msg_t msg = {RMF_MSG_DISCONNECT,0,0,0,0}; //{msgType,  msgData1, msgData2, msgData3, msgData4}
      SPINLOCK_ENTER(self->lock);
      rbfs_insert(&self->ringbuffer,(const uint8_t*) &msg);
      SPINLOCK_LEAVE(self->lock);
      SEMAPHORE_POST(self->semaphore);
   }
}

void apx_fileManager_triggerFileUpdatedEvent(apx_fileManager_t *self, apx_file_t *file, uint32_t offset, uint32_t length)
{
   if (self !=0 )
   {
      apx_msg_t msg = {RMF_MSG_WRITE_NOTIFY,0,0,0,0}; //{msgType,  msgData1, msgData2, msgData3, msgData4}
      msg.msgData1 = (uint32_t) offset;
      msg.msgData2 = (uint32_t) length;
      msg.msgData3 = file; //sent from node in nodeDataPtr
      SPINLOCK_ENTER(self->lock);
      rbfs_insert(&self->ringbuffer,(const uint8_t*) &msg);
      SPINLOCK_LEAVE(self->lock);
      SEMAPHORE_POST(self->semaphore);
   }
}

void apx_fileManager_triggerFileWriteCmdEvent(apx_fileManager_t *self, apx_file_t *file, const uint8_t *data, apx_offset_t offset, apx_size_t length)
{
   if (self !=0 )
   {
      uint8_t *dataCopy;
      apx_msg_t msg = {RMF_MSG_FILE_WRITE,0,0,0,0}; //{msgType,  msgData1, msgData2, msgData3, msgData4}
      msg.msgData1 = (uint32_t) offset;
      msg.msgData2 = (uint32_t) length;
      msg.msgData3 = file; //sent from node in nodeDataPtr
      dataCopy = apx_allocator_alloc(&self->allocator,length);
      if (dataCopy == 0)
      {
         APX_LOG_ERROR("[APX_REMOTE_FILE] apx_allocator out of memory while attempting to allocate %d bytes", (int)length);
      }
      else
      {
         memcpy(dataCopy, data, length);
         msg.msgData4 = dataCopy;
         SPINLOCK_ENTER(self->lock);
         rbfs_insert(&self->ringbuffer,(const uint8_t*) &msg);
         SPINLOCK_LEAVE(self->lock);
         SEMAPHORE_POST(self->semaphore);
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
               apx_fileManager_fileWriteNotifyHandler(self, (apx_file_t*) msg.msgData3, (apx_offset_t) msg.msgData1, (apx_size_t) msg.msgData2);
               break;
            case RMF_MSG_FILE_WRITE:
               apx_fileManager_fileWriteCmdHandler(self, (apx_file_t*) msg.msgData3, (const uint8_t*) msg.msgData4, (apx_offset_t) msg.msgData1, (apx_size_t) msg.msgData2);
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
               apx_file_t *file = (apx_file_t*)iter->pItem;
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
static void apx_fileManager_fileWriteNotifyHandler(apx_fileManager_t *self, apx_file_t *file, apx_offset_t offset, apx_size_t len)
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
static void apx_fileManager_fileWriteCmdHandler(apx_fileManager_t *self, apx_file_t *file, const uint8_t *data, apx_offset_t offset, apx_size_t len)
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

static void apx_fileManager_sendFileInfo(apx_fileManager_t *self, rmf_fileInfo_t *fileInfo)
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



static void apx_fileManager_parseCmdMsg(apx_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen)
{
   if (self != 0)
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
                  rmf_fileInfo_t cmdFileInfo;
                  result = rmf_deserialize_cmdFileInfo(msgBuf, msgLen, &cmdFileInfo);
                  if (result > 0)
                  {
                     apx_fileManager_processRemoteFileInfo(self, &cmdFileInfo);
                  }
                  else if (result < 0)
                  {
                     APX_LOG_ERROR("[APX_FILE_MANAGER] rmf_deserialize_cmdFileInfo failed with %d", (int) result);
                  }
                  else
                  {
                     APX_LOG_ERROR("[APX_FILE_MANAGER] rmf_deserialize_cmdFileInfo returned 0");
                  }
               }
               break;
            case RMF_CMD_FILE_OPEN:
               {
                  rmf_cmdOpenFile_t cmdOpenFile;
                  result = rmf_deserialize_cmdOpenFile(msgBuf, msgLen, &cmdOpenFile);
                  if (result > 0)
                  {
                     apx_fileManager_processOpenFile(self, &cmdOpenFile);
                  }
                  else if (result < 0)
                  {
                     APX_LOG_ERROR("[APX_FILE_MANAGER] rmf_deserialize_cmdOpenFile failed with %d", (int) result);
                  }
                  else
                  {
                     APX_LOG_ERROR("[APX_FILE_MANAGER] rmf_deserialize_cmdOpenFile returned 0");
                  }
               }
               break;
            case RMF_CMD_HEARTBEAT_RQST:
               ///TODO: implement
               break;
            case RMF_CMD_HEARTBEAT_RSP:
               ///TODO: implement
               break;
            case RMF_CMD_PING_RQST:
               ///TODO: implement
               break;
            case RMF_CMD_PING_RSP:
               ///TODO: implement
               break;

            default:
               APX_LOG_ERROR("[APX_FILE_MANAGER] not implemented cmdType: %d\n", cmdType);
         }
      }
   }
}

/**
 * called when a data message has been received.
 */
static void apx_fileManager_parseDataMsg(apx_fileManager_t *self, uint32_t address, const uint8_t *dataBuf, int32_t dataLen, bool more_bit)
{
   if (self != 0)
   {
      if ( (self->curFile != 0) && ((address < self->curFileStartAddress) || (address >= self->curFileEndAddress)) )
      {
         //invalidate cached file if address is outside range
         self->curFile = 0;
      }

      if ( self->curFile == 0)
      {
         self->curFile = apx_fileMap_findByAddress(&self->remoteFileMap,address);
         if (self->curFile == 0)
         {
            APX_LOG_ERROR("[APX_FILE_MANAGER(%s)] invalid write attempted at address %08X, len=%d",apx_fileManager_modeString(self), (int) address, (int) dataLen);
         }
         else
         {
            self->curFileStartAddress = self->curFile->fileInfo.address;
            self->curFileEndAddress = self->curFileStartAddress+self->curFile->fileInfo.length;
         }
      }

      //this section is valid for both cases where the file was cached and where it was non-cached
      if (self->curFile != 0)
      {
         apx_file_t *remoteFile = self->curFile;
         assert(address >= self->curFileStartAddress);
         if (address+dataLen > self->curFileEndAddress)
         {
            APX_LOG_ERROR("[APX_FILE_MANAGER(%s)] write outside file bounds attempted at address 0x%08X", apx_fileManager_modeString(self), (int) address);
         }
         else
         {
            //All checks out OK, continue with data copy
            int8_t result;
            uint32_t offset = address - remoteFile->fileInfo.address;
            if (remoteFile->nodeData != 0)
            {
               switch(remoteFile->fileType)
               {
                  case APX_DEFINITION_FILE:
                     result = apx_nodeData_writeDefinitionData(remoteFile->nodeData, dataBuf, offset, dataLen);
                     if (result != 0)
                     {
                        APX_LOG_ERROR("[APX_FILE_MANAGER] apx_nodeData_writeDefinitionData failed with %d", (int) result);
                     }
                     break;
                  case APX_INDATA_FILE:
                     result = apx_nodeData_writeInPortData(remoteFile->nodeData, dataBuf, offset, dataLen);
                     if (result != 0)
                     {
                        APX_LOG_ERROR("[APX_FILE_MANAGER] apx_nodeData_writeInPortData failed with %d", (int) result);
                     }
                     else
                     {
                        apx_nodeData_triggerInPortDataWritten(remoteFile->nodeData, offset, dataLen);
                     }
                     break;
                  case APX_OUTDATA_FILE:
                     result = apx_nodeData_writeOutPortData(remoteFile->nodeData, dataBuf, offset, dataLen);
                     if (result != 0)
                     {
                        APX_LOG_ERROR("[APX_FILE_MANAGER] apx_nodeData_writeOutPortData failed with %d\n", (int) result);
                     }
                     break;
                  default:
                     result=-1;
                     break;
               }
               if (result == 0)
               {
                  if ((more_bit == false) && (self->nodeManager != 0) )
                  {
                     apx_nodeManager_remoteFileWritten(self->nodeManager, self, remoteFile, offset, dataLen);
                  }
               }
            }
            else
            {
               APX_LOG_ERROR("[APX_FILE_MANAGER] write to file %s detected but no nodeData has been assigned to it", self->curFile->fileInfo.name);
            }
         }
      }
   }
}

/**
 * called when we see a new rmf_fileInfo_t in the input/parse stream
 */
static void apx_fileManager_processRemoteFileInfo(apx_fileManager_t *self, const rmf_fileInfo_t *cmdFileInfo)
{
   printf("apx_fileManager_processRemoteFileInfo\n");
   if ( (self != 0) && (cmdFileInfo != 0) )
   {
      apx_file_t *remoteFile = apx_file_new(APX_UNKNOWN_FILE, cmdFileInfo);
      if (remoteFile != 0)
      {
         SPINLOCK_ENTER(self->lock);
         apx_fileMap_insertFile(&self->remoteFileMap, remoteFile);
         SPINLOCK_LEAVE(self->lock);
         if(strcmp(remoteFile->fileInfo.name, APX_EVENT_CLI_FILE_NAME)==0)
         {
            printf("client event file seen\n");
         }
         else
         {
            if (self->nodeManager != 0)
            {
               apx_nodeManager_remoteFileAdded(self->nodeManager, self, remoteFile);
            }
         }
      }
      else
      {
         APX_LOG_ERROR("[APX_FILE_MANAGER] apx_file_newRemoteFile returned NULL");
      }
   }
}

static void apx_fileManager_processOpenFile(apx_fileManager_t *self, const rmf_cmdOpenFile_t *cmdOpenFile)
{
   if ( (self != 0) && (cmdOpenFile != 0) )
   {
      apx_file_t *localFile;
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

/*
* send an acknowledge message
*/
static void apx_fileManager_sendAck(apx_fileManager_t *self)
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
