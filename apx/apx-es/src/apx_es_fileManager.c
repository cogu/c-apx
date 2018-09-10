//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifndef APX_DEBUG_ENABLE
#define APX_DEBUG_ENABLE 0
#endif
#include <string.h>
#include <assert.h>
#if APX_DEBUG_ENABLE || defined(UNIT_TEST)
#include <stdio.h>
#endif
#include <limits.h>
#include "apx_es_fileManager.h"
#include "apx_msg.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int32_t apx_es_fileManager_runEventLoop(apx_es_fileManager_t *self);
static int32_t apx_es_fileManager_onInternalMessage(apx_es_fileManager_t *self, apx_msg_t *msg);
static void apx_es_fileManager_parseCmdMsg(apx_es_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen);
static void apx_es_fileManager_parseDataMsg(apx_es_fileManager_t *self, uint32_t address, const uint8_t *dataBuf, int32_t dataLen, bool more_bit);
static void apx_es_fileManager_processRemoteFileInfo(apx_es_fileManager_t *self, const rmf_fileInfo_t *fileInfo);
static void apx_es_fileManager_processOpenFile(apx_es_fileManager_t *self, const rmf_cmdOpenFile_t *cmdOpenFile);
static int32_t apx_es_processPendingWrite(apx_es_fileManager_t *self);
static int32_t apx_es_processPendingCmd(apx_es_fileManager_t *self);
#ifndef UNIT_TEST
DYN_STATIC int8_t apx_es_fileManager_removeRequestedAt(apx_es_fileManager_t *self, int32_t removeIndex);
#endif

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int8_t apx_es_fileManager_create(apx_es_fileManager_t *self, uint8_t *messageQueueBuf, uint16_t messageQueueLen, uint8_t *receiveBuf, uint16_t receiveBufLen)
{
   if ( (self != 0) && (messageQueueBuf != 0) && (receiveBuf != 0))
   {
      rbfs_create(&self->messageQueue, messageQueueBuf, messageQueueLen, (uint8_t) sizeof(apx_msg_t));
//      rbfd_create(&self->messageData, messageDataBuf, messageDataLen);
      self->receiveBuf=receiveBuf;
      self->receiveBufLen=(uint32_t) receiveBufLen;
      self->receiveBufOffset=0;
      self->receiveStartAddress = RMF_INVALID_ADDRESS;
      apx_es_fileMap_create(&self->localFileMap);
      apx_es_fileMap_create(&self->remoteFileMap);
      apx_es_fileManager_setTransmitHandler(self, 0);
      self->pendingWrite = false;
      self->pendingCmd = false;
      self->dropMessage = false;
      self->curFile=0;
      memset(&self->fileWriteInfo, 0, sizeof(apx_es_file_write_t));
      memset(&self->cmdInfo, 0, sizeof(apx_es_command_t));
      self->numRequestedFiles = 0;
      return 0;
   }
   return -1;
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
#ifndef APX_EMBEDDED
      SPINLOCK_ENTER(self->lock);
#endif
      if (handler == 0)
      {
         memset(&self->transmitHandler, 0, sizeof(apx_transmitHandler_t));
      }
      else
      {
         memcpy(&self->transmitHandler, handler, sizeof(apx_transmitHandler_t));
      }
#ifndef APX_EMBEDDED
      SPINLOCK_LEAVE(self->lock);
#endif
   }
}

void apx_es_fileManager_onConnected(apx_es_fileManager_t *self)
{
   if (self != 0)
   {
      int32_t end;
      int32_t i;
      end = apx_es_fileMap_length(&self->localFileMap);
      for(i=0;i<end;i++)
      {
         apx_file_t *file = apx_es_fileMap_get(&self->localFileMap,i);
         if (file != 0)
         {
            apx_msg_t msg = {RMF_MSG_FILEINFO,0,0,0};
            msg.msgData3 = file;
            rbfs_insert(&self->messageQueue,(uint8_t*) &msg);
         }
      }
   }
}

void apx_es_fileManager_onDisconnected(apx_es_fileManager_t *self)
{
   if (self != 0)
   {
      apx_es_fileMap_clear(&self->remoteFileMap);
   }
}

/**
 * triggered by lower layer when a message has been received
 */
void apx_es_fileManager_onMsgReceived(apx_es_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen)
{
   rmf_msg_t msg;
   int32_t result = rmf_unpackMsg(msgBuf, msgLen, &msg);
   if (result > 0)
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
         //discard
      }
   }
   else if (result < 0)
   {
#if APX_DEBUG_ENABLE
      fprintf(stderr, "rmf_unpackMsg failed with %d\n", result);
#endif
   }
   else
   {
      //MISRA
   }
}

void apx_es_fileManager_onFileUpdate(apx_es_fileManager_t *self, apx_file_t *file, uint32_t offset, uint32_t length)
{
   if ( (self != 0) && (file != 0) && (length > 0) )
   {
      apx_msg_t msg = {RMF_MSG_WRITE_NOTIFY, 0, 0, 0}; //{msgType,msgData1,msgData2,msgData3}
      msg.msgData1=offset;
      msg.msgData2=length;
      msg.msgData3=(void*)file;
      rbfs_insert(&self->messageQueue,(const uint8_t*) &msg);
   }
}

/**
 * runs the message handler until either the sendbuffer is full or the are no more messages to process
 */
void apx_es_fileManager_run(apx_es_fileManager_t *self)
{
   int32_t result;
   if (self->pendingWrite == true)
   {
      result = apx_es_processPendingWrite(self);
      if (result < 0)
      {
#if APX_DEBUG_ENABLE
         fprintf(stderr, "apx_es_processPendingWrite returned %d",result);
#endif
      }
   }
   if (self->pendingCmd == true)
   {
      result = apx_es_processPendingCmd(self);
      if (result < 0)
      {
#if APX_DEBUG_ENABLE
         fprintf(stderr, "apx_es_processPendingCmd returned %d",result);
#endif
      }
   }
   if ( (self->pendingWrite == false) && (self->pendingCmd == false) )
   {
      while(true)
      {
         result = apx_es_fileManager_runEventLoop(self);
         if (result < 0)
         {
#if APX_DEBUG_ENABLE
            fprintf(stderr, "apx_es_fileManager_runEventLoop returned %d",result);
#endif
         }
         else if ( (result == 0) || (self->pendingWrite == true) || (self->pendingCmd == true) )
         {
            break;
         }
         else
         {
            //MISRA
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/**
 * runs internal event loop
 * returns 0 when no more messages can be processed, -1 on error and 1 on success
 */
static int32_t apx_es_fileManager_runEventLoop(apx_es_fileManager_t *self)
{
   apx_msg_t msg;
   int32_t retval = 0;
   int8_t rc = rbfs_remove(&self->messageQueue, (uint8_t*) &msg);
   if (rc == E_BUF_OK)
   {
      retval = apx_es_fileManager_onInternalMessage(self, &msg);
   }
   return retval;
}

/**
 * this is the main event loop of apx_es_fileManager
 */
static int32_t apx_es_fileManager_onInternalMessage(apx_es_fileManager_t *self, apx_msg_t *msg)
{
   if ( (self != 0) && (msg != 0) )
   {
      int32_t retval=0;
      uint32_t sendAvail = 0;
      if ( (self->transmitHandler.getSendAvail != 0) && (self->transmitHandler.getSendBuffer != 0) )
      {
         sendAvail = (uint32_t )self->transmitHandler.getSendAvail(self->transmitHandler.arg);
      }
      switch(msg->msgType)
      {
      case RMF_MSG_CONNECT:
         break;
      case RMF_MSG_FILEINFO:
         {
            uint32_t headerLen = RMF_HIGH_ADDRESS_SIZE;
            uint32_t dataLen;
            uint32_t msgLen;
            apx_file_t *file = (apx_file_t*) msg->msgData3;
            int32_t nameLen=strlen(file->fileInfo.name);
            dataLen=CMD_FILE_INFO_BASE_SIZE+nameLen+1; //+1 for null terminator
            msgLen=headerLen+dataLen;
            if (msgLen<=sendAvail)
            {
               uint8_t* msgBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgLen);
               if (msgBuf != 0)
               {
                  (void) rmf_serialize_cmdFileInfo(&msgBuf[headerLen],dataLen,&file->fileInfo);
                  (void) rmf_packHeaderBeforeData(&msgBuf[headerLen],headerLen,RMF_CMD_START_ADDR,false);
                  self->transmitHandler.send(self->transmitHandler.arg,0,msgLen);
                  retval = msgLen;
               }
               else
               {
                  retval = -1;
               }
            }
            else if (msgLen <= RMF_MAX_CMD_BUF_SIZE)
            {
               self->pendingCmd = true;
               self->cmdInfo.length = msgLen;
               (void) rmf_serialize_cmdFileInfo(&self->cmdInfo.buf[headerLen],dataLen,&file->fileInfo);
               (void) rmf_packHeaderBeforeData(&self->cmdInfo.buf[headerLen],headerLen,RMF_CMD_START_ADDR,false);
            }
            else
            {
               //MISRA
            }
         }
         break;
      case RMF_MSG_FILE_OPEN: //sends file open command
         {
            uint32_t headerLen = RMF_HIGH_ADDRESS_SIZE;
            uint32_t dataLen;
            uint32_t msgLen;
            rmf_cmdOpenFile_t cmdOpenFile;
            cmdOpenFile.address = msg->msgData1;
            dataLen=(uint32_t) RMF_FILE_OPEN_CMD_LEN;
            msgLen=headerLen+dataLen;
            if (msgLen<=sendAvail)
            {
               uint8_t* msgBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgLen);
               if (msgBuf != 0)
               {
                  int32_t result;
                  result=rmf_serialize_cmdOpenFile(&msgBuf[headerLen], dataLen, &cmdOpenFile);
                  assert(result == (int32_t) dataLen);
                  result=rmf_packHeaderBeforeData(&msgBuf[headerLen],headerLen,RMF_CMD_START_ADDR,false);
                  assert(result == (int32_t) headerLen);
                  self->transmitHandler.send(self->transmitHandler.arg,0,msgLen);
                  retval = msgLen;
               }
               else
               {
                  retval = -1;
               }
            }
            else if (msgLen <= RMF_MAX_CMD_BUF_SIZE)
            {
               self->pendingCmd = true;
               self->cmdInfo.length = msgLen;
               (void) rmf_serialize_cmdOpenFile(&self->cmdInfo.buf[headerLen], dataLen, &cmdOpenFile);
               (void) rmf_packHeaderBeforeData(&self->cmdInfo.buf[headerLen],headerLen,RMF_CMD_START_ADDR,false);
            }
            else
            {
               //MISRA
            }
         }
         break;
      case RMF_MSG_WRITE_NOTIFY:
         if ( (self->transmitHandler.getSendBuffer != 0) && (self->transmitHandler.send != 0) )
         {
            uint32_t address;
            uint32_t msgLen;
            uint32_t headerLen;
            uint32_t dataLen;
            uint32_t offset;
            uint8_t *sendBuffer;
            apx_file_t *file;
            offset = msg->msgData1;
            dataLen = msg->msgData2;
            file = (apx_file_t*) msg->msgData3;
            //calculate address
            address = file->fileInfo.address+offset;
            headerLen = (address < RMF_DATA_HIGH_MIN_ADDR)? RMF_LOW_ADDRESS_SIZE : RMF_HIGH_ADDRESS_SIZE;
            msgLen = headerLen+dataLen;
            sendBuffer = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgLen);
            if (sendBuffer != 0)
            {
               int32_t result = rmf_packHeader(&sendBuffer[0], msgLen, address, false);
               if (result > 0)
               {
                  apx_file_read(file,&sendBuffer[headerLen],offset,dataLen);
                  self->transmitHandler.send(self->transmitHandler.arg,0,msgLen);
               }
               assert((uint32_t) result == headerLen);

            }
            else
            {
               //queue as pending write
               self->pendingWrite = true;
               self->fileWriteInfo.localFile=file;
               self->fileWriteInfo.readOffset=offset;
               self->fileWriteInfo.writeAddress=address;
               self->fileWriteInfo.remain=dataLen;
            }
         }
         break;
      case RMF_MSG_FILE_SEND: //sends file to remote side
         {
            apx_file_t *file = (apx_file_t*) msg->msgData3;

            if (sendAvail >= (int32_t) RMF_MIN_MSG_LEN)
            {
               uint32_t dataLen;
               uint32_t msgLen;
               bool more_bit=false;
               uint8_t* msgBuf;
               uint32_t headerLen = (file->fileInfo.address < RMF_DATA_LOW_MAX_ADDR)? RMF_LOW_ADDRESS_SIZE : RMF_HIGH_ADDRESS_SIZE;
               if (sendAvail < file->fileInfo.length)
               {
                  dataLen=sendAvail-headerLen;
                  more_bit=true;
               }
               else
               {
                  dataLen = file->fileInfo.length;
               }
               msgLen = headerLen+dataLen;
               msgBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgLen);
               if (msgBuf != 0)
               {
                  int8_t result;
                  (void) rmf_packHeaderBeforeData(&msgBuf[headerLen], headerLen, file->fileInfo.address, more_bit);
                  result = apx_file_read(file, &msgBuf[headerLen], 0, dataLen);
                  if (result == 0)
                  {
                     self->transmitHandler.send(self->transmitHandler.arg,0,msgLen);
                     retval = msgLen;
                     if (dataLen < file->fileInfo.length)
                     {
                        self->pendingWrite = true;
                        self->fileWriteInfo.localFile=file;
                        self->fileWriteInfo.readOffset=dataLen;
                        self->fileWriteInfo.writeAddress=file->fileInfo.address+dataLen;
                        self->fileWriteInfo.remain=file->fileInfo.length - dataLen;
                     }
                  }
                  else
                  {
                     retval = -1;
                  }
               }
               else
               {
                  retval = -1;
               }
            }
            else
            {
               //send file later
               self->pendingWrite = true;
               self->fileWriteInfo.localFile=file;
               self->fileWriteInfo.readOffset=0;
               self->fileWriteInfo.writeAddress=file->fileInfo.address;
               self->fileWriteInfo.remain=file->fileInfo.length;
            }
         }
         break;
      case RMF_MSG_FILE_WRITE:
         {
         }
         break;
      default:
         break;
      }
      return retval;
   }
   return -1; //invalid arguments
}

static void apx_es_fileManager_parseCmdMsg(apx_es_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen)
{
   if (self != 0)
   {
      uint32_t cmdType;
      int32_t result;
      result = rmf_deserialize_cmdType(msgBuf, msgLen, &cmdType);
      //printf("apx_fileManager_parseCmdMsg(%d)\n", msgLen);
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
   if ( (self != 0) && (dataBuf != 0) && (dataLen>=0) )
   {
      uint32_t offset;
      if ( (self->receiveStartAddress == RMF_INVALID_ADDRESS) )
      {
         //new reception
         apx_file_t *remoteFile = apx_es_fileMap_findByAddress(&self->remoteFileMap, address);
         if ( (remoteFile != 0) && (remoteFile->isOpen == true) )
         {
            offset=address-remoteFile->fileInfo.address;
            if (more_bit == false)
            {
               apx_file_write(remoteFile, dataBuf, offset, dataLen);
            }
            else if(((uint32_t)dataLen) <= self->receiveBufLen)
            {
               //start multi-message recepetion
               self->curFile = remoteFile;
               self->receiveStartAddress = address;
               self->receiveBufOffset=0;
               memcpy(&self->receiveBuf[self->receiveBufOffset], dataBuf, dataLen);
               self->receiveBufOffset+=dataLen;
            }
            else
            {
               //drop message
#if APX_DEBUG_ENABLE
               fprintf(stderr, "[APX_ES_FILEMANAGER] message too long (%d bytes), message dropped\n",dataLen);
#endif
            }
         }
      }
      else
      {
         //continuation reception
         offset = address-self->curFile->fileInfo.address;
         if (offset != self->receiveBufOffset)
         {
            self->dropMessage = true; //drop message since offsets don't match
#if APX_DEBUG_ENABLE
            fprintf(stderr, "[APX_ES_FILEMANAGER] invalid offset (%u), message dropped\n",offset);
#endif
         }
         else if(self->receiveBufOffset+dataLen<=self->receiveBufLen)
         {
            //copy data
            memcpy(&self->receiveBuf[self->receiveBufOffset], dataBuf, dataLen);
            self->receiveBufOffset+=dataLen;
         }
         else
         {
            self->dropMessage = true; //message too long
#if APX_DEBUG_ENABLE
            fprintf(stderr, "[APX_ES_FILEMANAGER] message too long (%d bytes), message dropped\n",dataLen);
#endif
         }

         if (more_bit==false)
         {
            if (self->dropMessage == false)
            {
               //send message to upper layer
               uint32_t startOffset=self->receiveStartAddress-self->curFile->fileInfo.address;
               apx_file_write(self->curFile, &self->receiveBuf[0], startOffset, self->receiveBufOffset);
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
static void apx_es_fileManager_processRemoteFileInfo(apx_es_fileManager_t *self, const rmf_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0) )
   {
      int32_t i;
      apx_file_t *file=0;
      int32_t removeIndex=-1;
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
                  fprintf(stderr, "[APX_ES_FILEMANAGER] unexpected file size off file %s. Expected %d, got %d\n",file->fileInfo.name,
                        file->fileInfo.length, fileInfo->length);
#endif
               }
            }
         }
      }
      if (removeIndex>=0)
      {
         apx_msg_t msg = {RMF_MSG_FILE_OPEN, 0, 0, 0};
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
         //add file to remoteFileMap
         apx_es_fileMap_insert(&self->remoteFileMap, file);
         apx_file_open(file);
         msg.msgData1 = file->fileInfo.address;
         rbfs_insert(&self->messageQueue, (const uint8_t*) &msg);
      }
   }
}

static void apx_es_fileManager_processOpenFile(apx_es_fileManager_t *self, const rmf_cmdOpenFile_t *cmdOpenFile)
{
   if ( (self != 0) && (cmdOpenFile != 0) )
   {
      apx_file_t *localFile = apx_es_fileMap_findByAddress(&self->localFileMap, cmdOpenFile->address);
      if (localFile != 0)
      {
         apx_msg_t msg = {RMF_MSG_FILE_SEND,0,0,0};
#if APX_DEBUG_ENABLE
         int32_t bytesToSend = localFile->fileInfo.length;
         printf("Opened %s, bytes to send: %d\n", localFile->fileInfo.name, bytesToSend);
#endif
         apx_file_open(localFile);         
         msg.msgData3 = localFile;
         rbfs_insert(&self->messageQueue,(uint8_t*) &msg);
      }
   }
}


static int32_t apx_es_processPendingWrite(apx_es_fileManager_t *self)
{
   if (self != 0)
   {
      int32_t retval=0;
      uint32_t sendAvail = 0;
      if ( (self->transmitHandler.getSendAvail != 0) && (self->transmitHandler.getSendBuffer != 0) )
      {
         sendAvail = (uint32_t )self->transmitHandler.getSendAvail(self->transmitHandler.arg);
      }
      if (sendAvail >= (int32_t) RMF_MIN_MSG_LEN)
      {
#if APX_DEBUG_ENABLE
         printf("apx_es_processPendingWrite, remain=%u, offset=%u, address=%08X\n",
               self->fileWriteInfo.remain, self->fileWriteInfo.readOffset, self->fileWriteInfo.writeAddress);
#endif
         uint32_t dataLen;
         uint32_t msgLen;
         bool more_bit=false;
         uint8_t* msgBuf;
         uint32_t headerLen = (self->fileWriteInfo.writeAddress < RMF_DATA_LOW_MAX_ADDR)? RMF_LOW_ADDRESS_SIZE : RMF_HIGH_ADDRESS_SIZE;
         if (sendAvail < self->fileWriteInfo.remain)
         {
            dataLen=sendAvail-headerLen;
            more_bit=true;
         }
         else
         {
            dataLen = self->fileWriteInfo.remain;
         }
         msgLen = headerLen+dataLen;
         msgBuf = self->transmitHandler.getSendBuffer(self->transmitHandler.arg, msgLen);
         if (msgBuf != 0)
         {
            int8_t result;
            (void) rmf_packHeaderBeforeData(&msgBuf[headerLen], headerLen, self->fileWriteInfo.writeAddress, more_bit);
            result = apx_file_read(self->fileWriteInfo.localFile, &msgBuf[headerLen], self->fileWriteInfo.readOffset, dataLen);
            if (result == 0)
            {
               self->transmitHandler.send(self->transmitHandler.arg,0,msgLen);
               retval = msgLen;
               self->fileWriteInfo.remain-=dataLen;
               if (self->fileWriteInfo.remain==0)
               {
                  assert(more_bit == false);
                  self->pendingWrite = false;
                  memset(&self->fileWriteInfo, 0, sizeof(apx_es_file_write_t));
               }
               else
               {
                  self->fileWriteInfo.writeAddress+=dataLen;
                  self->fileWriteInfo.readOffset+=dataLen;
               }
            }
            else
            {
               retval = -1;
            }
         }
         else
         {
            retval = -1;
         }
      }
      return retval;
   }
   return -1;
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

/**
 * a command is waiting to be transmitted.
 */
static int32_t apx_es_processPendingCmd(apx_es_fileManager_t *self)
{
   (void) self;
#if APX_DEBUG_ENABLE
   printf("apx_es_processPendingCmd\n");
#endif
   return -1; //not implemented
}
