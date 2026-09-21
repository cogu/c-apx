/*****************************************************************************
* \file      connection_interface.h
* \author    Conny Gustafsson
* \date      2021-01-01
* \brief     Abstract connection interface
*
* Copyright (c) 2021-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_CONNECTION_INTERFACE_H
#define APX_CONNECTION_INTERFACE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/file.h"
#include "apx/remotefile.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef apx_error_t (apx_connection_transmit_data_message_func)(void* arg, uint32_t write_address, bool more_bit, uint8_t const* data, int32_t size, int32_t* bytes_available);
typedef apx_error_t (apx_connection_transmit_direct_message_func)(void* arg, uint8_t const* data, int32_t size, int32_t* bytes_available);

typedef struct apx_connection_interface_tag
{
   void* arg;
   //Transmit methods
   int32_t(*transmit_max_buffer_size)(void* arg); //Returns largest possible buffer size the transmitter can provide
   int32_t(*transmit_current_bytes_avaiable)(void* arg); //Returns avaiable bytes in current buffer
   void (*transmit_begin)(void* arg); //Locks transmit resource
   void (*transmit_end)(void* arg); //Unlocks transmit resource
   apx_connection_transmit_data_message_func* transmit_data_message; //Message that is written into the remotefile address space
   apx_connection_transmit_direct_message_func* transmit_direct_message; //Message that is written outside the remotefile address space

   //Connection details
   uint32_t(*get_connection_id)(void* arg);
   rmf_version_id_t(*get_remotefile_protocol_version_id)(void* arg);
   apx_connection_type_t(*get_connection_type)(void* arg);


   // Notification callbacks
   apx_error_t (*remote_file_published_notification)(void* arg, apx_file_t* file);
   apx_error_t (*remote_file_write_notification)(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);
} apx_connection_interface_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_CONNECTION_INTERFACE_H
