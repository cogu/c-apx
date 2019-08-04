#ifndef APX_SERVER_H
#define APX_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#ifdef UNIT_TEST
#include "testsocket.h"
#else
#include "msocket_server.h"
#endif
#include "apx_routingTable.h"
#include "apx_eventListener.h"
#include "apx_connectionManager.h"



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//forward declarations


typedef struct apx_server_tag
{
#ifndef UNIT_TEST
   uint16_t tcpPort; //TCP port for tcpServer
   char *localServerFile; //path to socket file for unix domain sockets (used for localServer)
   msocket_server_t tcpServer; //tcp server
   msocket_server_t localServer; //unix domain socket server
#endif
   adt_list_t connectionEventListeners; //weak references to apx_serverEventListener_t
   apx_routingTable_t routingTable; //routing table for APX port connections
   apx_connectionManager_t connectionManager; //server connections
   int8_t debugMode; //TODO: remove this
}apx_server_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST
void apx_server_create(apx_server_t *self);
#else
void apx_server_create(apx_server_t *self, uint16_t port);
#endif
void apx_server_destroy(apx_server_t *self);
void apx_server_start(apx_server_t *self);
void apx_server_setDebugMode(apx_server_t *self, int8_t debugMode);
void* apx_server_registerEventListener(apx_server_t *self, apx_serverEventListener_t *eventListener);
void apx_server_unregisterEventListener(apx_server_t *self, void *handle);
void apx_server_acceptConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
void apx_server_closeConnection(apx_server_t *self, apx_serverConnectionBase_t *serverConnection);
apx_routingTable_t* apx_server_getRoutingTable(apx_server_t *self);
#ifdef UNIT_TEST
void apx_server_run(apx_server_t *self);
void apx_server_acceptTestSocket(apx_server_t *self, testsocket_t *socket);
apx_serverConnectionBase_t *apx_server_getLastConnection(apx_server_t *self);
#endif


#endif //APX_SERVER_H
