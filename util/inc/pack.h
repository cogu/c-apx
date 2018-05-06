#ifndef PACK_H
#define PACK_H
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

#define packU8(p,v) packBE(p,v,1),p+=1
#define packU16BE(p,v) packBE(p,v,2),p+=2
#define packU32BE(p,v) packBE(p,v,4),p+=4
#define packU16LE(p,v) packLE(p,v,2),p+=2
#define packU32LE(p,v) packLE(p,v,4),p+=4

#define unpackU8(p) (_UINT8) unpackBE(p,1),p+=1
#define unpackU16BE(p) (_UINT16) unpackBE(p,2),p+=2
#define unpackU32BE(p) (_UINT32) unpackBE(p,4),p+=4
#define unpackU16LE(p) (_UINT16) unpackLE(p,2),p+=2
#define unpackU32LE(p) (_UINT32) unpackLE(p,4),p+=4

#if  defined(__GNUC__) && defined(__LP64__)
#define packU64LE(p,v) packLE(p,v,8),p+=8
#define unpackU64LE(p,v) (_UINT64) packLE(p,v,4),p+=8
#endif


/***************** Public Function Declarations *******************/
void packBE(_UINT8* p, _PACK_BASE_TYPE value, _UINT8 u8Size);
void packLE(_UINT8* p, _PACK_BASE_TYPE value, _UINT8 u8Size);
_PACK_BASE_TYPE unpackBE(const _UINT8* p, _UINT8 u8Size);
_PACK_BASE_TYPE unpackLE(const _UINT8* p, _UINT8 u8Size);
void packLE64(_UINT8* p, _UINT64 value); //forces data to be 64-bits (8 bytes), even on 32-bit machines
_UINT64 unpackLE64(const _UINT8* p); //forces data to be 64-bits (8 bytes), even on 32-bit machines

#undef _UINT8
#undef _UINT32
#undef _UINT64
#undef _PACK_BASE_TYPE

#endif //PACK_H

