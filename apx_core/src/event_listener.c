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

apx_client_event_listener_t *apx_client_event_listener_clone(apx_client_event_listener_t *other)
{
   if (other != NULL)
   {
      apx_client_event_listener_t *self = (apx_client_event_listener_t*) malloc(sizeof(apx_client_event_listener_t));
      if (self != NULL)
      {
         *self = *other;
      }
      return self;
   }
   return NULL;
}

void apx_client_event_listener_delete(apx_client_event_listener_t *self)
{
   if (self != NULL)
   {
      free(self);
   }
}
void apx_client_event_listener_vdelete(void *arg)
{
   apx_client_event_listener_delete((apx_client_event_listener_t*) arg);
}

apx_server_event_listener_t *apx_server_event_listener_clone(apx_server_event_listener_t *other)
{
   if (other != NULL)
   {
      apx_server_event_listener_t *self = (apx_server_event_listener_t*) malloc(sizeof(apx_server_event_listener_t));
      if (self != NULL)
      {
         *self = *other;
      }
      return self;
   }
   return NULL;
}

void apx_server_event_listener_delete(apx_server_event_listener_t *self)
{
   if (self != NULL)
   {
      free(self);
   }
}

void apx_server_event_listener_vdelete(void *arg)
{
   apx_server_event_listener_delete((apx_server_event_listener_t*) arg);
}


apx_server_connection_event_listener_t *apx_connection_event_listener_clone(apx_server_connection_event_listener_t *other)
{
   if (other != NULL)
   {
      apx_server_connection_event_listener_t *self = (apx_server_connection_event_listener_t*) malloc(sizeof(apx_server_connection_event_listener_t));
      if (self != NULL)
      {
         *self = *other;
      }
      return self;
   }
   return NULL;
}
void apx_connection_event_listener_delete(apx_server_connection_event_listener_t *self)
{
   if (self != NULL)
   {
      free(self);
   }
}

void apx_connection_event_listener_vdelete(void *arg)
{
   apx_connection_event_listener_delete((apx_server_connection_event_listener_t*) arg);
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


