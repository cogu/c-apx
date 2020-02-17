/*****************************************************************************
* \file      apx_serverConnectionBase.h
* \author    Conny Gustafsson
* \date      2018-09-26
* \brief     Description
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#ifndef APX_SERVER_CONNECTION_BASE_H
#define APX_SERVER_CONNECTION_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_connectionBase.h"
#include "adt_list.h"
#include "adt_str.h"
#include "apx_eventListener.h"
#ifndef _WIN32
#include <pthread.h>
#else
#include <Windows.h>
#endif
#include "osmacro.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_server_tag;

typedef struct apx_serverConnectionBase_tag
{
   apx_connectionBase_t base;
   struct apx_server_tag *server; //parent object
   bool isGreetingParsed;
   bool isActive;
   adt_str_t *tag; //optional tag
}apx_serverConnectionBase_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverConnectionBase_create(apx_serverConnectionBase_t *self, apx_connectionBaseVTable_t *vtable);
void apx_serverConnectionBase_destroy(apx_serverConnectionBase_t *self);
//This class has no delete functions as it is an abstract base class

apx_fileManager_t *apx_serverConnectionBase_getFileManager(apx_serverConnectionBase_t *self);
int8_t apx_serverConnectionBase_dataReceived(apx_serverConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
void apx_serverConnectionBase_start(apx_serverConnectionBase_t *self);
void apx_serverConnectionBase_defaultEventHandler(void *arg, apx_event_t *event);
void apx_serverConnectionBase_setConnectionId(apx_serverConnectionBase_t *self, uint32_t connectionId);
void apx_serverConnectionBase_setServer(apx_serverConnectionBase_t *self, struct apx_server_tag *server);
uint32_t apx_serverConnectionBase_getConnectionId(apx_serverConnectionBase_t *self);
void apx_serverConnectionBase_close(apx_serverConnectionBase_t *self);
void apx_serverConnectionBase_detachNodes(apx_serverConnectionBase_t *self);
uint32_t apx_serverConnectionBase_getTotalPortReferences(apx_serverConnectionBase_t *self);
//void* apx_serverConnectionBase_registerNodeDataEventListener(apx_serverConnectionBase_t *self, apx_nodeDataEventListener_t *listener);
//void apx_serverConnectionBase_unregisterNodeDataEventListener(apx_serverConnectionBase_t *self, void *handle);

void* apx_serverConnectionBase_registerEventListener(apx_serverConnectionBase_t *self, apx_connectionEventListener_t *listener);
void apx_serverConnectionBase_unregisterEventListener(apx_serverConnectionBase_t *self, void *handle);

/*** Internal API (used only internally and by test classes) ***/

void apx_serverConnectionBase_onRemoteFileHeaderReceived(apx_serverConnectionBase_t *self);
apx_error_t apx_serverConnectionBase_fileInfoNotify(apx_serverConnectionBase_t *self, const rmf_fileInfo_t *remoteFileInfo);
void apx_serverConnectionBase_nodeInstanceFileWriteNotify(apx_serverConnectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);
void apx_serverConnectionBase_vnodeInstanceFileWriteNotify(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);



/*** UNIT TEST API ***/
#ifdef UNIT_TEST
void apx_serverConnectionBase_run(apx_serverConnectionBase_t *self);
#endif

#endif //APX_SERVER_CONNECTION_BASE_H
