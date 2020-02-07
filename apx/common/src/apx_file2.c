/*****************************************************************************
* \file      apx_file2.c
* \author    Conny Gustafsson
* \date      2018-08-30
* \brief     Improved version of apx_file
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include "apx_error.h"
#include "apx_file2.h"
#include "bstr.h"
#include "apx_eventListener2.h"
#include "apx_nodeData2.h"
#include "apx_fileManager2.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static void apx_file2_calcFileType(apx_file2_t *self);
static void apx_file2_lock(apx_file2_t *self);
static void apx_file2_unlock(apx_file2_t *self);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_file2_create_rmf(apx_file2_t *self, bool isRemoteFile, const rmf_fileInfo_t *rmfInfo)
{
   if (self != 0)
   {
      apx_error_t retval = APX_NO_ERROR;
      self->isFileOpen = false;
      self->hasFirstWrite = false;
      self->fileManager = (apx_fileManager2_t*) 0;
      self->fileType = APX_UNKNOWN_FILE_TYPE;
      memset(&self->notificationHandler, 0, sizeof(apx_fileNotificationHandler_t));

      adt_list_create(&self->eventListeners, apx_fileEventListener_vdelete);
      retval = apx_fileInfo_create_rmf(&self->fileInfo, rmfInfo, isRemoteFile);
      if (retval == APX_NO_ERROR)
      {
         apx_file2_calcFileType(self);
#ifndef APEX_EMBEDDED
         MUTEX_INIT(self->lock);
#endif
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file2_create(apx_file2_t *self, const apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0))
   {
      apx_error_t retval = APX_NO_ERROR;
      self->isFileOpen = false;
      self->fileManager = (apx_fileManager2_t*) 0;
      self->fileType = APX_UNKNOWN_FILE_TYPE;
      memset(&self->notificationHandler, 0, sizeof(apx_fileNotificationHandler_t));

      adt_list_create(&self->eventListeners, apx_fileEventListener_vdelete);
      retval = apx_fileInfo_assign(&self->fileInfo, fileInfo);
      if (retval == APX_NO_ERROR)
      {
         apx_file2_calcFileType(self);
   #ifndef APEX_EMBEDDED
         MUTEX_INIT(self->lock);
   #endif
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file2_destroy(apx_file2_t *self)
{
   if (self != 0)
   {
      apx_fileInfo_destroy(&self->fileInfo);
      adt_list_destroy(&self->eventListeners);
#ifndef APEX_EMBEDDED
      MUTEX_DESTROY(self->lock);
#endif
   }
}

apx_file2_t *apx_file2_new_rmf(bool isRemoteFile, const rmf_fileInfo_t *fileInfo)
{
   apx_file2_t *self = (apx_file2_t*) malloc(sizeof(apx_file2_t));
   if (self != 0)
   {
      int8_t result = apx_file2_create_rmf(self, isRemoteFile, fileInfo);
      if (result != 0)
      {
         free(self);
         self = 0;
      }
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

apx_file2_t *apx_file2_new(const apx_fileInfo_t *fileInfo)
{
   apx_file2_t *self = (apx_file2_t*) malloc(sizeof(apx_file2_t));
   if (self != 0)
   {
      apx_error_t result = apx_file2_create(self, fileInfo);
      if (result != APX_NO_ERROR)
      {
         free(self);
         self = (apx_file2_t*) 0;
      }
   }
   return self;
}

void apx_file2_delete(apx_file2_t *self)
{
   if (self != 0)
   {
      apx_file2_destroy(self);
      free(self);
   }
}

void apx_file2_vdelete(void *arg)
{
   apx_file2_delete((apx_file2_t*) arg);
}

void apx_file2_open(apx_file2_t *self)
{
   if (self != 0)
   {
      apx_file2_lock(self);
      self->isFileOpen = true;
      apx_file2_unlock(self);
   }
}

void apx_file2_close(apx_file2_t *self)
{
   if (self != 0)
   {
      apx_file2_lock(self);
      self->isFileOpen = false;
      apx_file2_unlock(self);
   }
}


void apx_file2_setNotificationHandler(apx_file2_t *self, const apx_fileNotificationHandler_t *handler)
{
   if (self != 0)
   {
      apx_file2_lock(self);
      if (handler == 0)
      {
         memset(&self->notificationHandler, 0, sizeof(apx_fileNotificationHandler_t));
      }
      else
      {
         memcpy(&self->notificationHandler, handler, sizeof(apx_fileNotificationHandler_t));
      }
      apx_file2_unlock(self);
   }
}

bool apx_file2_hasFirstWrite(apx_file2_t *self)
{
   if (self != 0)
   {
      return self->hasFirstWrite;
   }
   return false;
}

void apx_file2_setFirstWrite(apx_file2_t *self)
{
   if (self != 0)
   {
      self->hasFirstWrite = true;
   }
}

bool apx_file2_isOpen(apx_file2_t *self)
{
   if (self != 0)
   {
      bool retval;
      apx_file2_lock(self);
      retval = self->isFileOpen;
      apx_file2_unlock(self);
      return retval;
   }
   return false;
}

bool apx_file2_isLocalFile(apx_file2_t *self)
{
   if (self != 0)
   {
      return !apx_file2_isRemoteFile(self);
   }
   return false;
}

bool apx_file2_isRemoteFile(apx_file2_t *self)
{
   if (self != 0)
   {
      bool retval;
      apx_file2_lock(self);
      retval = apx_fileInfo_isRemoteAddress(&self->fileInfo);
      apx_file2_unlock(self);
      return retval;
   }
   return false;
}

struct apx_fileManager2_tag* apx_file2_getFileManager(apx_file2_t *self)
{
   if (self != 0)
   {
      return self->fileManager;
   }
   return (apx_fileManager2_t*) 0;
}

void apx_file2_setFileManager(apx_file2_t *self, struct apx_fileManager2_tag *fileManager)
{
   if (self != 0)
   {
      self->fileManager = fileManager;
   }
}


void* apx_file2_registerEventListener(apx_file2_t *self, apx_fileEventListener2_t *handlerTable)
{
   if ( (self != 0) && (handlerTable != 0) )
   {
      apx_fileEventListener2_t *handle = apx_fileEventListener_clone(handlerTable);
      if (handle != 0)
      {
         adt_list_insert_unique(&self->eventListeners, handle);
      }
      return handle;
   }
   return (void*) 0;
}

void apx_file2_unregisterEventListener(apx_file2_t *self, void *handle)
{
   if ( (self != 0) && (handle != 0) )
   {
      adt_list_remove(&self->eventListeners, handle);
   }
}

apx_fileType_t apx_file2_getApxFileType(const apx_file2_t *self)
{
   if (self != 0)
   {
      return self->fileType;
   }
   return APX_UNKNOWN_FILE_TYPE;
}

apx_error_t apx_file2_fileOpenNotify(apx_file2_t *self)
{
   if (self != 0)
   {
      if (self->notificationHandler.openNotify != 0)
      {
         return self->notificationHandler.openNotify(self->notificationHandler.arg, self);
      }
      return APX_INVALID_OPEN_HANDLER_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

uint32_t apx_file2_getStartAddress(const apx_file2_t *self)
{
   if (self != 0)
   {
      return self->fileInfo.address & RMF_ADDRESS_MASK_INTERNAL;
   }
   return 0u;
}

apx_size_t apx_file2_getFileSize(const apx_file2_t *self)
{
   if (self != 0)
   {
      return self->fileInfo.length;
   }
   return 0u;
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void apx_file2_calcFileType(apx_file2_t *self)
{
   if (apx_fileInfo_nameEndsWith(&self->fileInfo, APX_OUTDATA_FILE_EXT))
   {
      self->fileType = APX_OUTDATA_FILE_TYPE;
   }
   else if (apx_fileInfo_nameEndsWith(&self->fileInfo, APX_INDATA_FILE_EXT))
   {
      self->fileType = APX_INDATA_FILE_TYPE;
   }
   else if (apx_fileInfo_nameEndsWith(&self->fileInfo, APX_DEFINITION_FILE_EXT))
   {
      self->fileType = APX_DEFINITION_FILE_TYPE;
   }
   else
   {
      self->fileType = APX_UNKNOWN_FILE_TYPE;
   }
}

static void apx_file2_lock(apx_file2_t *self)
{
#ifndef APX_EMBEDDED
   if(self != 0)
   {
      MUTEX_LOCK(self->lock);
   }
#else
   (void) self;
#endif
}

static void apx_file2_unlock(apx_file2_t *self)
{
#ifndef APX_EMBEDDED
   if(self != 0)
   {
      MUTEX_UNLOCK(self->lock);
   }
#else
   (void) self;
#endif
}

