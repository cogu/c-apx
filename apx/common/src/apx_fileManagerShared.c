/*****************************************************************************
* \file      apx_fileManagerShared.c
* \author    Conny Gustafsson
* \date      2018-08-02
* \brief     APX Filemanager data component
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
#include "apx_fileManagerShared.h"
#include "rmf.h"
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
int8_t apx_fileManagerShared_create(apx_fileManagerShared_t *self)
{
   if (self != 0)
   {
      int8_t result = apx_allocator_create(&self->allocator, APX_MAX_NUM_MESSAGES);
      if (result == 0)
      {
         self->fmid = 0;
         self->isConnected = false;
         self->arg = (void*) 0;
         self->fileCreated = (void (*)(void *, const struct apx_file2_tag*, void *caller)) 0;
         self->sendFileInfo = (void (*)(void *arg, const struct apx_file2_tag *pFile)) 0;
         self->sendFileOpen = (void (*)(void *arg, const struct apx_file2_tag *pFile, void *caller)) 0;
         self->openFileRequest = (void (*)(void *arg, uint32_t address)) 0;
         apx_allocator_start(&self->allocator);
      }
      return result;
   }
   errno = EINVAL;
   return -1;
}

void apx_fileManagerShared_destroy(apx_fileManagerShared_t *self)
{
   if (self != 0)
   {
      apx_allocator_stop(&self->allocator);
      apx_allocator_destroy(&self->allocator);
   }
}

uint8_t *apx_fileManagerShared_alloc(apx_fileManagerShared_t *self, size_t size)
{
   if (self != 0)
   {
      return apx_allocator_alloc(&self->allocator, size);
   }
   return (uint8_t*) 0;
}

void apx_fileManagerShared_free(apx_fileManagerShared_t *self, uint8_t *ptr, size_t size)
{
   if (self != 0)
   {
      apx_allocator_free(&self->allocator, ptr, size);
   }
}

int32_t apx_fileManagerShared_calcFileInfoMsgSize(const struct rmf_fileInfo_tag *fileInfo)
{
   int32_t msgLen = RMF_CMD_HEADER_LEN+RMF_CMD_FILE_INFO_BASE_SIZE+1; //add 1 to fit string null terminator
   msgLen+=strlen(fileInfo->name);
   return msgLen;
}

int32_t apx_fileManagerShared_serializeFileInfo(uint8_t *bufData, int32_t bufLen, const struct rmf_fileInfo_tag *fileInfo)
{
   if ((bufData != 0) && (bufLen > 0) && (fileInfo != 0))
   {
      int32_t msgLen = apx_fileManagerShared_calcFileInfoMsgSize(fileInfo);
      if (msgLen<=bufLen)
      {
         int32_t result;
         result = rmf_packHeader(bufData, bufLen, RMF_CMD_START_ADDR, false);
         if (result > 0)
         {
            bufData+=result;
            bufLen-=result;
            result = rmf_serialize_cmdFileInfo(bufData, bufLen, fileInfo);
            if (result > 0)
            {
               result = msgLen;
            }
         }
         return result;
      }
      else
      {
         errno = EMSGSIZE; //message too large
         return -1;
      }
   }
   errno = EINVAL;
   return -1;
}



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


