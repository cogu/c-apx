#if defined(_MSC_PLATFORM_TOOLSET) && (_MSC_PLATFORM_TOOLSET<=100)
#ifndef _MSC_BOOL_DEFINED
#define _MSC_BOOL_DEFINED
#define false 0
#define true 1
typedef uint8_t bool;
#endif
#else
#include <stdbool.h>
#endif