/*****************************************************************************
* \file      apx_remoteLog.c
* \author    Conny Gustafsson
* \date      2018-04-15
* \brief     APX control file (stream)
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
#include <stdlib.h>
#include "apx_remoteLog.h"

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
void apx_remoteLog_create(apx_remoteLog_t *self)
{
   if (self != 0)
   {
      rmf_fileInfo_create(&self->fileInfo,  APX_LOG_FILE_NAME, APX_LOG_FILE_ADDRESS, APX_LOG_FILE_LEN, RMF_FILE_TYPE_STREAM);
      self->isOpen=false;
   }
}

void apx_remoteLog_destroy(apx_remoteLog_t *self)
{
   if (self != 0)
   {
      rmf_fileInfo_destroy(&self->fileInfo);
   }
}

apx_remoteLog_t *apx_remoteLog_new(void)
{
   apx_remoteLog_t *self = (apx_remoteLog_t*) malloc(sizeof(apx_remoteLog_t));
   if(self != 0)
   {
      apx_remoteLog_create(self);
   }
   return self;
}

void apx_remoteLog_delete(apx_remoteLog_t *self)
{
   if(self != 0)
   {
      apx_remoteLog_destroy(self);
      free(self);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


