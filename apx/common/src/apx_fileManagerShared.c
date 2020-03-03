/*****************************************************************************
* \file      apx_fileManagerShared.c
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
#if APX_DEBUG_ENABLE
#include <stdio.h>
#endif
#include "apx_fileManagerShared.h"
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
apx_error_t apx_fileManagerShared_create(apx_fileManagerShared_t *self)
{
   if (self != 0)
   {
      self->connectionId = APX_INVALID_CONNECTION_ID;
      self->arg = (void*) 0;
      self->freeAllocatedMemory = (apx_allocatorFreeFunc*) 0;
      self->isConnected = false;
      SPINLOCK_INIT(self->lock);
      apx_fileMap_create(&self->localFileMap);
      apx_fileMap_create(&self->remoteFileMap);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManagerShared_destroy(apx_fileManagerShared_t *self)
{
   if (self != 0)
   {
      apx_fileMap_destroy(&self->localFileMap);
      apx_fileMap_destroy(&self->remoteFileMap);
      SPINLOCK_DESTROY(self->lock);
   }
}


/**
 * Creates a local file and automatically assigns it an address in the local file map.
 * The address set in the fileInfo_t struct is assumed to be RMF_INVALID_ADDRESS
 */
apx_file_t *apx_fileManagerShared_createLocalFile(apx_fileManagerShared_t *self, const apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0) )
   {
      apx_file_t *localFile = apx_file_new(fileInfo);
      if (localFile != 0)
      {
         localFile->fileInfo.address = RMF_INVALID_ADDRESS;
         SPINLOCK_ENTER(self->lock);
         apx_fileMap_insertFile(&self->localFileMap, localFile);
         SPINLOCK_LEAVE(self->lock);
      }
      return localFile;
   }
   return (apx_file_t*) 0;
}

apx_file_t *apx_fileManagerShared_createRemoteFile(apx_fileManagerShared_t *self, const apx_fileInfo_t *fileInfo)
{
   if ( (self != 0) && (fileInfo != 0) )
   {
      apx_file_t *remoteFile = apx_file_new(fileInfo);
      if (remoteFile != 0)
      {
         remoteFile->fileInfo.address|=RMF_REMOTE_ADDRESS_BIT;
         SPINLOCK_ENTER(self->lock);
         apx_fileMap_insertFile(&self->remoteFileMap, remoteFile);
         SPINLOCK_LEAVE(self->lock);
      }
      return remoteFile;
   }
   return (apx_file_t*) 0;
}

int32_t apx_fileManagerShared_getNumLocalFiles(apx_fileManagerShared_t *self)
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

int32_t apx_fileManagerShared_getNumRemoteFiles(apx_fileManagerShared_t *self)
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

apx_file_t *apx_fileManagerShared_findLocalFileByName(apx_fileManagerShared_t *self, const char *name)
{
   if ( (self != 0) && (name != 0) )
   {
      apx_file_t *localFile;
      SPINLOCK_ENTER(self->lock);
      localFile = apx_fileMap_findByName(&self->localFileMap, name);
      SPINLOCK_LEAVE(self->lock);
      return localFile;
   }
   return (apx_file_t*) 0;
}

apx_file_t *apx_fileManagerShared_findRemoteFileByName(apx_fileManagerShared_t *self, const char *name)
{
   if ( (self != 0) && (name != 0) )
   {
      apx_file_t *localFile;
      SPINLOCK_ENTER(self->lock);
      localFile = apx_fileMap_findByName(&self->remoteFileMap, name);
      SPINLOCK_LEAVE(self->lock);
      return localFile;
   }
   return (apx_file_t*) 0;
}

apx_file_t *apx_fileManagerShared_findFileByAddress(apx_fileManagerShared_t *self, uint32_t address)
{
   if ( (self != 0) && (address != RMF_INVALID_ADDRESS))
   {
      apx_file_t *file;
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
   return (apx_file_t*) 0;
}


void apx_fileManagerShared_setConnectionId(apx_fileManagerShared_t *self, uint32_t connectionId)
{
   if (self != 0)
   {
      self->connectionId = connectionId;
   }
}

uint32_t apx_fileManagerShared_getConnectionId(const apx_fileManagerShared_t *self)
{
   if (self != 0)
   {
      return self->connectionId;
   }
   return APX_INVALID_CONNECTION_ID;
}

adt_ary_t *apx_fileManagerShared_getLocalFileList(apx_fileManagerShared_t *self)
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

void apx_fileManagerShared_freeAllocatedMemory(apx_fileManagerShared_t *self, uint8_t *data, uint32_t len)
{
   if (self != 0)
   {
      if (self->freeAllocatedMemory != 0)
      {
#if APX_DEBUG_ENABLE
//         printf("Freeing %d bytes\n", (int) len);
#endif
         self->freeAllocatedMemory(self->arg, data, len);
      }
   }
}

void apx_fileManagerShared_connect(apx_fileManagerShared_t *self)
{
   if (self != 0)
   {
      SPINLOCK_ENTER(self->lock);
      self->isConnected = true;
      SPINLOCK_LEAVE(self->lock);
   }
}

void apx_fileManagerShared_disconnect(apx_fileManagerShared_t *self)
{
   if (self != 0)
   {
      SPINLOCK_ENTER(self->lock);
      self->isConnected = false;
      SPINLOCK_LEAVE(self->lock);
#if APX_DEBUG_ENABLE
      printf("[%u] Disabled transmit handler\n", (unsigned int) self->connectionId);
#endif
   }
}

bool apx_fileManagerShared_isConnected(apx_fileManagerShared_t *self)
{
   if (self != 0)
   {
      bool retval;
      SPINLOCK_ENTER(self->lock);
      retval = self->isConnected;
      SPINLOCK_LEAVE(self->lock);
      return retval;
   }
   return false;
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


