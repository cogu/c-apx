/*****************************************************************************
* \file      apx_fileManagerEventListenerSpy.h
* \author    Conny Gustafsson
* \date      2018-08-21
* \brief     Test spy for apx_fileManagerEventListener
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
#ifndef APX_FILEMANAGER_EVENT_LISTENER_SPY_H
#define APX_FILEMANAGER_EVENT_LISTENER_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_eventListener.h"
#include "apx_fileManager.h"
#include "apx_file2.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_fileManagerEventListenerSpy_tag
{
   int32_t numfileCreateCalls;
   apx_fileManager_t *lastFileManager;
   const apx_file2_t *lastFile;
}apx_fileManagerEventListenerSpy_t;
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_fileManagerEventListenerSpy_create(apx_fileManagerEventListenerSpy_t *self);

void apx_fileManagerEventListenerSpy_fileCreate(void *arg, struct apx_fileManager_tag *fileManager, struct apx_file2_tag *file);


#endif //APX_FILEMANAGER_EVENT_LISTENER_SPY_H
