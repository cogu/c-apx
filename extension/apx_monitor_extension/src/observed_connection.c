/*****************************************************************************
* \file      observed_connection.c
* \author    Conny Gustafsson
* \date      2021-04-10
* \brief     Current connection state seen by a server monitor
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
#include "apx/extension/observed_connection.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_observedConnection_create(apx_observed_connection_t* self, apx_server_connection_t* server_connection)
{
   if ( (self != NULL) && (server_connection != NULL) )
   {
      adt_list_create(&self->file_list, apx_observedFile_vdelete);
      self->server_connection = server_connection;
      self->connection_id = apx_serverConnection_get_connection_id(server_connection);
      self->tag = apx_serverConnection_get_tag(server_connection);
      self->connection_type = APX_CONNECTION_TYPE_DEFAULT;
      self->connection_state = APX_CONNECTION_STATE_CONNECTING;
   }
}

void apx_observedConnection_destroy(apx_observed_connection_t* self)
{
   if (self != NULL)
   {
      adt_list_destroy(&self->file_list);
      adt_str_delete(self->tag);
   }
}

apx_observed_connection_t* apx_observedConnection_new(apx_server_connection_t* server_connection)
{
   apx_observed_connection_t* self = (apx_observed_connection_t*)malloc(sizeof(apx_observed_connection_t));
   if (self != NULL)
   {
      apx_observedConnection_create(self, server_connection);
   }
   return self;
}

void apx_observedConnection_delete(apx_observed_connection_t* self)
{
   if (self != NULL)
   {
      apx_observedConnection_destroy(self);
      free(self);
   }
}

void apx_observedConnection_vdelete(void* arg)
{
   apx_observedConnection_delete((apx_observed_connection_t*)arg);
}

char const* apx_observed_connection_tag(apx_observed_connection_t* const self)
{
   if ( (self != NULL) && (self->tag != NULL) )
   {
      return adt_str_cstr((adt_str_t*)self->tag);
   }
   return NULL;
}

apx_connection_id_t apx_observedConnection_connection_id(apx_observed_connection_t* const self)
{
   if (self != NULL)
   {
      return self->connection_id;
   }
   return APX_INVALID_CONNECTION_ID;
}

apx_connection_type_t apx_observedConnection_get_connection_type(apx_observed_connection_t* const self)
{
   if (self != NULL)
   {
      return self->connection_type;
   }
   return APX_CONNECTION_TYPE_DEFAULT;
}

void apx_observedConnection_set_connection_type(apx_observed_connection_t* self, apx_connection_type_t connection_type)
{
   if (self != NULL)
   {
      self->connection_type = connection_type;
   }
}

apx_connection_state_t apx_observedConnection_get_connection_state(apx_observed_connection_t* const self)
{
   if (self != NULL)
   {
      return self->connection_state;
   }
   return APX_CONNECTION_STATE_CREATED;
}

void apx_observedConnection_set_connection_state(apx_observed_connection_t* self, apx_connection_state_t connection_state)
{
   if (self != NULL)
   {
      self->connection_state = connection_state;
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
