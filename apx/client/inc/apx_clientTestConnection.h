/*****************************************************************************
* \file      apx_clientTestConnection.h
* \author    Conny Gustafsson
* \date      2018-01-15
* \brief     Test connection for APX clients
*
* Copyright (c) 2019 Conny Gustafsson
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
#ifndef APX_CLIENT_TEST_CONNECTION_H
#define APX_CLIENT_TEST_CONNECTION_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdbool.h>
#include "apx_error.h"
#include "apx_clientConnectionBase.h"
#include "rmf.h"
#include "adt_bytearray.h"
#include "adt_ary.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_client_tag;

typedef struct apx_clientTestConnection_tag
{
   apx_clientConnectionBase_t base;
   adt_ary_t *transmitLog; //strong references to adt_bytearray_t
   adt_bytearray_t *pendingMsg; //strong reference to adt_byterray_t
}apx_clientTestConnection_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_clientTestConnection_create(apx_clientTestConnection_t *self);
void apx_clientTestConnection_destroy(apx_clientTestConnection_t *self);
void apx_clientTestConnection_vdestroy(void *arg);
apx_clientTestConnection_t *apx_clientTestConnection_new(void);
void apx_clientTestConnection_delete(apx_clientTestConnection_t *self);
void apx_clientTestConnection_start(apx_clientTestConnection_t *self);
void apx_clientTestConnection_vstart(void *arg);
void apx_clientTestConnection_close(apx_clientTestConnection_t *self);
void apx_clientTestConnection_vclose(void *arg);

void apx_clientTestConnection_createRemoteFile(apx_clientTestConnection_t *self, const rmf_fileInfo_t *fileInfo);
void apx_clientTestConnection_writeRemoteData(apx_clientTestConnection_t *self, uint32_t address, const uint8_t* dataBuf, uint32_t dataLen, bool more);
void apx_clientTestConnection_openRemoteFile(apx_clientTestConnection_t *self, uint32_t address);
void apx_clientTestConnection_runEventLoop(apx_clientTestConnection_t *self);
void apx_clientTestConnection_connect(apx_clientTestConnection_t *self);
void apx_clientTestConnection_disconnect(apx_clientTestConnection_t *self);
void apx_clientTestConnection_headerAccepted(apx_clientTestConnection_t *self);
int32_t apx_clientTestConnection_getTransmitLogLen(apx_clientTestConnection_t *self);
adt_bytearray_t *apx_clientTestConnection_getTransmitLogMsg(apx_clientTestConnection_t *self, int32_t index);
void apx_clientTestConnection_clearTransmitLog(apx_clientTestConnection_t *self);

apx_error_t apx_clientTestConnection_onFileOpenMsgReceived(apx_clientTestConnection_t *self, const rmf_cmdOpenFile_t *openFileCmd);


#endif //APX_CLIENT_TEST_CONNECTION_H
