/**
 * file: apx_nodeData.h
 * description: apx_nodeData is the client version of an apx node. The server side uses the more complex class called apx_nodeInfo.
 */
#ifndef APX_NODE_DATA_H
#define APX_NODE_DATA_H
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include <stdint.h>
#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=110)
#include "msc_bool.h"
#else
#include <stdbool.h>
#endif
#include "apx_nodeData_cfg.h"
#ifndef APX_EMBEDDED
#  ifndef _WIN32
     //Linux-based system
#    include <pthread.h>
#  else
     //Windows-based system
#    include <Windows.h>
#  endif
#  include "osmacro.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_file_tag;
#ifdef APX_EMBEDDED
struct apx_es_fileManager_tag;
#else
struct apx_fileManager_tag;
struct apx_nodeInfo_tag;
#endif

//forward declaration
struct apx_nodeData_tag;

/**
 * function table of event handlers that apx_nodeData_t can call when events are triggererd
 */
typedef struct apx_nodeDataHandlerTable_tag
{
   void *arg; //user argument
   void (*inPortDataWritten)(void *arg, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len); //called when inDataFile was updated from file manager
   //TODO: add more event handler here, e.g. when ports are connected/disconnect in the server
}apx_nodeDataHandlerTable_t;

typedef struct apx_nodeData_tag
{
   bool isRemote; //true if this is a remote nodeData structure. Default: false
   bool isWeakref; //when true all pointers in this object is owned by some other part of the program. if false then all pointers are created/freed by this class.
   const char *name;
   uint8_t *inPortDataBuf;
   uint8_t *outPortDataBuf;
   uint32_t inPortDataLen;
   uint32_t outPortDataLen;
   uint8_t *definitionDataBuf;
   uint32_t definitionDataLen;
   uint8_t *inPortDirtyFlags;
   uint8_t *outPortDirtyFlags;
   apx_nodeDataHandlerTable_t handlerTable;
#ifdef APX_EMBEDDED
   //used for implementations that has no underlying operating system or runs an RTOS
   struct apx_es_fileManager_tag *fileManager;
#else
   //used for Windows/Linux implementations
   struct apx_fileManager_tag *fileManager;
   SPINLOCK_T inPortDataLock;
   SPINLOCK_T outPortDataLock;
   SPINLOCK_T definitionDataLock;
   SPINLOCK_T internalLock;
#endif
   struct apx_file_tag *outPortDataFile;
   struct apx_file_tag *inPortDataFile;
   struct apx_nodeInfo_tag *nodeInfo;
} apx_nodeData_t;

#if 0
typedef struct ApxWriteBuf16_Tag
{
   uint16_t *readOffset;
   uint16_t *writeOffset;
   uint16_t *numFree;
   uint8_t *dataBegin;
   uint8_t *dataEnd;
   uint16_t elemSize;
}ApxWriteBuf16_T;

#endif


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeData_create(apx_nodeData_t *self, const char *name, uint8_t *definitionBuf, uint32_t definitionDataLen,  uint8_t *inPortDataBuf, uint8_t *inPortDirtyFlags, uint32_t inPortDataLen, uint8_t *outPortDataBuf, uint8_t *outPortDirtyFlags, uint32_t outPortDataLen);
void apx_nodeData_destroy(apx_nodeData_t *self);
apx_nodeData_t *apx_nodeData_newRemote(const char *name, bool isWeakRef);
void apx_nodeData_delete(apx_nodeData_t *self);
void apx_nodeData_vdelete(void *arg);

bool apx_nodeData_isOutPortDataOpen(apx_nodeData_t *self);
bool apx_nodeData_isInPortDataOpen(apx_nodeData_t *self);
void apx_nodeData_setHandlerTable(apx_nodeData_t *self, apx_nodeDataHandlerTable_t *handlerTable);
int8_t apx_nodeData_readDefinitionData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
int8_t apx_nodeData_readOutPortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
int8_t apx_nodeData_readInPortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
void apx_nodeData_lockOutPortData(apx_nodeData_t *self);
void apx_nodeData_unlockOutPortData(apx_nodeData_t *self);
void apx_nodeData_lockInPortData(apx_nodeData_t *self);
void apx_nodeData_unlockInPortData(apx_nodeData_t *self);

//DEPRECATED INTERFACE (Regenerate your APX node using latest py-apx code generator)
void apx_nodeData_outPortDataNotify(apx_nodeData_t *self, uint32_t offset, uint32_t len);

void apx_nodeData_writeInPortDefaultDataIfNotDirty(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
int8_t apx_nodeData_writeInPortData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
int8_t apx_nodeData_writeOutPortData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
int8_t apx_nodeData_writeDefinitionData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
void apx_nodeData_inPortDataWriteNotify(apx_nodeData_t *self, uint32_t offset, uint32_t len);
int8_t apx_nodeData_outPortDataWriteNotify(apx_nodeData_t *self, uint32_t offset, uint32_t len, bool directWriteEnabled);
void apx_nodeData_setInPortDataFile(apx_nodeData_t *self, struct apx_file_tag *file);
void apx_nodeData_setOutPortDataFile(apx_nodeData_t *self, struct apx_file_tag *file);
#ifdef APX_EMBEDDED
void apx_nodeData_setFileManager(apx_nodeData_t *self, struct apx_es_fileManager_tag *fileManager);
#else
void apx_nodeData_setFileManager(apx_nodeData_t *self, struct apx_fileManager_tag *fileManager);
void apx_nodeData_setNodeInfo(apx_nodeData_t *self, struct apx_nodeInfo_tag *nodeInfo);
#endif
#endif //APX_NODE_DATA_H
