/*****************************************************************************
* \file      observed_file.c
* \author    Conny Gustafsson
* \date      2021-01-01
* \brief     Description
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
