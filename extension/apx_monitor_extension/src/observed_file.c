/*****************************************************************************
* \file      file.c
* \author    Conny Gustafsson
* \date      2021-01-01
* \brief     Description
*
* Copyright (c) 2021 Conny Gustafsson
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
#include <assert.h>
#include <malloc.h>
#include "apx/extension/observed_file.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_observedFile_create(apx_observedFile_t* self, rmf_extendedFileInfo_t* const file_info)
{
   if (self != NULL)
   {
      (void)file_info;
   }
}

void apx_observedFile_destroy(apx_observedFile_t* self)
{
   if (self != NULL)
   {

   }
}

apx_observedFile_t* apx_observedFile_new(rmf_extendedFileInfo_t* const file_info)
{
   apx_observedFile_t* self = (apx_observedFile_t*)malloc(sizeof(apx_observedFile_t));
   if (self != NULL)
   {
      apx_observedFile_create(self, file_info);
   }
   return self;
}

void apx_observedFile_delete(apx_observedFile_t* self)
{
   if (self != NULL)
   {
      apx_observedFile_destroy(self);
      free(self);
   }
}

void apx_observedFile_vdelete(void* arg)
{
   apx_observedFile_delete((apx_observedFile_t*)arg);
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
