/*****************************************************************************
 * \file      apx_diag_client_file.h
 * \author    Conny Gustafsson
 * \date      2026-10-30
 * \brief     Owned binary diagnostic file reception
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_DIAG_CLIENT_FILE_H
#define APX_DIAG_CLIENT_FILE_H

#include "apx/error.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct apx_diag_client_file_tag
{
  uint8_t *data;
  uint8_t *received;
  uint32_t size;
  uint32_t received_size;
} apx_diag_client_file_t;

apx_diag_client_file_t *apx_diag_client_file_new(uint32_t size);
void apx_diag_client_file_delete(apx_diag_client_file_t *self);
apx_error_t apx_diag_client_file_write(
  apx_diag_client_file_t *self, uint32_t offset, uint8_t const *data, uint32_t size);
bool apx_diag_client_file_is_complete(apx_diag_client_file_t const *self);
apx_error_t apx_diag_client_file_save(apx_diag_client_file_t const *self, char const *path);

#endif
