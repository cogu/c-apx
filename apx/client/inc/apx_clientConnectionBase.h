/*****************************************************************************
* \file      apx_clientConnectionBase.h
* \author    Conny Gustafsson
* \date      2018-12-31
* \brief     Base class for client connections
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
#ifndef APX_CLIENT_CONNECTION_BASE_H
#define APX_CLIENT_CONNECTION_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_connectionBase.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
struct apx_client_tag;
struct apx_nodeInstance_tag;

typedef struct apx_clientConnectionBase_tag
{
   apx_connectionBase_t base;
   struct apx_client_tag *client;
   bool isAcknowledgeSeen;
}apx_clientConnectionBase_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientConnectionBase_create(apx_clientConnectionBase_t *self, apx_connectionBaseVTable_t *vtable);
void apx_clientConnectionBase_destroy(apx_clientConnectionBase_t *self);
//This class has no delete functions as it is an abstract base class

apx_fileManager2_t *apx_clientConnectionBase_getFileManager(apx_clientConnectionBase_t *self);
void apx_clientConnectionBase_connectedCbk(apx_clientConnectionBase_t *self);
void apx_clientConnectionBase_disconnectedCbk(apx_clientConnectionBase_t *self);
int8_t apx_clientConnectionBase_onDataReceived(apx_clientConnectionBase_t *self, const uint8_t *dataBuf, uint32_t dataLen, uint32_t *parseLen);
void apx_clientConnectionBase_start(apx_clientConnectionBase_t *self);
void apx_clientConnectionBase_defaultEventHandler(void *arg, apx_event_t *event);
void apx_clientConnectionBase_close(apx_clientConnectionBase_t *self);
uint32_t apx_clientConnectionBase_getTotalBytesReceived(apx_clientConnectionBase_t *self);
uint32_t apx_clientConnectionBase_getTotalBytesSent(apx_clientConnectionBase_t *self);
void* apx_clientConnectionBase_registerEventListener(apx_clientConnectionBase_t *self, apx_connectionEventListener_t *listener);
void apx_clientConnectionBase_unregisterEventListener(apx_clientConnectionBase_t *self, void *handle);
void apx_clientConnectionBase_attachNodeInstance(apx_clientConnectionBase_t *self, struct apx_nodeInstance_tag *nodeInstance);

// Internal API
void apx_clientConnectionBaseInternal_headerAccepted(apx_clientConnectionBase_t *self);
apx_error_t apx_clientConnectionBaseInternal_onFileOpenMsgReceived(apx_clientConnectionBase_t *self, const rmf_cmdOpenFile_t *openFileCmd);

// Unit Test API
#ifdef UNIT_TEST
void apx_clientConnectionBase_run(apx_clientConnectionBase_t *self);
#endif


#endif //APX_CLIENT_CONNECTION_BASE_H
