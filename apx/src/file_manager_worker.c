/*****************************************************************************
* \file      file_manager_worker.c
* \author    Conny Gustafsson
* \date      2020-01-23
* \brief     APX Filemanager worker
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
#include <assert.h>
#include <string.h>
#include <malloc.h>
#ifdef _WIN32
#include <process.h>
#endif
#include "apx/file_manager_worker.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
/*
#ifdef UNIT_TEST
#define DYN_STATIC
#else
#define DYN_STATIC static
#endif
*/
//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static bool process_single_command(apx_fileManagerWorker_t* self, apx_command_t const* cmd);
static apx_error_t run_send_acknowledge(apx_fileManagerWorker_t* self);
static apx_error_t run_publish_local_file(apx_fileManagerWorker_t* self, rmf_fileInfo_t* file);
static apx_error_t run_send_local_const_data(apx_fileManagerWorker_t* self, uint32_t address, uint8_t const* data, uint32_t size);
static apx_error_t run_send_local_data(apx_fileManagerWorker_t* self, uint32_t address, uint8_t* data, uint32_t size);
static apx_error_t run_open_remote_file(apx_fileManagerWorker_t* self, uint32_t address);
static apx_error_t run_send_header_accepted(apx_fileManagerWorker_t* self, uint32_t connection_id);
static apx_error_t apx_fileManagerWorker_process_ringbuffer_error(adt_buf_err_t error_code);
#ifndef UNIT_TEST
static apx_error_t start_worker_thread(apx_fileManagerWorker_t* self);
static apx_error_t stop_worker_thread(apx_fileManagerWorker_t* self);
static THREAD_PROTO(worker_main, arg);
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_fileManagerWorker_create(apx_fileManagerWorker_t* self, apx_fileManagerShared_t* shared, apx_mode_t mode)
{
   if (self != NULL)
   {
      adt_buf_err_t buf_result = adt_rbfh_create(&self->queue, (uint8_t) APX_COMMAND_SIZE);
      if (buf_result != BUF_E_OK)
      {
         return APX_MEM_ERROR;
      }
      self->mode = mode;
      self->shared = shared;
      self->worker_thread_valid = false;
      MUTEX_INIT(self->mutex);
      (void)SPINLOCK_INIT(self->queue_lock);
      SEMAPHORE_CREATE(self->semaphore);
#ifdef _WIN32
      self->worker_thread = INVALID_HANDLE_VALUE;
      self->worker_thread_id = 0u;
#else
      self->worker_thread = 0;
#endif
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManagerWorker_destroy(apx_fileManagerWorker_t *self)
{
   if (self != NULL)
   {
      if (self->worker_thread_valid == true)
      {
#ifndef UNIT_TEST
         stop_worker_thread(self);
#endif
      }
      MUTEX_DESTROY(self->mutex);
      SPINLOCK_DESTROY(self->queue_lock);
      SEMAPHORE_DESTROY(self->semaphore);
      adt_rbfh_destroy(&self->queue);
   }
}

uint16_t apx_fileManagerWorker_num_pending_commands(apx_fileManagerWorker_t* self)
{
   if (self != NULL)
   {
      uint16_t retval;
      SPINLOCK_ENTER(self->queue_lock);
      retval = adt_rbfh_length(&self->queue);
      SPINLOCK_LEAVE(self->queue_lock);
      return retval;
   }
   return 0u;
}

#ifdef UNIT_TEST
bool apx_fileManagerWorker_run(apx_fileManagerWorker_t* self)
{
   if (self != NULL)
   {
      apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(self->shared);
      if ( connection != NULL )
      {
         assert(connection->transmit_begin != NULL);
         connection->transmit_begin(connection->arg);
      }
      while (adt_rbfh_length(&self->queue) > 0)
      {
         apx_command_t cmd;
         bool result;
         adt_rbfh_remove(&self->queue, (uint8_t*)&cmd);
         result = process_single_command(self, &cmd);
         if (!result)
         {
            if (connection != NULL)
            {
               assert(connection->transmit_end != NULL);
               connection->transmit_end(connection->arg);
            }
            return result;
         }
      }
      if (connection != NULL)
      {
         assert(connection->transmit_end != NULL);
         connection->transmit_end(connection->arg);
      }
      return true;
   }
   return false;
}

#else
apx_error_t apx_fileManagerWorker_start(apx_fileManagerWorker_t* self)
{
   if (self != NULL)
   {
      return start_worker_thread(self);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManagerWorker_stop(apx_fileManagerWorker_t* self)
{
   if (self != NULL)
   {
      stop_worker_thread(self);
   }
}
#endif

//Command API

apx_error_t apx_fileManagerWorker_prepare_acknowledge(apx_fileManagerWorker_t* self)
{
   if (self != NULL)
   {
      adt_buf_err_t rc;
      apx_command_t cmd = { APX_CMD_SEND_ACKNOWLEDGE, 0, 0, {0}, 0 };
      SPINLOCK_ENTER(self->queue_lock);
      rc = adt_rbfh_insert(&self->queue, (const uint8_t*)&cmd);
      SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
#endif
      return apx_fileManagerWorker_process_ringbuffer_error(rc);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManagerWorker_prepare_header_accepted(apx_fileManagerWorker_t* self, uint32_t connection_id)
{
   if (self != NULL)
   {
      adt_buf_err_t rc;
      apx_command_t cmd = { APX_CMD_SEND_HEADER_ACCEPTED, 0, 0, {0}, 0 };
      cmd.data1 = connection_id;
      SPINLOCK_ENTER(self->queue_lock);
      rc = adt_rbfh_insert(&self->queue, (const uint8_t*)&cmd);
      SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
#endif
      return apx_fileManagerWorker_process_ringbuffer_error(rc);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManagerWorker_prepare_publish_local_file(apx_fileManagerWorker_t* self, rmf_fileInfo_t* file_info)
{
   if (self != NULL)
   {
      adt_buf_err_t rc;
      apx_command_t cmd = { APX_CMD_PUBLISH_LOCAL_FILE, 0, 0, {0}, 0 };
      cmd.data3.ptr = (void*)file_info;
      SPINLOCK_ENTER(self->queue_lock);
      rc = adt_rbfh_insert(&self->queue, (const uint8_t*)&cmd);
      SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
#endif
      return apx_fileManagerWorker_process_ringbuffer_error(rc);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManagerWorker_prepare_send_local_const_data(apx_fileManagerWorker_t* self, uint32_t address, uint8_t const* data, uint32_t size)
{
   if (self != NULL)
   {
      adt_buf_err_t rc;
      apx_command_t cmd;
      apx_build_command_with_ptr(&cmd, APX_CMD_SEND_LOCAL_CONST_DATA, address, size, (void*) data, NULL);
      SPINLOCK_ENTER(self->queue_lock);
      rc = adt_rbfh_insert(&self->queue, (const uint8_t*)&cmd);
      SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
#endif
      return apx_fileManagerWorker_process_ringbuffer_error(rc);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManagerWorker_prepare_send_local_data(apx_fileManagerWorker_t* self, uint32_t address, uint8_t* data, uint32_t size)
{
   if (self != NULL)
   {
      adt_buf_err_t rc;
      apx_command_t cmd;
      apx_build_command_with_ptr(&cmd, APX_CMD_SEND_LOCAL_DATA, address, size, data, NULL); //TODO: Implement small data support
      SPINLOCK_ENTER(self->queue_lock);
      rc = adt_rbfh_insert(&self->queue, (const uint8_t*)&cmd);
      SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
#endif
      return apx_fileManagerWorker_process_ringbuffer_error(rc);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_error_t apx_fileManagerWorker_prepare_send_open_file_request(apx_fileManagerWorker_t* self, uint32_t address)
{
   if (self != NULL)
   {
      adt_buf_err_t rc;
      apx_command_t cmd = { APX_CMD_OPEN_REMOTE_FILE, 0, 0, {0}, 0 };
      cmd.data1 = address;
      SPINLOCK_ENTER(self->queue_lock);
      rc = adt_rbfh_insert(&self->queue, (const uint8_t*)&cmd);
      SPINLOCK_LEAVE(self->queue_lock);
#ifndef UNIT_TEST
      SEMAPHORE_POST(self->semaphore);
#endif
      return apx_fileManagerWorker_process_ringbuffer_error(rc);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static bool process_single_command(apx_fileManagerWorker_t* self, apx_command_t const* cmd)
{
   apx_error_t result = APX_NO_ERROR;
   assert((self != NULL) && (cmd != NULL));
#if APX_DEBUG_ENABLE
   printf("[FILE-MANAGER-WORKER %d] Processing command: %d\n", (int)apx_fileManagerShared_get_connection_id(self->shared), (int)cmd->cmd_type);
#endif
   switch (cmd->cmd_type)
   {
   case APX_CMD_EXIT:
      return false;
   case APX_CMD_SEND_ACKNOWLEDGE:
      result = run_send_acknowledge(self);
      break;
   case APX_CMD_SEND_ERROR_CODE:
      //TODO: Implement
      break;
   case APX_CMD_PUBLISH_LOCAL_FILE:
      result = run_publish_local_file(self, (rmf_fileInfo_t*)cmd->data3.ptr);
      break;
   case APX_CMD_REVOKE_LOCAL_FILE:
      //TODO: Implement
      break;
   case APX_CMD_OPEN_REMOTE_FILE:
      result = run_open_remote_file(self, cmd->data1);
      break;
   case APX_CMD_CLOSE_REMOTE_FILE:
      //TODO: Implement
      break;
   case APX_CMD_SEND_LOCAL_CONST_DATA:
      result = run_send_local_const_data(self, cmd->data1, (uint8_t const*)cmd->data3.ptr, cmd->data2);
      break;
   case APX_CMD_SEND_LOCAL_DATA:
      result = run_send_local_data(self, cmd->data1, (uint8_t*)cmd->data3.ptr, cmd->data2);
      break;
   case APX_CMD_SEND_HEADER_ACCEPTED:
      result = run_send_header_accepted(self, cmd->data1);
      break;
   default:
      return false;
   }
   if (result != APX_NO_ERROR)
   {
      //TODO: error handling
   }
   return true;
}

static apx_error_t run_send_acknowledge(apx_fileManagerWorker_t* self)
{
   uint8_t buffer[RMF_CMD_TYPE_SIZE];
   apx_size_t const encoded_size = rmf_encode_acknowledge_cmd(buffer, (apx_size_t)sizeof(buffer));
   apx_error_t retval = APX_NO_ERROR;
   if (encoded_size == 0u)
   {
      retval = APX_BUFFER_BOUNDARY_ERROR;
   }
   else
   {
      apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(self->shared);
      if (connection != NULL)
      {
         int32_t bytes_available = 0;
         retval = connection->transmit_data_message(connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
      }
      else
      {
         retval = APX_NOT_CONNECTED_ERROR;
      }
   }
   return retval;
}

static apx_error_t run_publish_local_file(apx_fileManagerWorker_t* self, rmf_fileInfo_t* file_info)
{
   uint8_t buffer[RMF_FILE_INFO_HEADER_SIZE + RMF_FILE_NAME_MAX_SIZE + 1] ; //add 1 byte for null-terminator
   apx_size_t const encoded_size = rmf_encode_publish_file_cmd(buffer, (apx_size_t)sizeof(buffer), file_info);
   apx_error_t retval = APX_NO_ERROR;
   if (encoded_size == 0u)
   {
      retval = APX_BUFFER_BOUNDARY_ERROR;
   }
   else
   {
      apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(self->shared);
      if (connection != NULL)
      {
         int32_t bytes_available = 0;
         retval = connection->transmit_data_message(connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
      }
      else
      {
         retval = APX_NOT_CONNECTED_ERROR;
      }
   }
   rmf_fileInfo_delete(file_info);
   return retval;
}

static apx_error_t run_send_local_const_data(apx_fileManagerWorker_t* self, uint32_t address, uint8_t const* data, uint32_t size)
{
   apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(self->shared);
   apx_error_t retval = APX_NO_ERROR;
   if (connection != NULL)
   {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(connection->arg, address, false, data, (int32_t)size, &bytes_available);
   }
   else
   {
      retval = APX_NOT_CONNECTED_ERROR;
   }
   return retval;
}

static apx_error_t run_send_local_data(apx_fileManagerWorker_t* self, uint32_t address, uint8_t* data, uint32_t size)
{
   apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(self->shared);
   apx_error_t retval = APX_NO_ERROR;
   if (connection != NULL)
   {
      int32_t bytes_available = 0;
      retval = connection->transmit_data_message(connection->arg, address, false, data, (int32_t)size, &bytes_available);
   }
   else
   {
      retval = APX_NOT_CONNECTED_ERROR;
   }
   free(data);
   return retval;
}

static apx_error_t run_open_remote_file(apx_fileManagerWorker_t* self, uint32_t address)
{
   uint8_t buffer[RMF_CMD_TYPE_SIZE + RMF_FILE_OPEN_CMD_SIZE]; //add 1 byte for null-terminator
   apx_size_t const encoded_size = rmf_encode_open_file_cmd(buffer, (apx_size_t)sizeof(buffer), address);
   apx_error_t retval = APX_NO_ERROR;
   if (encoded_size == 0u)
   {
      retval = APX_BUFFER_BOUNDARY_ERROR;
   }
   else
   {
      apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(self->shared);
      if (connection != NULL)
      {
         int32_t bytes_available = 0;
         retval = connection->transmit_data_message(connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
      }
      else
      {
         retval = APX_NOT_CONNECTED_ERROR;
      }
   }
   return retval;
}

static apx_error_t run_send_header_accepted(apx_fileManagerWorker_t* self, uint32_t connection_id)
{
   uint8_t buffer[RMF_CMD_TYPE_SIZE + UINT32_SIZE];
   apx_size_t const encoded_size = rmf_encode_header_accepted(buffer, (apx_size_t)sizeof(buffer), connection_id);
   apx_error_t retval = APX_NO_ERROR;
   if (encoded_size == 0u)
   {
      retval = APX_BUFFER_BOUNDARY_ERROR;
   }
   else
   {
      apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(self->shared);
      if (connection != NULL)
      {
         int32_t bytes_available = 0;
         retval = connection->transmit_data_message(connection->arg, RMF_CMD_AREA_START_ADDRESS, false, buffer, (int32_t)encoded_size, &bytes_available);
      }
      else
      {
         retval = APX_NOT_CONNECTED_ERROR;
      }
   }
   return retval;
}

static apx_error_t apx_fileManagerWorker_process_ringbuffer_error(adt_buf_err_t error_code)
{
   apx_error_t retval = APX_NO_ERROR;
   if (error_code == BUF_E_OVERFLOW)
   {
      retval = APX_BUFFER_FULL_ERROR;
   }
   else if (error_code == BUF_E_NOT_OK)
   {
      retval = APX_MEM_ERROR;
   }
   return retval;
}

#ifndef UNIT_TEST
static apx_error_t start_worker_thread(apx_fileManagerWorker_t* self)
{
   assert(self != NULL);
   if (self->worker_thread_valid == false) {
      self->worker_thread_valid = true;
#ifdef _WIN32
      THREAD_CREATE(self->worker_thread, worker_main, self, self->worker_thread_id);
      if (self->worker_thread == INVALID_HANDLE_VALUE)
      {
         self->worker_thread_valid = false;
         return APX_THREAD_CREATE_ERROR;
      }
#else
      int rc = THREAD_CREATE(self->worker_thread, worker_main, self);
      if (rc != 0)
      {
         self->worker_thread_valid = false;
         return APX_THREAD_CREATE_ERROR;
      }
#endif
      return APX_NO_ERROR;
   }
   return APX_INVALID_STATE_ERROR;
}

static apx_error_t stop_worker_thread(apx_fileManagerWorker_t* self)
{
   if (self->worker_thread_valid == true)
   {
#ifdef _WIN32
      DWORD result;
#endif
      apx_command_t cmd = { APX_CMD_EXIT, 0, 0, {0}, NULL };
      SPINLOCK_ENTER(self->queue_lock);
      adt_rbfh_insert(&self->queue, (const uint8_t*)&cmd);
      SPINLOCK_LEAVE(self->queue_lock);
      SEMAPHORE_POST(self->semaphore);
#ifdef _WIN32
      result = WaitForSingleObject(self->worker_thread, 5000);
      if (result == WAIT_TIMEOUT)
      {
         return APX_THREAD_JOIN_TIMEOUT_ERROR;
      }
      else if (result == WAIT_FAILED)
      {
         return APX_THREAD_JOIN_ERROR;
      }
      CloseHandle(self->worker_thread);
      self->worker_thread = INVALID_HANDLE_VALUE;
#else
      if (pthread_equal(pthread_self(), self->worker_thread) == 0)
      {
         void* status;
         int s = pthread_join(self->worker_thread, &status);
         if (s != 0)
         {
            return APX_THREAD_JOIN_ERROR;
         }
      }
      else
      {
         //pthread_join attempted from pthread_self. This is not allowed
         return APX_INTERNAL_ERROR;
      }
#endif
      self->worker_thread_valid = false;
   }
   return APX_NO_ERROR;
}

static THREAD_PROTO(worker_main, arg)
{
   if (arg != 0)
   {
      apx_fileManagerWorker_t* self;
      bool is_running = true;

      self = (apx_fileManagerWorker_t*)arg;

      while (is_running)
      {
         apx_connectionInterface_t const* connection = apx_fileManagerShared_connection(self->shared);

#ifdef _WIN32
         DWORD result = WaitForSingleObject(self->semaphore, INFINITE);
         if (result == WAIT_OBJECT_0)
#else

         int result = sem_wait(&self->semaphore);
         if (result == 0)
#endif
         {
            uint16_t queue_length = 0u;
            SPINLOCK_ENTER(self->queue_lock);
            queue_length = adt_rbfh_length(&self->queue);
            SPINLOCK_LEAVE(self->queue_lock);
            if (queue_length > 0)
            {
               if (connection != NULL)
               {
                  assert(connection->transmit_begin != NULL);
                  connection->transmit_begin(connection->arg);
               }
               while (queue_length > 0)
               {
                  apx_command_t cmd;
                  bool rc;
                  SPINLOCK_ENTER(self->queue_lock);
                  adt_rbfh_remove(&self->queue, (uint8_t*)&cmd);
                  SPINLOCK_LEAVE(self->queue_lock);
                  rc = process_single_command(self, &cmd);
                  if (!rc)
                  {
                     if (connection != NULL)
                     {
                        assert(connection->transmit_end != NULL);
                        connection->transmit_end(connection->arg);
                     }
                     is_running = false;
                     break;
                  }
                  SPINLOCK_ENTER(self->queue_lock);
                  queue_length = adt_rbfh_length(&self->queue);
                  SPINLOCK_LEAVE(self->queue_lock);
               }
               if (connection != NULL)
               {
                  assert(connection->transmit_end != NULL);
                  connection->transmit_end(connection->arg);
               }
            }
         }
         else
         {
            THREAD_RETURN(APX_SEMAPHORE_ERROR);
         }
      }
   }
   THREAD_RETURN(APX_NO_ERROR);
}
#endif //UNIT_TEST