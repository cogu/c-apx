/*****************************************************************************
* \file      apx_fileManagerSharedSpy.h
* \author    Conny Gustafsson
* \date      2018-08-28
* \brief     Description
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
#ifndef APX_FILE_MANAGER_SHARED_SPY_H
#define APX_FILE_MANAGER_SHARED_SPY_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "rmf.h"
#include "apx_file2.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_fileManagerSharedSpy_tag
{
   int32_t numFileCreatedCalls;
   int32_t numSendFileInfoCalls;
   int32_t numSendFileOpenCalls;
   int32_t numOpenFileRequestCalls;
} apx_fileManagerSharedSpy_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_fileManagerSharedSpy_create(apx_fileManagerSharedSpy_t *self);
void apx_fileManagerSharedSpy_destroy(apx_fileManagerSharedSpy_t *self);
apx_fileManagerSharedSpy_t *apx_fileManagerSharedSpy_new(void);
void apx_fileManagerSharedSpy_delete(apx_fileManagerSharedSpy_t *self);
void apx_fileManagerSharedSpy_fileCreated(void *arg, const struct apx_file2_tag *pFile, void *caller);
void apx_fileManagerSharedSpy_sendFileInfo(void *arg, const struct apx_file2_tag *pFile);
void apx_fileManagerSharedSpy_sendFileOpen(void *arg, const apx_file2_t *file, void *caller);
void apx_fileManagerSharedSpy_openFileRequest(void *arg, uint32_t address);

#endif //APX_FILE_MANAGER_SHARED_SPY_H
