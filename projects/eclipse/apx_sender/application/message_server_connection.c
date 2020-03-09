/*****************************************************************************
* \file      message_server_connection.c
* \author    Conny Gustafsson
* \date      2020-03-07
* \brief     Connection in the message server
*
* Copyright (c) 2020 Conny Gustafsson
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
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include "message_server_connection.h"
#include "message_server.h"
#include "numheader.h"
#include "dtl_json.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void message_server_connection_disconnected(void *arg);
static int8_t message_server_connection_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen); //return 0 on success, -1 on failure (this will force the socket to close)
static void message_server_connection_process_message(message_server_connection_t *self, const uint8_t *pBegin, const uint8_t *pEnd);
static void message_server_connection_process_hash_value(message_server_connection_t *self, dtl_hv_t *hv);
//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void message_server_connection_create(message_server_connection_t *self, msocket_t *msocket, apx_send_connection_t *apx_connection)
{
   if ( (self != 0) && (msocket != 0) && (apx_connection != 0) )
   {
      msocket_handler_t handler;
      self->msocket = msocket;
      self->apx_connection = apx_connection;
      memset(&handler, 0, sizeof(msocket_handler_t));
      handler.tcp_data = message_server_connection_data;
      handler.tcp_disconnected = message_server_connection_disconnected;
      msocket_sethandler(self->msocket, &handler, (void*) self);
   }
}

void message_server_connection_destroy(message_server_connection_t *self)
{
   if (self != 0)
   {
      msocket_delete(self->msocket);
   }
}

message_server_connection_t *message_server_connection_new(msocket_t *msocket, apx_send_connection_t *apx_connection)
{
   message_server_connection_t *self = (message_server_connection_t*) malloc(sizeof(message_server_connection_t));
   if (self != 0)
   {
      message_server_connection_create(self, msocket, apx_connection);
   }
   return self;
}

void message_server_connection_delete(message_server_connection_t *self)
{
   if (self != 0)
   {
      message_server_connection_destroy(self);
      free(self);
   }
}

void message_server_connection_vdelete(void *arg)
{
   message_server_connection_delete((message_server_connection_t*) arg);
}

void message_server_connection_start(message_server_connection_t *self)
{
   if (self != 0)
   {
      msocket_start_io(self->msocket);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void message_server_connection_disconnected(void *arg)
{
   message_server_connection_t *self = (message_server_connection_t*) arg;
   if (self != 0)
   {
      message_server_cleanup_connection(self);
   }
}

//return 0 on success, -1 on failure (this will force the socket to close)
static int8_t message_server_connection_data(void *arg, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen)
{
   message_server_connection_t *self = (message_server_connection_t*) arg;
   if (self != 0)
   {
      const uint8_t *pResult;
      const uint8_t *pEnd = dataBuf + dataLen;
      assert(parseLen != 0);
      uint32_t msgSize = 0u;
      *parseLen = 0;
      pResult = numheader_decode32(dataBuf, pEnd, &msgSize);
      if ( (pResult > dataBuf)  )
      {
         const uint8_t *pNext = pResult;
         if (pNext + msgSize <= pEnd)
         {
            message_server_connection_process_message(self, pNext, pNext+msgSize);
            pNext += msgSize;
            *parseLen = (pNext - dataBuf);
         }
      }
      return 0;
   }
   return -1;
}

static void message_server_connection_process_message(message_server_connection_t *self, const uint8_t *pBegin, const uint8_t *pEnd)
{
   dtl_dv_t *dv = dtl_json_load_bstr( pBegin, pEnd);
   if (dv != 0)
   {
      if (dtl_dv_type(dv) == DTL_DV_HASH)
      {
         message_server_connection_process_hash_value(self, (dtl_hv_t*) dv);
      }
      dtl_dv_dec_ref(dv);
   }
}

static void message_server_connection_process_hash_value(message_server_connection_t *self, dtl_hv_t *hv)
{
   const char *key;
   dtl_dv_t *dv;
   assert(self != 0);
   assert(hv != 0);
   dtl_hv_iter_init(hv);
   dv = dtl_hv_iter_next_cstr(hv, &key);
   while(dv != 0)
   {
      apx_error_t result;
      if (self->apx_connection != 0)
      {
         result = apx_send_connection_writeProvidePortData(self->apx_connection, key, dv);
      }
      else
      {
         result = APX_NULL_PTR_ERROR;
      }
      if (result != APX_NO_ERROR)
      {
         printf("%s: Write failed for signal with error code %d\n", key, (int) result);
      }
      dv = dtl_hv_iter_next_cstr(hv, &key);
   }
}
