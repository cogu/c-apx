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

apx_error_t apx_file_manager_receiver_create(apx_file_manager_receiver_t* self)
{
   if (self != NULL)
   {
      adt_bytearray_create(&self->buffer);
      self->start_address = RMF_INVALID_ADDRESS;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

void apx_file_manager_receiver_destroy(apx_file_manager_receiver_t* self)
{
   if (self != NULL)
   {
      adt_bytearray_destroy(&self->buffer);
   }
}

void apx_file_manager_receiver_reset(apx_file_manager_receiver_t* self)
{
   if (self != NULL)
   {
      self->start_address = RMF_INVALID_ADDRESS;
      adt_bytearray_clear(&self->buffer);
   }
}

apx_error_t apx_file_manager_receiver_write(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, uint32_t address, uint8_t const* data, apx_size_t size, bool more_bit)
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
   adt_bytearray_clear(&self->buffer);
   self->start_address = address;
   if (size > 0u)
   {
      adt_error_t const adt_err = adt_bytearray_append(&self->buffer, data, (uint32_t)size);
      if (adt_err != ADT_NO_ERROR)
      {
         self->start_address = RMF_INVALID_ADDRESS;
         return APX_MEM_ERROR;
      }
   }
   process_more_bit(self, result, more_bit);
   return APX_NO_ERROR;
}

static apx_error_t continue_reception(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, uint32_t address, uint8_t const* data, apx_size_t size, bool more_bit)
{
   assert( (self->start_address != RMF_INVALID_ADDRESS) && (data != NULL));
   uint32_t const current_len = adt_bytearray_length(&self->buffer);
   uint32_t const expected_address = self->start_address + current_len;
   if (expected_address != address)
   {
      return APX_INVALID_ADDRESS_ERROR;
   }
   if (((uint64_t)current_len + (uint64_t)size) > APX_MAX_FILE_SIZE)
   {
      return APX_FILE_TOO_LARGE_ERROR;
   }
   if (size > 0u)
   {
      adt_error_t const adt_err = adt_bytearray_append(&self->buffer, data, (uint32_t)size);
      if (adt_err != ADT_NO_ERROR)
      {
         return APX_MEM_ERROR;
      }
   }
   process_more_bit(self, result, more_bit);
   return APX_NO_ERROR;
}

static void process_more_bit(apx_file_manager_receiver_t* self, apx_file_manager_reception_result_t* result, bool more_bit)
{
   if (!more_bit)
   {
      result->is_complete = true;
      result->address = self->start_address;
      result->data = adt_bytearray_const_data(&self->buffer);
      result->size = (apx_size_t) adt_bytearray_length(&self->buffer);
      self->start_address = RMF_INVALID_ADDRESS;
   }
}