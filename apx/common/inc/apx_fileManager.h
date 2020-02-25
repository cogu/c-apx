/*****************************************************************************
* \file      apx_fileManager.h
* \author    Conny Gustafsson
* \date      2020-01-22
* \brief     APX file manager
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
#ifndef APX_FILE_MANAGER_H
#define APX_FILE_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "apx_file.h"
#include "apx_fileManagerDefs.h"
#include "apx_fileManagerShared.h"
#include "apx_fileManagerWorker.h"
#include "apx_fileManagerReceiver.h"
#include "adt_bytearray.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declaration
struct apx_connectionBase_tag;


typedef struct apx_fileManager_tag
{
   apx_fileManagerShared_t shared;
   apx_fileManagerWorker_t worker;
   apx_fileManagerReceiver_t receiver;
   struct apx_connectionBase_tag *parentConnection;
}apx_fileManager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManager_create(apx_fileManager_t *self, uint8_t mode, struct apx_connectionBase_tag *parentConnection);
void apx_fileManager_destroy(apx_fileManager_t *self);

void apx_fileManager_start(apx_fileManager_t *self);
void apx_fileManager_stop(apx_fileManager_t *self);


apx_file_t* apx_fileManager_findFileByAddress(apx_fileManager_t *self, uint32_t address);
apx_file_t* apx_fileManager_findRemoteFileByName(apx_fileManager_t *self, const char *name);
void apx_fileManager_setTransmitHandler(apx_fileManager_t *self, apx_transmitHandler_t *handler);
void apx_fileManager_copyTransmitHandler(apx_fileManager_t *self, apx_transmitHandler_t *handler);
void apx_fileManager_headerReceived(apx_fileManager_t *self);
void apx_fileManager_headerAccepted(apx_fileManager_t *self);
apx_error_t apx_fileManager_requestOpenFile(apx_fileManager_t *self, uint32_t address);
void apx_fileManager_setConnectionId(apx_fileManager_t *self, uint32_t connectionId);
int32_t apx_fileManager_getNumLocalFiles(apx_fileManager_t *self);
int32_t apx_fileManager_getNumRemoteFiles(apx_fileManager_t *self);

//Actions triggered by remote side
apx_error_t apx_fileManager_messageReceived(apx_fileManager_t *self, const uint8_t *msgBuf, int32_t msgLen);
apx_file_t *apx_fileManager_fileInfoNotify(apx_fileManager_t *self, const apx_fileInfo_t *fileInfo);

//Actions triggered on local side
apx_error_t apx_fileManager_writeConstData(apx_fileManager_t *self, uint32_t address, uint32_t len, apx_file_read_const_data_func *readFunc, void *arg);
apx_error_t apx_fileManager_writeDynamicData(apx_fileManager_t *self, uint32_t address, apx_size_t len, uint8_t *data);
apx_file_t *apx_fileManager_createLocalFile(apx_fileManager_t *self, const apx_fileInfo_t *fileInfo);
apx_error_t apx_fileManager_sendFileInfo(apx_fileManager_t *self, apx_fileInfo_t *fileInfo);
void apx_fileManager_disconnectNotify(apx_fileManager_t *self);

#ifdef UNIT_TEST
bool apx_fileManager_run(apx_fileManager_t *self);
int32_t apx_fileManager_numPendingMessages(apx_fileManager_t *self);
#endif

#endif //APX_FILE_MANAGER_H
