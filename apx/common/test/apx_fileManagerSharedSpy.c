/*****************************************************************************
* \file      apx_fileManagerSharedSpy.c
* \author    Conny Gustafsson
* \date      2018-08-28
* \brief     Description
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
#include "apx_fileManagerSharedSpy.h"
#include <malloc.h>
#include <errno.h>
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
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void apx_fileManagerSharedSpy_create(apx_fileManagerSharedSpy_t *self)
{
   if (self != 0)
   {
      self->numRemoteFileCreatedCalls = 0;
      self->numSendFileInfoCalls = 0;
      self->numSendFileOpenCalls = 0;
      self->numfileOpenRequestCalls = 0;
   }
}

void apx_fileManagerSharedSpy_destroy(apx_fileManagerSharedSpy_t *self)
{

}

apx_fileManagerSharedSpy_t *apx_fileManagerSharedSpy_new(void)
{
   apx_fileManagerSharedSpy_t *self = (apx_fileManagerSharedSpy_t*) malloc(sizeof(apx_fileManagerSharedSpy_t));
   if (self != 0)
   {
      apx_fileManagerSharedSpy_create(self);
   }
   else
   {
      errno = ENOMEM;
   }
   return self;
}

void apx_fileManagerSharedSpy_delete(apx_fileManagerSharedSpy_t *self)
{
   if (self != 0)
   {
      apx_fileManagerSharedSpy_destroy(self);
      free(self);
   }
}

void apx_fileManagerSharedSpy_remoteFileCreated(void *arg, apx_file_t *pFile)
{
   apx_fileManagerSharedSpy_t *self = (apx_fileManagerSharedSpy_t*) arg;
   if (self != 0)
   {
      self->numRemoteFileCreatedCalls++;
   }
}

void apx_fileManagerSharedSpy_sendFileInfo(void *arg, const struct apx_file_tag *pFile)
{
   apx_fileManagerSharedSpy_t *self = (apx_fileManagerSharedSpy_t*) arg;
   if (self != 0)
   {
      self->numSendFileInfoCalls++;
   }
}

void apx_fileManagerSharedSpy_sendFileOpen(void *arg, const apx_file_t *file, void *caller)
{
   apx_fileManagerSharedSpy_t *self = (apx_fileManagerSharedSpy_t*) arg;
   if (self != 0)
   {
      self->numSendFileOpenCalls++;
   }
}

void apx_fileManagerSharedSpy_fileOpenRequested(void *arg, apx_file_t *file)
{
   apx_fileManagerSharedSpy_t *self = (apx_fileManagerSharedSpy_t*) arg;
   if (self != 0)
   {
      self->numfileOpenRequestCalls++;
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


