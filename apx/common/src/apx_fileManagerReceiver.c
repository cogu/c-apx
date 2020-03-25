/*****************************************************************************
* \file      apx_fileManagerReceiver.c
* \author    Conny Gustafsson
* \date      2020-02-08
* \brief     Description
*
* Copyright (c) 2020 Conny Gustafsson
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
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "apx_fileManagerReceiver.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif


//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_fileManagerReceiver_startReception(apx_fileManagerReceiver_t *self, uint32_t address, const uint8_t *data, apx_size_t size, bool moreBit);
static apx_error_t apx_fileManagerReceiver_continueReception(apx_fileManagerReceiver_t *self, uint32_t address, const uint8_t *data, apx_size_t size, bool moreBit);
//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

void apx_fileManagerReceiver_create(apx_fileManagerReceiver_t *self)
{
   if (self != 0)
   {
      self->receiveBuf = (uint8_t*) 0;
      self->receiveBufSize = 0u;
      self->receiveBufPos = 0u;
      self->startAddress = RMF_INVALID_ADDRESS;
      self->isFragmentedWrite = false;
   }
}

void apx_fileManagerReceiver_destroy(apx_fileManagerReceiver_t *self)
{
   if (self != 0)
   {
      if (self->receiveBuf != 0)
      {
         free(self->receiveBuf);
      }
   }
}

void apx_fileManagerReceiver_reset(apx_fileManagerReceiver_t *self)
{
   if (self != 0)
   {
      self->startAddress = RMF_INVALID_ADDRESS;
      self->receiveBufPos = 0u;
      self->isFragmentedWrite = false;
   }
}

apx_error_t apx_fileManagerReceiver_reserve(apx_fileManagerReceiver_t *self, apx_size_t size)
{
   if ( (self != 0) && (size > 0) )
   {
      if (size > APX_MAX_FILE_SIZE)
      {
         return APX_FILE_TOO_LARGE_ERROR;
      }
      if (size > self->receiveBufSize)
      {
         uint8_t *oldBuf = self->receiveBuf;
         if (oldBuf != 0)
         {
            free(oldBuf);
         }
         self->receiveBuf = (uint8_t*) malloc(size);
         if (self->receiveBuf == 0)
         {
            return APX_MEM_ERROR;
         }
         self->receiveBufSize = size;
      }
      apx_fileManagerReceiver_reset(self);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

bool apx_fileManagerReceiver_isOngoing(apx_fileManagerReceiver_t *self)
{
   if (self != 0)
   {
      return self->isFragmentedWrite;
   }
   return false;
}

apx_error_t apx_fileManagerReceiver_write(apx_fileManagerReceiver_t *self, uint32_t address, const uint8_t *data, apx_size_t size, bool moreBit)
{
   if ( (self != 0) && (data != 0) && (address < RMF_INVALID_ADDRESS) )
   {
      if (size > APX_MAX_FILE_SIZE)
      {
         return APX_FILE_TOO_LARGE_ERROR;
      }
      if (self->startAddress == RMF_INVALID_ADDRESS)
      {
         return apx_fileManagerReceiver_startReception(self, address, data, size, moreBit);
      }
      else
      {
         return apx_fileManagerReceiver_continueReception(self, address, data, size, moreBit);
      }
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

/**
 * Returns the start address of last write
 */
uint32_t apx_fileManagerReceiver_getAddress(apx_fileManagerReceiver_t *self)
{
   if (self != 0)
   {
      return self->startAddress;
   }
   return RMF_INVALID_ADDRESS;
}

apx_size_t apx_fileManagerReceiver_getSize(apx_fileManagerReceiver_t *self, apx_size_t *size)
{
   if (self != 0)
   {
      return self->receiveBufPos;
   }
   return 0u;
}

/**
 * Fills in all relevant info from last write if the write is completed.
 * Returns APX_NO_ERROR on success.
 * Returns APX_DATA_NOT_PROCESSED_ERROR in case the write is still ongoing.
 * Return APX_INVALID_ADDRESS_ERROR in case no write has been done since last reset.
 */
apx_error_t apx_fileManagerReceiver_checkComplete(apx_fileManagerReceiver_t *self, apx_fileManagerReception_t *reception)
{
   if ( (self != 0) && (reception != 0) )
   {
      if (self->isFragmentedWrite)
      {
         return APX_DATA_NOT_COMPLETE_ERROR;
      }
      if (self->startAddress == RMF_INVALID_ADDRESS)
      {
         return APX_INVALID_ADDRESS_ERROR;
      }
      reception->startAddress = self->startAddress;
      reception->msgBuf = self->receiveBuf;
      reception->msgSize = self->receiveBufPos;
      apx_fileManagerReceiver_reset(self);
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static apx_error_t apx_fileManagerReceiver_startReception(apx_fileManagerReceiver_t *self, uint32_t address, const uint8_t *data, apx_size_t size, bool moreBit)
{
   if (self != 0)
   {
      if (self->receiveBufSize == 0)
      {
         return APX_MISSING_BUFFER_ERROR;
      }
      if (size > self->receiveBufSize)
      {
         return APX_BUFFER_FULL_ERROR;
      }
      if (size > 0)
      {
         memcpy(&self->receiveBuf[0u], data, size);
         self->receiveBufPos = size;
      }
      self->startAddress = address;
      self->isFragmentedWrite = moreBit;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}

static apx_error_t apx_fileManagerReceiver_continueReception(apx_fileManagerReceiver_t *self, uint32_t address, const uint8_t *data, apx_size_t size, bool moreBit)
{
   if (self != 0)
   {
      assert(self->startAddress != RMF_INVALID_ADDRESS);
      uint32_t expectedAddress = (self->startAddress + self->receiveBufPos);
      if (expectedAddress != address)
      {
         return APX_INVALID_ADDRESS_ERROR; //Not the address we expected to resume writing
      }
      if (self->receiveBufSize == 0)
      {
         return APX_MISSING_BUFFER_ERROR;
      }
      if ( (self->receiveBufPos + size) > self->receiveBufSize)
      {
         return APX_BUFFER_FULL_ERROR;
      }
      if (size > 0)
      {
         memcpy(&self->receiveBuf[self->receiveBufPos], data, size);
         self->receiveBufPos += size;
      }
      self->isFragmentedWrite = moreBit;
      return APX_NO_ERROR;
   }
   return APX_INVALID_ARGUMENT_ERROR;
}



