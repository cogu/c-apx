/*****************************************************************************
* \file      monitor_connection_state.h
* \author    Conny Gustafsson
* \date      2021-04-10
* \brief     Current connection state seen by a server monitor
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

void apx_observedConnection_create(apx_observedConnection_t* self, apx_serverConnection_t* server_connection)
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

void apx_observedConnection_destroy(apx_observedConnection_t* self)
{
   if (self != NULL)
   {
      adt_list_destroy(&self->file_list);
      adt_str_delete(self->tag);
   }
}

apx_observedConnection_t* apx_observedConnection_new(apx_serverConnection_t* server_connection)
{
   apx_observedConnection_t* self = (apx_observedConnection_t*)malloc(sizeof(apx_observedConnection_t));
   if (self != NULL)
   {
      apx_observedConnection_create(self, server_connection);
   }
   return self;
}

void apx_observedConnection_delete(apx_observedConnection_t* self)
{
   if (self != NULL)
   {
      apx_observedConnection_destroy(self);
      free(self);
   }
}

void apx_observedConnection_vdelete(void* arg)
{
   apx_observedConnection_delete((apx_observedConnection_t*)arg);
}

char const* apx_observedConnection_tag(apx_observedConnection_t* const self)
{
   if ( (self != NULL) && (self->tag != NULL) )
   {
      return adt_str_cstr((adt_str_t*)self->tag);
   }
   return NULL;
}

apx_connectionId_t apx_observedConnection_connection_id(apx_observedConnection_t* const self)
{
   if (self != NULL)
   {
      return self->connection_id;
   }
   return APX_INVALID_CONNECTION_ID;
}

apx_connectionType_t apx_observedConnection_get_connection_type(apx_observedConnection_t* const self)
{
   if (self != NULL)
   {
      return self->connection_type;
   }
   return APX_CONNECTION_TYPE_DEFAULT;
}

void apx_observedConnection_set_connection_type(apx_observedConnection_t* self, apx_connectionType_t connection_type)
{
   if (self != NULL)
   {
      self->connection_type = connection_type;
   }
}

apx_connectionState_t apx_observedConnection_get_connection_state(apx_observedConnection_t* const self)
{
   if (self != NULL)
   {
      return self->connection_state;
   }
   return APX_CONNECTION_STATE_CREATED;
}

void apx_observedConnection_set_connection_state(apx_observedConnection_t* self, apx_connectionState_t connection_state)
{
   if (self != NULL)
   {
      self->connection_state = connection_state;
   }
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
