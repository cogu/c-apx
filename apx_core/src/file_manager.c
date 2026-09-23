/*****************************************************************************
* \file      file_manager.c
* \author    Conny Gustafsson
* \date      2020-01-27
* \brief     New APX file manager
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
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
static void publish_local_files(apx_file_manager_t* self);
static apx_error_t process_message(apx_file_manager_t* self, uint32_t address, uint8_t const* data, apx_size_t size);
static apx_error_t process_command_message(apx_file_manager_t* self, uint8_t const* data, apx_size_t size);
static apx_error_t process_file_write_message(apx_file_manager_t* self, uint32_t address, uint8_t const* data, apx_size_t size);
static apx_error_t process_open_file_request(apx_file_manager_t* self, uint32_t start_address);
static apx_error_t process_close_file_request(apx_file_manager_t* self, uint32_t start_address);
static apx_error_t process_remote_file_published(apx_file_manager_t* self, rmf_file_info_t const* file_info);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_file_manager_create(apx_file_manager_t* self, uint8_t mode, apx_connection_interface_t const* parent_connection, apx_allocator_t* allocator)
{
   if (self != NULL)
   {
      apx_error_t result;
      self->mode = mode;
      result = apx_file_manager_receiver_create(&self->receiver);
      if (result == APX_NO_ERROR)
      {
         result = apx_file_manager_worker_create(&self->worker, &self->shared, mode);
         if (result == APX_NO_ERROR)
         {
            apx_file_manager_shared_create(&self->shared, parent_connection, allocator);
         }
         else
         {
            apx_file_manager_receiver_destroy(&self->receiver);
         }
      }
      return result;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_manager_destroy(apx_file_manager_t *self)
{
   if (self != NULL)
   {
      apx_file_manager_worker_destroy(&self->worker);
      apx_file_manager_receiver_destroy(&self->receiver);
      apx_file_manager_shared_destroy(&self->shared);
   }
}

void apx_file_manager_start(apx_file_manager_t *self)
{
   if (self != NULL)
   {
      apx_file_manager_shared_start(&self->shared);
#ifndef UNIT_TEST
      apx_error_t result = apx_file_manager_worker_start(&self->worker);
# if (APX_DEBUG_ENABLE)
      printf("[FILE-MANAGER %d] Worker thread started. Result: %d\n", (int)apx_file_manager_shared_get_connection_id(&self->shared), (int)result);
# else
      (void)result; //TODO: Check result?
# endif
#endif
   }
}

void apx_file_manager_stop(apx_file_manager_t *self)
{
   if (self != NULL)
   {
#ifndef UNIT_TEST
# if (APX_DEBUG_ENABLE)
      printf("[FILE-MANAGER %d] Stopping worker thread\n", (int)apx_file_manager_shared_get_connection_id(&self->shared));
# endif
      apx_file_manager_worker_stop(&self->worker);
# if (APX_DEBUG_ENABLE)
      printf("[FILE-MANAGER %d] Worker thread stopped\n", (int)apx_file_manager_shared_get_connection_id(&self->shared));
# endif
#endif
   }
}

void apx_file_manager_connected(apx_file_manager_t* self)
{
   if (self != NULL)
   {
      if (self->mode == APX_CLIENT_MODE)
      {
         publish_local_files(self);
      }
      else
      {
         apx_file_manager_shared_connected(&self->shared);
         rmf_version_id_t const version_id = apx_file_manager_shared_get_remotefile_version_id(&self->shared);
         if (version_id == RMF_PROTOCOL_VERSION_ID_1_1)
         {
            uint32_t connection_id = apx_file_manager_shared_get_connection_id(&self->shared);
            apx_file_manager_worker_prepare_header_accepted(&self->worker, connection_id);
         }
         else
         {
            apx_file_manager_worker_prepare_acknowledge(&self->worker);
         }
      }
   }
}

void apx_file_manager_disconnected(apx_file_manager_t* self)
{
   if (self != NULL)
   {
      apx_file_manager_shared_disconnected(&self->shared);
   }
}

apx_file_t* apx_file_manager_create_local_file(apx_file_manager_t* self, rmf_file_info_t const* file_info)
{
   if (self != NULL)
   {
      apx_file_t* file = apx_file_manager_shared_create_local_file(&self->shared, file_info);
      if (file != NULL)
      {
         apx_file_set_file_manager(file, self);
      }
      return file;
   }
   return NULL;
}

apx_error_t apx_file_manager_publish_local_file(apx_file_manager_t* self, rmf_file_info_t const* file_info)
{
   if ((self != NULL) && (file_info != NULL))
   {
      apx_error_t retval = APX_MEM_ERROR;
      rmf_file_info_t* cloned_info = rmf_file_info_clone(file_info);
      if (cloned_info != NULL)
      {
         retval = apx_file_manager_worker_prepare_publish_local_file(&self->worker, cloned_info);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_file_t* apx_file_manager_find_file_by_address(apx_file_manager_t* self, uint32_t address)
{
   if (self != NULL)
   {
      return apx_file_manager_shared_find_file_by_address(&self->shared, address);
   }
   return NULL;
}

apx_file_t* apx_file_manager_find_local_file_by_name(apx_file_manager_t* self, char const* name)
{
   if (self != NULL)
   {
      return apx_file_manager_shared_find_local_file_by_name(&self->shared, name);
   }
   return NULL;
}

apx_file_t* apx_file_manager_find_remote_file_by_name(apx_file_manager_t* self, char const* name)
{
   if (self != NULL)
   {
      return apx_file_manager_shared_find_remote_file_by_name(&self->shared, name);
   }
   return NULL;
}

apx_error_t apx_file_manager_message_received(apx_file_manager_t* self, uint8_t const* msg_data, apx_size_t msg_len)
{
   if ( (self != NULL) && (msg_data != NULL) )
   {
      uint32_t address = RMF_INVALID_ADDRESS;
      bool more_bit = false;
      apx_size_t const header_size = rmf_address_decode(msg_data, msg_data + msg_len, &address, &more_bit);
      if (header_size > 0)
      {
         apx_error_t error_code;
         apx_file_manager_reception_result_t result;
         assert(msg_len >= header_size);
         error_code = apx_file_manager_receiver_write(&self->receiver, &result, address, msg_data + header_size, msg_len - header_size, more_bit);
         if (error_code != APX_NO_ERROR)
         {
            return error_code;
         }
         else if (result.is_complete)
         {
            apx_error_t retval = process_message(self, result.address, result.data, result.size);
#if APX_DEBUG_ENABLE
            if (retval != APX_NO_ERROR)
            {
               printf("[FILE_MANAGER] Failed to process message. Error code: %d\n", (int)retval);
            }
#endif
            return retval;
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

apx_error_t apx_file_manager_send_local_const_data(apx_file_manager_t* self, uint32_t address, uint8_t const* data, apx_size_t size)
{
   if (self != NULL && data != NULL)
   {
      apx_file_t* file = apx_file_manager_shared_find_file_by_address(&self->shared, address);
      if (file == NULL)
      {
         return APX_FILE_NOT_FOUND_ERROR;
      }
      if (!apx_file_is_open(file))
      {
         return APX_FILE_NOT_OPEN_ERROR;
      }
      return apx_file_manager_worker_prepare_send_local_const_data(&self->worker, address, data, (uint32_t)size);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_local_data(apx_file_manager_t* self, uint32_t address, uint8_t* data, apx_size_t size)
{
   if (self != NULL && data != NULL)
   {
      apx_file_t* file = apx_file_manager_shared_find_file_by_address(&self->shared, address);
      if (file == NULL)
      {
         free(data);
         return APX_FILE_NOT_FOUND_ERROR;
      }
      if (!apx_file_is_open(file))
      {
         free(data);
         return APX_FILE_NOT_OPEN_ERROR;
      }
      apx_error_t retval = apx_file_manager_worker_prepare_send_local_data(&self->worker, address, data, (uint32_t)size);
      if (retval != APX_NO_ERROR)
      {
         free(data);
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_open_file_request(apx_file_manager_t* self, uint32_t address)
{
   if (self != NULL)
   {
      return apx_file_manager_worker_prepare_send_open_file_request(&self->worker, address);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_error_code(apx_file_manager_t* self, apx_error_t error_code)
{
   if (self != NULL)
   {
      (void)error_code;
      //TODO: Implement later
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_file_manager_send_connection_create(apx_file_manager_t* self, apx_connection_id_t connection_id, apx_connection_state_t connection_state, char const* tag)
{
   if (self != NULL)
   {
      return apx_file_manager_worker_prepare_send_connection_create(&self->worker, connection_id, connection_state, tag);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

uint16_t apx_file_manager_get_num_pending_worker_commands(apx_file_manager_t* self)
{
   if (self != NULL)
   {
      return apx_file_manager_worker_num_pending_commands(&self->worker);
   }
   return 0u;
}


#ifdef UNIT_TEST
bool apx_file_manager_run(apx_file_manager_t* self)
{
   if (self != NULL)
   {
      return apx_file_manager_worker_run(&self->worker);
   }
   return false;
}

#endif



//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static void publish_local_files(apx_file_manager_t* self)
{
   adt_ary_t local_file_list;
   int32_t i;
   int32_t num_files;
   adt_ary_create(&local_file_list, NULL);
   num_files = apx_file_manager_shared_copy_local_file_info(&self->shared, &local_file_list);
   for (i=0; i < num_files; i++)
   {
      rmf_file_info_t* file_info = (rmf_file_info_t*) adt_ary_value(&local_file_list, i);
      if (file_info != NULL)
      {
         //Worker takes memory ownership of file_info
         apx_error_t result = apx_file_manager_worker_prepare_publish_local_file(&self->worker, file_info);
         if (result != APX_NO_ERROR)
         {
            //TODO: Error handling
         }
      }
   }
   adt_ary_destroy(&local_file_list);
}

static apx_error_t process_message(apx_file_manager_t* self, uint32_t address, uint8_t const* data, apx_size_t size)
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

static apx_error_t process_command_message(apx_file_manager_t* self, uint8_t const* data, apx_size_t size)
{
   if (size < RMF_CMD_TYPE_SIZE)
   {
      return APX_INVALID_MSG_ERROR;
   }
   apx_error_t retval = APX_NO_ERROR;
   uint32_t const cmd_type = unpackLE(data, RMF_CMD_TYPE_SIZE);
   apx_size_t const cmd_size = size - RMF_CMD_TYPE_SIZE;
   uint8_t const* next = data + RMF_CMD_TYPE_SIZE;
   rmf_file_info_t* file_info;
   apx_size_t decoded_size;
   switch (cmd_type)
   {
   case RMF_CMD_PUBLISH_FILE_MSG:
      file_info = rmf_file_info_make_empty();
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
         rmf_file_info_delete(file_info);
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

static apx_error_t process_file_write_message(apx_file_manager_t* self, uint32_t address, uint8_t const* data, apx_size_t size)
{
   apx_file_t *file = apx_file_manager_shared_find_file_by_address(&self->shared, address | RMF_HIGH_ADDR_BIT);
   if (file == NULL)
   {
      return APX_INVALID_WRITE_ERROR;
   }
   if (!apx_file_is_open(file))
   {
      //Ignore writes on closed files
      return APX_NO_ERROR;
   }
   apx_connection_interface_t const* connection = apx_file_manager_shared_connection(&self->shared);
   if (connection == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   uint32_t const start_address = apx_file_get_address_without_flags(file);
   assert(start_address <= address);
   uint32_t const offset = address - start_address;
   return connection->remote_file_write_notification(connection->arg, file, offset, data, size);
}

static apx_error_t process_open_file_request(apx_file_manager_t* self, uint32_t start_address)
{
   apx_file_t* file = apx_file_manager_shared_find_file_by_address(&self->shared, start_address);
   if (file == NULL)
   {
      return APX_FILE_NOT_FOUND_ERROR;
   }
   apx_file_open(file);
   apx_file_open_notify(file);
   return APX_NO_ERROR;
}

static apx_error_t process_close_file_request(apx_file_manager_t* self, uint32_t start_address)
{
   (void)self;
   (void)start_address;
   return APX_NOT_IMPLEMENTED_ERROR;
}

static apx_error_t process_remote_file_published(apx_file_manager_t* self, rmf_file_info_t const* file_info)
{
   apx_file_t* file = apx_file_manager_shared_create_remote_file(&self->shared, file_info);
   if (file == NULL)
   {
      return APX_FILE_CREATE_ERROR;
   }
   apx_file_set_file_manager(file, self);
   apx_connection_interface_t const* connection = apx_file_manager_shared_connection(&self->shared);
   if (connection == NULL)
   {
      return APX_NULL_PTR_ERROR;
   }
   assert(connection->remote_file_published_notification != NULL);
   return connection->remote_file_published_notification(connection->arg, file);
}
