/*****************************************************************************
* \file      apx_fileManager.h
* \author    Conny Gustafsson
* \date      2018-08-04
* \brief     New APX file manager
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
#ifndef APX_FILE_MANAGER_H
#define APX_FILE_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_fileManagerDefs.h"
#include "apx_fileManagerShared.h"
#include "apx_fileManagerRemote.h"
#include "apx_fileManagerLocal.h"
#include "apx_transmitHandler.h"
#include "apx_error.h"
#include "apx_file2.h"
#include "apx_event.h"
#ifndef ADT_RBFS_ENABLE
#define ADT_RBFS_ENABLE 1
#endif
#include "adt_ringbuf.h"
#ifndef _WIN32
#include <semaphore.h>
#endif

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declaration
struct apx_fileManagerEventListener_tag;
struct apx_event_tag;
struct rmf_fileInfo_tag;
struct apx_connectionBase_tag;

typedef struct apx_fileManager_tag
{
   apx_fileManagerShared_t shared;
   apx_fileManagerRemote_t remote;
   apx_fileManagerLocal_t local;
   adt_list_t eventListeners; //contains strong references to apx_fileManagerEventListener_t
   apx_transmitHandler_t transmitHandler;
   struct apx_connectionBase_tag *parentConnection;
   MUTEX_T mutex; //for locking variables in this object
   MUTEX_T eventListenerMutex; //Specific lock just for eventListener list
   SPINLOCK_T lock; //used exclusively by workerThread message queue
   THREAD_T workerThread; //local transmit thread
   SEMAPHORE_T semaphore; //thread semaphore
   adt_rbfh_t messages; //pending transmit messages
   bool workerThreadValid; //Differences in Linux and Windows doesn't make it obvious if workerThread is valid without this flag
   int8_t headerSize;
   apx_mode_t mode;

#ifdef _WIN32
   unsigned int threadId;
#endif
}apx_fileManager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManager_create(apx_fileManager_t *self, uint8_t mode, struct apx_connectionBase_tag *parentConnection);
void apx_fileManager_destroy(apx_fileManager_t *self);

void* apx_fileManager_registerEventListener(apx_fileManager_t *self, struct apx_fileManagerEventListener_tag* listener);
void apx_fileManager_unregisterEventListener(apx_fileManager_t *self, void *handle);
int32_t apx_fileManager_getNumEventListeners(apx_fileManager_t *self);
void apx_fileManager_attachLocalFile(apx_fileManager_t *self, apx_file2_t *localFile, void *caller);
int32_t apx_fileManager_getNumLocalFiles(apx_fileManager_t *self);
bool apx_fileManager_isServerMode(apx_fileManager_t *self);
bool apx_fileManager_isClientMode(apx_fileManager_t *self);
void apx_fileManager_start(apx_fileManager_t *self);
void apx_fileManager_stop(apx_fileManager_t *self);
void apx_fileManager_onHeaderReceived(apx_fileManager_t *self); //used in server mode
void apx_fileManager_onHeaderAccepted(apx_fileManager_t *self); //used in client mode
int32_t apx_fileManager_processMessage(apx_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen);
uint32_t fileManager_getID(apx_fileManager_t *self);
void fileManager_setID(apx_fileManager_t *self, uint32_t fmid);
void apx_fileManager_setTransmitHandler(apx_fileManager_t *self, apx_transmitHandler_t *handler);
void apx_fileManager_getTransmitHandler(apx_fileManager_t *self, apx_transmitHandler_t *handler);
int8_t apx_fileManager_openRemoteFile(apx_fileManager_t *self, uint32_t address, void *caller);
void apx_fileManager_sendFileAlreadyExistsError(apx_fileManager_t *self, apx_file2_t *file);
apx_file2_t *apx_fileManager_findLocalFileByName(apx_fileManager_t *self, const char *name);
apx_file2_t *apx_fileManager_findRemoteFileByName(apx_fileManager_t *self, const char *name);
void apx_fileManager_sendApxErrorCode(apx_fileManager_t *self, uint32_t errorCode);
void apx_fileManager_eventHandler(apx_fileManager_t *self, struct apx_event_tag *event);

/*********** APX server internal API **************/
void apx_fileManager_onRemoteCmdFileInfo(apx_fileManager_t *self, const struct rmf_fileInfo_tag* fileInfo);
void apx_fileManager_onRemoteCmdFileOpen(apx_fileManager_t *self, uint32_t address);
void apx_fileManager_onWriteRemoteData(apx_fileManager_t *self, uint32_t address, const uint8_t *dataBuf, uint32_t dataLen, bool more);

/********* APX File Manager Events ****************/
void apx_fileManager_createPreStartEvent(apx_event_t *event, apx_fileManager_t *fileManager);
void apx_fileManager_createPostStopEvent(apx_event_t *event, apx_fileManager_t *fileManager);
void apx_fileManager_createHeaderCompleteEvent(apx_event_t *event, apx_fileManager_t *fileManager);
void apx_fileManager_createFileCreatedEvent(apx_event_t *event, apx_fileManager_t *fileManager, apx_file2_t *file, const void *caller);
void apx_fileManager_createFileRevokedEvent(apx_event_t *event, apx_fileManager_t *fileManager, apx_file2_t *file, const void *caller);
void apx_fileManager_createFileOpenedEvent(apx_event_t *event, apx_fileManager_t *fileManager, apx_file2_t *file, const void *caller);
void apx_fileManager_createFileClosedEvent(apx_event_t *event, apx_fileManager_t *fileManager, apx_file2_t *file, const void *caller);


#ifdef UNIT_TEST
bool apx_fileManager_run(apx_fileManager_t *self);
int32_t apx_fileManager_numPendingMessages(apx_fileManager_t *self);
#endif

#endif //APX_FILE_MANAGER_H
