#ifndef APX_NODE_MANAGER_H
#define APX_NODE_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "adt_hash.h"
#include "adt_list.h"
#include "apx_parser.h"
#include "apx_node.h"
#include "apx_nodeInfo.h"
#include "apx_stream.h"
#include "apx_file.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include "osmacro.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//forward declarations
struct apx_fileManager_tag;
struct apx_file_tag;
struct apx_router_tag;

typedef struct apx_nodeManager_tag
{
   apx_parser_t parser;
   adt_hash_t nodeInfoMap; //hash of strong references to apx_nodeInfo_t
   apx_istream_t apx_istream; //helper structure for parser
   struct apx_router_tag *router;
   adt_hash_t remoteNodeDataMap; //hash containing strong references to apx_nodeData_t remotely connected nodes, only used in server mode
   adt_hash_t localNodeDataMap; //hash containing weak references to apx_nodeData_t for locally connected nodes. only used in client mode
   adt_list_t fileManagerList; //linked list of attached file managers (so far there is a one-to-one relationship between connection and fileManager)
   int8_t debugMode;
   MUTEX_T lock; //locking mechanism
}apx_nodeManager_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_nodeManager_create(apx_nodeManager_t *self);
void apx_nodeManager_destroy(apx_nodeManager_t *self);
void apx_nodeManager_remoteFileAdded(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager, apx_file_t *remoteFile);
void apx_nodeManager_remoteFileRemoved(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager, apx_file_t *remoteFile);
void apx_nodeManager_remoteFileWritten(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager, apx_file_t *remoteFile, uint32_t offset, int32_t length);
void apx_nodeManager_setRouter(apx_nodeManager_t *self, struct apx_router_tag *router);
void apx_nodeManager_attachLocalNode(apx_nodeManager_t *self, apx_nodeData_t *nodeData);
void apx_nodeManager_attachFileManager(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager);
void apx_nodeManager_shutdownFileManager(apx_nodeManager_t *self, struct apx_fileManager_tag *fileManager);
void apx_nodeManager_setDebugMode(apx_nodeManager_t *self, int8_t debugMode);

#endif //APX_NODE_MANAGER_H
