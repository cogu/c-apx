#ifndef APX_SERVER_H
#define APX_SERVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "msocket_server.h"
#include "apx_nodeManager.h"
#include "apx_serverConnection.h"
#include "apx_router.h"
#include "adt_list.h"



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_server_tag
{
   uint16_t tcpPort; //TCP port for tcpServer
   char *localServerFile; //path to socket file for unix domain sockets (used for localServer)
   msocket_server_t tcpServer; //tcp server
   msocket_server_t localServer; //unix domain socket server (to be implemented later)
   adt_list_t connections; //linked list of strong references to apx_serverConnection_t
   apx_nodeManager_t nodeManager; //the server has a single instance of the node manager, all connections interface with this object
   apx_router_t router; //this component handles all routing tables within the server
   MUTEX_T mutex;
}apx_server_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_server_create(apx_server_t *self, uint16_t port);
void apx_server_destroy(apx_server_t *self);
void apx_server_start(apx_server_t *self);


#endif //APX_SERVER_H
