
/********************************* Includes **********************************/
#include "pack.h"
#ifdef USE_PLATFORM_TYPES
#include "Platform_Types.h"
#define _UINT8 uint8
#define _UINT32 uint32
#else
#include <stdint.h>
#define _UINT8 uint8_t
#define _UINT32 uint32_t
#endif

/**************************** Constants and Types ****************************/

/********************************* Variables *********************************/

/************************* Local Function Prototypes *************************/

/***************************** Exported Functions ****************************/
void packBE(_UINT8* p, _UINT32 u32Val, _UINT8 u8Size)
{
   if ((u8Size > 0) && (u8Size < 5))
   {
      register _UINT32 u32Tmp = u32Val;
      p += (u8Size - 1);
      while (u8Size > 0)
      {
         *(p--) = (_UINT8)u32Tmp;
         u32Tmp = u32Tmp >> 8;
         u8Size--;
      }
   }
}


void packLE(_UINT8* p, _UINT32 u32Val, _UINT8 u8Size)
{
   if ((u8Size > 0) && (u8Size < 5))
   {
      register _UINT32 u32Tmp = u32Val;
      while (u8Size > 0)
      {
         *(p++) = (_UINT8)u32Tmp;
         u32Tmp = u32Tmp >> 8;
         u8Size--;
      }
   }
}


_UINT32 unpackBE(const _UINT8* p, _UINT8 u8Size)
{
   if ((u8Size > 0) && (u8Size < 5))
   {
      register _UINT32 u32Tmp = 0;
      while (u8Size > 0)
      {
         u32Tmp = (u32Tmp << 8) | *(p++);
         u8Size--;
      }
      return u32Tmp;
   }
   return 0;
}


_UINT32 unpackLE(const _UINT8* p, _UINT8 u8Size)
{
   if ((u8Size > 0) && (u8Size < 5))
   {
      register _UINT32 u32Tmp = 0;
      p += (u8Size - 1);
      while (u8Size > 0)
      {
         u32Tmp = (u32Tmp << 8) | *(p--);
         u8Size--;
      }
      return u32Tmp;
   }
   return 0;
}


/****************************** Local Functions ******************************/

