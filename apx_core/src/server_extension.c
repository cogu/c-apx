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
void apx_serverExtension_create(apx_server_extension_t *self, const char *name, const apx_server_extension_handler_t *handler, dtl_dv_t *config)
{
   if ( (self != NULL) && (name != NULL) && (handler != NULL) )
   {
      self->name = STRDUP(name);
      memcpy(&self->handler, handler, sizeof(apx_server_extension_handler_t));
      self->config = config;
      if (self->config != NULL)
      {
         dtl_dv_inc_ref(self->config);
      }
   }
}

void apx_serverExtension_destroy(apx_server_extension_t *self)
{
   if (self != NULL)
   {
      if (self->name != NULL)
      {
         free(self->name);
      }
      if (self->config != NULL)
      {
         dtl_dv_dec_ref(self->config);
         self->config = NULL;
      }
   }
}

apx_server_extension_t* apx_serverExtension_new(const char *name, const apx_server_extension_handler_t *handler, dtl_dv_t *config)
{
   apx_server_extension_t *self = (apx_server_extension_t*) malloc(sizeof(apx_server_extension_t));
   if (self != NULL)
   {
      apx_serverExtension_create(self, name, handler, config);
   }
   return self;
}


void apx_serverExtension_delete(apx_server_extension_t *self)
{
   if (self != NULL)
   {
      apx_serverExtension_destroy(self);
      free(self);
   }
}

void apx_serverExtension_vdelete(void *arg)
{
   apx_serverExtension_delete((apx_server_extension_t*) arg);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


