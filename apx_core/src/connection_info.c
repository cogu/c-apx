/*****************************************************************************
* \file      connection_info.c
* \author    Conny Gustafsson
* \date      2021-04-05
* \brief     Connection info used by monitor connections
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/connection_info.h"
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_connectionInfo_create(apx_connectionInfo_t* self, apx_connectionId_t connection_id, adt_str_t* tag)
{
   if (self != NULL)
   {
      self->connection_id = connection_id;
      if (tag != NULL)
      {
         self->tag = adt_str_clone(tag);
      }
   }
}

void apx_connectionInfo_destroy(apx_connectionInfo_t* self)
{
   if (self != NULL)
   {
      adt_str_delete(self->tag); //delete function already has built-in null check
   }
}

apx_connectionInfo_t* apx_connectionInfo_new(apx_connectionId_t connection_id, adt_str_t* tag)
{
   apx_connectionInfo_t* self = (apx_connectionInfo_t*)malloc(sizeof(apx_connectionInfo_t));
   if (self != NULL)
   {
      apx_connectionInfo_create(self, connection_id, tag);
   }
   return self;
}

void apx_connectionInfo_delete(apx_connectionInfo_t* self)
{
   if (self != NULL)
   {
      apx_connectionInfo_destroy(self);
      free(self);
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
