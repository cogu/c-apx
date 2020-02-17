/*****************************************************************************
* \file      apx_serverTestConnection.h
* \author    Conny Gustafsson
* \date      2018-12-09
* \brief     Description
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
#ifndef APX_SERVER_TEST_CONNECTION_H
#define APX_SERVER_TEST_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdbool.h>
#include "apx_error.h"
#include "apx_serverConnectionBase.h"
#include "adt_bytearray.h"
#include "adt_ary.h"
#include "apx_fileInfo.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_serverTestConnection_tag
{
   apx_serverConnectionBase_t base;
   adt_ary_t *transmitLog; //strong references to adt_bytearray_t
   adt_bytearray_t *pendingMsg; //strong reference to adt_byterray_t
}apx_serverTestConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_serverTestConnection_create(apx_serverTestConnection_t *self);
void apx_serverTestConnection_destroy(apx_serverTestConnection_t *self);
void apx_serverTestConnection_vdestroy(void *arg);
apx_serverTestConnection_t *apx_serverTestConnection_new(void);
void apx_serverTestConnection_delete(apx_serverTestConnection_t *self);
void apx_serverTestConnection_start(apx_serverTestConnection_t *self);
void apx_serverTestConnection_vstart(void *arg);
void apx_serverTestConnection_close(apx_serverTestConnection_t *self);
void apx_serverTestConnection_vclose(void *arg);

void apx_serverTestConnection_createRemoteFile(apx_serverTestConnection_t *self, const rmf_fileInfo_t *fileInfo);
void apx_serverTestConnection_writeRemoteData(apx_serverTestConnection_t *self, uint32_t address, const uint8_t* dataBuf, uint32_t dataLen, bool more);
void apx_serverTestConnection_openRemoteFile(apx_serverTestConnection_t *self, uint32_t address);
void apx_serverTestConnection_runEventLoop(apx_serverTestConnection_t *self);
int32_t apx_serverTestConnection_getTransmitLogLen(apx_serverTestConnection_t *self);
adt_bytearray_t *apx_serverTestConnection_getTransmitLogMsg(apx_serverTestConnection_t *self, int32_t index);
apx_error_t apx_serverTestConnection_onFileInfoMsgReceived(apx_serverTestConnection_t *self, const rmf_fileInfo_t *remoteFileInfo);
apx_error_t apx_serverTestConnection_onSerializedMsgReceived(apx_serverTestConnection_t *self, const uint8_t *msgBuf, int32_t msgLen);

#endif //APX_SERVER_TEST_CONNECTION_H
