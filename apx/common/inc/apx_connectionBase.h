/*****************************************************************************
* \file      apx_connectionBase.h
* \author    Conny Gustafsson
* \date      2018-12-09
* \brief     Base class for all connections (client and server)
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
#ifndef APX_CONNECTION_BASE_H
#define APX_CONNECTION_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "apx_fileManager.h"
#include "apx_nodeManager.h"
#include "apx_eventLoop.h"
#include "apx_allocator.h"
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
# include <semaphore.h>
#endif
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_nodeData_tag;
struct apx_portConnectionTable_tag;
struct apx_file_tag;
struct apx_fileInfo_tag;
struct apx_transmitHandler_tag;


typedef void (apx_fileInfoNotifyFunc)(void *arg, const struct apx_fileInfo_tag *fileInfo);
typedef apx_error_t (apx_fillTransmitHandlerFunc)(void *arg, struct apx_transmitHandler_tag *handler);
typedef void (apx_nodeFileWriteNotifyFunc)(void *arg, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);

typedef struct apx_connectionBaseVTable_tag
{
   apx_voidPtrFunc *destructor;
   apx_voidPtrFunc *start;
   apx_voidPtrFunc *close;
   apx_fileInfoNotifyFunc *fileInfoNotify;
   apx_nodeFileWriteNotifyFunc *nodeFileWriteNotify;
   apx_fillTransmitHandlerFunc *fillTransmitHandler;
} apx_connectionBaseVTable_t;

typedef struct apx_connectionBase_tag
{
   apx_fileManager_t fileManager;
   apx_nodeManager_t nodeManager;
   apx_eventLoop_t eventLoop;
   apx_allocator_t allocator;
   adt_list_t connectionEventListeners; //weak references to apx_connectionEventListener_t
   //adt_list_t fileEventListeners; //weak references to apx_fileEventListener_t
   MUTEX_T eventListenerMutex; //thread-protection for nodeDataEventListeners
   uint32_t connectionId;
   uint8_t numHeaderLen; //0, 2 or 4
   apx_connectionBaseVTable_t vtable;
   THREAD_T workerThread;
   bool workerThreadValid;
   apx_eventHandlerFunc_t *eventHandler;
   void *eventHandlerArg;
   uint32_t totalBytesReceived;
   uint32_t totalBytesSent;
   apx_mode_t mode;
} apx_connectionBase_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connectionBaseVTable_create(apx_connectionBaseVTable_t *self, apx_voidPtrFunc *destructor, apx_voidPtrFunc *start, apx_voidPtrFunc *close, apx_fillTransmitHandlerFunc *fillTransmitHandler);
apx_error_t apx_connectionBase_create(apx_connectionBase_t *self, apx_mode_t mode, apx_connectionBaseVTable_t *vtable);
void apx_connectionBase_destroy(apx_connectionBase_t *self);
void apx_connectionBase_delete(apx_connectionBase_t *self);
void apx_connectionBase_vdelete(void *arg);
apx_fileManager_t *apx_connectionBase_getFileManager(apx_connectionBase_t *self);
void apx_connectionBase_setEventHandler(apx_connectionBase_t *self, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg);
void apx_connectionBase_start(apx_connectionBase_t *self);
void apx_connectionBase_stop(apx_connectionBase_t *self);
void apx_connectionBase_close(apx_connectionBase_t *self);
void apx_connectionBase_attachNodeInstance(apx_connectionBase_t *self, apx_nodeInstance_t *nodeInstance);
apx_error_t apx_connectionBase_processMessage(apx_connectionBase_t *self, const uint8_t *msgBuf, int32_t msgLen);
uint8_t *apx_connectionBase_alloc(apx_connectionBase_t *self, size_t size);
void apx_connectionBase_free(apx_connectionBase_t *self, uint8_t *ptr, size_t size);


/*** Internal Callback API ***/

apx_error_t apx_connectionBase_fileInfoNotify(apx_connectionBase_t *self, const rmf_fileInfo_t *remoteFileInfo);
apx_error_t apx_connectionBase_fileOpenNotify(apx_connectionBase_t *self, uint32_t address);
apx_error_t apx_connectionBase_fileWriteNotify(apx_connectionBase_t *self, apx_file_t *file, uint32_t offset, const uint8_t *data, uint32_t len);
apx_error_t apx_connectionBase_nodeInstanceFileWriteNotify(apx_connectionBase_t *self, apx_nodeInstance_t *nodeInstance, apx_fileType_t fileType, uint32_t offset, const uint8_t *data, uint32_t len);

/*** Event triggering API ***/

void apx_connectionBase_triggerRemoteFileHeaderCompleteEvent(apx_connectionBase_t *self);

//void apx_connectionBase_emitFileManagerPreStartEvent(apx_connectionBase_t *self);
//void apx_connectionBase_emitFileManagerPostStopEvent(apx_connectionBase_t *self);
//void apx_connectionBase_emitFileCreatedEvent(apx_connectionBase_t *self, struct apx_file_tag *file, const void *caller);

void apx_connectionBase_emitFileRevokedEvent(apx_connectionBase_t *self, struct apx_file_tag *file, const void *caller);
void apx_connectionBase_emitFileOpenedEvent(apx_connectionBase_t *self, struct apx_file_tag *file, const void *caller);
void apx_connectionBase_emitRemoteFileWrittenType1(apx_connectionBase_t *self, struct apx_file_tag *remoteFile, struct apx_file_tag *file, uint32_t offset, const uint8_t *data, uint32_t len, bool moreBit);
void apx_connectionBase_emitNodeComplete(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData);
void apx_connectionBase_emitHeaderAccepted(apx_connectionBase_t *self);
void apx_connectionBase_emitGenericEvent(apx_connectionBase_t *self, apx_event_t *event);
void apx_connectionBase_defaultEventHandler(apx_connectionBase_t *self, apx_event_t *event);
void apx_connectionBase_setConnectionId(apx_connectionBase_t *self, uint32_t connectionId);
uint32_t apx_connectionBase_getConnectionId(apx_connectionBase_t *self);
void apx_connectionBase_getTransmitHandler(apx_connectionBase_t *self, apx_transmitHandler_t *transmitHandler);
uint16_t apx_connectionBase_getNumPendingEvents(apx_connectionBase_t *self);

/*** Event triggering API ***/

void* apx_connectionBase_registerEventListener(apx_connectionBase_t *self, apx_connectionEventListener_t *listener);
void apx_connectionBase_unregisterEventListener(apx_connectionBase_t *self, void *handle);

void apx_connectionBase_triggerDefinitionDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len);
void apx_connectionBase_triggerInPortDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len);
void apx_connectionBase_triggerOutPortDataWritten(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, uint32_t offset, uint32_t len);

void apx_connectionBase_triggerRequirePortsConnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable);
void apx_connectionBase_triggerProvidePortsConnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable);
void apx_connectionBase_triggerRequirePortsDisconnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable);
void apx_connectionBase_triggerProvidePortsDisconnected(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData, struct apx_portConnectionTable_tag *portConnectionTable);


/*** Event handler API ***/

void apx_connectionBase_onFileCreated(apx_connectionBase_t *self, apx_connectionBase_t *connection, struct apx_fileInfo_tag *fileInfo, void *caller);
void apx_connectionBase_onHeaderAccepted(apx_connectionBase_t *self, apx_connectionBase_t *connection);

#ifdef UNIT_TEST
void apx_connectionBase_runAll(apx_connectionBase_t *self);
#endif


#endif //APX_CONNECTION_BASE_H
