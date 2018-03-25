#ifndef APX_TYPES_H
#define APX_TYPES_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef uint32_t apx_offset_t; //use uint16_t for 64KB offset, use uint32_t for 4GB offset
typedef uint32_t apx_size_t; //use uint16_t  to send up to 64KB, use uint32_t for 4GB.

typedef struct apx_dataWriteCmd_tag
{
   apx_offset_t offset;
   apx_size_t len;
} apx_dataWriteCmd_t;

#define APX_DATA_WRITE_CMD_SIZE sizeof(apx_dataWriteCmd_t)

#define APX_CONNECTION_TYPE_TEST_SOCKET       0
#define APX_CONNECTION_TYPE_TCP_SOCKET        1
#define APX_CONNECTION_TYPE_LOCAL_SOCKET      2

#define APX_DEBUG_NONE     0
#define APX_DEBUG_LOW      1
#define APX_DEBUG_HIGH     2

#define APX_DEBUG_INFO_MAX_LEN 20

#endif //APX_TYPES_H
