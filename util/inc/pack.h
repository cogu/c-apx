#ifndef PACK_H
#define PACK_H
#ifdef USE_PLATFORM_TYPES
#include "Platform_Types.h"
#define _UINT8 uint8
#define _UINT32 uint32
#else
#include <stdint.h>
#define _UINT8 uint8_t
#define _UINT32 uint32_t
#endif

#define packU8(p,v) packBE(p,v,1),p+=1
#define packU16BE(p,v) packBE(p,v,2),p+=2
#define packU32BE(p,v) packBE(p,v,4),p+=4
#define packU16LE(p,v) packLE(p,v,2),p+=2
#define packU32LE(p,v) packLE(p,v,4),p+=4

#define unpackU8(p) (_UINT8) unpackBE(p,1),p+=1
#define unpackU16BE(p) (uint16) unpackBE(p,2),p+=2
#define unpackU32BE(p) (uint32) unpackBE(p,4),p+=4
#define unpackU16LE(p) (uint16) unpackLE(p,2),p+=2
#define unpackU32LE(p) (uint32) unpackLE(p,4),p+=4


/***************** Public Function Declarations *******************/
void packBE(_UINT8* p, _UINT32 u32Val, _UINT8 u8Size);
void packLE(_UINT8* p, _UINT32 u32Val, _UINT8 u8Size);
_UINT32 unpackBE(const _UINT8* p, _UINT8 u8Size);
_UINT32 unpackLE(const _UINT8* p, _UINT8 u8Size);

#undef _UINT8
#undef _UINT32

#endif //PACK_H

