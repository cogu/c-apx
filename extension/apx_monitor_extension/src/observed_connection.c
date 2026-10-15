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
#include "apx/extension/observed_connection.h"
#include <assert.h>
#include <malloc.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_observed_connection_create(apx_observed_connection_t *self, apx_server_connection_t *server_connection)
{
  if ((self != NULL) && (server_connection != NULL))
  {
    adt_list_create(&self->file_list, apx_observed_file_vdelete);
    self->server_connection = server_connection;
    self->connection_id = apx_server_connection_get_connection_id(server_connection);
    self->tag = apx_server_connection_get_tag(server_connection);
    self->connection_type = APX_CONNECTION_TYPE_DEFAULT;
    self->connection_state = APX_CONNECTION_STATE_CONNECTING;
  }
}

void apx_observed_connection_destroy(apx_observed_connection_t *self)
{
  if (self != NULL)
  {
    adt_list_destroy(&self->file_list);
    adt_str_delete(self->tag);
  }
}

apx_observed_connection_t *apx_observed_connection_new(apx_server_connection_t *server_connection)
{
  apx_observed_connection_t *self = (apx_observed_connection_t *)malloc(sizeof(apx_observed_connection_t));
  if (self != NULL)
  {
    apx_observed_connection_create(self, server_connection);
  }
  return self;
}

void apx_observed_connection_delete(apx_observed_connection_t *self)
{
  if (self != NULL)
  {
    apx_observed_connection_destroy(self);
    free(self);
  }
}

void apx_observed_connection_vdelete(void *arg) { apx_observed_connection_delete((apx_observed_connection_t *)arg); }

char const *apx_observed_connection_tag(apx_observed_connection_t *const self)
{
  if ((self != NULL) && (self->tag != NULL))
  {
    return adt_str_cstr((adt_str_t *)self->tag);
  }
  return NULL;
}

apx_connection_id_t apx_observed_connection_connection_id(apx_observed_connection_t *const self)
{
  if (self != NULL)
  {
    return self->connection_id;
  }
  return APX_INVALID_CONNECTION_ID;
}

apx_connection_type_t apx_observed_connection_get_connection_type(apx_observed_connection_t *const self)
{
  if (self != NULL)
  {
    return self->connection_type;
  }
  return APX_CONNECTION_TYPE_DEFAULT;
}

void apx_observed_connection_set_connection_type(apx_observed_connection_t *self, apx_connection_type_t connection_type)
{
  if (self != NULL)
  {
    self->connection_type = connection_type;
  }
}

apx_connection_state_t apx_observed_connection_get_connection_state(apx_observed_connection_t *const self)
{
  if (self != NULL)
  {
    return self->connection_state;
  }
  return APX_CONNECTION_STATE_CREATED;
}

void apx_observed_connection_set_connection_state(
  apx_observed_connection_t *self, apx_connection_state_t connection_state)
{
  if (self != NULL)
  {
    self->connection_state = connection_state;
  }
}

apx_error_t apx_observed_connection_add_file(apx_observed_connection_t *self, apx_observed_file_t *file)
{
  if ((self != NULL) && (file != NULL))
  {
    adt_list_insert(&self->file_list, file);
    return APX_NO_ERROR;
  }
  return APX_INVALID_ARGUMENT_ERROR;
}

apx_observed_file_t *apx_observed_connection_find_file_by_name(apx_observed_connection_t *self, const char *name)
{
  if ((self != NULL) && (name != NULL))
  {
    adt_list_elem_t *iter = adt_list_iter_first(&self->file_list);
    while (iter != NULL)
    {
      apx_observed_file_t *file = (apx_observed_file_t *)iter->pItem;
      if ((file != NULL) && (strcmp(apx_observed_file_get_name(file), name) == 0))
      {
        return file;
      }
      iter = adt_list_iter_next(iter);
    }
  }
  return NULL;
}

apx_observed_file_t *apx_observed_connection_find_file_by_address(apx_observed_connection_t *self, uint32_t address)
{
  if (self != NULL)
  {
    adt_list_elem_t *iter = adt_list_iter_first(&self->file_list);
    while (iter != NULL)
    {
      apx_observed_file_t *file = (apx_observed_file_t *)iter->pItem;
      if (file != NULL)
      {
        uint32_t const file_addr = apx_observed_file_get_address(file) & RMF_ADDRESS_MASK_INTERNAL;
        uint32_t const target_addr = address & RMF_ADDRESS_MASK_INTERNAL;
        uint32_t const file_size = apx_observed_file_get_size(file);
        if ((target_addr >= file_addr) && (target_addr < (file_addr + file_size)))
        {
          return file;
        }
      }
      iter = adt_list_iter_next(iter);
    }
  }
  return NULL;
}

int32_t apx_observed_connection_num_files(apx_observed_connection_t const *self)
{
  if (self != NULL)
  {
    return adt_list_length(&self->file_list);
  }
  return -1;
}

adt_list_t *apx_observed_connection_get_file_list(apx_observed_connection_t *self)
{
  if (self != NULL)
  {
    return &self->file_list;
  }
  return NULL;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
