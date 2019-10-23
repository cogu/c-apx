/*****************************************************************************
* \file      apx_file2.h
* \author    Conny Gustafsson
* \date      2018-08-30
* \brief     Improved version of apx_file
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
#ifndef APX_FILE2_H
#define APX_FILE2_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include "apx_types.h"
#include "rmf.h"
#include "apx_error.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_file2_tag;
struct apx_fileManager_tag;

typedef apx_error_t (apx_file_read_func)(void *arg, struct apx_file2_tag *file, uint8_t *dest, uint32_t offset, uint32_t len);
typedef apx_error_t (apx_file_write_func)(void *arg, struct apx_file2_tag *file, const uint8_t *src, uint32_t offset, uint32_t len, bool more);

typedef struct apx_file_handler_tag
{
   void *arg;
   apx_file_read_func *read;
   apx_file_write_func *write;
}apx_file_handler_t;

typedef struct apx_file2_tag
{
   bool isRemoteFile;
   bool isOpen;
   bool isDataValid;
   uint8_t fileType;
   rmf_fileInfo_t fileInfo;
   apx_file_handler_t handler;
   char *basename;
   char *extension;
   struct apx_fileManager_tag *fileManager;
}apx_file2_t;

//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
int8_t apx_file2_create(apx_file2_t *self, bool isRemoteFile, const rmf_fileInfo_t *fileInfo, const apx_file_handler_t *handler);
#ifndef APX_EMBEDDED
void apx_file2_destroy(apx_file2_t *self);
apx_file2_t *apx_file2_new(bool isRemoteFile, const rmf_fileInfo_t *fileInfo, const apx_file_handler_t *handler);
# define apx_file2_newLocal(fileInfo, handler) apx_file2_new(false, fileInfo, handler);
# define apx_file2_newRemote(fileInfo, handler) apx_file2_new(true, fileInfo, handler);
void apx_file2_delete(apx_file2_t *self);
void apx_file2_vdelete(void *arg);
#endif
const char *apx_file2_basename(const apx_file2_t *self);
const char *apx_file2_extension(const apx_file2_t *self);
void apx_file2_open(apx_file2_t *self);
void apx_file2_close(apx_file2_t *self);
apx_error_t apx_file2_read(apx_file2_t *self, uint8_t *pDest, uint32_t offset, uint32_t length);
apx_error_t apx_file2_write(apx_file2_t *self, const uint8_t *pSrc, uint32_t offset, uint32_t length, bool more);
bool apx_file2_hasReadHandler(apx_file2_t *self);
bool apx_file2_hasWriteHandler(apx_file2_t *self);
void apx_file2_setHandler(apx_file2_t *self, const apx_file_handler_t *handler);
bool apx_file2_isDataValid(apx_file2_t *self);
void apx_file2_setDataValid(apx_file2_t *self);
bool apx_file2_isOpen(apx_file2_t *self);
bool apx_file2_isLocal(apx_file2_t *self);
bool apx_file2_isRemote(apx_file2_t *self);
struct apx_fileManager_tag* apx_file2_getFileManager(apx_file2_t *self);
void apx_file2_setFileManager(apx_file2_t *self, struct apx_fileManager_tag *fileManager);


#endif //APX_FILE2_H

