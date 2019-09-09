/*****************************************************************************
* \file      apx_serverSocketConnection.c
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Description
*
* Copyright (c) 2018 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h> //Debug only
#ifdef _MSC_VER
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
#include <Windows.h>
#endif
#ifdef UNIT_TEST
#include "testsocket.h"
#else
#include "msocket.h"
#endif
#include "apx_serverSocketConnection.h"
#include "apx_logging.h"
#include "apx_transmitHandler.h"
#include "apx_fileManager.h"
#include "numheader.h"
#include "bstr.h"
#include "apx_server.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define SEND_BUFFER_GROW_SIZE 4096 //4KB
//#define MAX_DEBUG_BYTES 100
//#define MAX_DEBUG_MSG_SIZE 400
//#define HEX_DATA_LEN 3u

#ifdef UNIT_TEST
#define SOCKET_TYPE testsocket_t
#define SOCKET_DELETE testsocket_delete
#define SOCKET_START_IO(x)
#define SOCKET_SET_HANDLER testsocket_setServerHandler
#define SOCKET_SEND testsocket_serverSend
#define SOCKET_OBJECT_CLOSE(x)
#else
#define SOCKET_DELETE msocket_delete
#define SOCKET_TYPE msocket_t
#define SOCKET_START_IO(x) msocket_start_io(x)
#define SOCKET_SET_HANDLER msocket_sethandler
#define SOCKET_SEND msocket_send
#define SOCKET_OBJECT_CLOSE(x) msocket_close(x)
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void apx_serverSocketConnection_registerTransmitHandler(apx_serverSocketConnection_t *self);
static uint8_t *apx_serverSocketConnection_getSendBuffer(void *arg, int32_t msgLen);
static int32_t apx_serverSocketConnection_send(void *arg, int32_t offset, int32_t msgLen);
static int8_t apx_serverSocketConnection_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void apx_serverSocketConnection_disconnected(void *arg);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverSocketConnection_create(apx_serverSocketConnection_t *self, SOCKET_TYPE *socketObject, struct apx_server_tag *server)
{
   if (self != 0)
   {
      apx_connectionBaseVTable_t vtable;
      apx_connectionBaseVTable_create(&vtable, apx_serverSocketConnection_vdestroy, apx_serverSocketConnection_vstart, apx_serverSocketConnection_vclose);
      apx_error_t result = apx_serverConnectionBase_create(&self->base, server, &vtable);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      apx_serverSocketConnection_registerTransmitHandler(self);
      adt_bytearray_create(&self->sendBuffer, SEND_BUFFER_GROW_SIZE);
      self->socketObject = socketObject;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_serverSocketConnection_destroy(apx_serverSocketConnection_t *self)
{
   if (self != 0)
   {
      apx_serverConnectionBase_destroy(&self->base);
      adt_bytearray_destroy(&self->sendBuffer);
      SOCKET_DELETE(self->socketObject);
   }
}

void apx_serverSocketConnection_vdestroy(void *arg)
{
   apx_serverSocketConnection_destroy((apx_serverSocketConnection_t*) arg);
}

apx_serverSocketConnection_t *apx_serverSocketConnection_new(SOCKET_TYPE *socketObject, struct apx_server_tag *server)
{
   apx_serverSocketConnection_t *self = (apx_serverSocketConnection_t*) malloc(sizeof(apx_serverSocketConnection_t));
   if (self != 0)
   {
      int8_t result = apx_serverSocketConnection_create(self, socketObject, server);
      if (result != 0)
      {
         free(self);
         self = (apx_serverSocketConnection_t*) 0;
      }
   }
   return self;
}

void apx_serverSocketConnection_delete(apx_serverSocketConnection_t *self)
{
   if (self != 0)
   {
      apx_serverSocketConnection_destroy(self);
      free(self);
   }
}

void apx_serverSocketConnection_vdelete(void *arg)
{
   apx_serverSocketConnection_delete((apx_serverSocketConnection_t*) arg);
}

void apx_serverSocketConnection_start(apx_serverSocketConnection_t *self)
{
   if ( (self != 0) && (self->socketObject != 0))
   {
      msocket_handler_t handlerTable;
      memset(&handlerTable,0,sizeof(handlerTable));
      apx_serverConnectionBase_start(&self->base);
      handlerTable.tcp_data = apx_serverSocketConnection_data;
      handlerTable.tcp_disconnected = apx_serverSocketConnection_disconnected;
      SOCKET_SET_HANDLER(self->socketObject, &handlerTable, self);
      SOCKET_START_IO(self->socketObject);
   }
}

void apx_serverSocketConnection_vstart(void *arg)
{
   apx_serverSocketConnection_start((apx_serverSocketConnection_t*) arg);
}

void apx_serverSocketConnection_close(apx_serverSocketConnection_t *self)
{
   if (self != 0)
   {
      SOCKET_OBJECT_CLOSE(self->socketObject);
   }
}

void apx_serverSocketConnection_vclose(void *arg)
{
   apx_serverSocketConnection_close((apx_serverSocketConnection_t*) arg);
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_serverSocketConnection_registerTransmitHandler(apx_serverSocketConnection_t *self)
{
   apx_transmitHandler_t serverTransmitHandler;
   serverTransmitHandler.arg = self;
   serverTransmitHandler.send = apx_serverSocketConnection_send;
   serverTransmitHandler.getSendAvail = 0;
   serverTransmitHandler.getSendBuffer = apx_serverSocketConnection_getSendBuffer;
   apx_fileManager_setTransmitHandler(&self->base.base.fileManager, &serverTransmitHandler);
}

static uint8_t *apx_serverSocketConnection_getSendBuffer(void *arg, int32_t msgLen)
{
   apx_serverSocketConnection_t *self = (apx_serverSocketConnection_t*) arg;
   if (self != 0)
   {
      int8_t result=0;
      int32_t requestedLen;
      //create a buffer where we have room to encode the message header (the length of the message) in addition to the user requested length
      int32_t currentLen = adt_bytearray_length(&self->sendBuffer);
      requestedLen= msgLen + self->base.base.numHeaderLen;
      if (currentLen<requestedLen)
      {
         result = adt_bytearray_resize(&self->sendBuffer, (uint32_t) requestedLen);
      }
      if (result == 0)
      {
         uint8_t *data = adt_bytearray_data(&self->sendBuffer);
         assert(data != 0);
         return &data[self->base.base.numHeaderLen];
      }
   }
   return 0;

}

static int32_t apx_serverSocketConnection_send(void *arg, int32_t offset, int32_t msgLen)
{
   apx_serverSocketConnection_t *self = (apx_serverSocketConnection_t*) arg;
   if ( (self != 0) && (offset>=0) && (msgLen>=0))
   {
      int32_t sendBufferLen;
      uint8_t *sendBuffer = adt_bytearray_data(&self->sendBuffer);
      sendBufferLen = adt_bytearray_length(&self->sendBuffer);
      if ((sendBuffer != 0) && (msgLen+self->base.base.numHeaderLen<=sendBufferLen) )
      {
         uint8_t header[sizeof(uint32_t)];
         uint8_t headerLen;
         uint8_t *headerEnd;
         uint8_t *pBegin;
         if (self->base.base.numHeaderLen == (uint8_t) sizeof(uint32_t))
         {
            headerEnd = header+numheader_encode32(header, (uint32_t) sizeof(header), msgLen);
            if (headerEnd>header)
            {
               headerLen=headerEnd-header;
            }
            else
            {
               assert(0);
               return -1; //header buffer too small
            }
         }
         else
         {
            return -1; //not yet implemented
         }
         //place header just before user data begin
         pBegin = sendBuffer+(self->base.base.numHeaderLen+offset-headerLen); //the part in the parenthesis is where the user data begins
         memcpy(pBegin, header, headerLen);
#if 0
         if (self->debugMode >= APX_DEBUG_4_HIGH)
         {
            int i;
            char msg[MAX_DEBUG_MSG_SIZE];
            char *pMsg = &msg[0];
            char *pMsgEnd = pMsg + MAX_DEBUG_MSG_SIZE;
            pMsg += sprintf(msg, "(%p) Sending %d+%d bytes:", (void*)self, (int)headerLen, (int)msgLen);
            for (i = 0; i < MAX_DEBUG_BYTES; i++)
            {
               if ((i >= msgLen + headerLen) || ( (pMsg + HEX_DATA_LEN) > pMsgEnd))
               {
                  break;
               }
               pMsg += sprintf(pMsg, " %02X", (int)pBegin[i]);
            }
            APX_LOG_DEBUG("[APX_SRV_CONNECTION] %s", msg);
         }
#endif
         SOCKET_SEND(self->socketObject, pBegin, msgLen+headerLen);
         return 0;
      }
      else
      {
         assert(0);
      }
   }
   return -1;
}

static int8_t apx_serverSocketConnection_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   apx_serverSocketConnection_t *self = (apx_serverSocketConnection_t*) arg;
   //printf("data %d\n", (int) dataLen);
   return apx_serverConnectionBase_dataReceived(&self->base, dataBuf, dataLen, parseLen);
}

static void apx_serverSocketConnection_disconnected(void *arg)
{
   apx_serverSocketConnection_t *self = (apx_serverSocketConnection_t*) arg;
   if (self != 0)
   {
      apx_server_closeConnection(self->base.server, &self->base);
   }
}


