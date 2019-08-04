/*****************************************************************************
* \file      apx_fileManagerLocal.h
* \author    Conny Gustafsson
* \date      2018-08-02
* \brief     APX Filemanager local representation
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
#ifndef APX_FILE_MANAGER_LOCAL_H
#define APX_FILE_MANAGER_LOCAL_H

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
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_file2_tag;

typedef struct apx_fileManagerLocal_tag
{
   apx_fileManagerShared_t *shared;
   apx_fileMap_t localFileMap;
   MUTEX_T mutex;
} apx_fileManagerLocal_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_fileManagerLocal_create(apx_fileManagerLocal_t *self, apx_fileManagerShared_t *shared);
void apx_fileManagerLocal_destroy(apx_fileManagerLocal_t *self);
//void apx_fileManagerLocal_start(apx_fileManagerLocal_t *self);
//void apx_fileManagerLocal_stop(apx_fileManagerLocal_t *self);
void apx_fileManagerLocal_attachFile(apx_fileManagerLocal_t *self, struct apx_file2_tag *localFile, void *caller);
int32_t apx_fileManagerLocal_getNumFiles(apx_fileManagerLocal_t *self);
int32_t apx_fileManagerLocal_serializeFileInfo(apx_fileManagerLocal_t *self, uint8_t *bufData, int32_t bufLen, uint8_t headerSize);
void apx_fileManagerLocal_sendFileInfo(apx_fileManagerLocal_t *self);
struct apx_file2_tag *apx_fileManagerLocal_find(apx_fileManagerLocal_t *self, uint32_t address);
struct apx_file2_tag *apx_fileManagerLocal_findByName(apx_fileManagerLocal_t *self, const char *name);

#endif //APX_FILE_MANAGER_LOCAL_H
