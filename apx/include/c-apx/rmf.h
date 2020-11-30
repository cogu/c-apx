#ifndef RMF_H
#define RMF_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "rmf_cfg.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


#define RMF_DATA_LOW_MIN_ADDR ((uint32_t)0x0)               //0
#define RMF_DATA_LOW_MAX_ADDR ((uint32_t)0x3FFF)            //16KB-1
#define RMF_DATA_HIGH_MIN_ADDR ((uint32_t)0x4000)            //16KB
#define RMF_DATA_HIGH_MAX_ADDR ((uint32_t)0x3FFFFBFF)       //1GB

#define RMF_CMD_START_ADDR ((uint32_t) 0x3FFFFC00)
#define RMF_CMD_END_ADDR ((uint32_t) 0x3FFFFFFF)
#define RMF_CMD_HIGH_BIT ((uint32_t) 0x80000000)
#define RMF_CMD_MORE_BIT ((uint32_t) 0x40000000)
#define RMF_REMOTE_ADDRESS_BIT ((uint32_t) 0x80000000) //This is overlayed with RMF_CMD_HIGH_BIT
#define RMF_ADDRESS_MASK          ((uint32_t) 0x3FFFFFFF) //A true remote address can at most be 30 bits long
#define RMF_ADDRESS_MASK_INTERNAL ((uint32_t) 0x7FFFFFFF) //This is the address without the remote address bit
#define RMF_INVALID_ADDRESS (uint32_t) (0x7FFFFFFF) //This is outside the valid address region of 30 bits

#define RMF_DIGEST_SIZE          32u //32 bytes is suitable for storing a sha256 hash
#define RMF_DIGEST_TYPE_NONE     0u
#define RMF_DIGEST_TYPE_SHA1     1u
#define RMF_DIGEST_TYPE_SHA256   2u

#define RMF_LOW_ADDRESS_SIZE 2u
#define RMF_HIGH_ADDRESS_SIZE 4u
#define RMF_MAX_HEADER_SIZE        RMF_HIGH_ADDRESS_SIZE
#define RMF_CMD_TYPE_LEN           4u
#define RMF_CMD_ADDRESS_LEN        4u
#define RMF_CMD_FILE_INFO_BASE_SIZE (RMF_CMD_TYPE_LEN+RMF_CMD_ADDRESS_LEN+4+2+2+RMF_DIGEST_SIZE) //48 bytes total
#define RMF_CMD_FILE_INFO_MAX_SIZE (RMF_CMD_FILE_INFO_BASE_SIZE + RMF_MAX_FILE_NAME +1)
#define RMF_CMD_FILE_OPEN_LEN (RMF_CMD_TYPE_LEN+RMF_CMD_ADDRESS_LEN)
#define RMF_CMD_ACK_LEN RMF_CMD_TYPE_LEN
#define RMF_ERROR_INVALID_READ_HANDLER_LEN (RMF_CMD_TYPE_LEN+RMF_CMD_ADDRESS_LEN)
#define RMF_CMD_FILE_COMPRESS_INFO_LEN (RMF_CMD_TYPE_LEN+4)
#define RMF_ERROR_CODE_BASE_LEN (RMF_CMD_TYPE_LEN+4)

#define RMF_CMD_ACK                    (uint32_t) 0u  //command successful
#define RMF_CMD_NACK                   (uint32_t) 1u  //negative response
#define RMF_CMD_EOT                    (uint32_t) 2u  //end of transmission (used to indicate end of a list)
#define RMF_CMD_FILE_INFO              (uint32_t) 3u  //serialized file info data structure
#define RMF_CMD_REVOKE_FILE            (uint32_t) 4u  //used by server to tell clients that the file is no longer available
#define RMF_CMD_HEARTBEAT_RQST         (uint32_t) 5u   //heartbeat request
#define RMF_CMD_HEARTBEAT_RSP          (uint32_t) 6u   //heartbeat response
#define RMF_CMD_PING_RQST              (uint32_t) 7u   //ping request (similar to hearbeat but also has timestamp)
#define RMF_CMD_PING_RSP               (uint32_t) 8u   //ping response (similar to hearbeat but also has timestamp)
#define RMF_CMD_FILE_OPEN              (uint32_t) 10u  //opens a file
#define RMF_CMD_FILE_CLOSE             (uint32_t) 11u  //closes a file
#define RMF_CMD_FILE_READ              (uint32_t) 12u  //read parts of an open file (TBD)
#define RMF_CMD_COMPRESS_INFO          (uint32_t) 13u  //additional meta-data for compressed file types

#define RMF_INFO_FILE_OPEN_SUCCESS     (uint32_t) 100u //File was successfully open but it currently has no data

//Errors on RMF layer uses the range 400-499
#define RMF_ERROR_INVALID_CMD          (uint32_t) 400u //invalid command
#define RMF_ERROR_INVALID_WRITE        (uint32_t) 401u //remote attempted to write in an invalid memory area
#define RMF_ERROR_INVALID_READ_HANDLER (uint32_t) 402u //Unable to get a valid read handler for the file

//Errors on application layer (like APX) uses range 500 and up
#define RMF_USER_ERROR_BEGIN           (uint32_t) 500u

//Implemented
#define RMF_FILE_TYPE_FIXED            0u //memory mapped file with with fixed length (default)
//To be implemented later
#define RMF_FILE_TYPE_DYNAMIC8         1u //memory mapped file with 8-bit dynamic length (max length is set by length attribute in cmdFileInfo_t).
#define RMF_FILE_TYPE_DYNAMIC16        2u //memory mapped file with 16-bit dynamic length (max length is set by length attribute in cmdFileInfo_t).
#define RMF_FILE_TYPE_DYNAMIC32        3u //memory mapped file with 32-bit dynamic length (max length is set by length attribute in cmdFileInfo_t).
#define RMF_FILE_TYPE_STREAM           4u //chunk in a file stream.
#define RMF_FILE_TYPE_COMPRESSED_FIXED 5u //same as fixed file but its data is compressed. In addition to RMF_CMD_FILE_INFO structure it also needs a RMF_CMD_COMPRESS_INFO

#define RMF_MAX_CMD_BUF_SIZE 1024u

#define RMF_MIN_MSG_LEN (RMF_HIGH_ADDRESS_SIZE+1u)

#define RMF_GREETING_MAX_LEN 127
#define RMF_GREETING_START "RMFP/1.0\n"
#define RMF_NUMHEADER_FORMAT_HDR "NumHeader-Format:"



/**
 * abstract rmf message class
 */
typedef struct rmf_msg_tag
{
   uint32_t address;
   int32_t dataLen;
   const uint8_t *data;
   bool more_bit;
}rmf_msg_t;

typedef struct rmf_cmdOpenFile_tag
{
   uint32_t address;
} rmf_cmdOpenFile_t;

typedef struct rmf_cmdCloseFile_tag
{
   uint32_t address;
} rmf_cmdCloseFile_t;

typedef struct rmf_fileInfo_tag
{
   uint32_t address;
   uint32_t length;
   uint16_t fileType;
   uint16_t digestType;
   uint8_t digestData[RMF_DIGEST_SIZE];
   char name[RMF_MAX_FILE_NAME+1];
}rmf_fileInfo_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

/* rmf serialize/deserialize API */
int32_t rmf_packHeader(uint8_t *dataBuf, int32_t bufLen, uint32_t address, bool more_bit);
int32_t rmf_packHeaderBeforeData(uint8_t *dataBuf, int32_t bufLen, uint32_t address, bool more_bit);
int32_t rmf_unpackMsg(const uint8_t *buf, int32_t bufLen, rmf_msg_t *msg);
uint32_t rmf_unpackAddress(const uint8_t *buf, int32_t bufLen);
int32_t rmf_serialize_cmdFileInfo(uint8_t *buf, int32_t bufLen, const rmf_fileInfo_t *fileInfo);
int32_t rmf_deserialize_cmdFileInfo(const uint8_t *buf, int32_t bufLen, rmf_fileInfo_t *fileInfo);
int32_t rmf_serialize_cmdOpenFile(uint8_t *buf, int32_t bufLen, rmf_cmdOpenFile_t *cmdOpenFile);
int32_t rmf_deserialize_cmdOpenFile(const uint8_t *buf, int32_t bufLen, rmf_cmdOpenFile_t *cmdOpenFile);
int32_t rmf_serialize_cmdCloseFile(uint8_t *buf, int32_t bufLen, rmf_cmdCloseFile_t *cmdCloseFile);
int32_t rmf_deserialize_cmdCloseFile(const uint8_t *buf, int32_t bufLen, rmf_cmdCloseFile_t *cmdCloseFile);
int32_t rmf_deserialize_cmdType(const uint8_t *buf, int32_t bufLen, uint32_t *cmdType);
int32_t rmf_serialize_acknowledge(uint8_t *buf, int32_t bufLen);

/* rmf_fileInfo_t API */
int8_t rmf_fileInfo_create(rmf_fileInfo_t *self, const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType);
void rmf_fileInfo_destroy(rmf_fileInfo_t *self);
#ifndef APX_EMBEDDED
rmf_fileInfo_t *rmf_fileInfo_new(const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType);
void rmf_fileInfo_delete(rmf_fileInfo_t *info);
void rmf_fileInfo_vdelete(void *arg);
#endif
int8_t rmf_fileInfo_setDigestData(rmf_fileInfo_t *info, uint16_t digestType, const uint8_t *digestData, uint32_t digestDataLen);

#endif //RMF_H
