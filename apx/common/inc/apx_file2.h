/*****************************************************************************
* \file      apx_file2.h
* \author    Conny Gustafsson
* \date      2018-08-30
* \brief     Improved version of apx_file
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
#ifndef APX_FILE2_H
#define APX_FILE2_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_types.h"
#include "apx_error.h"
#include "apx_fileInfo.h"
#include "apx_eventListener2.h"
#include "adt_list.h"

#ifndef APX_EMBEDDED
# ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#   define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>
# else
#  include <pthread.h>
# endif
#include "osmacro.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_file2_tag;
struct apx_fileManager2_tag;

typedef apx_error_t (apx_file_open_notify_func)(void *arg, struct apx_file2_tag *file);
typedef apx_error_t (apx_file_write_notify_func)(void *arg, struct apx_file2_tag *file, uint32_t offset, const uint8_t *src, uint32_t len);
typedef apx_error_t (apx_file_read_const_data_func)(void *arg, struct apx_file2_tag *file, uint32_t offset, uint8_t *dest, uint32_t len);

typedef struct apx_fileNotificationHandler_tag
{
   void *arg;
   apx_file_open_notify_func *openNotify; //Notifies file owner that his file was openened on remote end (use with local files)
   apx_file_write_notify_func *writeNotify; //Notifies file owner that his file has just been written to (use with remote files)
} apx_fileNotificationHandler_t;

typedef struct apx_file2_tag
{
   bool isFileOpen;
   bool hasFirstWrite;

   apx_fileType_t fileType;
   apx_fileInfo_t fileInfo;
   apx_fileNotificationHandler_t notificationHandler;
   struct apx_fileManager2_tag *fileManager;
   adt_list_t eventListeners; //strong references to apx_fileEventListener2_t
   MUTEX_T lock;
} apx_file2_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_file2_create_rmf(apx_file2_t *self, bool isRemoteFile, const rmf_fileInfo_t *fileInfo);
apx_error_t apx_file2_create(apx_file2_t *self, const apx_fileInfo_t *fileInfo);
#ifndef APX_EMBEDDED
void apx_file2_destroy(apx_file2_t *self);
apx_file2_t *apx_file2_new_rmf(bool isRemoteFile, const rmf_fileInfo_t *fileInfo);
apx_file2_t *apx_file2_new(const apx_fileInfo_t *fileInfo);
# define apx_file2_newLocal(fileInfo) apx_file2_new_rmf(false, fileInfo);
# define apx_file2_newRemote(fileInfo) apx_file2_new_rmf(true, fileInfo);
void apx_file2_delete(apx_file2_t *self);
void apx_file2_vdelete(void *arg);
#endif
const char *apx_file2_basename(const apx_file2_t *self);
const char *apx_file2_extension(const apx_file2_t *self);
void apx_file2_open(apx_file2_t *self);
void apx_file2_close(apx_file2_t *self);
void apx_file2_setNotificationHandler(apx_file2_t *self, const apx_fileNotificationHandler_t *handler);
bool apx_file2_hasFirstWrite(apx_file2_t *self);
void apx_file2_setFirstWrite(apx_file2_t *self);
bool apx_file2_isOpen(apx_file2_t *self);
bool apx_file2_isLocalFile(apx_file2_t *self);
bool apx_file2_isRemoteFile(apx_file2_t *self);

struct apx_fileManager2_tag* apx_file2_getFileManager(apx_file2_t *self);
void apx_file2_setFileManager(apx_file2_t *self, struct apx_fileManager2_tag *fileManager);
apx_fileType_t apx_file2_getApxFileType(const apx_file2_t *self);
void apx_file2_setApxFileType(const apx_file2_t *self, apx_fileType_t);
uint16_t apx_file2_getRmfFileType(const apx_file2_t *self);

uint32_t apx_file2_getStartAddress(const apx_file2_t *self);
apx_size_t apx_file2_getFileSize(const apx_file2_t *self);

apx_error_t apx_file2_fileOpenNotify(apx_file2_t *self);
apx_error_t apx_file2_fileWriteNotify(apx_file2_t *self, uint32_t offset, const uint8_t *src, uint32_t len);

#endif //APX_FILE2_H

