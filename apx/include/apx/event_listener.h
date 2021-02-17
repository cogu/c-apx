/*****************************************************************************
* \file      apx_event_listener.h
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Event listener API
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
#ifndef APX_EVENT_LISTENER_H
#define APX_EVENT_LISTENER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"

//forward declarations
struct apx_serverConnection_tag;
struct apx_clientConnection_tag;
struct apx_portConnectionTable_tag;
struct rmf_fileInfo_tag;
struct apx_fileInfo_tag;
struct apx_file_tag;
struct apx_connectionBase_tag;
struct apx_nodeInstance_tag;
struct apx_portInstance_tag;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//typedef void (apx_eventListener2_portConnectFunc_t)(void *arg, struct apx_nodeInstance_tag *inst, struct apx_portConnectionTable_tag *connectionTable);
//typedef void (apx_eventListener2_fileEvent_t)(void *arg, struct apx_connectionBase_tag *connection, const struct apx_fileInfo_tag *fileInfo);
//typedef void (*remoteFilePreWriteFuncType1)(void *arg, struct apx_file_tag *remoteFile, uint32_t offset, const uint8_t *data, uint32_t len, bool moreBit);
//typedef void (*remoteFileWriteFuncType1)(void *arg, struct apx_file_tag *remoteFile, uint32_t offset, const uint8_t *data, uint32_t len);


//Client/Server typedefs
typedef void (apx_clientConnectionEventFunc_t)(void* arg, struct apx_clientConnection_tag* connection);
typedef void (apx_serverConnectionEventFunc_t)(void* arg, struct apx_serverConnection_tag* connection);
typedef void (apx_serverLogWritEventFunc_t)(void* arg, apx_logLevel_t level, const char* label, const char* msg);

//Connection typedefs
typedef void (apx_protocolHeaderAcceptedFunc_t)(void* arg, struct apx_connectionBase_tag* connection);
typedef void (apx_portDataWriteFunc1_t)(void* arg, struct apx_portInstance_tag* port_instance, uint8_t const* data, apx_size_t size);
typedef void (apx_fileEventFunc_t)(void* arg, struct apx_connectionBase_tag* connection, const struct rmf_fileInfo_tag* file_info);

typedef struct apx_clientEventListener_tag
{
   void *arg;
   apx_clientConnectionEventFunc_t* connected1;
   apx_clientConnectionEventFunc_t* disconnected1;
   apx_portDataWriteFunc1_t* require_port_write1;
} apx_clientEventListener_t;

typedef struct apx_serverEventListener_tag
{
   void *arg;
   apx_serverConnectionEventFunc_t* new_connection2;
   apx_serverConnectionEventFunc_t* connection_closed2;
   apx_serverLogWritEventFunc_t* server_write_log2;
} apx_serverEventListener_t;

typedef struct apx_connectionEventListener_tag
{
   void *arg;
   apx_protocolHeaderAcceptedFunc_t* protocol_header_accepted;
   apx_fileEventFunc_t* file_published;
   apx_fileEventFunc_t* file_revoked;
} apx_connectionEventListener_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_clientEventListener_t *apx_clientEventListener_clone(apx_clientEventListener_t *other);
void apx_clientEventListener_delete(apx_clientEventListener_t *self);
void apx_clientEventListener_vdelete(void *arg);

apx_serverEventListener_t *apx_serverEventListener_clone(apx_serverEventListener_t *other);
void apx_serverEventListener_delete(apx_serverEventListener_t *self);
void apx_serverEventListener_vdelete(void *arg);

apx_connectionEventListener_t *apx_connectionEventListener_clone(apx_connectionEventListener_t *other);
void apx_connectionEventListener_delete(apx_connectionEventListener_t *self);
void apx_connectionEventListener_vdelete(void *arg);



#endif //APX_EVENT_LISTENER_H
