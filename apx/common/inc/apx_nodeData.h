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
#include "apx_error.h"
#include "apx_eventListener.h"

#include <stdint.h>
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
struct apx_file2_tag;
#ifdef APX_EMBEDDED
struct apx_es_fileManager_tag;
#else
struct apx_fileManager_tag;
struct apx_portDataMap_tag;

#endif
struct apx_node_tag;
struct apx_parser_tag;
struct apx_connectionBase_tag;
struct apx_portDataRef_tag;
struct apx_portDataProps_tag;
struct apx_portConnectionTable_tag;


/**
 * function table of event handlers that apx_nodeData_t can call when events are triggererd
 */

typedef void (apx_nodeData_nodeCbkFunc)(uint32_t offset, uint32_t len);

typedef struct apx_nodeData_tag
{
   bool isRemote; //true if this is a remote nodeData structure. Default: false
   bool isWeakref; //when true all pointers in this object is owned by some other part of the program. if false then all pointers are created/freed by this class.
   bool isDynamic; //true if this nodeData object shoule to generate its own port programs
   const char *name; //only used when node is code-generated
   uint8_t *inPortDataBuf;
   uint8_t *outPortDataBuf;
   uint32_t inPortDataLen;
   uint32_t outPortDataLen;
   uint8_t *definitionDataBuf;
   uint32_t definitionDataLen;
   uint8_t *inPortDirtyFlags;
   uint8_t *outPortDirtyFlags;
   apx_connectionCount_t *requirePortConnectionCount; //Number of active connections to each require-port
   apx_connectionCount_t *providePortConnectionCount; //Number of active connections to each provide-port
   uint32_t portConnectionsTotal; //Total number of active port connections
   apx_portId_t numRequirePorts; //Number of require-ports in the underlying node
   apx_portId_t numProvidePorts; //Number of provide-ports in the underlying node
   uint32_t definitionStartOffset; //used when more_bit=true during large writes
   uint32_t outPortDataStartOffset; //used when more_bit=true during large writes
   uint32_t inPortDataStartOffset; //used when more_bit=true during large writes
   uint32_t inPortConnectionCountTotal; //Sum of all values in inPortConnectionCount array
   uint32_t outPortConnectionCountTotal; //Sum of all values in outPortConnectionCount array
   apx_nodeData_nodeCbkFunc *apxNodeWriteCbk; //This is used to trigger callbacks in static ApxNode files generated by Python
   uint8_t checksumType;
   uint8_t checksumData[APX_CHECKSUMLEN_SHA256];
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
   struct apx_node_tag *node;
   struct apx_portDataMap_tag *portDataMap; //contains internal information about the ports in this node such as offsets and data lengths
   struct apx_connectionBase_tag *connection;
   struct apx_portConnectionTable_tag *requirePortConnections; //temporary data structure used by apx_routingTableEntry_t (to build connect/disconnect events)
   struct apx_portConnectionTable_tag *providePortConnections; //temporary data structure used by apx_routingTableEntry_t (to build connect/disconnect events)

#endif
   struct apx_file2_tag *definitionFile;
   struct apx_file2_tag *outPortDataFile;
   struct apx_file2_tag *inPortDataFile;
} apx_nodeData_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeData_create(apx_nodeData_t *self, const char *name, uint8_t *definitionBuf, uint32_t definitionDataLen, uint8_t *inPortDataBuf, uint8_t *inPortDirtyFlags, uint32_t inPortDataLen, uint8_t *outPortDataBuf, uint8_t *outPortDirtyFlags, uint32_t outPortDataLen);
void apx_nodeData_destroy(apx_nodeData_t *self);
#ifndef APX_EMBEDDED
apx_nodeData_t *apx_nodeData_new(uint32_t definitionDataLen);
void apx_nodeData_delete(apx_nodeData_t *self);
void apx_nodeData_vdelete(void *arg);
apx_nodeData_t *apx_nodeData_makeFromString(struct apx_parser_tag *parser, const char* apx_text, apx_error_t *errorCode);
#endif
apx_error_t apx_nodeData_setChecksumData(apx_nodeData_t *self, uint8_t checksumType, uint8_t *checksumData);
bool apx_nodeData_isOutPortDataOpen(apx_nodeData_t *self);
void apx_nodeData_setEventListener(apx_nodeData_t *self, apx_nodeDataEventListener_t *eventListener);
void apx_nodeData_setApxNodeCallback(apx_nodeData_t *self, apx_nodeData_nodeCbkFunc* cbk);

apx_error_t apx_nodeData_readDefinitionData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_readOutPortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_readInPortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
uint32_t apx_nodeData_getInPortDataLen(apx_nodeData_t *self);
uint32_t apx_nodeData_getOutPortDataLen(apx_nodeData_t *self);
void apx_nodeData_lockOutPortData(apx_nodeData_t *self);
void apx_nodeData_unlockOutPortData(apx_nodeData_t *self);
void apx_nodeData_lockInPortData(apx_nodeData_t *self);
void apx_nodeData_unlockInPortData(apx_nodeData_t *self);
void apx_nodeData_setInPortDataFile(apx_nodeData_t *self, struct apx_file2_tag *file);
void apx_nodeData_setOutPortDataFile(apx_nodeData_t *self, struct apx_file2_tag *file);
void apx_nodeData_setDefinitionFile(apx_nodeData_t *self, struct apx_file2_tag *file);
struct apx_file2_tag *apx_nodeData_getDefinitionFile(apx_nodeData_t *self);
struct apx_file2_tag *apx_nodeData_getInPortDataFile(apx_nodeData_t *self);
struct apx_file2_tag *apx_nodeData_getOutPortDataFile(apx_nodeData_t *self);

//Old API
void apx_nodeData_outPortDataNotify(apx_nodeData_t *self, apx_offset_t offset, apx_size_t length); //DEPRECATED

// Read/Write API
apx_error_t apx_nodeData_updatePortDataDirect(apx_nodeData_t *destNodeData, struct apx_portDataProps_tag *destDatProps, apx_nodeData_t *srcNodeData, struct apx_portDataProps_tag *srcDataProps);
apx_error_t apx_nodeData_updatePortDataDirectById(apx_nodeData_t *destNodeData, apx_portId_t destPortId, apx_nodeData_t *srcNodeData, apx_portId_t srcPortId);
apx_error_t apx_nodeData_updateOutPortData(apx_nodeData_t *self, const uint8_t *data, uint32_t offset, uint32_t len, bool directWriteEnabled);
apx_error_t apx_nodeData_writeInPortData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_writeOutPortData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_writeDefinitionData(apx_nodeData_t *self, const uint8_t *src, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_readDefinitionData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_readOutPortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);
apx_error_t apx_nodeData_readInPortData(apx_nodeData_t *self, uint8_t *dest, uint32_t offset, uint32_t len);



//Port Connection count API
apx_connectionCount_t apx_nodeData_getRequirePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
apx_connectionCount_t apx_nodeData_getProvidePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
void apx_nodeData_incRequirePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
void apx_nodeData_incProvidePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
void apx_nodeData_decRequirePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
void apx_nodeData_decProvidePortConnectionCount(apx_nodeData_t *self, apx_portId_t portId);
uint32_t apx_nodeData_getPortConnectionsTotal(apx_nodeData_t *self);

#ifdef APX_EMBEDDED
void apx_nodeData_setFileManager(apx_nodeData_t *self, struct apx_es_fileManager_tag *fileManager);
#else
struct apx_file2_tag *apx_nodeData_createLocalDefinitionFile(apx_nodeData_t *self);
struct apx_file2_tag *apx_nodeData_createLocalOutPortDataFile(apx_nodeData_t *self);

void apx_nodeData_setFileManager(apx_nodeData_t *self, struct apx_fileManager_tag *fileManager);
apx_error_t apx_nodeData_createPortDataBuffers(apx_nodeData_t *self);

void apx_nodeData_setNode(apx_nodeData_t *self, struct apx_node_tag *node);
struct apx_node_tag *apx_nodeData_getNode(apx_nodeData_t *self);

struct apx_nodeProgramContainer_tag*  apx_nodeData_initPortPrograms(apx_nodeData_t *self);

//PortDataMap API
apx_error_t apx_nodeData_createPortDataMap(apx_nodeData_t *self, uint8_t mode);
struct apx_portDataMap_tag* apx_nodeData_getPortDataMap(apx_nodeData_t *self);
void apx_nodeData_setPortDataMap(apx_nodeData_t *self, struct apx_portDataMap_tag *portDataMap);
struct apx_portDataRef_tag *apx_nodeData_getRequirePortDataRef(apx_nodeData_t *self, apx_portId_t portId);
struct apx_portDataRef_tag *apx_nodeData_getProvidePortDataRef(apx_nodeData_t *self, apx_portId_t portId);

//APX Connection API
void apx_nodeData_setConnection(apx_nodeData_t *self, struct apx_connectionBase_tag *connection);
struct apx_connectionBase_tag* apx_nodeData_getConnection(apx_nodeData_t *self);

//Port Connection API
struct apx_portConnectionTable_tag* apx_nodeData_getRequirePortConnections(apx_nodeData_t *self);
struct apx_portConnectionTable_tag* apx_nodeData_getProvidePortConnections(apx_nodeData_t *self);

//Utility functions
const char *apx_nodeData_getName(apx_nodeData_t *self);
bool apx_nodeData_isComplete(apx_nodeData_t *self);
uint32_t apx_nodeData_getConnectionId(apx_nodeData_t *self);

#ifdef UNIT_TEST

struct apx_file2_tag *apx_nodeData_newLocalDefinitionFile(apx_nodeData_t *self);
struct apx_file2_tag *apx_nodeData_newLocalOutPortDataFile(apx_nodeData_t *self);
struct apx_file2_tag *apx_nodeData_newLocalInPortDataFile(apx_nodeData_t *self);
#endif //UNIT_TEST
#endif //APX_EMBEDDED
#endif //APX_NODE_DATA_H
