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

typedef struct mockTransmitter_tag
{
   int32_t writeOffset;
   int32_t readOffset;
   int32_t currentBufLen;
   int32_t maxBufLen;
   int32_t numWrites;
   uint8_t dataBuf[MOCK_TRANSMIT_MAX_LEN];
} mockTransmitter_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void mockTransmitter_create(mockTransmitter_t *self);
void mockTransmitter_reset(mockTransmitter_t *self, int32_t newBufLen);
void mockTransmitter_autoReset(mockTransmitter_t *self);
int32_t mockTransmitter_writeAvail(mockTransmitter_t *self);
int32_t mockTransmitter_readAvail(mockTransmitter_t *self);
uint8_t* mockTransmitter_getData(mockTransmitter_t *self);
int32_t mockTransmitter_write(mockTransmitter_t *self, const uint8_t *masg, int32_t msgLen);
int32_t mockTransmitter_getNumWrites(mockTransmitter_t *self);
void mockTransmitter_trimLeft(mockTransmitter_t *self, int32_t dataLen);
#endif //MOCK_TRANSMITTER_H
