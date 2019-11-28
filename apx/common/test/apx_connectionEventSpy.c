/*****************************************************************************
* \file      apx_connectionEventSpy.c
* \author    Conny Gustafsson
* \date      2018-08-21
* \brief     Test spy for apx_fileManagerEventListener
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
#include <string.h>
#include <stdio.h>
#include "apx_connectionEventSpy.h"
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
void apx_connectionEventSpy_create(apx_connectionEventSpy_t *self)
{
   if (self != 0)
   {
      self->headerAcceptedCount = 0;
      self->fileCreateCount = 0;
      self->lastConnection = (apx_connectionBase_t*) 0;
      self->lastFileInfo = (apx_fileInfo_t*) 0;
   }
}

void apx_connectionEventSpy_destroy(apx_connectionEventSpy_t *self)
{
   if (self != 0)
   {
      if (self->lastFileInfo != 0)
      {
         apx_fileInfo_delete(self->lastFileInfo);
      }
   }
}

void apx_connectionEventSpy_register(apx_connectionEventSpy_t *self, apx_connectionBase_t *connection)
{
   if ((self != 0) && (connection != 0) )
   {
      apx_connectionEventListener_t handler;
      memset(&handler, 0, sizeof(handler));
      handler.arg = (void*) self;
      handler.headerAccepted2 = apx_connectionEventSpy_headerAccepted;
      handler.fileCreate2 = apx_connectionEventSpy_fileCreate;
      (void)apx_connectionBase_registerEventListener(connection, &handler);
   }
}

void apx_connectionEventSpy_headerAccepted(void *arg, apx_connectionBase_t *connection)
{
   apx_connectionEventSpy_t *self = (apx_connectionEventSpy_t*) arg;
   if ( (self != 0) && (connection != 0) )
   {

      self->headerAcceptedCount++;
      self->lastConnection = connection;
   }
}

void apx_connectionEventSpy_fileCreate(void *arg, apx_connectionBase_t *connection, const apx_fileInfo_t *fileInfo)
{
   apx_connectionEventSpy_t *self = (apx_connectionEventSpy_t*) arg;
   if ( (self != 0) && (connection != 0) && (fileInfo != 0))
   {
      self->fileCreateCount++;
      self->lastConnection = connection;
      if (self->lastFileInfo != 0)
      {
         apx_fileInfo_delete(self->lastFileInfo);
      }
      self->lastFileInfo = apx_fileInfo_clone(fileInfo);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


