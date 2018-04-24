//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_testServer.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_testServer_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void apx_testServer_disconnected(void *arg);

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_testServer_create(apx_testServer_t *self)
{
   if(self != 0)
   {
      apx_nodeManager_create(&self->nodeManager);
      apx_router_create(&self->router);
      apx_nodeManager_setRouter(&self->nodeManager, &self->router);
      adt_list_create(&self->connections, apx_serverConnection_vdelete);
   }
}

void apx_testServer_destroy(apx_testServer_t *self)
{
   if (self != 0)
   {
      adt_list_destroy(&self->connections);
      apx_nodeManager_destroy(&self->nodeManager);
      apx_router_destroy(&self->router);
   }
}

void apx_testServer_accept(apx_testServer_t *self, testsocket_t *socket)
{
   if (self != 0)
   {
      apx_serverConnection_t *newConnection = apx_serverConnection_new(socket, self);
      if (newConnection != 0)
      {
         msocket_handler_t handlerTable;
         adt_list_insert(&self->connections, newConnection);
         memset(&handlerTable,0,sizeof(handlerTable));
         handlerTable.tcp_data = apx_testServer_data;
         handlerTable.tcp_disconnected = apx_testServer_disconnected;
         testsocket_setServerHandler(socket, &handlerTable, newConnection);
         apx_serverConnection_start(newConnection);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static int8_t apx_testServer_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   apx_serverConnection_t *clientConnection = (apx_serverConnection_t*) arg;
   return apx_serverConnection_dataReceived(clientConnection, dataBuf, dataLen, parseLen);
}

static void apx_testServer_disconnected(void *arg)
{

}

