#ifndef APX_SERVER_H
#define APX_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_serverExtension.h"
#include "apx_routingTable.h"
#include "apx_eventListener.h"
#include "apx_connectionManager.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


typedef struct apx_server_tag
{
   adt_list_t connectionEventListeners; //weak references to apx_serverEventListener_t
   apx_routingTable_t routingTable; //routing table for APX port connections
   apx_connectionManager_t connectionManager; //server connections
   adt_list_t extensionManager; //replace with extensionManager class
}apx_server_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_server_create(apx_server_t *self);
void apx_server_destroy(apx_server_t *self);
void apx_server_start(apx_server_t *self);
void* apx_server_registerEventListener(apx_server_t *self, apx_serverEventListener_t *eventListener);
void apx_server_unregisterEventListener(apx_server_t *self, void *handle);
void apx_server_acceptConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
void apx_server_closeConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
apx_routingTable_t* apx_server_getRoutingTable(apx_server_t *self);
apx_error_t apx_server_addExtension(apx_server_t *self, apx_serverExtension_t *extension, dtl_dv_t *config);

#ifdef UNIT_TEST
void apx_server_run(apx_server_t *self);
apx_serverConnectionBase_t *apx_server_getLastConnection(apx_server_t *self);
#endif


#endif //APX_SERVER_H
