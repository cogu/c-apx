/*****************************************************************************
* \file      apx_fileManagerShared.h
* \author    Conny Gustafsson
* \date      2020-01-23
* \brief     APX Filemanager shared data
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
#ifndef APX_FILE_MANAGER_SHARED_H
#define APX_FILE_MANAGER_SHARED_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_allocator.h"
#include "apx_fileMap.h"
#include "apx_fileManagerDefs.h"
#include "apx_file.h"
#include "apx_fileInfo.h"
#include "apx_error.h"
#include "rmf.h"
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
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_file_tag;
struct rmf_fileInfo_tag;

typedef struct apx_fileManagerShared_tag
{
   void *arg;
   apx_allocator_t allocator;
   apx_fileMap_t localFileMap;
   apx_fileMap_t remoteFileMap;
   uint32_t connectionId;
   SPINLOCK_T lock;
   void (*remoteFileCreated)(void *arg, apx_file_t *remoteFile);
   void (*fileOpenRequested)(void *arg, apx_file_t *localFile);
   void (*remoteFileWritten)(void *arg, apx_file_t *remoteFile, uint32_t offset, const uint8_t *msgData, uint32_t msgLen, bool moreBit);
} apx_fileManagerShared_t;


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManagerShared_create(apx_fileManagerShared_t *self);
void apx_fileManagerShared_destroy(apx_fileManagerShared_t *self);
uint8_t *apx_fileManagerShared_alloc(apx_fileManagerShared_t *self, size_t size);
void apx_fileManagerShared_free(apx_fileManagerShared_t *self, uint8_t *ptr, size_t size);
apx_file_t *apx_fileManagerShared_createLocalFile(apx_fileManagerShared_t *self, const apx_fileInfo_t *fileInfo);
apx_file_t *apx_fileManagerShared_createRemoteFile(apx_fileManagerShared_t *self, const apx_fileInfo_t *fileInfo);
int32_t apx_fileManagerShared_getNumLocalFiles(apx_fileManagerShared_t *self);
int32_t apx_fileManagerShared_getNumRemoteFiles(apx_fileManagerShared_t *self);
apx_file_t *apx_fileManagerShared_findLocalFileByName(apx_fileManagerShared_t *self, const char *name);
apx_file_t *apx_fileManagerShared_findRemoteFileByName(apx_fileManagerShared_t *self, const char *name);
apx_file_t *apx_fileManagerShared_findFileByAddress(apx_fileManagerShared_t *self, uint32_t address);
void apx_fileManagerShared_setConnectionId(apx_fileManagerShared_t *self, uint32_t connectionId);
uint32_t apx_fileManagerShared_getConnectionId(const apx_fileManagerShared_t *self);
adt_ary_t *apx_fileManagerShared_getLocalFileList(apx_fileManagerShared_t *self);


#endif //APX_FILE_MANAGER_SHARED_H
