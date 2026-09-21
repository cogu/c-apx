/*****************************************************************************
* \file      server_extension.c
* \author    Conny Gustafsson
* \date      2019-09-05
* \brief     APX server extension data structure
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include "apx/server_extension.h"
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
void apx_serverExtension_create(apx_serverExtension_t *self, const char *name, const apx_serverExtensionHandler_t *handler, dtl_dv_t *config)
{
   if ( (self != 0) && (name != 0) && (handler != 0) )
   {
      self->name = STRDUP(name);
      memcpy(&self->handler, handler, sizeof(apx_serverExtensionHandler_t));
      self->config = config;
      if (self->config != 0)
      {
         dtl_dv_inc_ref(self->config);
      }
   }
}

void apx_serverExtension_destroy(apx_serverExtension_t *self)
{
   if (self != 0)
   {
      if (self->name != 0)
      {
         free(self->name);
      }
      if (self->config != 0)
      {
         dtl_dv_dec_ref(self->config);
         self->config = (dtl_dv_t*) 0;
      }
   }
}

apx_serverExtension_t* apx_serverExtension_new(const char *name, const apx_serverExtensionHandler_t *handler, dtl_dv_t *config)
{
   apx_serverExtension_t *self = (apx_serverExtension_t*) malloc(sizeof(apx_serverExtension_t));
   if (self != 0)
   {
      apx_serverExtension_create(self, name, handler, config);
   }
   return self;
}


void apx_serverExtension_delete(apx_serverExtension_t *self)
{
   if (self != 0)
   {
      apx_serverExtension_destroy(self);
      free(self);
   }
}

void apx_serverExtension_vdelete(void *arg)
{
   apx_serverExtension_delete((apx_serverExtension_t*) arg);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


