#ifndef COMSTACK_TYPES_H
#define COMSTACK_TYPES_H

#include "Std_Types.h"

typedef uint8 PduIdType;
typedef uint32 PduLengthType;

typedef struct {
   uint8* SduDataPtr;
   PduLengthType SduLength;
}PduInfoType;

#endif  /* COMSTACK_TYPES_H */




