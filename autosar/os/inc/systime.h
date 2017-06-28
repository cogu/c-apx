#ifndef SYS_TIME_H
#define SYS_TIME_H


/********************************* Includes **********************************/
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

/********************************* Functions *********************************/
#ifdef UNIT_TEST
void SysTime_initSimulated(void);
void SysTime_tick(_UINT32 tickMs);
#endif
int SysTime_init(_UINT32 tickMs);

void SysTime_destroy(void);
_UINT8 SysTime_wait(int isBlocking);
_UINT32 SysTime_getTime(void);
void SysTime_reset(void);

#undef _UINT8
#undef _UINT32

#endif //SYS_TIME_H

