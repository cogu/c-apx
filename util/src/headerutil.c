#include "headerutil.h"
#include "pack.h"

/**
 * encodes the 16-bit number value into the buffer buf.
 * If the value is less than 128 it writes 1 byte of data
 * If the value is less than 32896 it writes 2 bytes of data
 * Returns 0 on error, otherwise it returns the end pointer of the written data
 * (pointer is at offset 1 or 2 from start of buf depending on value, counting from zero)
 */
uint8_t *headerutil_numEncode16(uint8_t *buf, uint32_t maxBufLen, uint16_t value)
{
   uint8_t *retval = (uint8_t *) 0;
   if ( (buf == 0) || (maxBufLen == 0) || (value > HEADERUTIL16_MAX_NUM_LONG) )
   {
      return retval;
   }
   if (value <= HEADERUTIL16_MAX_NUM_SHORT)
   {
      retval = &buf[1];
      buf[0]=(uint8_t) value; //encode 1-byte word with longbit set to 0
   }
   else
   {
      if(maxBufLen<(uint32_t) sizeof(uint16_t))
      {
         return retval;
      }
      retval = &buf[2];
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
const uint8_t *headerutil_numDecode(const uint8_t *pBegin, const uint8_t *pEnd, uint16_t *value)
{
   const uint8_t*pNext=pBegin;
   if(pBegin<pEnd)
   {
      uint8_t c = *pNext++;
      if(c & 0x80) //is long_bit set?
      {
         if(pNext<pEnd)
         {
            uint16_t tmp = (uint16_t) unpackBE(pBegin,2);
            tmp&=(uint16_t)0x7FFF;
            if(tmp<128)
            {
               tmp+=32768;
            }
            if(value != 0)
            {
               *value=tmp;
            }
            pNext++;
         }
      }
      else
      {
         if(value != 0)
         {
            *value=(uint8_t) c;
         }
      }
   }
   return pNext;
}

uint8_t *headerutil_numEncode32(uint8_t *buf, uint32_t maxBufLen, uint32_t value)
{
   uint8_t *retval = (uint8_t *) 0;
      if ( (buf == 0) || (maxBufLen == 0) || (value > HEADERUTIL32_MAX_NUM_LONG) )
      {
         return retval;
      }
      if (value <= HEADERUTIL32_MAX_NUM_SHORT)
      {
         retval = &buf[1];
         buf[0]=(uint8_t) value; //encode 1-byte word with longbit set to 0
      }
      else
      {
         if(maxBufLen< (uint32_t) sizeof(uint32_t))
         {
            return retval; //buffer to small
         }
         retval = &buf[4];
         value |= 0x80000000; //activate long bit (highest significant bit)
         packBE(&buf[0], value, (uint8_t) sizeof(uint32_t));
      }
      return retval;
}

const uint8_t *headerutil_numDecode32(const uint8_t *pBegin, const uint8_t *pEnd, uint32_t *value)
{
   const uint8_t*pNext=pBegin;
   if(pBegin<pEnd)
   {
      uint8_t c = *pNext;
      if(c & 0x80) //is long_bit set?
      {
         if( pNext+4<=pEnd ) //an additional 3 bytes is needed from buffer
         {
            if (value != 0)
            {
               *value = (uint32_t) unpackBE(pNext,(uint8_t) 4);
               *value &= 0x7FFFFFFF; //deactivate highest significant bit
            }
            pNext+=4;
         }
      }
      else
      {
         if(value != 0)
         {
            *value=(uint8_t) c;
         }
         pNext+=1;
      }
   }
   return pNext;
}


