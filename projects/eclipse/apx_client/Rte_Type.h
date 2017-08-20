#ifndef RTE_TYPE_H
#define RTE_TYPE_H

/*********************************************************************************************************************
* Includes
*********************************************************************************************************************/
#include "Std_Types.h"

/*********************************************************************************************************************
* Data Type Definitions
*********************************************************************************************************************/

#define Rte_TypeDef_OffOn_T
typedef uint8 OffOn_T;
#define OffOn_T_LowerLimit ((OffOn_T)0u)
#define OffOn_T_UpperLimit ((OffOn_T)3u)
#define RTE_CONST_OffOn_Off (0u)
#define RTE_CONST_OffOn_On (1u)
#define RTE_CONST_OffOn_Error (2u)
#define RTE_CONST_OffOn_NotAvailable (3u)

#define OffOn_Off ((OffOn_T)0u)
#define OffOn_On ((OffOn_T)1u)
#define OffOn_Error ((OffOn_T)2u)
#define OffOn_NotAvailable ((OffOn_T)3u)

#define Rte_TypeDef_Percent_T
typedef uint8 Percent_T;
#define Percent_T_LowerLimit ((Percent_T)0u)
#define Percent_T_UpperLimit ((Percent_T)255u)

#define Rte_TypeDef_VehicleSpeed_T
typedef uint16 VehicleSpeed_T;
#define VehicleSpeed_T_LowerLimit ((VehicleSpeed_T)0u)
#define VehicleSpeed_T_UpperLimit ((VehicleSpeed_T)65535u)


#ifndef RTE_SUPPRESS_UNUSED_DATATYPES

typedef boolean Boolean;

typedef sint16 SInt16;
#define SInt16_LowerLimit ((SInt16)-32768)
#define SInt16_UpperLimit ((SInt16)32767)

typedef sint32 SInt32;
#define SInt32_LowerLimit ((SInt32)-2147483648)
#define SInt32_UpperLimit ((SInt32)2147483647)

typedef sint8 SInt8;
#define SInt8_LowerLimit ((SInt8)-128)
#define SInt8_UpperLimit ((SInt8)127)

typedef uint16 UInt16;
#define UInt16_LowerLimit ((UInt16)0u)
#define UInt16_UpperLimit ((UInt16)65535u)

typedef uint32 UInt32;
#define UInt32_LowerLimit ((UInt32)0u)
#define UInt32_UpperLimit ((UInt32)4294967295u)

typedef uint8 UInt8;
#define UInt8_LowerLimit ((UInt8)0u)
#define UInt8_UpperLimit ((UInt8)255u)

#endif

#endif //RTE_TYPE_H

