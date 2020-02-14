/*****************************************************************************
* \file      apx_eventListener.h
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
#include "apx_types.h"

//forward declarations
struct apx_serverConnectionBase_tag;
struct apx_clientConnectionBase_tag;
struct apx_portConnectionTable_tag;
struct rmf_fileInfo_tag;
struct apx_fileInfo_tag;
struct apx_file_tag;
struct apx_connectionBase_tag;
struct apx_nodeInstance_tag;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef void (apx_eventListener2_portConnectFunc_t)(void *arg, struct apx_nodeInstance_tag *inst, struct apx_portConnectionTable_tag *connectionTable);
typedef void (apx_eventListener2_fileEvent_t)(void *arg, struct apx_connectionBase_tag *connection, const struct apx_fileInfo_tag *fileInfo);
typedef void (*remoteFilePreWriteFuncType1)(void *arg, struct apx_file_tag *remoteFile, uint32_t offset, const uint8_t *data, uint32_t len, bool moreBit);
typedef void (*remoteFileWriteFuncType1)(void *arg, struct apx_file_tag *remoteFile, uint32_t offset, const uint8_t *data, uint32_t len);


typedef struct apx_clientEventListener_tag
{
   void *arg;
   void (*clientConnect2)(void *arg, struct apx_clientConnectionBase_tag *clientConnection);
   void (*clientDisconnect2)(void *arg, struct apx_clientConnectionBase_tag *clientConnection);
} apx_clientEventListener_t;

typedef struct apx_serverEventListener_tag
{
   void *arg;
   void (*serverConnect2)(void *arg, struct apx_serverConnectionBase_tag *connection);
   void (*serverDisconnect2)(void *arg, struct apx_serverConnectionBase_tag *connection);
} apx_serverEventListener_t;

typedef struct apx_connectionEventListener_tag
{
   void *arg;
   void (*headerAccepted2)(void *arg, struct apx_connectionBase_tag *connection);
   apx_eventListener2_fileEvent_t *fileCreate2;
   apx_eventListener2_fileEvent_t *fileRevoke2;
   apx_eventListener2_fileEvent_t *fileOpen2;
   apx_eventListener2_fileEvent_t *fileClose2;
   remoteFilePreWriteFuncType1 *filePreWrite1;
   void (*nodeComplete2) (void *arg, struct apx_nodeInstance_tag *inst);
   apx_eventListener2_portConnectFunc_t *requirePortsConnected1;
   apx_eventListener2_portConnectFunc_t *requirePortsDisconnected1;
   apx_eventListener2_portConnectFunc_t *providePortsConnected1;
   apx_eventListener2_portConnectFunc_t *providePortsDisconnected1;
} apx_connectionEventListener_t;


typedef struct apx_fileEventListener2_tag
{
   remoteFileWriteFuncType1 remoteFileWrite1;
} apx_fileEventListener2_t;


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

apx_fileEventListener2_t *apx_fileEventListener_clone(apx_fileEventListener2_t *other);
void apx_fileEventListener_delete(apx_fileEventListener2_t *self);
void apx_fileEventListener_vdelete(void *arg);



#endif //APX_EVENT_LISTENER_H
