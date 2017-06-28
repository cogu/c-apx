#ifndef STD_TYPES_H
#define STD_TYPES_H

#include "Platform_Types.h"

#define STD_HIGH     1U /* Voltage level 5V or 3.3V */
#define STD_LOW      0U /* Voltage level 0V */

#define STD_ACTIVE   1U /* Logical level active */
#define STD_IDLE     0U /* Logical level idle */

#define STD_ON       1U
#define STD_OFF      0U

#ifndef STATUSTYPEDEFINED
#define STATUSTYPEDEFINED
#define E_OK 0U
#endif
#define E_NOT_OK 1u

typedef uint8 Std_ReturnType;

#endif  /* STD_TYPES_H */
