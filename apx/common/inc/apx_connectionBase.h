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
#include "apx_nodeDataManager.h"
#include "apx_eventLoop.h"
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

typedef struct apx_connectionBaseVTable_tag
{
   void (*destructor)(void *arg);
   void (*start)(void *arg);
   void (*close)(void *arg);
} apx_connectionBaseVTable_t;

typedef struct apx_connectionBase_tag
{
   apx_fileManager_t fileManager;
   apx_nodeDataManager_t nodeDataManager;
   apx_eventLoop_t eventLoop;
   uint32_t connectionId;
   uint8_t numHeaderLen; //0, 2 or 4
   apx_connectionBaseVTable_t vtable;
   THREAD_T workerThread;
   bool workerThreadValid;
   apx_eventHandlerFunc_t *eventHandler;
   void *eventHandlerArg;
   uint32_t totalBytesReceived;
   uint32_t totalBytesSent;
}apx_connectionBase_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connectionBaseVTable_create(apx_connectionBaseVTable_t *self, void (*destructor)(void *arg), void (*start)(void *arg), void (*close)(void *arg));
apx_error_t apx_connectionBase_create(apx_connectionBase_t *self, apx_mode_t mode, apx_connectionBaseVTable_t *vtable);
void apx_connectionBase_destroy(apx_connectionBase_t *self);
void apx_connectionBase_delete(apx_connectionBase_t *self);
void apx_connectionBase_vdelete(void *arg);
apx_fileManager_t *apx_connectionBase_getFileManager(apx_connectionBase_t *self);
void apx_connectionBase_setEventHandler(apx_connectionBase_t *self, apx_eventHandlerFunc_t *eventHandler, void *eventHandlerArg);
void apx_connectionBase_start(apx_connectionBase_t *self);
void apx_connectionBase_stop(apx_connectionBase_t *self);
void apx_connectionBase_close(apx_connectionBase_t *self);

void apx_connectionBase_emitFileManagerPreStartEvent(apx_connectionBase_t *self);
void apx_connectionBase_emitFileManagerPostStopEvent(apx_connectionBase_t *self);
void apx_connectionBase_emitFileManagerHeaderCompleteEvent(apx_connectionBase_t *self);
void apx_connectionBase_emitFileCreatedEvent(apx_connectionBase_t *self, struct apx_file2_tag *file, const void *caller);
void apx_connectionBase_emitFileRevokedEvent(apx_connectionBase_t *self, struct apx_file2_tag *file, const void *caller);
void apx_connectionBase_emitFileOpenedEvent(apx_connectionBase_t *self, struct apx_file2_tag *file, const void *caller);
void apx_connectionBase_emitNodeComplete(apx_connectionBase_t *self, struct apx_nodeData_tag *nodeData);
void apx_connectionBase_emitGenericEvent(apx_connectionBase_t *self, apx_event_t *event);
void apx_connectionBase_defaultEventHandler(apx_connectionBase_t *self, apx_event_t *event);
void apx_connectionBase_setConnectionId(apx_connectionBase_t *self, uint32_t connectionId);
uint32_t apx_connectionBase_getConnectionId(apx_connectionBase_t *self);
void apx_connectionBase_getTransmitHandler(apx_connectionBase_t *self, apx_transmitHandler_t *transmitHandler);
uint16_t apx_connectionBase_getNumPendingEvents(apx_connectionBase_t *self);

#ifdef UNIT_TEST
void apx_connectionBase_runAll(apx_connectionBase_t *self);
#endif


#endif //APX_CONNECTION_BASE_H
