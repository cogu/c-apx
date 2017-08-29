
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_server.h"
#include <stdio.h>


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_serverInfo_tag
{
   uint8_t addressFamily;
}apx_serverInfo_t;

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_server_accept(void *arg,msocket_server_t *srv,msocket_t *msocket);
static int8_t apx_server_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void apx_server_disconnected(void *arg);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_server_create(apx_server_t *self, uint16_t tcpPort)
{
   if (self != 0)
   {
      msocket_handler_t serverHandler;

      adt_list_create(&self->connections,apx_serverConnection_vdelete);
      self->tcpPort = tcpPort;
      memset(&serverHandler,0,sizeof(serverHandler));
      msocket_server_create(&self->tcpServer,AF_INET, apx_serverConnection_vdelete);
#ifndef _MSC_VER
      msocket_server_create(&self->localServer,AF_LOCAL, apx_serverConnection_vdelete);
#endif
      serverHandler.tcp_accept = apx_server_accept;
      msocket_server_sethandler(&self->tcpServer,&serverHandler,self);
      apx_nodeManager_create(&self->nodeManager);
      apx_router_create(&self->router);
      apx_nodeManager_setRouter(&self->nodeManager, &self->router);
   }
}


void apx_server_start(apx_server_t *self)
{
   if (self != 0)
   {
      msocket_server_start(&self->tcpServer,0,0,self->tcpPort);
   }
}

void apx_server_destroy(apx_server_t *self)
{
   if (self != 0)
   {
      //close and delete all open server connections
      adt_list_destroy(&self->connections);
      //destroy the tcp server
      msocket_server_destroy(&self->tcpServer);
      //destroy the local socket server
#ifndef _MSC_VER
      msocket_server_destroy(&self->localServer);
#endif
      apx_nodeManager_destroy(&self->nodeManager);
      apx_router_destroy(&self->router);
   }
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_server_accept(void *arg,msocket_server_t *srv,msocket_t *msocket)
{
   apx_server_t *self = (apx_server_t*) arg;
   printf("apx_server_accept\n");
   if (self != 0)
   {
      apx_serverConnection_t *newConnection = apx_serverConnection_new(msocket, self);

      if (newConnection != 0)
      {
         msocket_handler_t handlerTable;

         //add it to our list of connections. The linked list is used to keep track of all open connections
         adt_list_insert(&self->connections,newConnection);

         //attach our (single) instance of the nodeManager with the connection
         //apx_serverConnection_attachNodeManager()

         //on the new msocket, setup its vtable of handler functions.
         memset(&handlerTable,0,sizeof(handlerTable));
         handlerTable.tcp_data = apx_server_data;
         handlerTable.tcp_disconnected = apx_server_disconnected;
         msocket_sethandler(msocket, &handlerTable, newConnection);

         if (msocket->addressFamily == AF_INET)
         {
            printf("connection accepted %s\n",msocket->tcpInfo.addr);
         }
         else
         {
            printf("connection accepted\n");
         }
         //now that the handler is setup, start the internal listening thread in the msocket
         msocket_start_io(msocket);
         //trigger the new connection to send the greeting message (in case there is any to be sent)
         apx_serverConnection_start(newConnection);
      }
      else
      {
         perror("apx_serverConnection_new() returned 0");
      }
   }
}

static int8_t apx_server_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   apx_serverConnection_t *clientConnection = (apx_serverConnection_t*) arg;
   return apx_serverConnection_dataReceived(clientConnection, dataBuf, dataLen, parseLen);
}

/**
 * called by msocket worker thread when it detects a disconnect event on the msocket
 * //TODO: make this function thread-safe! If two sockets disconnect at exactly the same time the program probably crashes
 */
static void apx_server_disconnected(void *arg)
{
   apx_serverConnection_t *connection;
   printf("apx_server_disconnected\n");
   connection = (apx_serverConnection_t*) arg;
   if (connection != 0)
   {
      adt_list_remove(&connection->server->connections, connection);
      //the thread inside the msocket class cannot shutdown itself, instead use the cleanup thread to do the job of shutting it down
      apx_nodeManager_detachFileManager(&connection->server->nodeManager, &connection->fileManager);
      switch (connection->msocket->addressFamily)
      {
         case AF_INET: //intentional fallthrough
         case AF_INET6:
            msocket_server_cleanup_connection(&connection->server->tcpServer,arg);
            break;
#ifndef _MSC_VER
         case AF_LOCAL:
            msocket_server_cleanup_connection(&connection->server->localServer,arg);
            break;
#endif
         default:
            break;
      }
   }
}

