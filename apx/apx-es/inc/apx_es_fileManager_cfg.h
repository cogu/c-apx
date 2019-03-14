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

#ifndef APX_ES_FILE_WRITE_MSG_FRAGMENTATION_THRESHOLD
// Fragmentation will not occur for smaller files or write notifications
// Do not configure below RMF_MIN_MSG_LEN
#define APX_ES_FILE_WRITE_MSG_FRAGMENTATION_THRESHOLD (RMF_HIGH_ADDRESS_SIZE + sizeof(uint64_t))
#endif

#ifndef APX_ES_FILEMANAGER_OPTIMIZE_WRITE_NOTIFICATIONS
// When set to 1 the fileManager avoids adding duplicates to the message queue
#define APX_ES_FILEMANAGER_OPTIMIZE_WRITE_NOTIFICATIONS 1
#endif

#endif //APX_ES_FILE_MANAGER_CFG_H

