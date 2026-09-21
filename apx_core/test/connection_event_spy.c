/*****************************************************************************
* \file      connection_event_spy.c
* \author    Conny Gustafsson
* \date      2019-05-27
* \brief     Connection event spy
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <stdio.h>
#include "connection_event_spy.h"
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
void apx_connection_event_spy_create(apx_connection_event_spy_t *self)
{
   if (self != NULL)
   {
      self->headerAcceptedCount = 0;
      self->fileCreateCount = 0;
      self->lastConnection = NULL;
      self->lastFileInfo = NULL;
   }
}

void apx_connection_event_spy_destroy(apx_connection_event_spy_t *self)
{
   if (self != NULL)
   {
      if (self->lastFileInfo != NULL)
      {
         apx_file_info_delete(self->lastFileInfo);
      }
   }
}

void apx_connection_event_spy_register(apx_connection_event_spy_t *self, apx_connection_base_t *connection)
{
   if ((self != NULL) && (connection != NULL) )
   {
      apx_connection_event_listener_t handler;
      memset(&handler, 0, sizeof(handler));
      handler.arg = (void*) self;
      handler.headerAccepted2 = apx_connection_event_spy_header_accepted;
      handler.fileCreate2 = apx_connection_event_spy_file_create;
      (void)apx_connection_base_register_event_listener(connection, &handler);
   }
}

void apx_connection_event_spy_header_accepted(void *arg, apx_connection_base_t *connection)
{
   apx_connection_event_spy_t *self = (apx_connection_event_spy_t*) arg;
   if ( (self != NULL) && (connection != NULL) )
   {

      self->headerAcceptedCount++;
      self->lastConnection = connection;
   }
}

void apx_connection_event_spy_file_create(void *arg, apx_connection_base_t *connection, const apx_file_info_t *file_info)
{
   apx_connection_event_spy_t *self = (apx_connection_event_spy_t*) arg;
   if ( (self != NULL) && (connection != NULL) && (file_info != NULL))
   {
      self->fileCreateCount++;
      self->lastConnection = connection;
      if (self->lastFileInfo != NULL)
      {
         apx_file_info_delete(self->lastFileInfo);
      }
      self->lastFileInfo = apx_file_info_clone(file_info);
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


