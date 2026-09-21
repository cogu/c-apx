/*****************************************************************************
* \file      numheader.h
* \author    Conny Gustafsson
* \date      2018-08-11
* \brief     NumHeader implementation (replaces headerutil)
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef NUMHEADER_H
#define NUMHEADER_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define NUMHEADER16_MAX_NUM_SHORT ((uint16_t) 127u)
#define NUMHEADER16_MAX_NUM_LONG ((uint16_t) 32895u) //128+32767
#define NUMHEADER16_SHORT_SIZE 1u
#define NUMHEADER16_LONG_SIZE 2u

#define NUMHEADER32_MAX_NUM_SHORT ((uint32_t) 127u)
#define NUMHEADER32_MAX_NUM_LONG ((uint32_t) 2147483647ul) //should be the same as INT_MAX
#define NUMHEADER32_SHORT_SIZE 1u
#define NUMHEADER32_LONG_SIZE 4u

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


int32_t numheader_encode16(uint8_t *buf, int32_t maxBufLen, uint16_t value);
const uint8_t * numheader_decode16(const uint8_t *pBegin, const uint8_t *pEnd, uint16_t *value);

int32_t numheader_encode32(uint8_t *buf, int32_t maxBufLen, uint32_t value);
const uint8_t *numheader_decode32(const uint8_t *pBegin, const uint8_t *pEnd, uint32_t *value);


#endif //NUMHEADER_H
