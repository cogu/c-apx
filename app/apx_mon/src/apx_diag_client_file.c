/*****************************************************************************
 * \file      apx_diag_client_file.c
 * \author    Conny Gustafsson
 * \date      2026-10-30
 * \brief     Binary diagnostic file assembly and saving
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "apx_diag_client_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef MEM_LEAK_CHECK
# include "CMemLeak.h"
#endif

apx_diag_client_file_t *apx_diag_client_file_new(uint32_t size)
{
  apx_diag_client_file_t *self = (apx_diag_client_file_t *)calloc(1u, sizeof(apx_diag_client_file_t));
  if (self != NULL)
  {
    self->size = size;
    if (size > 0u)
    {
      self->data = (uint8_t *)malloc(size);
      self->received = (uint8_t *)calloc((size_t)(size / 8u) + (size % 8u != 0u), 1u);
      if (self->data == NULL || self->received == NULL)
      {
        apx_diag_client_file_delete(self);
        return NULL;
      }
    }
  }
  return self;
}

void apx_diag_client_file_delete(apx_diag_client_file_t *self)
{
  if (self != NULL)
  {
    free(self->data);
    free(self->received);
    free(self);
  }
}

apx_error_t apx_diag_client_file_write(
  apx_diag_client_file_t *self, uint32_t offset, uint8_t const *data, uint32_t size)
{
  if (self == NULL || (data == NULL && size != 0u))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (offset > self->size || size > self->size - offset)
  {
    return APX_BUFFER_BOUNDARY_ERROR;
  }
  if (size > 0u)
  {
    memcpy(self->data + offset, data, size);
    for (uint32_t i = offset; i < offset + size; ++i)
    {
      uint8_t const bit = (uint8_t)(1u << (i % 8u));
      if ((self->received[i / 8u] & bit) == 0u)
      {
        self->received[i / 8u] |= bit;
        self->received_size++;
      }
    }
  }
  return APX_NO_ERROR;
}

bool apx_diag_client_file_is_complete(apx_diag_client_file_t const *self)
{
  return self != NULL && self->received_size == self->size;
}

apx_error_t apx_diag_client_file_save(apx_diag_client_file_t const *self, char const *path)
{
  if (self == NULL || path == NULL)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if (!apx_diag_client_file_is_complete(self))
  {
    return APX_DATA_NOT_COMPLETE_ERROR;
  }
  FILE *file = fopen(path, "wb");
  if (file == NULL)
  {
    return APX_FILE_CREATE_ERROR;
  }
  apx_error_t result = APX_NO_ERROR;
  if (self->size > 0u && fwrite(self->data, 1u, self->size, file) != self->size)
  {
    result = APX_FILE_WRITE_ERROR;
  }
  if (fclose(file) != 0)
  {
    result = APX_FILE_WRITE_ERROR;
  }
  return result;
}
