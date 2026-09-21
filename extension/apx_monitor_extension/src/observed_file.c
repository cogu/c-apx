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

void apx_observed_file_create(apx_observed_file_t* self, rmf_extended_file_info_t* const file_info)
{
   if (self != NULL)
   {
      (void)file_info;
   }
}

void apx_observed_file_destroy(apx_observed_file_t* self)
{
   if (self != NULL)
   {

   }
}

apx_observed_file_t* apx_observed_file_new(rmf_extended_file_info_t* const file_info)
{
   apx_observed_file_t* self = (apx_observed_file_t*)malloc(sizeof(apx_observed_file_t));
   if (self != NULL)
   {
      apx_observed_file_create(self, file_info);
   }
   return self;
}

void apx_observed_file_delete(apx_observed_file_t* self)
{
   if (self != NULL)
   {
      apx_observed_file_destroy(self);
      free(self);
   }
}

void apx_observed_file_vdelete(void* arg)
{
   apx_observed_file_delete((apx_observed_file_t*)arg);
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
