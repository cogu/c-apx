#ifndef APX_FILE_MANAGER_SHARED2_H
#define APX_FILE_MANAGER_SHARED2_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_allocator.h"
#include "apx_fileMap.h"
#include "apx_fileManagerDefs.h"
#include "apx_file2.h"
#include "apx_fileInfo.h"
#include "apx_error.h"
#include "rmf.h"
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_file2_tag;
struct rmf_fileInfo_tag;

typedef struct apx_fileManagerShared2_tag
{
   void *arg;
   apx_allocator_t allocator;
   apx_fileMap_t localFileMap;
   apx_fileMap_t remoteFileMap;
   uint32_t connectionId;
   SPINLOCK_T lock;
   void (*remoteFileCreated)(void *arg, apx_file2_t *remoteFile);
   void (*fileOpenRequested)(void *arg, apx_file2_t *localFile);
   void (*remoteFileWritten)(void *arg, apx_file2_t *remoteFile, uint32_t offset, const uint8_t *msgData, uint32_t msgLen, bool moreBit);
} apx_fileManagerShared2_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManagerShared2_create(apx_fileManagerShared2_t *self);
void apx_fileManagerShared2_destroy(apx_fileManagerShared2_t *self);
uint8_t *apx_fileManagerShared2_alloc(apx_fileManagerShared2_t *self, size_t size);
void apx_fileManagerShared2_free(apx_fileManagerShared2_t *self, uint8_t *ptr, size_t size);
apx_file2_t *apx_fileManagerShared2_createLocalFile(apx_fileManagerShared2_t *self, const apx_fileInfo_t *fileInfo);
apx_file2_t *apx_fileManagerShared2_createRemoteFile(apx_fileManagerShared2_t *self, const apx_fileInfo_t *fileInfo);
int32_t apx_fileManagerShared2_getNumLocalFiles(apx_fileManagerShared2_t *self);
int32_t apx_fileManagerShared2_getNumRemoteFiles(apx_fileManagerShared2_t *self);
apx_file2_t *apx_fileManagerShared2_findLocalFileByName(apx_fileManagerShared2_t *self, const char *name);
apx_file2_t *apx_fileManagerShared2_findRemoteFileByName(apx_fileManagerShared2_t *self, const char *name);
apx_file2_t *apx_fileManagerShared2_findFileByAddress(apx_fileManagerShared2_t *self, uint32_t address);
void apx_fileManagerShared2_setConnectionId(apx_fileManagerShared2_t *self, uint32_t connectionId);
uint32_t apx_fileManagerShared2_getConnectionId(const apx_fileManagerShared2_t *self);
adt_ary_t *apx_fileManagerShared2_getLocalFileList(apx_fileManagerShared2_t *self);


#endif //APX_FILE_MANAGER_SHARED_H
