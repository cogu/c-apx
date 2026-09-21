/*****************************************************************************
* \file      numheader.c
* \author    Conny Gustafsson
* \date      2018-08-11
* \brief     NumHeader implementation (replaces headerutil)
*
* Copyright (c) 2018-2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/numheader.h"
#include "pack.h"

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

/**
 * encodes the 16-bit number value into the buffer buf.
 * If the value is less than 128 it writes 1 byte of data
 * If the value is less than 32896 it writes 2 bytes of data
 * Returns -1 on error, otherwise it returns number of bytes written into buf
 */
int32_t numheader_encode16(uint8_t *buf, int32_t max_buf_len, uint16_t value)
{
   int32_t retval = 0;
   if ( (buf == NULL) || (max_buf_len == 0) || (value > NUMHEADER16_MAX_NUM_LONG) )
   {
      return -1;
   }
   if (value <= NUMHEADER16_MAX_NUM_SHORT)
   {
      retval = NUMHEADER16_SHORT_SIZE;
      buf[0]=(uint8_t) value; //encode 1-byte word with longbit set to 0
   }
   else
   {
      if(max_buf_len<(int32_t) sizeof(uint16_t))
      {
         return -1;
      }
      retval = NUMHEADER16_LONG_SIZE;
      //use the following trick to store values 32768-32895 into the 2-byte header as value 0-127 with longbit set to 1
      if(value>(uint16_t)32767u)
      {
         value-=(uint16_t)32768u;
      }
      buf[0]=((uint8_t)0x80) | ((uint8_t) (value>>((uint16_t)8u))); //encode 2-byte word with longbit set to 1
      buf[1]=(uint8_t) value;
   }
   return retval;
}

/**
 * decodes the number stored in buf. If higest bit is set (0x80) it treats it as a 15-bit value (0-32768),
 * otherwise it treats it as a 7-bit value (0-127)
 * When the high bit is set (0x80) the 15-bit value 0-127 is treated as special range 32768-32895
 * Returns 0 on error, otherwise it returns the end pointer of the written data
 * (pointer is at offset 1 or 2 from start of buf depending on value counting from zeo)
 */
const uint8_t *numheader_decode16(const uint8_t *p_begin, const uint8_t *p_end, uint16_t *value)
{
   const uint8_t*pNext=p_begin;
   if(p_begin<p_end)
   {
      uint8_t c = *pNext++;
      if(c & 0x80) //is long_bit set?
      {
         if(pNext<p_end)
         {
            uint16_t tmp = (uint16_t) unpackBE(p_begin,2);
            tmp&=(uint16_t)0x7FFF;
            if(tmp<128)
            {
               tmp+=32768;
            }
            if(value != NULL)
            {
               *value=tmp;
            }
            pNext++;
         }
      }
      else
      {
         if(value != NULL)
         {
            *value=(uint8_t) c;
         }
      }
   }
   return pNext;
}

int32_t numheader_encode32(uint8_t *buf, int32_t max_buf_len, uint32_t value)
{
   int32_t retval = 0;
   if ( (buf == NULL) || (max_buf_len == 0) || (value > NUMHEADER32_MAX_NUM_LONG) )
   {
      return -1;
   }
   if (value <= NUMHEADER32_MAX_NUM_SHORT)
   {
      retval = NUMHEADER32_SHORT_SIZE;
      buf[0]=(uint8_t) value; //encode 1-byte word with longbit set to 0
   }
   else
   {
      if(max_buf_len< (int32_t) sizeof(uint32_t))
      {
         return -1; //buffer to small
      }
      retval = NUMHEADER32_LONG_SIZE;
      value |= 0x80000000; //activate long bit (highest significant bit)
      packBE(&buf[0], value, (uint8_t) sizeof(uint32_t));
   }
   return retval;
}

const uint8_t *numheader_decode32(const uint8_t *p_begin, const uint8_t *p_end, uint32_t *value)
{
   const uint8_t*pNext=p_begin;
   if(p_begin<p_end)
   {
      uint8_t c = *pNext;
      if(c & 0x80) //is long_bit set?
      {
         if( pNext+4<=p_end ) //an additional 3 bytes is needed from buffer
         {
            if (value != NULL)
            {
               *value = (uint32_t) unpackBE(pNext,(uint8_t) 4);
               *value &= 0x7FFFFFFF; //deactivate highest significant bit
            }
            pNext+=4;
         }
      }
      else
      {
         if(value != NULL)
         {
            *value=(uint8_t) c;
         }
         pNext+=1;
      }
   }
   return pNext;
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


