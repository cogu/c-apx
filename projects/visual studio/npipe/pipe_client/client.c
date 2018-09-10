#include <stdio.h>
#include <string.h>
#include "npipe.h"

#define greeting "HTTP/1.1\r\nGET HELLO\r\nAccepted: text/html\r\n\n"

static int m_count = 0;

static void onPipeConnected(void *arg)
{
   uint8_t val;
   npipe_t *pipe = (npipe_t*)arg;
   int8_t result = npipe_send(pipe, greeting, strlen(greeting));
   printf("npipe_send result=%d\n", (int)result);
}

static int8_t onPipeData(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   uint8_t val;
   npipe_t *pipe = (npipe_t*)arg;
   *parseLen = 2;
   printf("m_count=%d\n", m_count);
   if (m_count < 10)
   {
      val = (uint8_t)m_count++;
      npipe_send(pipe, &val, sizeof(uint8_t));
   }
   return 0;
}

int main(int argc, char **argv)
{
   npipe_t pipe;
   npipe_handlerTable_t handlerTable;
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.onConnected = onPipeConnected;
   handlerTable.onData = onPipeData;
   handlerTable.arg = &pipe;
   npipe_create(&pipe);
   npipe_sethandlerTable(&pipe, &handlerTable);
   int8_t rc = npipe_connect(&pipe, "\\\\.\\pipe\\apx");
   for (;;)
   {
      Sleep(1000); //main thread sleeps, waiting for ctrl-c from stdin
   }
}