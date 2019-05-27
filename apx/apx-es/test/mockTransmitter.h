/*****************************************************************************
* \file      mockTransmitter.h
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
