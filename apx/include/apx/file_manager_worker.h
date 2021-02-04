/*****************************************************************************
* \file      file_manager_worker.h
* \author    Conny Gustafsson
* \date      2020-01-22
* \brief     APX file manager worker
*
* Copyright (c) 2020-2021 Conny Gustafsson
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
#include "apx/file_manager_defs.h"
#include "apx/file_manager_shared.h"
#include "apx/error.h"
#include "apx/file.h"
#include "apx/event.h"
#include "apx/command.h"
#include "apx/file_info.h"
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

typedef struct apx_fileManagerWorker_tag
{
   apx_fileManagerShared_t *shared; //weak reference
   MUTEX_T mutex; //for locking variables in this object
   SPINLOCK_T queue_lock; //used exclusively by workerThread command queue
   THREAD_T worker_thread; //local transmit thread
   SEMAPHORE_T semaphore; //queue semaphore
   adt_rbfh_t queue; //pending actions
   bool worker_thread_valid; //is worker_thread handle valid (required to support both Windows and Linux)
   apx_mode_t mode; //server or client mode?
#ifdef _WIN32
   unsigned int worker_thread_id;
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
uint16_t apx_fileManagerWorker_num_pending_commands(apx_fileManagerWorker_t* self);
#ifdef UNIT_TEST
bool apx_fileManagerWorker_run(apx_fileManagerWorker_t* self);
#else
apx_error_t apx_fileManagerWorker_start(apx_fileManagerWorker_t* self);
void apx_fileManagerWorker_stop(apx_fileManagerWorker_t* self);
#endif

//Command API
apx_error_t apx_fileManagerWorker_preare_acknowledge(apx_fileManagerWorker_t* self);
apx_error_t apx_fileManagerWorker_prepare_publish_local_file(apx_fileManagerWorker_t* self, rmf_fileInfo_t* file_info); //ownership is taken of the file_info object
apx_error_t apx_fileManagerWorker_prepare_send_local_const_data(apx_fileManagerWorker_t* self, uint32_t address, uint8_t const* data, uint32_t size);
apx_error_t apx_fileManagerWorker_prepare_send_local_data(apx_fileManagerWorker_t* self, uint32_t address, uint8_t* data, uint32_t size);
apx_error_t apx_fileManagerWorker_prepare_send_open_file_request(apx_fileManagerWorker_t* self, uint32_t address);

#endif //APX_FILE_MANAGER_WORKER_H
