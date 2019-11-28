#ifndef APX_SERVER_H
#define APX_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <pthread.h>
//#include <semaphore.h>
#endif
#include "apx_serverExtension.h"
#include "apx_routingTable.h"
#include "apx_eventListener2.h"
#include "apx_connectionManager.h"
#include "apx_eventLoop.h"
#include "soa.h"
#include "adt_str.h"


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


typedef struct apx_server_tag
{
   adt_list_t serverEventListeners; //weak references to apx_serverEventListener_t
   //apx_routingTable_t routingTable; //routing table for APX port connections
   apx_connectionManager_t connectionManager; //server connections
   adt_list_t extensionManager; //TODO: replace with extensionManager class
   THREAD_T workerThread; //local worker thread
   bool isWorkerThreadValid; //true if workerThread is a valid variable
   soa_t soa; //small object allocator
   apx_eventLoop_t eventLoop; //event loop used by workerThread
   MUTEX_T mutex;
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
void apx_server_start(apx_server_t *self);
void apx_server_stop(apx_server_t *self);
void* apx_server_registerEventListener(apx_server_t *self, apx_serverEventListener2_t *eventListener);
void apx_server_unregisterEventListener(apx_server_t *self, void *handle);
void apx_server_acceptConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
void apx_server_closeConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
apx_routingTable_t* apx_server_getRoutingTable(apx_server_t *self);
apx_error_t apx_server_addExtension(apx_server_t *self, const char *name, apx_serverExtensionHandler_t *handler, dtl_dv_t *config);
void apx_server_logEvent(apx_server_t *self, apx_logLevel_t level, const char *label, const char *msg);

#ifdef UNIT_TEST
void apx_server_run(apx_server_t *self);
apx_serverConnectionBase_t *apx_server_getLastConnection(apx_server_t *self);
#endif


#endif //APX_SERVER_H
