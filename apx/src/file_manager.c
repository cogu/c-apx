/*****************************************************************************
* \file      file_manager.c
* \author    Conny Gustafsson
* \date      2020-01-27
* \brief     New APX file manager
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include <assert.h>
#include <stdio.h> //DEBUG only
#include "apx/connection_base.h"
#include "apx/node_data.h"
#include "pack.h"

#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void publish_local_files(apx_fileManager_t* self);
static apx_error_t process_message(apx_fileManager_t* self, uint32_t address, uint8_t const* data, apx_size_t size);
static apx_error_t process_command_message(apx_fileManager_t* self, uint8_t const* data, apx_size_t size);
static apx_error_t process_file_write_message(apx_fileManager_t* self, uint32_t address, uint8_t const* data, apx_size_t size);
static apx_error_t process_open_file_request(apx_fileManager_t* self, uint32_t start_address);
static apx_error_t process_close_file_request(apx_fileManager_t* self, uint32_t start_address);
static apx_error_t process_remote_file_published(apx_fileManager_t* self, rmf_fileInfo_t const* file_info);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_fileManager_create(apx_fileManager_t* self, uint8_t mode, apx_connectionInterface_t const* parent_connection, apx_allocator_t* allocator)
{
   if (self != NULL)
   {
      apx_error_t result;
      self->mode = mode;
      result = apx_fileManagerReceiver_create(&self->receiver);
      if (result == APX_NO_ERROR)
      {
         result = apx_fileManagerWorker_create(&self->worker, &self->shared, mode);
         if (result == APX_NO_ERROR)
         {
            apx_fileManagerShared_create(&self->shared, parent_connection, allocator);
         }
         else
         {
            apx_fileManagerReceiver_destroy(&self->receiver);
         }
      }
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManager_destroy(apx_fileManager_t *self)
{
   if (self != NULL)
   {
      apx_fileManagerWorker_destroy(&self->worker);
      apx_fileManagerReceiver_destroy(&self->receiver);
      apx_fileManagerShared_destroy(&self->shared);
   }
}

void apx_fileManager_start(apx_fileManager_t *self)
{
   if (self != NULL)
   {
#ifndef UNIT_TEST
      apx_fileManagerWorker_start(&self->worker);
#endif
   }
}

void apx_fileManager_stop(apx_fileManager_t *self)
{
   if (self != NULL)
   {
#ifndef UNIT_TEST
      apx_fileManagerWorker_stop(&self->worker);
#endif
   }
}

void apx_fileManager_connected(apx_fileManager_t* self)
{
   if (self != NULL)
   {
      if (self->mode == APX_CLIENT_MODE)
      {
         publish_local_files(self);
      }
      else
      {
         apx_fileManagerWorker_preare_acknowledge(&self->worker);
      }
   }
}

void apx_fileManager_disconnected(apx_fileManager_t* self)
{
   if (self != NULL)
   {
      apx_fileManagerShared_disconnected(&self->shared);
   }
}

apx_file_t* apx_fileManager_create_local_file(apx_fileManager_t* self, rmf_fileInfo_t const* file_info)
{
   if (self != NULL)
   {
      apx_file_t* file = apx_fileManagerShared_create_local_file(&self->shared, file_info);
      if (file != NULL)
      {
         apx_file_set_file_manager(file, self);
      }
      return file;
   }
   return NULL;
}

apx_error_t apx_fileManager_publish_local_file(apx_fileManager_t* self, rmf_fileInfo_t const* file_info)
{
   if ((self != NULL) && (file_info != NULL))
   {
      apx_error_t retval = APX_MEM_ERROR;
      rmf_fileInfo_t* cloned_info = rmf_fileInfo_clone(file_info);
      if (cloned_info != NULL)
      {
         retval = apx_fileManagerWorker_prepare_publish_local_file(&self->worker, cloned_info);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_file_t* apx_fileManager_find_file_by_address(apx_fileManager_t* self, uint32_t address)
{
   if (self != NULL)
   {
      return apx_fileManagerShared_find_file_by_address(&self->shared, address);
   }
   return NULL;
}

apx_file_t* apx_fileManager_find_local_file_by_name(apx_fileManager_t* self, char const* name)
{
   if (self != NULL)
   {
      return apx_fileManagerShared_find_local_file_by_name(&self->shared, name);
   }
   return NULL;
}

apx_file_t* apx_fileManager_find_remote_file_by_name(apx_fileManager_t* self, char const* name)
{
   if (self != NULL)
   {
      return apx_fileManagerShared_find_remote_file_by_name(&self->shared, name);
   }
   return NULL;
}

apx_error_t apx_fileManager_message_received(apx_fileManager_t* self, uint8_t const* msg_data, apx_size_t msg_len)
{
   if ( (self != NULL) && (msg_data != NULL) )
   {
      uint32_t address = RMF_INVALID_ADDRESS;
      bool more_bit = false;
      apx_size_t const header_size = rmf_address_decode(msg_data, msg_data + msg_len, &address, &more_bit);
      if (header_size > 0)
      {
         apx_error_t error_code;
         apx_fileManagerReceptionResult_t result;
         assert(msg_len >= header_size);
         error_code = apx_fileManagerReceiver_write(&self->receiver, &result, address, msg_data + header_size, msg_len - header_size, more_bit);
         if (error_code != APX_NO_ERROR)
         {
            return error_code;
         }
         else if (result.is_complete)
         {
            return process_message(self, result.address, result.data, result.size);
         }
         else
         {
            //Wait for more data to arrive
         }
      }
      else
      {
         return APX_INVALID_MSG_ERROR;
      }
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManager_send_local_const_data(apx_fileManager_t* self, uint32_t address, uint8_t const* data, apx_size_t size)
{
   if (self != NULL && data != NULL)
   {
      apx_file_t* file = apx_fileManagerShared_find_file_by_address(&self->shared, address);
      if (file == NULL)
      {
         return APX_FILE_NOT_FOUND_ERROR;
      }
      if (!apx_file_is_open(file))
      {
         return APX_FILE_NOT_OPEN_ERROR;
      }
      return apx_fileManagerWorker_prepare_send_local_const_data(&self->worker, address, data, (uint32_t)size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManager_send_local_data(apx_fileManager_t* self, uint32_t address, uint8_t* data, apx_size_t size)
{
   if (self != NULL && data != NULL)
   {
      apx_file_t* file = apx_fileManagerShared_find_file_by_address(&self->shared, address);
      if (file == NULL)
      {
         return APX_FILE_NOT_FOUND_ERROR;
      }
      if (!apx_file_is_open(file))
      {
         return APX_FILE_NOT_OPEN_ERROR;
      }
      return apx_fileManagerWorker_prepare_send_local_data(&self->worker, address, data, (uint32_t)size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManager_send_open_file_request(apx_fileManager_t* self, uint32_t address)
{
   if (self != NULL)
   {
      return apx_fileManagerWorker_prepare_send_open_file_request(&self->worker, address);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManager_send_error_code(apx_fileManager_t* self, apx_error_t error_code)
{
   if (self != NULL)
   {
      (void)error_code;
      //TODO: Implement later
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

uint16_t apx_fileManager_get_num_pending_worker_commands(apx_fileManager_t* self)
{
   if (self != NULL)
   {
      return apx_fileManagerWorker_num_pending_commands(&self->worker);
   }
   return 0u;
}

#ifdef UNIT_TEST
bool apx_fileManager_run(apx_fileManager_t* self)
{
   if (self != NULL)
   {
      return apx_fileManagerWorker_run(&self->worker);
   }
   return false;
}

apx_size_t apx_fileManager_num_pending_commands(apx_fileManager_t* self)
{
   if (self != NULL)
   {
      return apx_fileManagerWorker_num_pending_commands(&self->worker);
   }
   return 0;
}
#endif



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void publish_local_files(apx_fileManager_t* self)
{
   adt_ary_t local_file_list;
   int32_t i;
   int32_t num_files;
   adt_ary_create(&local_file_list, NULL);
   num_files = apx_fileManagerShared_copy_local_file_info(&self->shared, &local_file_list);
   for (i=0; i < num_files; i++)
   {
      rmf_fileInfo_t* file_info = (rmf_fileInfo_t*) adt_ary_value(&local_file_list, i);
      if (file_info != NULL)
      {
         //Worker takes memory ownership of file_info
         apx_error_t result = apx_fileManagerWorker_prepare_publish_local_file(&self->worker, file_info);
         if (result != APX_NO_ERROR)
         {
            //TODO: Error handling
         }
      }
   }
   adt_ary_destroy(&local_file_list);
}

static apx_error_t process_message(apx_fileManager_t* self, uint32_t address, uint8_t const* data, apx_size_t size)
{
   if (address == RMF_CMD_AREA_START_ADDRESS)
   {
      return process_command_message(self, data, size);
   }
   else if (address < RMF_CMD_AREA_START_ADDRESS)
   {
      return process_file_write_message(self, address, data, size);
   }
   return APX_INVALID_ADDRESS_ERROR;
}

static apx_error_t process_command_message(apx_fileManager_t* self, uint8_t const* data, apx_size_t size)
{
   if (size < RMF_CMD_TYPE_SIZE)
   {
      return APX_INVALID_MSG_ERROR;
   }
   apx_error_t retval = APX_NO_ERROR;
   uint32_t const cmd_type = unpackLE(data, RMF_CMD_TYPE_SIZE);
   apx_size_t const cmd_size = size - RMF_CMD_TYPE_SIZE;
   uint8_t const* next = data + RMF_CMD_TYPE_SIZE;
   rmf_fileInfo_t* file_info;
   apx_size_t decoded_size;
   switch (cmd_type)
   {
   case RMF_CMD_PUBLISH_FILE_MSG:
      file_info = rmf_fileInfo_make_empty();
      if (file_info == NULL)
      {
         retval = APX_MEM_ERROR;
      }
      else
      {
         decoded_size = rmf_decode_publish_file_cmd(data, size, file_info);
         if (decoded_size > 0u)
         {
            retval = process_remote_file_published(self, file_info);
         }
         else
         {
            retval = APX_INVALID_MSG_ERROR;
         }
         rmf_fileInfo_delete(file_info);
      }
      break;
   case RMF_CMD_REVOKE_FILE_MSG:
      break;
   case RMF_CMD_OPEN_FILE_MSG:
      if (cmd_size == RMF_FILE_OPEN_CMD_SIZE)
      {
         uint32_t const address = unpackLE(next, UINT32_SIZE);
         retval = process_open_file_request(self, address);
      }
      else
      {
         retval = APX_INVALID_MSG_ERROR;
      }
      break;
   case RMF_CMD_CLOSE_FILE_MSG:
      if (cmd_size == RMF_FILE_CLOSE_CMD_SIZE)
      {
         uint32_t const address = unpackLE(next, UINT32_SIZE);
         retval = process_close_file_request(self, address);
      }
      else
      {
         retval = APX_INVALID_MSG_ERROR;
      }
      break;
   default:
      retval = APX_UNSUPPORTED_ERROR;
   }
   return retval;
}

static apx_error_t process_file_write_message(apx_fileManager_t* self, uint32_t address, uint8_t const* data, apx_size_t size)
{
   apx_file_t *file = apx_fileManagerShared_find_file_by_address(&self->shared, address | RMF_HIGH_ADDR_BIT);
   if (file == NULL)
   {
      return APX_INVALID_WRITE_ERROR;
   }
   if (!apx_file_is_open(file))
   {
      //Ignore writes on closed files
      return APX_NO_ERROR;
   }
   apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(&self->shared);
   if (connection == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   uint32_t const start_address = apx_file_get_address_without_flags(file);
   assert(start_address <= address);
   uint32_t const offset = address - start_address;
   return connection->remote_file_write_notification(connection->arg, file, offset, data, size);
}

static apx_error_t process_open_file_request(apx_fileManager_t* self, uint32_t start_address)
{
   apx_file_t* file = apx_fileManagerShared_find_file_by_address(&self->shared, start_address);
   if (file == NULL)
   {
      return APX_FILE_NOT_FOUND_ERROR;
   }
   apx_file_open(file);
   apx_file_open_notify(file);
   return APX_NO_ERROR;
}

static apx_error_t process_close_file_request(apx_fileManager_t* self, uint32_t start_address)
{
   (void)self;
   (void)start_address;
   return APX_NOT_IMPLEMENTED_ERROR;
}

static apx_error_t process_remote_file_published(apx_fileManager_t* self, rmf_fileInfo_t const* file_info)
{
   apx_file_t* file = apx_fileManagerShared_create_remote_file(&self->shared, file_info);
   if (file == NULL)
   {
      return APX_FILE_CREATE_ERROR;
   }
   apx_file_set_file_manager(file, self);
   apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(&self->shared);
   if (connection == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   assert(connection->remote_file_published_notification != NULL);
   return connection->remote_file_published_notification(connection->arg, file);
}
