#ifndef RMF_H
#define RMF_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#include "rmf_cfg.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


#define RMF_DATA_LOW_MIN_ADDR ((uint32_t)0x0)               //0
#define RMF_DATA_LOW_MAX_ADDR ((uint32_t)0x3FFF)            //16KB
#define RMF_DATA_HIGH_MAX_ADDR ((uint32_t)0x3FFFFBFF)       //1GB

#define RMF_CMD_START_ADDR ((uint32_t) 0x3FFFFC00)
#define RMF_CMD_END_ADDR ((uint32_t) 0x3FFFFFFF)
#define RMF_CMD_HIGH_BIT ((uint32_t) 0x80000000)
#define RMF_CMD_MORE_BIT ((uint32_t) 0x40000000)

#define RMF_LOW_ADDRESS_SIZE 2u
#define RMF_HIGH_ADDRESS_SIZE 4u
#define RMF_MAX_HEADER_SIZE RMF_HIGH_ADDRESS_SIZE
#define RMF_CMD_TYPE_LEN           4u

//server command messages
#define RMF_CMD_ACK                (uint32_t) 0   //reserved for future use
#define RMF_CMD_NACK               (uint32_t) 1   //negative response
#define RMF_CMD_EOT                (uint32_t) 2   //end of transmission (used to indicate end of a list)
#define RMF_CMD_FILE_INFO          (uint32_t) 3   //serialized file info data structure
#define RMF_CMD_REVOKE_FILE        (uint32_t) 4   //used by server to tell clients that the file is no longer available
#define RMF_CMD_PAGE_SELECT        (uint32_t) 5   //reserved for future use

//client command messages
#define RMF_CMD_GET_FILE_LIST     (uint32_t) 8   //request list of files
#define RMF_CMD_GET_FILE_INFO     (uint32_t) 9   //request info about specific file
#define RMF_CMD_FILE_OPEN         (uint32_t) 10  //opens a file
#define RMF_CMD_FILE_CLOSE        (uint32_t) 11  //closes a file
#define RMF_CMD_FILE_READ         (uint32_t) 12  //read parts of an open file (TBD)
#define RMF_CMD_INVALID_MSG       (uint32_t) 0xFFFFFFFF //invalid command (default value)

#define RMF_DIGEST_SIZE          32u //32 bytes is suitable for storing a sha256 hash
#define RMF_DIGEST_TYPE_NONE     0u
#define RMF_DIGEST_TYPE_SHA256   1u

//Implemented
#define RMF_FILE_TYPE_FIXED      0u //memory mapped file with with fixed length (default)
//To be implemented later
#define RMF_FILE_TYPE_DYNAMIC    1u //memory mapped file with dynamic length (max length is set by length attribute in cmdFileInfo_t).
#define RMF_FILE_TYPE_STREAM     2u //chunk in a file stream.

#define RMF_MAX_CMD_BUF_SIZE 1024u

#define RMF_MIN_MSG_LEN (RMF_HIGH_ADDRESS_SIZE+1u)

#define RMF_GREETING_MAX_LEN 127
#define RMF_GREETING_START "RMFP/1.0\n"
#define RMF_NUMHEADER_FORMAT "NumHeader-Format:"

#define RMF_INVALID_ADDRESS (uint32_t) (0xFFFFFFFF)
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

#define CMD_FILE_INFO_BASE_SIZE (4+4+4+2+2+RMF_DIGEST_SIZE) //44 bytes plus additional 4 bytes to store value of RMF_FILE_INFO
#define RMF_FILE_OPEN_CMD_LEN 8

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//int32_t rmf_packMsg(uint8_t *buf, int32_t bufLen, uint32_t address, uint8_t *data, int32_t dataLen, int32_t *consumed);
int32_t rmf_packHeaderBeforeData(uint8_t *dataBuf, int32_t bufLen, uint32_t address, bool more_bit);
int32_t rmf_unpackMsg(const uint8_t *buf, int32_t bufLen, rmf_msg_t *msg);
int32_t rmf_serialize_cmdFileInfo(uint8_t *buf, int32_t bufLen, rmf_fileInfo_t *fileInfo);
int32_t rmf_deserialize_cmdFileInfo(const uint8_t *buf, int32_t bufLen, rmf_fileInfo_t *fileInfo);
int32_t rmf_serialize_cmdOpenFile(uint8_t *buf, int32_t bufLen, rmf_cmdOpenFile_t *cmdOpenFile);
int32_t rmf_deserialize_cmdOpenFile(const uint8_t *buf, int32_t bufLen, rmf_cmdOpenFile_t *cmdOpenFile);
int32_t rmf_serialize_cmdCloseFile(uint8_t *buf, int32_t bufLen, rmf_cmdCloseFile_t *cmdCloseFile);
int32_t rmf_deserialize_cmdCloseFile(const uint8_t *buf, int32_t bufLen, rmf_cmdCloseFile_t *cmdCloseFile);
int32_t rmf_deserialize_cmdType(const uint8_t *buf, int32_t bufLen, uint32_t *cmdType);
int32_t rmf_serialize_acknowledge(uint8_t *buf, int32_t bufLen);
int8_t rmf_fileInfo_create(rmf_fileInfo_t *self, const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType);
void rmf_fileInfo_destroy(rmf_fileInfo_t *info);
#ifndef APX_EMBEDDED
rmf_fileInfo_t *rmf_fileInfo_new(const char *name, uint32_t startAddress, uint32_t length, uint16_t fileType);
void rmf_fileInfo_delete(rmf_fileInfo_t *info);
void rmf_fileInfo_vdelete(void *arg);
#endif
int8_t rmf_fileInfo_setDigestData(rmf_fileInfo_t *info, uint16_t digestType, const uint8_t *digestData, uint32_t digestDataLen);

#endif //RMF_H
