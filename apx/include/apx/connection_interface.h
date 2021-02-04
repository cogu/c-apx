/*****************************************************************************
* \file      connection_interface.h
* \author    Conny Gustafsson
* \date      2021-01-01
* \brief     Abstract connection interface
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
#ifndef APX_CONNECTION_INTERFACE_H
#define APX_CONNECTION_INTERFACE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/error.h"
#include "apx/file.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
typedef apx_error_t (apx_connection_transmit_data_message_func)(void* arg, uint32_t write_address, bool more_bit, uint8_t const* data, int32_t size, int32_t* bytes_available);
typedef apx_error_t (apx_connection_transmit_direct_message_func)(void* arg, uint8_t const* data, int32_t size, int32_t* bytes_available);

typedef struct apx_connectionInterface_tag
{
   void* arg;
   //Transmit methods
   int32_t(*transmit_max_buffer_size)(void* arg); //Returns largest possible buffer size the transmitter can provide
   int32_t(*transmit_current_bytes_avaiable)(void* arg); //Returns avaiable bytes in current buffer
   void (*transmit_begin)(void* arg); //Locks transmit resource
   void (*transmit_end)(void* arg); //Unlocks transmit resource
   apx_connection_transmit_data_message_func* transmit_data_message; //Message that is written into the remotefile address space
   apx_connection_transmit_direct_message_func* transmit_direct_message; //Message that is written outside the remotefile address space

   // Notification callbacks
   apx_error_t (*remote_file_published_notification)(void* arg, apx_file_t* file);
   apx_error_t (*remote_file_write_notification)(void* arg, apx_file_t* file, uint32_t offset, uint8_t const* data, apx_size_t size);
} apx_connectionInterface_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //APX_CONNECTION_INTERFACE_H
