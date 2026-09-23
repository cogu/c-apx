/*****************************************************************************
* \file      mock_transmitter.h
* \author    Conny Gustafsson
* \date      2019-05-26
* \brief     A mock transmitter used for unit testing
*
* Copyright (c) 2019-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef MOCK_TRANSMITTER_H
#define MOCK_TRANSMITTER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MOCK_TRANSMIT_MAX_LEN 1024

typedef struct mock_transmitter_tag
{
   int32_t writeOffset;
   int32_t readOffset;
   int32_t currentBufLen;
   int32_t maxBufLen;
   int32_t numWrites;
   uint8_t dataBuf[MOCK_TRANSMIT_MAX_LEN];
} mock_transmitter_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void mock_transmitter_create(mock_transmitter_t *self);
void mock_transmitter_reset(mock_transmitter_t *self, int32_t new_buf_len);
void mock_transmitter_auto_reset(mock_transmitter_t *self);
int32_t mock_transmitter_write_avail(mock_transmitter_t *self);
int32_t mock_transmitter_read_avail(mock_transmitter_t *self);
uint8_t* mock_transmitter_get_data(mock_transmitter_t *self);
int32_t mock_transmitter_write(mock_transmitter_t *self, const uint8_t *masg, int32_t msg_len);
int32_t mock_transmitter_get_num_writes(mock_transmitter_t *self);
void mock_transmitter_trim_left(mock_transmitter_t *self, int32_t data_len);
#endif //MOCK_TRANSMITTER_H
