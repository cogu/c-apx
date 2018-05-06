
/********************************* Includes **********************************/
#include "pack.h"
#ifdef USE_PLATFORM_TYPES
#include "Platform_Types.h"
#define _UINT8 uint8
#define _UINT32 uint32
#define _UINT64 uint64
#else
#include <stdint.h>
#define _UINT8 uint8_t
#define _UINT32 uint32_t
#define _UINT64 uint64_t
#endif

#if  defined(__GNUC__) && defined(__LP64__)
#define _PACK_BASE_TYPE _UINT64
#else
#define _PACK_BASE_TYPE _UINT32
#endif


/**************************** Constants and Types ****************************/

/********************************* Variables *********************************/

/************************* Local Function Prototypes *************************/

/***************************** Exported Functions ****************************/
void packBE(_UINT8* p, _PACK_BASE_TYPE value, _UINT8 u8Size)
{
   if ((u8Size > 0) && (u8Size < 9))
   {
      register _PACK_BASE_TYPE tmp = value;
      p += (u8Size - 1);
      while (u8Size > 0)
      {
         *(p--) = (_UINT8)tmp;
         tmp = tmp >> 8;
         u8Size--;
      }
   }
}


void packLE(_UINT8* p, _PACK_BASE_TYPE value, _UINT8 u8Size)
{
   if ((u8Size > 0) && (u8Size < 9))
   {
      register _PACK_BASE_TYPE tmp = value;
      while (u8Size > 0)
      {
         *(p++) = (_UINT8)tmp;
         tmp = tmp >> 8;
         u8Size--;
      }
   }
}


_PACK_BASE_TYPE unpackBE(const _UINT8* p, _UINT8 u8Size)
{
   if ((u8Size > 0) && (u8Size < 9))
   {
      register _PACK_BASE_TYPE tmp = 0;
      while (u8Size > 0)
      {
         tmp = (tmp << 8) | *(p++);
         u8Size--;
      }
      return tmp;
   }
   return 0;
}


_PACK_BASE_TYPE unpackLE(const _UINT8* p, _UINT8 u8Size)
{
   if ((u8Size > 0) && (u8Size < 9))
   {
      register _PACK_BASE_TYPE tmp = 0;
      p += (u8Size - 1);
      while (u8Size > 0)
      {
         tmp = (tmp << 8) | *(p--);
         u8Size--;
      }
      return tmp;
   }
   return 0;
}

/**
 * Special version of packLE that can be used to pack 64-bit integers on 32-bit machines
 */
void packLE64(_UINT8* p, _UINT64 value)
{
   _UINT8 u8Size = 8u;
   _UINT64 tmp = value;
   while (u8Size > 0)
   {
      *(p++) = (_UINT8)tmp;
      tmp = tmp >> 8;
      u8Size--;
   }
}

/**
 * Special version of unpackLE that can be used to unpack 64-bit integers on 32-bit machines
 */
_UINT64 unpackLE64(const _UINT8* p)
{
   _UINT8 u8Size = 8u;
   _UINT64 tmp = 0;
   p += (u8Size - 1);
   while (u8Size > 0)
   {
      tmp = (tmp << 8) | *(p--);
      u8Size--;
   }
   return tmp;
}

/****************************** Local Functions ******************************/

