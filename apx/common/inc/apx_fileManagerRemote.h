/*****************************************************************************
* \file      apx_fileManagerRemote.h
* \author    Conny Gustafsson
* \date      2018-08-02
* \brief     APX remote side representative
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
#ifndef APX_FILEMANAGER_REMOTE_H
#define APX_FILEMANAGER_REMOTE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_fileManagerShared.h"
#include "apx_fileMap.h"
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "osmacro.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_fileManagerRemote_tag
{
   apx_fileManagerShared_t *shared; //weak reference (do not delete on destruction)
   apx_fileMap_t remoteFileMap;
   MUTEX_T mutex;
} apx_fileManagerRemote_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_fileManagerRemote_create(apx_fileManagerRemote_t *self, apx_fileManagerShared_t *shared);
void apx_fileManagerRemote_destroy(apx_fileManagerRemote_t *self);
int32_t apx_fileManagerRemote_processMessage(apx_fileManagerRemote_t *self, const uint8_t *msgBuf, int32_t msgLen);
int8_t apx_fileManageRemote_openFile(apx_fileManagerRemote_t *self, uint32_t address, void *caller);
struct apx_file2_tag *apx_fileManagerRemote_findByName(apx_fileManagerRemote_t *self, const char *name);

void apx_fileManagerRemote_processFileInfo(apx_fileManagerRemote_t *self, const rmf_fileInfo_t *cmdFileInfo);
void apx_fileManagerRemote_processCmdMsg(apx_fileManagerRemote_t *self, const uint8_t *msgBuf, int32_t msgLen);
void apx_fileManagerRemote_processDataMsg(apx_fileManagerRemote_t *self, uint32_t address, const uint8_t *msgBuf, int32_t msgLen, bool more_bit);


#endif //APX_FILEMANAGER_REMOTE_H
