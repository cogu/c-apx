/*****************************************************************************
* \file      file_manager.h
* \author    Conny Gustafsson
* \date      2020-01-22
* \brief     APX file manager
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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


typedef struct apx_file_manager_tag
{
   apx_file_manager_shared_t shared;
   apx_file_manager_worker_t worker;
   apx_file_manager_receiver_t receiver;
   apx_mode_t mode;
}apx_file_manager_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
apx_error_t apx_file_manager_create(apx_file_manager_t* self, uint8_t mode, apx_connection_interface_t const* parent_connection, apx_allocator_t* allocator);
void apx_file_manager_destroy(apx_file_manager_t* self);
void apx_file_manager_start(apx_file_manager_t* self);
void apx_file_manager_stop(apx_file_manager_t* self);
void apx_file_manager_connected(apx_file_manager_t* self);
void apx_file_manager_disconnected(apx_file_manager_t* self);
apx_file_t* apx_file_manager_create_local_file(apx_file_manager_t* self, rmf_file_info_t const* file_info);
apx_error_t apx_file_manager_publish_local_file(apx_file_manager_t* self, rmf_file_info_t const* file_info);
apx_file_t* apx_file_manager_find_file_by_address(apx_file_manager_t* self, uint32_t address);
apx_file_t* apx_file_manager_find_local_file_by_name(apx_file_manager_t* self, char const* name);
apx_file_t* apx_file_manager_find_remote_file_by_name(apx_file_manager_t* self, char const* name);
apx_error_t apx_file_manager_message_received(apx_file_manager_t* self, uint8_t const* msg_data, apx_size_t msg_len);
apx_error_t apx_file_manager_send_local_const_data(apx_file_manager_t* self, uint32_t address, uint8_t const* data, apx_size_t size);
apx_error_t apx_file_manager_send_local_data(apx_file_manager_t* self, uint32_t address, uint8_t* data, apx_size_t size); //file_manager takes ownership of data when called
apx_error_t apx_file_manager_send_open_file_request(apx_file_manager_t* self, uint32_t address);
apx_error_t apx_file_manager_send_error_code(apx_file_manager_t* self, apx_error_t error_code);
uint16_t apx_file_manager_get_num_pending_worker_commands(apx_file_manager_t* self);
apx_error_t apx_file_manager_send_connection_create(apx_file_manager_t* self, apx_connection_id_t connection_id, apx_connection_state_t connection_state, char const* tag);
#ifdef UNIT_TEST
bool apx_file_manager_run(apx_file_manager_t* self);
#endif

#endif //APX_FILE_MANAGER_H
