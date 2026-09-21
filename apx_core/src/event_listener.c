/*****************************************************************************
* \file      event_listener.c
* \author    Conny Gustafsson
* \date      2020-01-03
* \brief     Event listener API
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/event_listener.h"
#include <malloc.h>
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

apx_clientEventListener_t *apx_clientEventListener_clone(apx_clientEventListener_t *other)
{
   if (other != NULL)
   {
      apx_clientEventListener_t *self = (apx_clientEventListener_t*) malloc(sizeof(apx_clientEventListener_t));
      if (self != NULL)
      {
         *self = *other;
      }
      return self;
   }
   return NULL;
}

void apx_clientEventListener_delete(apx_clientEventListener_t *self)
{
   if (self != NULL)
   {
      free(self);
   }
}
void apx_clientEventListener_vdelete(void *arg)
{
   apx_clientEventListener_delete((apx_clientEventListener_t*) arg);
}

apx_serverEventListener_t *apx_serverEventListener_clone(apx_serverEventListener_t *other)
{
   if (other != NULL)
   {
      apx_serverEventListener_t *self = (apx_serverEventListener_t*) malloc(sizeof(apx_serverEventListener_t));
      if (self != NULL)
      {
         *self = *other;
      }
      return self;
   }
   return NULL;
}

void apx_serverEventListener_delete(apx_serverEventListener_t *self)
{
   if (self != NULL)
   {
      free(self);
   }
}

void apx_serverEventListener_vdelete(void *arg)
{
   apx_serverEventListener_delete((apx_serverEventListener_t*) arg);
}


apx_serverConnectionEventListener_t *apx_connectionEventListener_clone(apx_serverConnectionEventListener_t *other)
{
   if (other != NULL)
   {
      apx_serverConnectionEventListener_t *self = (apx_serverConnectionEventListener_t*) malloc(sizeof(apx_serverConnectionEventListener_t));
      if (self != NULL)
      {
         *self = *other;
      }
      return self;
   }
   return NULL;
}
void apx_connectionEventListener_delete(apx_serverConnectionEventListener_t *self)
{
   if (self != NULL)
   {
      free(self);
   }
}

void apx_connectionEventListener_vdelete(void *arg)
{
   apx_connectionEventListener_delete((apx_serverConnectionEventListener_t*) arg);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


