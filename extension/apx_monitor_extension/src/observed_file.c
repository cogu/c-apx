/*****************************************************************************
 * \file      observed_file.c
 * \author    Conny Gustafsson
 * \date      2021-01-01
 * \brief     Current state of an observed file. Used by monitor extension.
 *
 * Copyright (c) 2021-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/observed_file.h"
#include <assert.h>
#include <malloc.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_observed_file_create(apx_observed_file_t *self, rmf_extended_file_info_t const *file_info)
{
  if (self != NULL)
  {
    if (file_info != NULL)
    {
      rmf_extended_file_info_create_copy(&self->file_info, file_info);
    }
    else
    {
      rmf_extended_file_info_create(&self->file_info, APX_INVALID_CONNECTION_ID, RMF_INVALID_ADDRESS, 0u, NULL,
        RMF_FILE_TYPE_FIXED, APX_DATA_STATE_INIT, RMF_DIGEST_TYPE_NONE, NULL);
    }
  }
}

void apx_observed_file_destroy(apx_observed_file_t *self)
{
  if (self != NULL)
  {
    rmf_extended_file_info_destroy(&self->file_info);
  }
}

apx_observed_file_t *apx_observed_file_new(rmf_extended_file_info_t const *file_info)
{
  apx_observed_file_t *self = (apx_observed_file_t *)malloc(sizeof(apx_observed_file_t));
  if (self != NULL)
  {
    apx_observed_file_create(self, file_info);
  }
  return self;
}

apx_observed_file_t *apx_observed_file_new_from_file_info(
  uint32_t connection_id, rmf_file_info_t const *file_info, apx_data_state_t data_state)
{
  if (file_info != NULL)
  {
    apx_observed_file_t *self = (apx_observed_file_t *)malloc(sizeof(apx_observed_file_t));
    if (self != NULL)
    {
      apx_error_t rc =
        rmf_extended_file_info_create_from_file_info(&self->file_info, connection_id, file_info, data_state);
      if (rc != APX_NO_ERROR)
      {
        free(self);
        self = NULL;
      }
    }
    return self;
  }
  return NULL;
}

void apx_observed_file_delete(apx_observed_file_t *self)
{
  if (self != NULL)
  {
    apx_observed_file_destroy(self);
    free(self);
  }
}

void apx_observed_file_vdelete(void *arg) { apx_observed_file_delete((apx_observed_file_t *)arg); }

rmf_extended_file_info_t *apx_observed_file_get_file_info(apx_observed_file_t *self)
{
  if (self != NULL)
  {
    return &self->file_info;
  }
  return NULL;
}

const char *apx_observed_file_get_name(apx_observed_file_t const *self)
{
  if (self != NULL)
  {
    return rmf_extended_file_info_name(&self->file_info);
  }
  return NULL;
}

uint32_t apx_observed_file_get_address(apx_observed_file_t const *self)
{
  if (self != NULL)
  {
    return rmf_extended_file_info_address(&self->file_info);
  }
  return RMF_INVALID_ADDRESS;
}

uint32_t apx_observed_file_get_size(apx_observed_file_t const *self)
{
  if (self != NULL)
  {
    return rmf_extended_file_info_size(&self->file_info);
  }
  return 0u;
}

apx_data_state_t apx_observed_file_get_data_state(apx_observed_file_t const *self)
{
  if (self != NULL)
  {
    return rmf_extended_file_info_data_state(&self->file_info);
  }
  return APX_DATA_STATE_INIT;
}

void apx_observed_file_set_data_state(apx_observed_file_t *self, apx_data_state_t data_state)
{
  if (self != NULL)
  {
    rmf_extended_file_info_set_data_state(&self->file_info, data_state);
  }
}
