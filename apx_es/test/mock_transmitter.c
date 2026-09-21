/*****************************************************************************
* \file      mock_transmitter.c
* \author    Conny Gustafsson
* \date      2019-05-26
* \brief     A mock transmitter used for unit testing
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include <string.h>
#include "mockTransmitter.h"
#include "apx_transmit_handler.h"
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
void mock_transmitter_create(mock_transmitter_t *self)
{
   self->maxBufLen = MOCK_TRANSMIT_MAX_LEN;
   mock_transmitter_auto_reset(self);
}

void mock_transmitter_reset(mock_transmitter_t *self, int32_t new_buf_len)
{
   if (self != NULL)
   {
      self->currentBufLen = new_buf_len;
      self->numWrites = 0;
      self->writeOffset = 0;
      self->readOffset = 0;
   }
}
void mock_transmitter_auto_reset(mock_transmitter_t *self)
{
   if (self != NULL)
   {
      mock_transmitter_reset(self, self->maxBufLen);
   }
}

int32_t mock_transmitter_write_avail(mock_transmitter_t *self)
{
   if (self != NULL)
   {
      return self->currentBufLen - self->writeOffset;
   }
   return -1;
}

int32_t mock_transmitter_read_avail(mock_transmitter_t *self)
{
   if (self != NULL)
   {
      return (self->writeOffset-self->readOffset);
   }
   return -1;
}

uint8_t* mock_transmitter_get_data(mock_transmitter_t *self)
{
   if (self != NULL)
   {
      return &self->dataBuf[self->readOffset];
   }
   return NULL;
}

int32_t mock_transmitter_write(mock_transmitter_t *self, const uint8_t *msg, int32_t msg_len)
{
   if ( (self != NULL) && (msg_len <= HEADERUTIL16_MAX_NUM_LONG) )
   {
      int32_t writeAvail = mock_transmitter_write_avail(self);
      int32_t headerLen = (msg_len <= HEADERUTIL16_MAX_NUM_SHORT)? HEADERUTIL16_SIZE_SHORT : HEADERUTIL16_SIZE_LONG;
      int32_t totaLen = headerLen+msg_len;
      if (writeAvail >= totaLen )
      {
         (void) headerutil_numEncode16(&self->dataBuf[self->writeOffset], (uint32_t) writeAvail, (uint16_t) msg_len);
         self->writeOffset+=headerLen;
         memcpy(&self->dataBuf[self->writeOffset], msg, msg_len);
         self->writeOffset+=msg_len;
         self->numWrites++;
         assert(self->writeOffset <= self->currentBufLen);
         return totaLen;
      }
      return APX_TRANSMIT_HANDLER_BUFFER_OVERFLOW_ERROR;
   }
   return APX_TRANSMIT_HANDLER_INVALID_ARGUMENT_ERROR;
}

int32_t mock_transmitter_get_num_writes(mock_transmitter_t *self)
{
   if (self != NULL)
   {
      return self->numWrites;
   }
   return -1;
}

void mock_transmitter_trim_left(mock_transmitter_t *self, int32_t data_len)
{
   if ( (self != NULL) && ((self->readOffset+data_len)<=self->writeOffset))
   {
      self->readOffset+=data_len;
      if (self->readOffset >= self->writeOffset)
      {
         mock_transmitter_reset(self, self->currentBufLen);
      }
   }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


