/*****************************************************************************
* \file      mockTransmitter.c
* \author    Conny Gustafsson
* \date      2019-05-26
* \brief     A mock transmitter used for unit testing
*
* Copyright (c) 2019 Conny Gustafsson
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
#include "mockTransmitter.h"
#include "apx_transmitHandler.h"
#include "headerutil.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void mockTransmitter_create(mockTransmitter_t *self)
{
   self->maxBufLen = MOCK_TRANSMIT_MAX_LEN;
   mockTransmitter_autoReset(self);
}

void mockTransmitter_reset(mockTransmitter_t *self, int32_t newBufLen)
{
   if (self != 0)
   {
      self->currentBufLen = newBufLen;
      self->numWrites = 0;
      self->writeOffset = 0;
      self->readOffset = 0;
   }
}
void mockTransmitter_autoReset(mockTransmitter_t *self)
{
   if (self != 0)
   {
      mockTransmitter_reset(self, self->maxBufLen);
   }
}

int32_t mockTransmitter_writeAvail(mockTransmitter_t *self)
{
   if (self != 0)
   {
      return self->currentBufLen - self->writeOffset;
   }
   return -1;
}

int32_t mockTransmitter_readAvail(mockTransmitter_t *self)
{
   if (self != 0)
   {
      return (self->writeOffset-self->readOffset);
   }
   return -1;
}

uint8_t* mockTransmitter_getData(mockTransmitter_t *self)
{
   if (self != 0)
   {
      return &self->dataBuf[self->readOffset];
   }
   return (uint8_t*) 0;
}

int32_t mockTransmitter_write(mockTransmitter_t *self, const uint8_t *msg, int32_t msgLen)
{
   if ( (self != 0) && (msgLen <= HEADERUTIL16_MAX_NUM_LONG) )
   {
      int32_t writeAvail = mockTransmitter_writeAvail(self);
      int32_t headerLen = (msgLen <= HEADERUTIL16_MAX_NUM_SHORT)? HEADERUTIL16_SIZE_SHORT : HEADERUTIL16_SIZE_LONG;
      int32_t totaLen = headerLen+msgLen;
      if (writeAvail >= totaLen )
      {
         (void) headerutil_numEncode16(&self->dataBuf[self->writeOffset], (uint32_t) writeAvail, (uint16_t) msgLen);
         self->writeOffset+=headerLen;
         memcpy(&self->dataBuf[self->writeOffset], msg, msgLen);
         self->writeOffset+=msgLen;
         self->numWrites++;
         assert(self->writeOffset <= self->currentBufLen);
         return totaLen;
      }
      return APX_TRANSMIT_HANDLER_BUFFER_OVERFLOW_ERROR;
   }
   return APX_TRANSMIT_HANDLER_INVALID_ARGUMENT_ERROR;
}

int32_t mockTransmitter_getNumWrites(mockTransmitter_t *self)
{
   if (self != 0)
   {
      return self->numWrites;
   }
   return -1;
}

void mockTransmitter_trimLeft(mockTransmitter_t *self, int32_t dataLen)
{
   if ( (self != 0) && ((self->readOffset+dataLen)<=self->writeOffset))
   {
      self->readOffset+=dataLen;
      if (self->readOffset >= self->writeOffset)
      {
         mockTransmitter_reset(self, self->currentBufLen);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


