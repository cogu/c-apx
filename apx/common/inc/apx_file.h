#ifndef APX_FILE_H
#define APX_FILE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif


#include "apx_nodeData.h"
#include "rmf.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
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

typedef struct apx_file_tag
{
   bool isRemoteFile;
   bool isOpen;
   uint8_t fileType;
   apx_nodeData_t *nodeData;
   rmf_fileInfo_t fileInfo;
} apx_file_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t apx_file_createLocalFileFromNodeData(apx_file_t *self, uint8_t fileType, apx_nodeData_t *nodeData);
int8_t apx_file_createRemoteFile(apx_file_t *self, const rmf_fileInfo_t *fileInfo);
#ifndef APX_EMBEDDED
void apx_file_destroy(apx_file_t *self);
apx_file_t *apx_file_newLocalFileFromNodeData(uint8_t fileType, apx_nodeData_t *nodeData);
apx_file_t *apx_file_newLocalDefinitionFile(apx_nodeData_t *nodeData);
apx_file_t *apx_file_newLocalOutPortDataFile(apx_nodeData_t *nodeData);
apx_file_t *apx_file_newLocalInPortDataFile(apx_nodeData_t *nodeData);
apx_file_t *apx_file_newRemoteFile(const rmf_fileInfo_t *fileInfo);
void apx_file_delete(apx_file_t *self);
void apx_file_vdelete(void *arg);
#endif
char *apx_file_basename(const apx_file_t *self);
void apx_file_open(apx_file_t *self);
void apx_file_close(apx_file_t *self);
int8_t apx_file_read(apx_file_t *self, uint8_t *pDest, uint32_t offset, uint32_t length);
int8_t apx_file_write(apx_file_t *self, const uint8_t *pSrc, uint32_t offset, uint32_t length);

#endif //APX_FILE_H

