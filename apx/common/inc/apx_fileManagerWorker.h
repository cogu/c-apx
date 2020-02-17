/*****************************************************************************
* \file      apx_fileManagerWorker.h
* \author    Conny Gustafsson
* \date      2020-01-22
* \brief     APX file manager worker
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
#ifndef APX_FILE_MANAGER_WORKER_H
#define APX_FILE_MANAGER_WORKER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_fileManagerDefs.h"
#include "apx_fileManagerShared.h"
#include "apx_transmitHandler.h"
#include "apx_error.h"
#include "apx_file.h"
#include "apx_event.h"
#include "apx_msg.h"
#ifndef ADT_RBFS_ENABLE
#define ADT_RBFS_ENABLE 1
#endif
#include "adt_ringbuf.h"
#include "rmf.h"
#ifndef _WIN32
#include <semaphore.h>
#endif

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declaration

typedef struct apx_fileManagerWorker_tag
{
   apx_fileManagerShared_t *shared; //weak reference (do not delete on destruction)
   MUTEX_T mutex; //for locking variables in this object
   SPINLOCK_T lock; //used exclusively by workerThread message queue
   THREAD_T workerThread; //local transmit thread
   SEMAPHORE_T semaphore; //thread semaphore
   adt_rbfh_t messages; //pending actions
   bool workerThreadValid; //Differences in Linux and Windows doesn't make it obvious if workerThread is valid without this flag
   apx_transmitHandler_t transmitHandler;
   int8_t numHeaderSize; //Number of bits used in numHeader (16 or 32)
   apx_mode_t mode; //server or client mode?
#ifdef _WIN32
   unsigned int threadId;
#endif
} apx_fileManagerWorker_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManagerWorker_create(apx_fileManagerWorker_t *self, apx_fileManagerShared_t *shared, apx_mode_t mode);
void apx_fileManagerWorker_destroy(apx_fileManagerWorker_t *self);
apx_error_t apx_fileManagerWorker_start(apx_fileManagerWorker_t *self);
void apx_fileManagerWorker_stop(apx_fileManagerWorker_t *self);

//Direct API
void apx_fileManagerWorker_setTransmitHandler(apx_fileManagerWorker_t *self, apx_transmitHandler_t *handler);
void apx_fileManagerWorker_copyTransmitHandler(apx_fileManagerWorker_t *self, apx_transmitHandler_t *handler);
void apx_fileManagerWorker_setNumHeaderSize(apx_fileManagerWorker_t *self, uint8_t bits);

//Message API
void apx_fileManagerWorker_sendFileInfoMsg(apx_fileManagerWorker_t *self, apx_fileInfo_t *fileInfo);
void apx_fileManagerWorker_sendFileOpenMsg(apx_fileManagerWorker_t *self, uint32_t address);
apx_error_t apx_fileManagerWorker_sendHeaderAckMsg(apx_fileManagerWorker_t *self);
apx_error_t apx_fileManagerWorker_sendConstData(apx_fileManagerWorker_t *self, uint32_t address, uint32_t len, apx_file_read_const_data_func *readFunc, void *arg);
apx_error_t apx_fileManagerWorker_sendDynamicData(apx_fileManagerWorker_t *self, uint32_t address, uint32_t len, uint8_t *data);

//UNIT TEST API
#ifdef UNIT_TEST
bool apx_fileManagerWorker_run(apx_fileManagerWorker_t *self);
int32_t apx_fileManagerWorker_numPendingMessages(apx_fileManagerWorker_t *self);
#endif

#endif //APX_FILE_MANAGER_WORKER_H
