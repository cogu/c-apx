#ifndef _RTE_H
#define _RTE_H
#include "Std_Types.h"

/* common errors */
#define RTE_E_OK               ((Std_ReturnType)  0)
#define RTE_E_INVALID          ((Std_ReturnType)  1)

/* overlayed errors */
#define RTE_E_LOST_DATA        ((Std_ReturnType) 64)
#define RTE_E_MAX_AGE_EXCEEDED ((Std_ReturnType) 64)

/* infrastructure errors */
#define RTE_E_TIMEOUT          ((Std_ReturnType)129)
#define RTE_E_LIMIT            ((Std_ReturnType)130)
#define RTE_E_NO_DATA          ((Std_ReturnType)131)

#endif /* _RTE_H */
