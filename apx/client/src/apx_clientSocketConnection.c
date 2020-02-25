/*****************************************************************************
* \file      apx_clientSocketConnection.c
* \author    Conny Gustafsson
* \date      2018-12-31
* \brief     Description
*
* Copyright (c) 2018-2019 Conny Gustafsson
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
#include "apx_clientSocketConnection.h"
#include "apx_logging.h"
#include "apx_transmitHandler.h"
#include "apx_fileManager.h"
#include "numheader.h"
#include "bstr.h"
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
#define SOCKET_SET_HANDLER testsocket_setClientHandler
#define SOCKET_SEND testsocket_clientSend
#else
#define SOCKET_DELETE msocket_delete
#define SOCKET_TYPE msocket_t
#define SOCKET_START_IO(x) msocket_start_io(x)
#define SOCKET_SET_HANDLER msocket_sethandler
#define SOCKET_SEND msocket_send
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_clientSocketConnection_fillTransmitHandler(apx_clientSocketConnection_t *self, apx_transmitHandler_t *handler);
static apx_error_t apx_clientSocketConnection_vfillTransmitHandler(void *arg, apx_transmitHandler_t *handler);
static void apx_clientSocketConnection_registerSocketHandler(apx_clientSocketConnection_t *self, SOCKET_TYPE *socketObject);
static uint8_t *apx_clientSocketConnection_getSendBuffer(void *arg, int32_t msgLen);
static int32_t apx_clientSocketConnection_send(void *arg, int32_t offset, int32_t msgLen);
static void apx_clientSocketConnection_connected(void *arg, const char *addr, uint16_t port);
static int8_t apx_clientSocketConnection_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
static void apx_clientSocketConnection_disconnected(void *arg);

static void apx_clientSocketConnection_close(apx_clientSocketConnection_t *self);
static void apx_clientSocketConnection_vclose(void *arg);
static void apx_clientSocketConnection_start(apx_clientSocketConnection_t *self);
static void apx_clientSocketConnection_vstart(void *arg);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientSocketConnection_create(apx_clientSocketConnection_t *self, SOCKET_TYPE *socketObject)
{
   if (self != 0)
   {
      apx_connectionBaseVTable_t vtable;
      apx_connectionBaseVTable_create(&vtable,
            apx_clientSocketConnection_vdestroy,
            apx_clientSocketConnection_vstart,
            apx_clientSocketConnection_vclose,
            apx_clientSocketConnection_vfillTransmitHandler);
      apx_error_t result = apx_clientConnectionBase_create(&self->base, &vtable);
      if (result != APX_NO_ERROR)
      {
         return result;
      }
      adt_bytearray_create(&self->sendBuffer, SEND_BUFFER_GROW_SIZE);
      self->socketObject = socketObject;
      apx_connectionBase_start(&self->base.base);///TODO: Don't call start from the constructor
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_clientSocketConnection_destroy(apx_clientSocketConnection_t *self)
{
   if (self != 0)
   {
      apx_clientConnectionBase_destroy(&self->base);
      adt_bytearray_destroy(&self->sendBuffer);
      SOCKET_DELETE(self->socketObject);
   }
}

void apx_clientSocketConnection_vdestroy(void *arg)
{
   apx_clientSocketConnection_destroy((apx_clientSocketConnection_t*) arg);
}

apx_clientSocketConnection_t *apx_clientSocketConnection_new(SOCKET_TYPE *socketObject)
{
   apx_clientSocketConnection_t *self = (apx_clientSocketConnection_t*) malloc(sizeof(apx_clientSocketConnection_t));
   if (self != 0)
   {
      apx_error_t errorCode = apx_clientSocketConnection_create(self, socketObject);
      if (errorCode != APX_NO_ERROR)
      {
         free(self);
         self = (apx_clientSocketConnection_t*) 0;
      }
   }
   return self;
}




#ifdef UNIT_TEST
apx_error_t apx_clientSocketConnection_connect(apx_clientSocketConnection_t *self)
{
   if ( (self != 0) && (self->socketObject != 0) )
   {
      apx_clientSocketConnection_registerSocketHandler(self, self->socketObject);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

#else
apx_error_t apx_clientConnection_tcp_connect(apx_clientSocketConnection_t *self, const char *address, uint16_t port)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      msocket_t *socketObject = msocket_new(AF_INET);
      if (socketObject != 0)
      {
         int8_t result = 0;
         apx_clientSocketConnection_registerSocketHandler(self, socketObject);
         result = msocket_connect(socketObject, address, port);
         if (result != 0)
         {
            msocket_delete(socketObject);
            self->socketObject = (SOCKET_TYPE*) 0;
            retval = APX_CONNECTION_ERROR;
         }
         else
         {

         }
      }
      else
      {
         retval = APX_MEM_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_clientConnection_unix_connect(apx_clientSocketConnection_t *self, const char *socketPath)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      msocket_t *socketObject = msocket_new(AF_LOCAL);
      if (socketObject != 0)
      {
         int8_t result = 0;
         apx_clientSocketConnection_registerSocketHandler(self, socketObject);
         result = msocket_unix_connect(socketObject, socketPath);
         if (result != 0)
         {
            msocket_delete(socketObject);
            self->socketObject = (SOCKET_TYPE*) 0;
            retval = APX_CONNECTION_ERROR;
         }
         else
         {

         }
      }
      else
      {
         retval = APX_MEM_ERROR;
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

#endif




//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_clientSocketConnection_fillTransmitHandler(apx_clientSocketConnection_t *self, apx_transmitHandler_t *handler)
{
   if ( (self != 0) && (handler != 0) )
   {
      handler->arg = self;
      handler->send = apx_clientSocketConnection_send;
      handler->getSendAvail = 0;
      handler->getSendBuffer = apx_clientSocketConnection_getSendBuffer;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_clientSocketConnection_vfillTransmitHandler(void *arg, apx_transmitHandler_t *handler)
{
   return apx_clientSocketConnection_fillTransmitHandler((apx_clientSocketConnection_t*) arg, handler);
}

static void apx_clientSocketConnection_registerSocketHandler(apx_clientSocketConnection_t *self, SOCKET_TYPE *socketObject)
{
   if (socketObject != 0)
   {
      msocket_handler_t handlerTable;
      memset(&handlerTable,0,sizeof(handlerTable));
      handlerTable.tcp_connected=apx_clientSocketConnection_connected;
      handlerTable.tcp_data=apx_clientSocketConnection_data;
      handlerTable.tcp_disconnected = apx_clientSocketConnection_disconnected;
      SOCKET_SET_HANDLER(socketObject, &handlerTable, self);
      self->socketObject = socketObject;
   }
}

static uint8_t *apx_clientSocketConnection_getSendBuffer(void *arg, int32_t msgLen)
{
   apx_clientSocketConnection_t *self = (apx_clientSocketConnection_t*) arg;
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

/**
 * Returns the number transmitted (msgLen) or less. On error it returns -1;
 */
static int32_t apx_clientSocketConnection_send(void *arg, int32_t offset, int32_t msgLen)
{
   apx_clientSocketConnection_t *self = (apx_clientSocketConnection_t*) arg;
   if ( (self != 0) && (self->socketObject != 0) && (offset>=0) && (msgLen>=0) )
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
         int8_t result;
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
            return -1; //16-bit header not yet implemented
         }
         //place header just before user data begin
         pBegin = sendBuffer+(self->base.base.numHeaderLen+offset-headerLen); //the part in the parenthesis is where the user data begins
         memcpy(pBegin, header, headerLen);
         result = SOCKET_SEND(self->socketObject, pBegin, msgLen+headerLen);
         if (result == 0)
         {
            self->base.base.totalBytesSent+=msgLen+headerLen;
         }
         return msgLen;
      }
      else
      {
         assert(0);
      }
   }
   return -1;
}

static void apx_clientSocketConnection_connected(void *arg, const char *addr, uint16_t port)
{
   apx_clientSocketConnection_t *self;
   (void) addr;
   (void) port;
#if APX_DEBUG_ENABLE
   printf("[CLIENT-SOCKET] Connected\n");
#endif
   self = (apx_clientSocketConnection_t*) arg;
   apx_clientConnectionBase_connectedCbk(&self->base);
}


static int8_t apx_clientSocketConnection_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   apx_clientSocketConnection_t *self = (apx_clientSocketConnection_t*) arg;
#if APX_DEBUG_ENABLE
   printf("[CLIENT-SOCKET] Received %d bytes\n", (int) dataLen);
#endif
   return apx_clientConnectionBase_onDataReceived(&self->base, dataBuf, dataLen, parseLen);
}

static void apx_clientSocketConnection_disconnected(void *arg)
{
   apx_clientSocketConnection_t *self = (apx_clientSocketConnection_t*) arg;
#if APX_DEBUG_ENABLE
   printf("[CLIENT-SOCKET] Disconnected\n");
#endif
   apx_clientConnectionBase_disconnectedCbk(&self->base);
}

static void apx_clientSocketConnection_close(apx_clientSocketConnection_t *self)
{
   printf("client close\n");
}

static void apx_clientSocketConnection_vclose(void *arg)
{
   apx_clientSocketConnection_close((apx_clientSocketConnection_t*) arg);
}

static void apx_clientSocketConnection_start(apx_clientSocketConnection_t *self)
{
   apx_clientConnectionBase_start(&self->base);
}

static void apx_clientSocketConnection_vstart(void *arg)
{
   apx_clientSocketConnection_start((apx_clientSocketConnection_t*) arg);
}
