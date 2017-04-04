#include <stdio.h>
#include <string.h>
#include "npipe.h"
#include "headerutil.h"

static npipe_t *m_pipe;

static void onConnected(void *arg);
static void onDisconnected(void *arg);
static int8_t onData(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);

#define RMF_GREETING_START "RMFP/1.0\n"
#define RMF_NUMHEADER_FORMAT "NumHeader-Format:"


int main(int argc, char **argv)
{   
   npipe_handlerTable_t handlerTable;
   m_pipe = npipe_new();
   memset(&handlerTable, 0, sizeof(handlerTable));
   handlerTable.arg = m_pipe;
   handlerTable.onConnected = onConnected;
   handlerTable.onData = onData;
   npipe_sethandlerTable(m_pipe, &handlerTable);
   npipe_connect(m_pipe, "\\\\.\\pipe\\apx");
   for (;;)
   {
      SLEEP(1000);
   }
   return 0;
}

static void onConnected(void *arg)
{
   npipe_t *pipe = (npipe_t*)arg;
   if (arg != 0)
   {
      char msgData[1024];
      char *p;
      uint8_t *pNext;
      uint32_t bytesToSend;
      printf("onConnected\n");
      memset(msgData, 0, sizeof(msgData));      
      strcat(msgData, RMF_GREETING_START);
      p = msgData + strlen(msgData);
      sprintf(msgData, "%s: %d\n\n", RMF_NUMHEADER_FORMAT, 32);
      uint8_t msgBuf[1024];
      pNext = headerutil_numEncode32(msgBuf, sizeof(msgBuf), strlen(msgData));
      memcpy(pNext, msgData, strlen(msgData));
      bytesToSend = (pNext - msgBuf) + strlen(msgData);
      printf("sending %d+%d bytes\n", (int)(pNext - msgBuf), strlen(msgData));
      npipe_send(pipe, msgBuf, bytesToSend);
   }   
}

static void onDisconnected(void *arg)
{
   npipe_t *pipe = (npipe_t*)arg;
   if (arg != 0)
   {
      printf("onDisconnected\n");
   }
}

static int8_t onData(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   npipe_t *pipe = (npipe_t*)arg;
   *parseLen = dataLen;
   if (arg != 0)
   {
      if (dataLen > 4)
      {
         printf("onData (%d): %02X %02X %02X %02X\n", dataLen, (int) dataBuf[0], (int)dataBuf[1], (int)dataBuf[2], (int)dataBuf[3]);
      }
      else
      {
         printf("onData (%d)\n", dataLen);
      }
      
      return 0;
   }
   return -1;
}
