//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "apx_client.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif



//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static int8_t tcp_client_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void tcp_client_disconnected(void *arg);
void tcp_client_connected(void *arg,const char *addr,uint16_t port);

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
int8_t apx_client_create(apx_client_t *self)
{
   if( self != 0 )
   {
      self->connection = 0;
      apx_nodeManager_create(&self->nodeManager);
      return 0;
   }
   errno=EINVAL;
   return -1;
}

void apx_client_destroy(apx_client_t *self)
{
   if (self != 0)
   {
      if (self->connection != 0)
      {
         apx_clientConnection_delete(self->connection);
      }
      apx_nodeManager_destroy(&self->nodeManager);
   }
}

apx_client_t *apx_client_new(void)
{
   apx_client_t *self = (apx_client_t*) malloc(sizeof(apx_client_t));
   if(self != 0){
      int8_t result = apx_client_create(self);
      if (result != 0)
      {
         free(self);
         self=0;
      }
   }
   else{
      errno = ENOMEM;
   }
   return self;
}

void apx_client_delete(apx_client_t *self)
{
   if (self != 0)
   {
      apx_client_destroy(self);
      free(self);
   }
}

void apx_client_vdelete(void *arg)
{
   apx_client_delete((apx_client_t*) arg);
}

int8_t apx_client_connect_tcp(apx_client_t *self, const char *address, uint16_t port)
{
   int8_t retval = 0;
   msocket_t *msocket = msocket_new(AF_INET);
   if (msocket != 0)
   {
      msocket_handler_t handlerTable;
      self->connection = apx_clientConnection_new(msocket,self);
      assert(self->connection != 0);
      memset(&handlerTable,0,sizeof(handlerTable));
      handlerTable.tcp_connected=tcp_client_connected;
      handlerTable.tcp_data=tcp_client_data;
      handlerTable.tcp_disconnected = tcp_client_disconnected;
      msocket_sethandler(msocket,&handlerTable,self->connection);
      retval = msocket_connect(msocket, address, port);
      if (retval != 0)
      {
         fprintf(stderr, "[apx_client] msocket_connect failed with %d\n",retval);
      }
   }
   else
   {
      fprintf(stderr, "[apx_client] msocket_new returned NULL\n");
   }
   return retval;
}

/**
 * attached the nodeData to the local nodeManager in the client
 */
void apx_client_attachLocalNode(apx_client_t *self, apx_nodeData_t *nodeData)
{
   if ( (self != 0) && (nodeData != 0) )
   {
      apx_nodeManager_attachLocalNode(&self->nodeManager, nodeData); //client1 is the proxy
   }
}


//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static int8_t tcp_client_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   apx_clientConnection_t *clientConnection = (apx_clientConnection_t*) arg;
   return apx_clientConnection_dataReceived(clientConnection, dataBuf, dataLen, parseLen);
}

static void tcp_client_disconnected(void *arg)
{
   printf("[apx_client] server closed connection\n");
}

void tcp_client_connected(void *arg,const char *addr,uint16_t port)
{
   apx_clientConnection_t *clientConnection = (apx_clientConnection_t*) arg;   
   apx_clientConnection_start(clientConnection);
}


