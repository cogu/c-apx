#ifndef PLATFORM_H
#define PLATFORM_H

#define STDLIB_AVAILABLE 1

/********************************* Includes **********************************/
#if STDLIB_AVAILABLE
#include <string.h>
#define MEMCPY(d,s,l) memcpy(d,s,l);
#endif
/**************************** Constants and Types ****************************/

/********************************* Functions *********************************/


#endif //PLATFORM_H
