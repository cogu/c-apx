#ifndef APX_ES_FILE_MANAGER_CFG_H
#define APX_ES_FILE_MANAGER_CFG_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

#ifndef APX_ES_FILEMANAGER_MAX_NUM_REQUEST_FILES
// Number of remote files supported
#define APX_ES_FILEMANAGER_MAX_NUM_REQUEST_FILES 10
#endif

#ifndef APX_ES_FILEMANAGER_MAX_CMD_BUF_SIZE
// CMD_FILE_INFO_BASE_SIZE + RMF_HIGH_ADDRESS_SIZE + CMD_FILE_INFO_BASE_SIZE +
// max supported file name len incl suffix and str termination
#define APX_ES_FILEMANAGER_MAX_CMD_BUF_SIZE 256u
#endif

#ifndef APX_ES_FILEMANAGER_MIN_BUFFER_TRESHOLD
# define APX_ES_FILEMANAGER_MIN_BUFFER_TRESHOLD 128 //File manager will wait until this many bytes is available before any transmit
#endif

#ifndef APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD
# define APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD 128 //Small writes are sent in atomic chunks.
#endif                                                 //Large writes are potentially fragmented depending on available buffer size
//sanity check
#if (APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD < RMF_HIGH_ADDRESS_SIZE)
#error("APX_ES_FILE_WRITE_FRAGMENTATION_THRESHOLD cannot be set smaller than RMF_HIGH_ADDRESS_SIZE")
#endif

#endif //APX_ES_FILE_MANAGER_CFG_H

