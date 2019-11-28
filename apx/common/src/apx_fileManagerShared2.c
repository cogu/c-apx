/*****************************************************************************
* \file      apx_fileManagerShared2.c
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "apx_fileManagerShared2.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManagerShared2_create(apx_fileManagerShared2_t *self)
{
   if (self != 0)
   {
      apx_error_t result = apx_allocator_create(&self->allocator, APX_MAX_NUM_MESSAGES);
      if (result == 0)
      {
         self->connectionId = APX_INVALID_CONNECTION_ID;
         self->arg = (void*) 0;
         self->remoteFileCreated = (void (*)(void *arg, apx_file2_t* file)) 0;
         self->fileOpenRequested = (void (*)(void *arg, apx_file2_t* file)) 0;
         self->remoteFileWritten = (void (*)(void *arg, apx_file2_t *remoteFile, uint32_t offset, const uint8_t *msgData, uint32_t msgLen, bool moreBit)) 0;
         SPINLOCK_INIT(self->lock);
         apx_fileMap_create(&self->localFileMap);
         apx_fileMap_create(&self->remoteFileMap);
         apx_allocator_start(&self->allocator);
      }
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManagerShared2_destroy(apx_fileManagerShared2_t *self)
{
   if (self != 0)
   {
      apx_allocator_stop(&self->allocator);
      apx_allocator_destroy(&self->allocator);
      apx_fileMap_destroy(&self->localFileMap);
      apx_fileMap_destroy(&self->remoteFileMap);
      SPINLOCK_DESTROY(self->lock);
   }
}

uint8_t *apx_fileManagerShared2_alloc(apx_fileManagerShared2_t *self, size_t size)
{
   if (self != 0)
   {
      return apx_allocator_alloc(&self->allocator, size);
   }
   return (uint8_t*) 0;
}

void apx_fileManagerShared2_free(apx_fileManagerShared2_t *self, uint8_t *ptr, size_t size)
{
   if (self != 0)
   {
      apx_allocator_free(&self->allocator, ptr, size);
   }
}

/**
 * Creates a local file and automatically assigns it an address in the local file map.
 * The address set in the fileInfo_t struct is assumed to be RMF_INVALID_ADDRESS
 */
apx_file2_t *apx_fileManagerShared2_createLocalFile(apx_fileManagerShared2_t *self, const apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0) )
   {
      apx_file2_t *localFile = apx_file2_new(fileInfo);
      if (localFile != 0)
      {
         localFile->fileInfo.address = RMF_INVALID_ADDRESS;
         SPINLOCK_ENTER(self->lock);
         apx_fileMap_insertFile(&self->localFileMap, localFile);
         SPINLOCK_LEAVE(self->lock);
      }
      return localFile;
   }
   return (apx_file2_t*) 0;
}

apx_file2_t *apx_fileManagerShared2_createRemoteFile(apx_fileManagerShared2_t *self, const apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0) )
   {
      apx_file2_t *remoteFile = apx_file2_new(fileInfo);
      if (remoteFile != 0)
      {
         remoteFile->fileInfo.address|=RMF_REMOTE_ADDRESS_BIT;
         SPINLOCK_ENTER(self->lock);
         apx_fileMap_insertFile(&self->remoteFileMap, remoteFile);
         SPINLOCK_LEAVE(self->lock);
      }
      return remoteFile;
   }
   return (apx_file2_t*) 0;
}

int32_t apx_fileManagerShared2_getNumLocalFiles(apx_fileManagerShared2_t *self)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->lock);
      retval = apx_fileMap_length(&self->localFileMap);
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return -1;
}

int32_t apx_fileManagerShared2_getNumRemoteFiles(apx_fileManagerShared2_t *self)
{
   if (self != 0)
   {
      int32_t retval;
      SPINLOCK_ENTER(self->lock);
      retval = apx_fileMap_length(&self->remoteFileMap);
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return -1;
}

apx_file2_t *apx_fileManagerShared2_findLocalFileByName(apx_fileManagerShared2_t *self, const char *name)
{
   if ( (self != 0) && (name != 0) )
   {
      apx_file2_t *localFile;
      SPINLOCK_ENTER(self->lock);
      localFile = apx_fileMap_findByName(&self->localFileMap, name);
      SPINLOCK_LEAVE(self->lock);
      return localFile;
   }
   return (apx_file2_t*) 0;
}

apx_file2_t *apx_fileManagerShared2_findRemoteFileByName(apx_fileManagerShared2_t *self, const char *name)
{
   if ( (self != 0) && (name != 0) )
   {
      apx_file2_t *localFile;
      SPINLOCK_ENTER(self->lock);
      localFile = apx_fileMap_findByName(&self->remoteFileMap, name);
      SPINLOCK_LEAVE(self->lock);
      return localFile;
   }
   return (apx_file2_t*) 0;
}

apx_file2_t *apx_fileManagerShared2_findFileByAddress(apx_fileManagerShared2_t *self, uint32_t address)
{
   if ( (self != 0) && (address != RMF_INVALID_ADDRESS))
   {
      apx_file2_t *file;
      uint32_t addressWithoutFlags = address & RMF_ADDRESS_MASK_INTERNAL;
      SPINLOCK_ENTER(self->lock);
      if ( (address & RMF_REMOTE_ADDRESS_BIT) != 0u)
      {
         file = apx_fileMap_findByAddress(&self->remoteFileMap, addressWithoutFlags);
      }
      else
      {
         file = apx_fileMap_findByAddress(&self->localFileMap, addressWithoutFlags);
      }
      SPINLOCK_LEAVE(self->lock);
      return file;
   }
   return (apx_file2_t*) 0;
}


void apx_fileManagerShared2_setConnectionId(apx_fileManagerShared2_t *self, uint32_t connectionId)
{
   if (self != 0)
   {
      self->connectionId = connectionId;
   }
}

uint32_t apx_fileManagerShared2_getConnectionId(const apx_fileManagerShared2_t *self)
{
   if (self != 0)
   {
      return self->connectionId;
   }
   return APX_INVALID_CONNECTION_ID;
}

adt_ary_t *apx_fileManagerShared2_getLocalFileList(apx_fileManagerShared2_t *self)
{
   if (self != 0)
   {
      adt_ary_t *array;
      SPINLOCK_ENTER(self->lock);
      array = apx_fileMap_makeFileInfoArray(&self->localFileMap);
      SPINLOCK_LEAVE(self->lock);
      return array;
   }
   return (adt_ary_t*) 0;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


