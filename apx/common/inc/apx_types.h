#ifndef APX_TYPES_H
#define APX_TYPES_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx_cfg.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef void (apx_voidPtrFunc)(void *arg);

typedef int32_t apx_offset_t;
typedef uint32_t apx_size_t; //use uint16_t  to send up to 64KB, use uint32_t for 4GB.
typedef APX_PORT_ID_TYPE apx_portId_t; //int32_t is default. Use int16_t for smaller memory footprint
typedef APX_PORT_ID_TYPE apx_portCount_t;
typedef uint32_t apx_uniquePortId_t; //highest significant bit is 0 when it contains a require port ID and 1 when it contains a provide port ID
typedef uint8_t apx_portType_t; //APX_REQUIRE_PORT, APX_PROVIDE_PORT
typedef uint8_t apx_dynLenType_t; //APX_DYN_LEN_NONE, APX_DYN_LEN_U8, APX_DYN_LEN_U16, APX_DYN_LEN_U32
typedef uint8_t apx_queLenType_t; //APX_QUE_LEN_NONE, APX_QUE_LEN_U8, APX_QUE_LEN_U16, APX_QUE_LEN_U32
typedef APX_CONNECTION_COUNT_TYPE apx_connectionCount_t;
typedef uint8_t apx_mode_t; //APX_NO_MODE, APX_CLIENT_MODE, APX_SERVER_MODE
typedef uint16_t apx_eventId_t;
typedef uint8_t apx_programType_t; //APX_PACK_PROGRAM, APX_UNPACK_PROGRAM
typedef uint8_t apx_fileType_t;

typedef struct apx_dataWriteCmd_tag
{
   apx_offset_t offset;
   apx_size_t len;
} apx_dataWriteCmd_t;

#define APX_DATA_WRITE_CMD_SIZE sizeof(apx_dataWriteCmd_t)

#define APX_CONNECTION_TYPE_TEST_SOCKET       0
#define APX_CONNECTION_TYPE_TCP_SOCKET        1
#define APX_CONNECTION_TYPE_LOCAL_SOCKET      2

#define APX_DEBUG_NONE                        0
#define APX_DEBUG_1_PROFILE                   1  // Used to profile node connect performance using timestamped log
#define APX_DEBUG_2_LOW                       2
#define APX_DEBUG_3_MID                       3
#define APX_DEBUG_4_HIGH                      4

#define APX_DEBUG_INFO_MAX_LEN 20

#define APX_UNKNOWN_FILE_TYPE     ((apx_fileType_t) 0u)
#define APX_DEFINITION_FILE_TYPE  ((apx_fileType_t) 1u)
#define APX_OUTDATA_FILE_TYPE     ((apx_fileType_t) 2u)
#define APX_INDATA_FILE_TYPE      ((apx_fileType_t) 3u)
//3-63 Reserved for APX future growth
#define APX_CUSTOM_FILE_TYPE_BEGIN      ((apx_fileType_t) 64u)

#define APX_MAX_FILE_EXT_LEN      4 //'.xxx'
#define APX_OUTDATA_FILE_EXT      ".out"
#define APX_INDATA_FILE_EXT       ".in"
#define APX_DEFINITION_FILE_EXT   ".apx"
#define APX_EVENT_FILE_EXT        ".event"

#define APX_CHECKSUM_NONE         0u
#define APX_CHECKSUM_SHA256       1u

#define APX_NO_MODE              ((apx_mode_t) 0u)
#define APX_CLIENT_MODE          ((apx_mode_t) 1u)
#define APX_SERVER_MODE          ((apx_mode_t) 2u)

#define APX_CHECKSUMLEN_SHA256    32u

#define APX_REQUIRE_PORT ((apx_portType_t) 0u)
#define APX_PROVIDE_PORT ((apx_portType_t) 1u)

//Select which type of dynamic array option is used for this port (APX 1.3/future implementation)
#define APX_DYN_LEN_NONE  ((apx_dynLenType_t) 0u) //This port is not a dynamic array
#define APX_DYN_LEN_U8    ((apx_dynLenType_t) 1u) //This port has a dynamic array length < 256 elements
#define APX_DYN_LEN_U16   ((apx_dynLenType_t) 2u) //This port has a dynamic array length < 65536 elements
#define APX_DYN_LEN_U32   ((apx_dynLenType_t) 3u) //This port has a dynamic array length < 2^32 elements

//Select which type of queue length option is used for this port (APX 1.3/future implementation)
#define APX_QUE_LEN_NONE  ((apx_queLenType_t) 0u) //This port is not queued
#define APX_QUE_LEN_U8    ((apx_queLenType_t) 1u) //This port has < 256 queued elements
#define APX_QUE_LEN_U16   ((apx_queLenType_t) 2u) //This port has < 65536 queued elements
#define APX_QUE_LEN_U32   ((apx_queLenType_t) 3u) //This port has < 2^32 queued elements

#define APX_PORT_ID_PROVIDE_PORT 0x80000000 //used inside an uint32_t to carry either a provide port ID and a require port ID.
#define APX_PORT_ID_MASK         0x7FFFFFFF //used to clear the port flag (ready to cast it into an int32_t)

#define APX_BASE_TYPE_NONE     -1
#define APX_BASE_TYPE_UINT8    0 //'C' (uint8)
#define APX_BASE_TYPE_UINT16   1 //'S' (uint16)
#define APX_BASE_TYPE_UINT32   2 //'L' (uin32)
#define APX_BASE_TYPE_UINT64   3 //'U' (uint64)
#define APX_BASE_TYPE_SINT8    4 //'c'
#define APX_BASE_TYPE_SINT16   5 //'s'
#define APX_BASE_TYPE_SINT32   6 //'l'
#define APX_BASE_TYPE_SINT64   7 //'u'
#define APX_BASE_TYPE_STRING   8 //'a' (string)
#define APX_BASE_TYPE_RECORD   9 //"{}" (record)
#define APX_BASE_TYPE_REF_ID   10 //type ID
#define APX_BASE_TYPE_REF_NAME 11 //type name
#define APX_BASE_TYPE_REF_PTR  12 //pointer to type (this is achieved only after derived has been called on data signature)
typedef int8_t apx_baseType_t;

#define APX_UNPACK_PROGRAM   ((apx_programType_t) 0)
#define APX_PACK_PROGRAM     ((apx_programType_t) 1)

#define UINT8_SIZE   1u
#define UINT16_SIZE  2u
#define UINT32_SIZE  4u
#define UINT64_SIZE  8u
#define SINT8_SIZE   1u
#define SINT16_SIZE  2u
#define SINT32_SIZE  4u
#define SINT64_SIZE  8u
#define BOOL_SIZE    sizeof(bool)


#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

#define APX_NODE_DEFAULT_VERSION_MAJOR 1
#define APX_NODE_DEFAULT_VERSION_MINOR 3

#define APX_LOG_LEVEL_CRITICAL 0    //SYSLOG_LEVEL 2 (+2)
#define APX_LOG_LEVEL_ERROR    1    //SYSLOG_LEVEL 3 (+2)
#define APX_LOG_LEVEL_WARNING  2    //SYSLOG_LEVEL 4 (+2)
#define APX_LOG_LEVEL_INFO     3    //SYSLOG_LEVEL 6 (+3)
#define APX_LOG_LEVEL_DEBUG    4    //SYSLOG_LEVEL 7 (+3)
#define APX_MAX_LOG_LEVEL      APX_LOG_LEVEL_DEBUG
typedef uint8_t apx_logLevel_t;

#define APX_MAX_LOG_LEN 1024

#define APX_LOG_LABEL_MAX_LEN 16

#define APX_INVALID_CONNECTION_ID 0xFFFFFFFF

#define APX_ADDRESS_PORT_DATA_BOUNDARY   0x400 //1KB, this must be a power of 2
#define APX_ADDRESS_DEFINITION_BOUNDARY  0x100000u //1MB, this must be a power of 2
#define APX_ADDRESS_DEFINITION_START     0x4000000 //64MB, this must be a power of 2

typedef uint8_t apx_nodeState_t;
#define APX_NODE_STATE_STAGING  ((apx_nodeState_t) 0u)
#define APX_NODE_STATE_RUNNING  ((apx_nodeState_t) 1u)
#define APX_NODE_STATE_CLEANUP  ((apx_nodeState_t) 2u)
#define APX_NODE_STATE_INVALID  ((apx_nodeState_t) 3u)


#endif //APX_TYPES_H
