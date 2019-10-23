/*****************************************************************************
* \file      apx_file2.c
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include "apx_error.h"
#include "apx_file2.h"
#include "bstr.h"
#include "apx_eventListener.h"
#include "apx_nodeData.h"
#include "apx_fileManager.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

static apx_error_t apx_file2_processFileName(apx_file2_t *self);
static void apx_file2_calcFileType(apx_file2_t *self);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

int8_t apx_file2_create(apx_file2_t *self, bool isRemoteFile, const rmf_fileInfo_t *fileInfo, const apx_file_handler_t *handler)
{
   if (self != 0)
   {
      int8_t result;
      self->isRemoteFile = isRemoteFile;
      self->isOpen = false;
      self->isDataValid = false;
      self->fileManager = (apx_fileManager_t*) 0;
      self->fileType = APX_UNKNOWN_FILE;

      result = rmf_fileInfo_create(&self->fileInfo, fileInfo->name, fileInfo->address, fileInfo->length, fileInfo->fileType);
      if (result == 0)
      {
         (void) apx_file2_processFileName(self);
         rmf_fileInfo_setDigestData(&self->fileInfo, fileInfo->digestType, fileInfo->digestData, 0);
         apx_file2_setHandler(self, handler);
         if (self->fileType == APX_UNKNOWN_FILE)
         {
            apx_file2_calcFileType(self);
         }
      }
      return result;
   }
   errno = EINVAL;
   return -1;
}

void apx_file2_destroy(apx_file2_t *self)
{
   if (self != 0)
   {
      rmf_fileInfo_destroy(&self->fileInfo);
      if (self->basename != 0)
      {
         free(self->basename);
      }
      if (self->extension != 0)
      {
         free(self->extension);
      }
   }
}

apx_file2_t *apx_file2_new(bool isRemoteFile, const rmf_fileInfo_t *fileInfo, const apx_file_handler_t *handler)
{
   apx_file2_t *self = (apx_file2_t*) malloc(sizeof(apx_file2_t));
   if (self != 0)
   {
      int8_t result = apx_file2_create(self, isRemoteFile, fileInfo, handler);
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

const char *apx_file2_basename(const apx_file2_t *self)
{
   if ( self != 0 )
   {
      return self->basename;
   }
   return (char*) 0;
}

const char *apx_file2_extension(const apx_file2_t *self)
{
   return self->extension;
}

void apx_file2_open(apx_file2_t *self)
{
   if (self != 0)
   {
      self->isOpen = true;
   }
}

void apx_file2_close(apx_file2_t *self)
{
   if (self != 0)
   {
      self->isOpen = false;
   }
}

apx_error_t apx_file2_read(apx_file2_t *self, uint8_t *pDest, uint32_t offset, uint32_t length)
{
   if (self != 0)
   {
      if ( apx_file2_hasReadHandler(self) != false)
      {
         return self->handler.read(self->handler.arg, self, pDest, offset, length);
      }
      else
      {
         return APX_UNSUPPORTED_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file2_write(apx_file2_t *self, const uint8_t *pSrc, uint32_t offset, uint32_t length, bool more)
{
   if (self != 0)
   {
      if ( apx_file2_hasWriteHandler(self) != false )
      {
         return self->handler.write(self->handler.arg, self, pSrc, offset, length, more);
      }
      else
      {
         return APX_UNSUPPORTED_ERROR;
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

bool apx_file2_hasReadHandler(apx_file2_t *self)
{
   if ( (self !=0) && (self->handler.read != 0) )
   {
      return true;
   }
   return false;
}

bool apx_file2_hasWriteHandler(apx_file2_t *self)
{
   if ( (self !=0) && (self->handler.write != 0) )
   {
      return true;
   }
   return false;
}

void apx_file2_setHandler(apx_file2_t *self, const apx_file_handler_t *handler)
{
   if (handler == 0)
   {
      memset(&self->handler, 0, sizeof(apx_file_handler_t));
   }
   else
   {
      memcpy(&self->handler, handler, sizeof(apx_file_handler_t));
   }
}

bool apx_file2_isDataValid(apx_file2_t *self)
{
   if (self != 0)
   {
      return self->isDataValid;
   }
   return false;
}

void apx_file2_setDataValid(apx_file2_t *self)
{
   if ( (self != 0) && (!self->isDataValid) )
   {
      self->isDataValid = true;
   }
}

bool apx_file2_isOpen(apx_file2_t *self)
{
   if (self != 0)
   {
      return self->isOpen;
   }
   return false;
}

bool apx_file2_isLocal(apx_file2_t *self)
{
   if (self != 0)
   {
      return !self->isRemoteFile;
   }
   return false;
}

bool apx_file2_isRemote(apx_file2_t *self)
{
   if (self != 0)
   {
      return self->isRemoteFile;
   }
   return false;
}

struct apx_fileManager_tag* apx_file2_getFileManager(apx_file2_t *self)
{
   if (self != 0)
   {
      return self->fileManager;
   }
   return (apx_fileManager_t*) 0;
}

void apx_file2_setFileManager(apx_file2_t *self, struct apx_fileManager_tag *fileManager)
{
   if (self != 0)
   {
      self->fileManager = fileManager;
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


static apx_error_t apx_file2_processFileName(apx_file2_t *self)
{
   if (self != 0)
   {
      const char *pBegin;
      const char *pEnd;
      size_t len = strlen(self->fileInfo.name);
      pBegin = self->fileInfo.name;
      pEnd = self->fileInfo.name+len;
      if (len > 0u )
      {
         const char *p = pEnd-1;
         //search in string backwards to find a '.' character
         while(p>=pBegin)
         {
            if (*p == '.')
            {
               self->basename = (char*) bstr_make_cstr((const uint8_t*) pBegin, (const uint8_t*) p);
               self->extension = (char*) bstr_make_cstr((const uint8_t*) p, (const uint8_t*) pEnd);
               return APX_NO_ERROR;
            }
            --p;
         }
      }
      self->basename = (char*) bstr_make_cstr((const uint8_t*) pBegin, (const uint8_t*) pEnd);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;

}

static void apx_file2_calcFileType(apx_file2_t *self)
{
   if (self->extension != 0)
   {
      if (strcmp(self->extension, APX_OUTDATA_FILE_EXT)==0)
      {
         self->fileType = APX_OUTDATA_FILE;
      }
      else if (strcmp(self->extension, APX_INDATA_FILE_EXT)==0)
      {
         self->fileType = APX_INDATA_FILE;
      }
      else if (strcmp(self->extension, APX_DEFINITION_FILE_EXT)==0)
      {
         self->fileType = APX_DEFINITION_FILE;
      }
   }
}
