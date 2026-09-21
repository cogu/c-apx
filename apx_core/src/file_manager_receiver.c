/*****************************************************************************
* \file      file_manager_receiver.c
* \author    Conny Gustafsson
* \date      2020-02-08
* \brief     Receive buffer mechanism for file manager
*
* Copyright (c) 2020-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "apx/file_manager_receiver.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t start_new_reception(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, uint32_t address, uint8_t const* data, apx_size_t size, bool more_bit);
static apx_error_t continue_reception(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, uint32_t address, uint8_t const* data, apx_size_t size, bool more_bit);
static void process_more_bit(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, bool more_bit);

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_error_t apx_fileManagerReceiver_create(apx_file_manager_receiver_t* self)
{
   if (self != NULL)
   {
      self->buf_data = NULL;
      self->buf_size = 0u;
      self->buf_pos = 0u;
      self->start_address = RMF_INVALID_ADDRESS;
      return apx_fileManagerReceiver_reserve(self, RMF_CMD_AREA_SIZE);
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_fileManagerReceiver_destroy(apx_file_manager_receiver_t* self)
{
   if (self != NULL)
   {
      if (self->buf_data != 0)
      {
         free(self->buf_data);
      }
   }
}

void apx_fileManagerReceiver_reset(apx_file_manager_receiver_t* self)
{
   if (self != NULL)
   {
      self->buf_pos = 0u;
      self->start_address = RMF_INVALID_ADDRESS;
   }
}

apx_error_t apx_fileManagerReceiver_reserve(apx_file_manager_receiver_t* self, apx_size_t size)
{
   if ( (self != NULL) && (size > 0u) )
   {
      if (size > APX_MAX_FILE_SIZE)
      {
         return APX_FILE_TOO_LARGE_ERROR;
      }
      if (size > self->buf_size)
      {
         uint8_t *old_data = self->buf_data;
         if (old_data != NULL)
         {
            free(old_data);
         }
         self->buf_data = (uint8_t*) malloc(size);
         if (self->buf_data == NULL)
         {
            return APX_MEM_ERROR;
         }
         self->buf_size = size;
      }
      //apx_fileManagerReceiver_reset(self);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

apx_size_t apx_fileManagerReceiver_buffer_size(apx_file_manager_receiver_t const* self)
{
   if (self != NULL)
   {
      return self->buf_size;
   }
   return 0u;
}

apx_error_t apx_fileManagerReceiver_write(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, uint32_t address, uint8_t const* data, apx_size_t size, bool more_bit)
{
   if ( (self != NULL) && (result != NULL) && (data != NULL) && (address < RMF_INVALID_ADDRESS) )
   {
      apx_error_t retval = APX_NO_ERROR;
      if (size > APX_MAX_FILE_SIZE)
      {
         retval = APX_FILE_TOO_LARGE_ERROR;
      }
      else
      {
         result->is_complete = false;
         result->address = RMF_INVALID_ADDRESS;
         result->data = NULL;
         result->size = 0u;

         if (self->start_address == RMF_INVALID_ADDRESS)
         {
            retval = start_new_reception(self, result, address, data, size, more_bit);
         }
         else
         {
            retval = continue_reception(self, result, address, data, size, more_bit);
         }
      }
      return retval;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t start_new_reception(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, uint32_t address, uint8_t const* data, apx_size_t size, bool more_bit)
{
   assert(data != NULL);
   apx_error_t retval = APX_NO_ERROR;
   if ( (self->buf_size == 0u) || (self->buf_data == NULL))
   {
      retval = APX_MISSING_BUFFER_ERROR;
   }
   else if (size > self->buf_size)
   {
      retval = APX_BUFFER_FULL_ERROR;
   }
   else
   {
      if (size > 0u)
      {
         memcpy(self->buf_data, data, size);
         self->buf_pos = size;
      }
      self->start_address = address;
      process_more_bit(self, result, more_bit);
      retval = APX_NO_ERROR;
   }
   return retval;
}

static apx_error_t continue_reception(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, uint32_t address, uint8_t const* data, apx_size_t size, bool more_bit)
{
   apx_error_t retval = APX_NO_ERROR;
   assert( (self->start_address != RMF_INVALID_ADDRESS) && (data != NULL));
   uint32_t expected_address = (self->start_address + (uint32_t)self->buf_pos);
   if (expected_address != address)
   {
      retval = APX_INVALID_ADDRESS_ERROR;
   }
   if (retval == APX_NO_ERROR)
   {
      if ((self->buf_size == 0u) || (self->buf_data == NULL))
      {
         retval = APX_MISSING_BUFFER_ERROR;
      }
      else if ((self->buf_pos + size) > self->buf_size)
      {
         retval = APX_BUFFER_FULL_ERROR;
      }
      else
      {
         if (size > 0u)
         {
            memcpy(self->buf_data + self->buf_pos, data, size);
            self->buf_pos += size;
         }
         process_more_bit(self, result, more_bit);
         retval = APX_NO_ERROR;
      }
   }
   return retval;
}

static void process_more_bit(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, bool more_bit)
{
   if (!more_bit)
   {
      result->is_complete = true;
      result->address = self->start_address;
      result->data = self->buf_data;
      result->size = self->buf_pos;
      apx_fileManagerReceiver_reset(self);
   }
}