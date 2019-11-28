/*****************************************************************************
* \file      apx_connectionEventSpy.h
* \author    Conny Gustafsson
* \date      2018-08-21
* \brief     Test spy for apx_fileManagerEventListener
*
* Copyright (c) 2018-2020 Conny Gustafsson
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
#ifndef APX_FILEMANAGER_EVENT_LISTENER_SPY_H
#define APX_FILEMANAGER_EVENT_LISTENER_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_eventListener2.h"
#include "apx_fileInfo.h"
#include "apx_connectionBase.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_connectionEventSpy_tag
{
   int32_t headerAcceptedCount;
   int32_t fileCreateCount;
   apx_connectionBase_t *lastConnection;
   apx_fileInfo_t *lastFileInfo;
}apx_connectionEventSpy_t;
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_connectionEventSpy_create(apx_connectionEventSpy_t *self);
void apx_connectionEventSpy_destroy(apx_connectionEventSpy_t *self);
void apx_connectionEventSpy_register(apx_connectionEventSpy_t *self, apx_connectionBase_t *connection);

void apx_connectionEventSpy_headerAccepted(void *arg, apx_connectionBase_t *connection);
void apx_connectionEventSpy_fileCreate(void *arg, apx_connectionBase_t *connection, const apx_fileInfo_t *fileInfo);



#endif //APX_FILEMANAGER_EVENT_LISTENER_SPY_H
