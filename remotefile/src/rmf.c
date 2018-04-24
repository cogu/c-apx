//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <errno.h>
#ifndef APX_EMBEDDED
#include <malloc.h>
#include <assert.h>
#else
#define assert(x)
#endif
#include "rmf.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define HIGH_BIT_MASK 0x80
#define MORE_BIT_MASK 0x40

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * returns number of bytes encoded in buf
 * on error it returns 0 when bufLen is too small or -1 if one or more arguments are invalid
 */
#if 0 //DEPCRECATED
int32_t rmf_packMsg(uint8_t *buf, int32_t bufLen, uint32_t address, uint8_t *data, int32_t dataLen, int32_t *consumed)
{
   if ( (buf != 0) && (bufLen > 0) &&  (data != 0) && (dataLen > 0) && (consumed != 0) )
   {
      bool high_bit;
      int32_t addressLen;
      address &= RMF_CMD_END_ADDR;
      if (address <= RMF_DATA_LOW_MAX_ADDR)
      {
         high_bit = false;
         addressLen=RMF_LOW_ADDRESS_SIZE;
      }
      else
      {
         high_bit = true;
         addressLen=RMF_HIGH_ADDRESS_SIZE;
      }
      if (addressLen+1 > bufLen)
      {
         //buffer is too small to fit any data in it. return 0 to let the user know to retry later with a bigger buffer
         return 0;
      }
      packBE(buf,address, (uint8_t) addressLen);
      if (high_bit == true)
      {
         buf[0]|= (uint8_t) HIGH_BIT_MASK;
      }
      if(addressLen+dataLen > bufLen)
      {
         //all data will not fit into the message, activate more_bit and send whatever fits into the buffer
         int32_t copyLen = bufLen-addressLen;
         buf[0]|= (uint8_t) MORE_BIT_MASK;
         memcpy(&buf[addressLen],data,copyLen);
         *consumed=copyLen;
         return bufLen;
      }
      else
      {
         memcpy(&buf[addressLen],data,dataLen);
         *consumed=dataLen;
         return (addressLen+dataLen);
      }
   }
   return -1;
}
#endif

/**
 * encodes the address header into dataBuf
 * returns number of bytes written into the header, 0 when buffer is too small and -1 if any of the arguments are incorrect
 */
int32_t rmf_packHeader(uint8_t *dataBuf, int32_t bufLen, uint32_t address, bool more_bit)
{
   if ( (dataBuf != 0) && (address <= RMF_CMD_END_ADDR) )
   {
      int32_t headerLen = (address < RMF_DATA_HIGH_MIN_ADDR)? RMF_LOW_ADDRESS_SIZE : RMF_HIGH_ADDRESS_SIZE;
      if (headerLen > bufLen)
      {
         return 0;
      }
      packBE(dataBuf, address, (uint8_t) headerLen);
      if (headerLen == RMF_HIGH_ADDRESS_SIZE)
      {
         dataBuf[0]|= (uint8_t) HIGH_BIT_MASK;
      }
      if (more_bit == true)
      {
         dataBuf[0]|= (uint8_t) MORE_BIT_MASK;
      }
      return headerLen;
   }
   return -1;
}
/**
 * encodes the address header into the bytes just before dataBuf.
 * It's imperative that the user have allocated at least 4 bytes of free memory _before_ the dataBuf pointer.
 * returns number of bytes written into the header
 */
int32_t rmf_packHeaderBeforeData(uint8_t *dataBuf, int32_t bufLen, uint32_t address, bool more_bit)
{
   bool high_bit;
   int32_t headerLen;
   address &= RMF_CMD_END_ADDR;
   if (address <= RMF_DATA_LOW_MAX_ADDR)
   {
      high_bit = false;
      headerLen=2;
   }
   else
   {
      high_bit = true;
      headerLen=4;
   }
   if (headerLen > bufLen)
   {
      //buffer is too small to fit header
      return 0;
   }
   packBE(dataBuf-headerLen, address, (uint8_t) headerLen);
   if (high_bit == true)
   {
      dataBuf[-headerLen]|= (uint8_t) HIGH_BIT_MASK;
   }
   if (more_bit == true)
   {
      dataBuf[-headerLen]|= (uint8_t) MORE_BIT_MASK;
   }
   return headerLen;
}

/**
 * returns number of bytes read from buf
 * on error it returns 0 when data was parsed or -1 if one or more arguments are invalid
 */

int32_t rmf_unpackMsg(const uint8_t *buf, int32_t bufLen, rmf_msg_t *msg)
{
   if ( (buf != 0) && (bufLen>0) && (msg != 0) )
   {
      int32_t addressLen;
      uint8_t c = (uint8_t) *buf;
      bool high_bit = (c & HIGH_BIT_MASK) ? true : false;
      msg->more_bit = (c & MORE_BIT_MASK) ? true : false;
      addressLen = high_bit ? 4 : 2;
      if (bufLen < addressLen)
      {
         return 0; //no bytes consumed, retry later
      }
      msg->address = unpackBE(buf, addressLen);
      msg->address &= RMF_CMD_END_ADDR; //the highest address also happens to be the bit-mask
      msg->dataLen=bufLen-addressLen;
      msg->data = &buf[addressLen];
      return bufLen;
   }
   return -1;
}


/**
 * On failure: returns 0 if buffer is too small, -1 on any other error
 * On success: returns number of bytes written in buffer
 */
int32_t rmf_serialize_cmdFileInfo(uint8_t *buf, int32_t bufLen, rmf_fileInfo_t *fileInfo)
{
   if ( (buf != 0) && (fileInfo !=0) && (bufLen>0) )
   {
      uint8_t *p;
      uint32_t totalLen;
      uint32_t baseSize = (uint32_t) CMD_FILE_INFO_BASE_SIZE;
      uint32_t nameLen = (uint32_t) strlen(fileInfo->name);

      totalLen = baseSize+nameLen+1; //add 1 for null terminator
      if ( (uint32_t) bufLen < totalLen )
      {
         return 0; //buffer too small
      }
      p=buf;
      packLE(p, RMF_CMD_FILE_INFO, (uint8_t) sizeof(uint32_t)); p+=sizeof(uint32_t);
      packLE(p, fileInfo->address, (uint8_t) sizeof(uint32_t)); p+=sizeof(uint32_t);
      packLE(p, fileInfo->length, (uint8_t) sizeof(uint32_t)); p+=sizeof(uint32_t);
      packLE(p, fileInfo->fileType, (uint8_t) sizeof(uint16_t)); p+=sizeof(uint16_t);
      packLE(p, fileInfo->digestType, (uint8_t) sizeof(uint16_t)); p+=sizeof(uint16_t);
      memcpy(p, &fileInfo->digestData[0],RMF_DIGEST_SIZE); p+=RMF_DIGEST_SIZE;
      strcpy( (char*)p, fileInfo->name);
      return totalLen;
   }
   return -1;
}

/**
 * On failure: returns 0 if buffer is too small, -1 on any other error
 * On success: returns number of bytes parsed from buffer
 */
int32_t rmf_deserialize_cmdFileInfo(const uint8_t *buf, int32_t bufLen, rmf_fileInfo_t *fileInfo)
{
   if ( (buf != 0) && (fileInfo !=0) )
   {
      const uint8_t *pNext;
      const uint8_t *pEnd;
      const uint8_t *pMark;
      char *pStrNext;
      char *pStrEnd;
      uint32_t totalLen;
      uint32_t baseSize = (uint32_t) CMD_FILE_INFO_BASE_SIZE;
      uint32_t cmdType;
      pEnd = buf+bufLen;
      pStrNext = &fileInfo->name[0];
      pStrEnd = pStrNext+RMF_MAX_FILE_NAME; //it is intentional that we do not include the null terminator in this length

      if ((uint32_t) bufLen < baseSize )
      {
         return 0; //buffer too small
      }
      pNext=buf;
      cmdType = unpackLE(pNext, (uint8_t) sizeof(uint32_t)); pNext+=sizeof(uint32_t);
      if (cmdType != RMF_CMD_FILE_INFO)
      {
         return -1; //invalid header start
      }
      fileInfo->address = unpackLE(pNext, (uint8_t) sizeof(uint32_t)); pNext+=sizeof(uint32_t);
      fileInfo->length = unpackLE(pNext,  (uint8_t) sizeof(uint32_t)); pNext+=sizeof(uint32_t);
      fileInfo->fileType = (uint16_t) unpackLE(pNext,  (uint8_t) sizeof(uint16_t)); pNext+=sizeof(uint16_t);
      fileInfo->digestType = unpackLE(pNext, (uint8_t) sizeof(uint16_t)); pNext+=sizeof(uint16_t);
      memcpy(&fileInfo->digestData[0], pNext ,RMF_DIGEST_SIZE); pNext+=RMF_DIGEST_SIZE;

      pMark = pNext; //save pointer to where the string started
      while( (pNext < pEnd) && (pStrNext < pStrEnd) )
      {
         char c = (char) *pNext++;
         *pStrNext++ = c;
         if (c == '\0')
         {
            break;
         }
      }
      //since we did not include the length of the null terminator above when we calculated pStrEnd, pStrNext should still be within the bounds of cmdFileInfo->name array
      assert( (pStrNext>=&fileInfo->name[0]) && (pStrNext<=&fileInfo->name[RMF_MAX_FILE_NAME+1]) );
      *pStrNext=0; //this should be a safe operation
      totalLen = baseSize+ ((uint32_t)(pNext-pMark)); //add 1 for null terminator
      return totalLen; //add 1 for null terminator

   }
   return -1;
}

/**
 * On failure: returns 0 if buffer is too small, -1 on any other error
 * On success: returns number of bytes written to buffer
 */
int32_t rmf_serialize_cmdOpenFile(uint8_t *buf, int32_t bufLen, rmf_cmdOpenFile_t *cmdOpenFile)
{
   if ( (buf != 0) && (cmdOpenFile !=0) )
   {
      uint8_t *p = buf;
      uint32_t totalLen = sizeof(uint32_t)*2u;

      if ((uint32_t) bufLen < totalLen )
      {
         return 0; //buffer too small
      }
      packLE(p, RMF_CMD_FILE_OPEN, (uint8_t) sizeof(uint32_t));
      p+=sizeof(uint32_t);
      packLE(p, cmdOpenFile->address, (uint8_t) sizeof(uint32_t));
      return totalLen;
   }
   return -1;
}

/**
 * On failure: returns 0 if buffer is too small, -1 on any other error
 * On success: returns number of bytes parsed from buffer
 */
int32_t rmf_deserialize_cmdOpenFile(const uint8_t *buf, int32_t bufLen, rmf_cmdOpenFile_t *cmdOpenFile)
{
   if ( (buf != 0) && (cmdOpenFile !=0) )
   {
      const uint8_t *p = buf;
      uint32_t totalLen = sizeof(uint32_t)*2u;
      uint32_t cmdType;
      if ((uint32_t) bufLen < totalLen )
      {
         return 0; //buffer too small
      }
      cmdType = unpackLE(p, (uint8_t) sizeof(uint32_t));
      p+=sizeof(uint32_t);
      cmdOpenFile->address = unpackLE(p, (uint8_t) sizeof(uint32_t));
      if(cmdType != RMF_CMD_FILE_OPEN)
      {
         //this is not the right deserializer
         return -1;
      }
      return totalLen;
   }
   return -1;
}

/**
 * On failure: returns 0 if buffer is too small, -1 on any other error
 * On success: returns number of bytes written to buffer
 */
int32_t rmf_serialize_cmdCloseFile(uint8_t *buf, int32_t bufLen, rmf_cmdCloseFile_t *cmdCloseFile)
{
   if ( (buf != 0) && (cmdCloseFile !=0) )
   {
      uint8_t *p = buf;
      uint32_t totalLen = sizeof(uint32_t)*2u;

      if ((uint32_t) bufLen < totalLen )
      {
         return 0; //buffer too small
      }
      packLE(p, RMF_CMD_FILE_CLOSE, (uint8_t) sizeof(uint32_t));
      p+=sizeof(uint32_t);
      packLE(p, cmdCloseFile->address, (uint8_t) sizeof(uint32_t));
      return totalLen;
   }
   return -1;
}

/**
 * On failure: returns 0 if buffer is too small, -1 on any other error
 * On success: returns number of bytes parsed from buffer
 */
int32_t rmf_deserialize_cmdCloseFile(const uint8_t *buf, int32_t bufLen, rmf_cmdCloseFile_t *cmdCloseFile)
{
   if ( (buf != 0) && (cmdCloseFile !=0) )
   {
      const uint8_t *p;
      uint32_t totalLen = sizeof(uint32_t)*2u;
      uint32_t cmdType;
      if ((uint32_t)bufLen < totalLen )
      {
         return 0; //buffer too small
      }
      p=buf;
      cmdType = unpackLE(p, (uint8_t) sizeof(uint32_t));
      p+=sizeof(uint32_t);
      cmdCloseFile->address = unpackLE(p, (uint8_t) sizeof(uint32_t));
      if(cmdType != RMF_CMD_FILE_CLOSE)
      {
         //this is not the right deserializer
         return -1;
      }
      return totalLen;
   }
   return -1;
}

/**
 * parses cmdType from buf. returns number of bytes parsed or -1 on error. It also returns 0 when buffer is too short (try again later)
 */
int32_t rmf_deserialize_cmdType(const uint8_t *buf, int32_t bufLen, uint32_t *cmdType)
{
   int32_t unpackLen = sizeof(uint32_t);
   if ( (buf == 0) || (cmdType == 0))
   {
      errno=EINVAL;
      return -1;
   }
   if (bufLen < unpackLen)
   {
      return 0;
   }
   *cmdType = unpackLE(buf, (uint8_t) unpackLen);
   return unpackLen;
}


int8_t rmf_fileInfo_create(rmf_fileInfo_t *self, const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType)
{
   if ( (self != 0) && (name != 0) && ( (startAddress < RMF_DATA_HIGH_MAX_ADDR) || (startAddress == RMF_INVALID_ADDRESS) ) && (fileType <= RMF_FILE_TYPE_STREAM) )
   {
      size_t len = strlen(name);
      if (len<=RMF_MAX_FILE_NAME)
      {
         memcpy(self->name, name, len);
         self->name[len]=0;
      }
      self->address=startAddress;
      self->length=length;
      self->fileType = fileType;
      self->digestType = RMF_DIGEST_TYPE_NONE;
      memset(&self->digestData, 0, RMF_DIGEST_SIZE);
      return 0;
   }
   errno = EINVAL;
   return -1;
}
void rmf_fileInfo_destroy(rmf_fileInfo_t *self)
{
   //nothing to do
   (void) self;
}

#ifndef APX_EMBEDDED
rmf_fileInfo_t *rmf_fileInfo_new(const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType)
{
   rmf_fileInfo_t *self = (rmf_fileInfo_t*) malloc(sizeof(rmf_fileInfo_t));
   if(self != 0)
   {
      int8_t result = rmf_fileInfo_create(self, name, startAddress, length, fileType);
      if (result<0)
      {
         free(self);
         self=0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void rmf_fileInfo_delete(rmf_fileInfo_t *self)
{
   if (self != 0)
   {
      rmf_fileInfo_destroy(self);
   }
}

void rmf_fileInfo_vdelete(void *arg)
{
   rmf_fileInfo_delete((rmf_fileInfo_t*) arg);
}
#endif

/**
 * returns 0 on success, -1 on failure
 */
int8_t rmf_fileInfo_setDigestData(rmf_fileInfo_t *info, uint16_t digestType, const uint8_t *digestData, uint32_t digestDataLen)
{
   if ( (info != 0) && (digestType <= RMF_FILE_TYPE_STREAM) && (digestData != 0) && ( (digestDataLen == 0) || (digestDataLen == RMF_DIGEST_SIZE) ) )
   {
      info->digestType = digestType;
      memcpy(info->digestData, digestData, RMF_DIGEST_SIZE);
      return 0;
   }
   errno = EINVAL;
   return -1;
}

/**
 * On failure: returns 0 if buffer is too small, -1 on any other error
 * On success: returns number of bytes written to buffer
 */
int32_t rmf_serialize_acknowledge(uint8_t *buf, int32_t bufLen)
{
   if ( buf != 0 )
     {
        uint8_t *p;
        uint32_t totalLen = sizeof(uint32_t);

        if ((uint32_t) bufLen < totalLen )
        {
           return 0; //buffer too small
        }
        p=buf;
        packLE(p, RMF_CMD_ACK, (uint8_t) sizeof(uint32_t));
        return totalLen; //add 1 for null terminator
     }
     return -1;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


