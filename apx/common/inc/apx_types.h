#ifndef APX_TYPES_H
#define APX_TYPES_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "apx_cfg.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef int32_t apx_offset_t;
typedef uint32_t apx_size_t; //use uint16_t  to send up to 64KB, use uint32_t for 4GB.
typedef APX_PORT_ID_TYPE apx_portId_t; //int32_t is default. Use int16_t for smaller memory footprint
typedef uint32_t apx_uniquePortId_t; //highest significant bit is 0 when it contains a require port ID and 1 when it contains a provide port ID
typedef uint8_t apx_portType_t; //APX_REQUIRE_PORT, APX_PROVIDE_PORT
typedef uint8_t apx_dynLenType_t; //APX_DYN_LEN_NONE, APX_DYN_LEN_U8, APX_DYN_LEN_U16, APX_DYN_LEN_U32
typedef uint8_t apx_queLenType_t; //APX_QUE_LEN_NONE, APX_QUE_LEN_U8, APX_QUE_LEN_U16, APX_QUE_LEN_U32
typedef APX_CONNECTION_COUNT_TYPE apx_connectionCount_t;
typedef uint8_t apx_mode_t; //APX_NO_MODE, APX_CLIENT_MODE, APX_SERVER_MODE
typedef uint16_t apx_eventId_t;


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

#define APX_UNKNOWN_FILE          0
#define APX_OUTDATA_FILE          1
#define APX_INDATA_FILE           2
#define APX_DEFINITION_FILE       3
#define APX_USER_DATA_FILE        4
#define APX_EVENT_FILE            5

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

#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

#define APX_NODE_DEFAULT_VERSION_MAJOR 1
#define APX_NODE_DEFAULT_VERSION_MINOR 3

#endif //APX_TYPES_H
