/*****************************************************************************
* \file      apx_file_manager.h
* \author    Conny Gustafsson
* \date      2020-01-22
* \brief     APX file manager
*
* Copyright (c) 2020-2021 Conny Gustafsson
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
#ifndef APX_FILE_MANAGER_H
#define APX_FILE_MANAGER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/file.h"
//#include "apx/file_manager_defs.h"
#include "apx/file_manager_shared.h"
#include "apx/file_manager_worker.h"
#include "apx/file_manager_receiver.h"
#include "adt_bytearray.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
//forward declaration


typedef struct apx_fileManager_tag
{
   apx_fileManagerShared_t shared;
   apx_fileManagerWorker_t worker;
   apx_fileManagerReceiver_t receiver;
   apx_mode_t mode;
}apx_fileManager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_fileManager_create(apx_fileManager_t* self, uint8_t mode, apx_connectionInterface_t const* parent_connection, apx_allocator_t* allocator);
void apx_fileManager_destroy(apx_fileManager_t* self);
void apx_fileManager_start(apx_fileManager_t* self);
void apx_fileManager_stop(apx_fileManager_t* self);
void apx_fileManager_connected(apx_fileManager_t* self);
void apx_fileManager_disconnected(apx_fileManager_t* self);
apx_file_t* apx_fileManager_create_local_file(apx_fileManager_t* self, rmf_fileInfo_t const* file_info);
apx_error_t apx_fileManager_publish_local_file(apx_fileManager_t* self, rmf_fileInfo_t const* file_info);
apx_file_t* apx_fileManager_find_file_by_address(apx_fileManager_t* self, uint32_t address);
apx_file_t* apx_fileManager_find_local_file_by_name(apx_fileManager_t* self, char const* name);
apx_file_t* apx_fileManager_find_remote_file_by_name(apx_fileManager_t* self, char const* name);
apx_error_t apx_fileManager_message_received(apx_fileManager_t* self, uint8_t const* msg_data, apx_size_t msg_len);
apx_error_t apx_fileManager_send_local_const_data(apx_fileManager_t* self, uint32_t address, uint8_t const* data, apx_size_t size);
apx_error_t apx_fileManager_send_local_data(apx_fileManager_t* self, uint32_t address, uint8_t* data, apx_size_t size); //file_manager takes ownership of data when called
apx_error_t apx_fileManager_send_open_file_request(apx_fileManager_t* self, uint32_t address);
apx_error_t apx_fileManager_send_error_code(apx_fileManager_t* self, apx_error_t error_code);
uint16_t apx_fileManager_get_num_pending_worker_commands(apx_fileManager_t* self);
#ifdef UNIT_TEST
bool apx_fileManager_run(apx_fileManager_t* self);
apx_size_t apx_fileManager_num_pending_commands(apx_fileManager_t* self);
#endif

#endif //APX_FILE_MANAGER_H
