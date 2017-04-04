#include <Windows.h>
#include <stdio.h>
#include <string.h>
#include "npipe.h"
#include "npipe_server.h"

static int8_t onData(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void onNewConnection(void *arg, npipe_server_t *server, npipe_t *npipe);
static void onDisconnected(void *arg);

static npipe_t *m_connection = 0;
static bool m_first = true;

int main(int argc, char **argv)
{
   int count=0;
   npipe_server_t npipe_server;
   npipe_handlerTable_t handlerTable;
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.onAccept = onNewConnection;
   npipe_server_create(&npipe_server, 0);   
   npipe_server_sethandlerTable(&npipe_server, &handlerTable);
   npipe_server_start(&npipe_server, "\\\\.\\pipe\\my_pipe");
   for (;;)
   {
      Sleep(1000);
/*      count++;
      if (count > 10)
      {
         break;
      }*/
   }
   if (m_connection != 0)
   {
      npipe_delete(m_connection);
   }
   return 0;
}

static void onNewConnection(void *arg, npipe_server_t *server, npipe_t *npipe)
{
   npipe_handlerTable_t handlerTable;
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.onData = onData;
   handlerTable.onDisconnected = onDisconnected;
   handlerTable.arg = (void*)npipe;
   npipe_sethandlerTable(npipe, &handlerTable);
   npipe_start_io(npipe);
   m_connection = npipe;
}


static int8_t onData(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   npipe_t *npipe = (npipe_t*)arg;
   printf("onData %d\n", dataLen);
   char *OK = "OK";
   npipe_send(npipe, OK, strlen(OK));
   *parseLen = dataLen;
   return 0;
}

static void onDisconnected(void *arg)
{
   printf("onDisconnected\n");
}