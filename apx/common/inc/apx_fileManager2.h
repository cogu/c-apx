/*****************************************************************************
* \file      apx_fileManager2.h
* \author    Conny Gustafsson
* \date      2020-01-22
* \brief     New APX file manager
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
#ifndef APX_FILE_MANAGER2_H
#define APX_FILE_MANAGER2_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "apx_file2.h"
#include "apx_fileManagerDefs.h"
#include "apx_fileManagerShared2.h"
#include "apx_fileManagerWorker.h"
#include "apx_fileManagerReceiver.h"
#include "adt_bytearray.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declaration
struct apx_connectionBase_tag;


typedef struct apx_fileManager2_tag
{
   apx_fileManagerShared2_t shared;
   apx_fileManagerWorker_t worker;
   apx_fileManagerReceiver_t receiver;
   struct apx_connectionBase_tag *parentConnection;
}apx_fileManager2_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManager2_create(apx_fileManager2_t *self, uint8_t mode, struct apx_connectionBase_tag *parentConnection);
void apx_fileManager2_destroy(apx_fileManager2_t *self);

void apx_fileManager2_start(apx_fileManager2_t *self);
void apx_fileManager2_stop(apx_fileManager2_t *self);


apx_file2_t* apx_fileManager2_findFileByAddress(apx_fileManager2_t *self, uint32_t address);
void apx_fileManager2_setTransmitHandler(apx_fileManager2_t *self, apx_transmitHandler_t *handler);
void apx_fileManager2_copyTransmitHandler(apx_fileManager2_t *self, apx_transmitHandler_t *handler);
void apx_fileManager2_headerReceived(apx_fileManager2_t *self);
void apx_fileManager2_headerAccepted(apx_fileManager2_t *self);
apx_error_t apx_fileManager2_requestOpenFile(apx_fileManager2_t *self, uint32_t address);
void apx_fileManager2_setConnectionId(apx_fileManager2_t *self, uint32_t connectionId);
int32_t apx_fileManager2_getNumLocalFiles(apx_fileManager2_t *self);
int32_t apx_fileManager2_getNumRemoteFiles(apx_fileManager2_t *self);
apx_file2_t *apx_fileManager2_createLocalFile(apx_fileManager2_t *self, const apx_fileInfo_t *fileInfo);
apx_file2_t *apx_fileManager2_createRemoteFile(apx_fileManager2_t *self, const apx_fileInfo_t *fileInfo);

//Remote actions
apx_error_t apx_fileManager2_onFileOpenNotify(apx_fileManager2_t *self, uint32_t address);

//Local actions
apx_error_t apx_fileManager2_writeConstData(apx_fileManager2_t *self, uint32_t address, uint32_t len, apx_file_read_const_data_func *readFunc, void *arg);

#ifdef UNIT_TEST
bool apx_fileManager2_run(apx_fileManager2_t *self);
int32_t apx_fileManager2_numPendingMessages(apx_fileManager2_t *self);
#endif

#endif //APX_FILE_MANAGER_H
