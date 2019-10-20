/*****************************************************************************
* \file      apx_fileManagerLocal.c
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "apx_fileManagerLocal.h"
#include "rmf.h"
#include "apx_file2.h"
#include "numheader.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//temporary include
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_fileManagerLocal_create(apx_fileManagerLocal_t *self, apx_fileManagerShared_t *shared)
{
   if (self != 0)
   {
      MUTEX_INIT(self->mutex);
      apx_fileMap_create(&self->localFileMap);
      self->shared = shared;
   }
}

void apx_fileManagerLocal_destroy(apx_fileManagerLocal_t *self)
{
   if (self != 0)
   {
      apx_fileMap_destroy(&self->localFileMap);
      MUTEX_DESTROY(self->mutex);
   }
}

void apx_fileManagerLocal_attachFile(apx_fileManagerLocal_t *self, struct apx_file2_tag *localFile, void *caller)
{
   if ((self != 0) && (localFile != 0))
   {
      apx_fileMap_insertFile(&self->localFileMap, localFile);
      if (self->shared->fileCreated != 0)
      {
         self->shared->fileCreated(self->shared->arg, localFile, caller);
      }
      if ( (self->shared->isConnected) && (self->shared->sendFileInfo != 0) )
      {
         self->shared->sendFileInfo(self->shared->arg, localFile);
      }
   }
}

int32_t apx_fileManagerLocal_getNumFiles(apx_fileManagerLocal_t *self)
{
   if (self != 0)
   {
      return apx_fileMap_length(&self->localFileMap);
   }
   errno = EINVAL;
   return -1;
}

void apx_fileManagerLocal_sendFileInfo(apx_fileManagerLocal_t *self)
{
   if ( (self != 0) && (self->shared->sendFileInfo != 0) )
   {
      MUTEX_LOCK(self->mutex);
      adt_list_t *files = apx_fileMap_getList(&self->localFileMap);
      MUTEX_UNLOCK(self->mutex);
      if (files != 0)
      {
         adt_list_elem_t *iter = adt_list_iter_first(files);
         while (iter != 0)
         {
            apx_file2_t *file = (apx_file2_t*)iter->pItem;
            self->shared->sendFileInfo(self->shared->arg, file);
            iter = adt_list_iter_next(iter);
         }
      }
   }
}

struct apx_file2_tag *apx_fileManagerLocal_find(apx_fileManagerLocal_t *self, uint32_t address)
{
   apx_file2_t *localFile = (apx_file2_t *) 0;
   if (self != 0)
   {
      MUTEX_LOCK(self->mutex);
      localFile = apx_fileMap_findByAddress(&self->localFileMap, address);
      MUTEX_UNLOCK(self->mutex);
   }
   return localFile;
}

struct apx_file2_tag *apx_fileManagerLocal_findByName(apx_fileManagerLocal_t *self, const char *name)
{
   apx_file2_t *localFile = (apx_file2_t *) 0;
   if (self != 0)
   {
      MUTEX_LOCK(self->mutex);
      localFile = apx_fileMap_findByName(&self->localFileMap, name);
      MUTEX_UNLOCK(self->mutex);
   }
   return localFile;
}

bool apx_fileManagerLocal_isFileAttached(apx_fileManagerLocal_t *self, apx_file2_t *localFile)
{
   bool retval = false;
   if (self != 0)
   {
      MUTEX_LOCK(self->mutex);
      retval = apx_fileMap_exist(&self->localFileMap, localFile);
      MUTEX_UNLOCK(self->mutex);
   }
   return retval;
}

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

