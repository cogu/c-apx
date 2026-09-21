/*****************************************************************************
* \file      file_manager_receiver.h
* \author    Conny Gustafsson
* \date      2020-02-08
* \brief     Receive buffer mechanism for file manager
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_FILEMANAGER_RECEIVER_H
#define APX_FILEMANAGER_RECEIVER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "remotefile.h"


//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef struct apx_file_manager_receiver_tag
{
   uint8_t *buf_data;
   apx_size_t buf_size;
   apx_size_t buf_pos;
   uint32_t start_address;
} apx_file_manager_receiver_t;

typedef struct apx_file_manager_reception_result_tag
{
   bool is_complete;
   uint32_t address;
   uint8_t const* data;
   apx_size_t size;
} apx_file_manager_reception_result_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManagerReceiver_create(apx_file_manager_receiver_t *self);
void apx_fileManagerReceiver_destroy(apx_file_manager_receiver_t *self);
void apx_fileManagerReceiver_reset(apx_file_manager_receiver_t *self);
apx_error_t apx_fileManagerReceiver_reserve(apx_file_manager_receiver_t *self, apx_size_t size);
apx_size_t apx_fileManagerReceiver_buffer_size(apx_file_manager_receiver_t const* self);
apx_error_t apx_fileManagerReceiver_write(apx_file_manager_receiver_t *self, apx_file_manager_reception_result_t *result, uint32_t address, uint8_t const* data, apx_size_t size, bool more_bit);

#endif //APX_FILEMANAGER_RECEIVER_H
