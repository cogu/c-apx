#ifndef APX_SERVER_H
#define APX_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include "apx_portSignatureMap.h"
#include "apx_serverExtension.h"
#include "apx_portSignatureMap.h"
#include "apx_eventListener.h"
#include "apx_connectionManager.h"
#include "apx_eventLoop.h"
#include "apx_nodeInstance.h"
#include "soa.h"
#include "adt_str.h"
#include "adt_ary.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


typedef struct apx_server_tag
{
   adt_list_t serverEventListeners; //weak references to apx_serverEventListener_t
   apx_portSignatureMap_t portSignatureMap; //This is the global map that is used to build all port connectors.
                                            //Any access to this structure must be protected by acquiring the globalLock.
   apx_connectionManager_t connectionManager; //server connections
   adt_list_t extensionManager; //TODO: replace with extensionManager class
   adt_ary_t modifiedNodes; //weak references to apx_nodeInstance_t. Used to keep track of which nodes have modified port connectors.
   THREAD_T eventThread; //local worker thread (for playing server-global events such as log events)
   bool isEventThreadValid; //true if workerThread is a valid variable
   soa_t soa; //small object allocator
   apx_eventLoop_t eventLoop; //event loop used by workerThread
   MUTEX_T eventLoopLock; //for protecting the event loop
   MUTEX_T globalLock; //Protects the portSignatureMap and connectionManager
                       //synchronize data routing execution as well as
                       //controlling access to the global portSignatureMap.
   SPINLOCK_T eventListenerLock; //Used to protect access to serverEventListeners
#ifdef _MSC_VER
   unsigned int threadId;
#endif
} apx_server_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_server_create(apx_server_t *self);
void apx_server_destroy(apx_server_t *self);
apx_server_t *apx_server_new(void);
void apx_server_delete(apx_server_t *self);
void apx_server_start(apx_server_t *self);
void apx_server_stop(apx_server_t *self);
void* apx_server_registerEventListener(apx_server_t *self, apx_serverEventListener_t *eventListener);
void apx_server_unregisterEventListener(apx_server_t *self, void *handle);
void apx_server_acceptConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
void apx_server_detachConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
apx_error_t apx_server_addExtension(apx_server_t *self, const char *name, apx_serverExtensionHandler_t *handler, dtl_dv_t *config);
void apx_server_logEvent(apx_server_t *self, apx_logLevel_t level, const char *label, const char *msg);
void apx_server_takeGlobalLock(apx_server_t *self);
void apx_server_releaseGlobalLock(apx_server_t *self);
apx_error_t apx_server_connectNodeInstanceProvidePorts(apx_server_t *self, apx_nodeInstance_t *nodeInstance);
apx_error_t apx_server_connectNodeInstanceRequirePorts(apx_server_t *self, apx_nodeInstance_t *nodeInstance);
apx_error_t apx_server_disconnectNodeInstanceProvidePorts(apx_server_t *self, apx_nodeInstance_t *nodeInstance);
apx_error_t apx_server_disconnectNodeInstanceRequirePorts(apx_server_t *self, apx_nodeInstance_t *nodeInstance);
apx_error_t apx_server_processRequirePortConnectorChanges(apx_server_t *self, apx_nodeInstance_t *requireNodeInstance, apx_portConnectorChangeTable_t *connectorChanges);
apx_error_t apx_server_processProvidePortConnectorChanges(apx_server_t *self, apx_nodeInstance_t *provideNodeInstance, apx_portConnectorChangeTable_t *connectorChanges);
apx_error_t apx_server_insertModifiedNode(apx_server_t *self, apx_nodeInstance_t *nodeInstance);
adt_ary_t *apx_server_getModifiedNodes(const apx_server_t *self);
void apx_server_clearPortConnectorChanges(apx_server_t *self);


#ifdef UNIT_TEST
void apx_server_run(apx_server_t *self);
apx_serverConnectionBase_t *apx_server_getLastConnection(apx_server_t *self);
apx_portSignatureMap_t *apx_server_getPortSignatureMap(apx_server_t *self);
#endif


#endif //APX_SERVER_H
