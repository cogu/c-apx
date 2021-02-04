/*****************************************************************************
* \file      vm_defs.h
* \author    Conny Gustafsson
* \date      2019-01-03
* \brief     APX virtual machine shared definitions
*
* Copyright (c) 2019-2020 Conny Gustafsson
* Permission is hereby granted, free of charge, to any person obtaining a copy of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
* the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
* COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
* IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/
#ifndef APX_VM_DEFS_H
#define APX_VM_DEFS_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "adt_bytearray.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

      /*
      * APX VM 2.0 PROGRAM HEADER (Varies between 2 and 10 bytes)
      * Byte 0 (bits 4-7): program flags
      * Byte 0 (bit 3): pack or unpack program (0-1)
      * Byte 0 (bits 0-2): data size variant (VARIANT_U8, VARIANT_U16, VARIANT_U32). This determines the value of N in the next entry.
      * Bytes 1..(N-1): DataSize (Maximum data size required by this program) (variable-size encoded integer)
      *            - If Byte 1 (bits 0-2) has value VARIANT_U8 this is encoded as uint8. (N=2)
      *            - If Byte 1 (bits 0-2) has value VARIANT_U16 this is encoded as uint16le (little endian). (N=3)
      *            - If Byte 1 (bits 0-2) has value VARIANT_U32 this is encoded as uint32le. (N=5)
      * After the the previous integer is encoded the header usually ends. However, if HEADER_FLAG_QUEUED_DATA was set among program flags (Byte 0)
      * The header continues with an encoded DATA_SIZE instruction where variant must be value 3 or above.
      * Since length of previous header field varies we call the first byte after the encoded integer "Byte N".
      * Byte N: DATA_SIZE instruction header (Determines the value of QueueStorageSize)
      * Bytes (N+1)..(N+4): ElementSize (variable-size encoded integer)
      *
      * When HEADER_FLAG_QUEUED_DATA is set the queue size can be calculated from using the DataSize and ElementSize numbers.
      * The DataSize value has been previously incremented by the value 1, 2 or 4. Which of these it is can be determined from variant in the DATA_SIZE instruction.
      * The queue length can be calculated by using this formula:
      *
      * NumberOfQueuedElements = (DataSize-QueueStorageSize)/ElementSize
      * where QueueStorageSize is either 1, 2, or 4 (which can be determined from the variant on the DATA_SIZE instruction).
      * The result of NumberOfQueuedElements should always be an integer without fraction (otherwise the header have been incorrectly encoded).
      *
      */

#define APX_VM_MAJOR_VERSION ((uint8_t) 2u) //NO LONGER USED IN PROGRAM HEADER. Moved to file cache header instead.
#define APX_VM_MINOR_VERSION ((uint8_t) 0u) //NO LONGER USED IN PROGRAM HEADER. Moved to file cache header instead.
#define APX_VM_VERSION_SIZE 2u              //NO LONGER USED IN PROGRAM HEADER. Moved to file cache header instead.
#define APX_VM_HEADER_DATA_VARIANT_MASK ((uint8_t) 0x07) // Mask for bits 0..2 which can hold APX_VM_VARIANT_UINT8, APX_VM_VARIANT_UINT16 or APX_VM_VARIANT_UINT32
                                                         // (with an extra spare bit for future use)
#define APX_VM_HEADER_PROG_TYPE_UNPACK ((uint8_t) 0x00)
#define APX_VM_HEADER_PROG_TYPE_PACK   ((uint8_t) 0x08)
#define APX_VM_HEADER_FIXED_SIZE 2u

#define APX_VM_HEADER_FLAG_DYNAMIC_DATA ((uint8_t) 0x10) //This is just an indicator if any dynamic arrays are present inside the data.
#define APX_VM_HEADER_FLAG_QUEUED_DATA ((uint8_t) 0x20) //When this is active, the very next instruction must be OPCODE_DATA_SIZE.


/* APX VM 2.0 Instruction Format

          +-------------+----------------+---------------+
          | 1 FLag bit  | 4 variant bits | 3 opcode bits |
          +-------------+----------------+---------------+

          OP CODES
          0: UNPACK:   13 variants
             FLAG: is_array(true,false)
             0: U8
             1: U16
             2: U32
             3: U64
             4: S8
             5: S16
             6: S32
             7: S64
             8: BOOL
             9: BYTE (immutable bytes object)
             10: RECORD
             11: ARRAY
             12: ASCII_CHAR
             13: CHAR8
             14: CHAR16
             15: CHAR32

          1: PACK  13 variants
             FLAG: is_array(true,false)
             0: U8
             1: U16
             2: U32
             3: U64
             4: S8
             5: S16
             6: S32
             7: S64
             8: ARRAY
             9: RECORD
             10: BOOL
             11: BYTE (immutable bytes object)
             12: CHAR

          2: DATA_SIZE     : 6 variants
             FLAG: is_dynamic_array(true, false)
             0: ARRAY_SIZE_U8
             1: ARRAY_SIZE_U16
             2: ARRAY_SIZE_U32
             3: ELEMENT_SIZE_U8_QUEUE_SIZE_U8
             4: ELEMENT_SIZE_U8_QUEUE_SIZE_U16
             5: ELEMENT_SIZE_U8_QUEUE_SIZE_U32
             6: ELEMENT_SIZE_U16_QUEUE_SIZE_U8
             7: ELEMENT_SIZE_U16_QUEUE_SIZE_U16
             8: ELEMENT_SIZE_U16_QUEUE_SIZE_U32
             9: ELEMENT_SIZE_U32_QUEUE_SIZE_U8
             10: ELEMENT_SIZE_U32_QUEUE_SIZE_U16
             11: ELEMENT_SIZE_U32_QUEUE_SIZE_U32

          3: DATA_CTRL  : 9 variants
             0: RECORD_SELECT
             1: LIMIT_CHECK_U8
             2: LIMIT_CHECK_U16
             3: LIMIT_CHECK_U32
             4: LIMIT_CHECK_U64
             5: LIMIT_CHECK_S8
             6: LIMIT_CHECK_S16
             7: LIMIT_CHECK_S32
             8: LIMIT_CHECK_S64
             FLAG(variant 0): When true: This is the last record field.
                  When false: More record fields to follow
             FLAG (variant 1..8): When true, the limit check applies to non-scalar value (such as array of u8, u16 etc.)
          4: FLOW_CTRL     : 1 variant
             0: ARRAY_NEXT
          5: UNPACK2 (reserved for 16 additional data types)
          6: PACK2 (reserved for 16 additional data types)
          7: RESERVED
          */

//OPCODE UNPACK
#define APX_VM_OPCODE_UNPACK               ((uint8_t) 0u)
//If flagbit is set it means the next instruction is an opcode ARRAY
//PACK VARIANTS
#define APX_VM_VARIANT_UINT8            ((uint8_t) 0u)
#define APX_VM_VARIANT_UINT16           ((uint8_t) 1u)
#define APX_VM_VARIANT_UINT32           ((uint8_t) 2u)
#define APX_VM_VARIANT_UINT64           ((uint8_t) 3u)
#define APX_VM_VARIANT_INT8             ((uint8_t) 4u)
#define APX_VM_VARIANT_INT16            ((uint8_t) 5u)
#define APX_VM_VARIANT_INT32            ((uint8_t) 6u)
#define APX_VM_VARIANT_INT64            ((uint8_t) 7u)
#define APX_VM_VARIANT_BOOL             ((uint8_t) 8u)
#define APX_VM_VARIANT_BYTE             ((uint8_t) 9u)
#define APX_VM_VARIANT_RECORD           ((uint8_t) 10u)
#define APX_VM_VARIANT_ARRAY            ((uint8_t) 11u)
#define APX_VM_VARIANT_CHAR             ((uint8_t) 12u) //Latin1 encoding
#define APX_VM_VARIANT_CHAR8            ((uint8_t) 13u) //UTF-8 encoding
#define APX_VM_VARIANT_CHAR16           ((uint8_t) 14u) //UTF-16 encoding
#define APX_VM_VARIANT_CHAR32           ((uint8_t) 15u) //UTF-32 encoding
#define APX_VM_VARIANT_LAST             APX_VM_VARIANT_CHAR32
#define APX_VM_VARIANT_INVALID          ((uint8_t) 255u)

//OPCODE PACK
#define APX_VM_OPCODE_PACK              ((uint8_t) 1u)
//same variants as OPCODE_UNPACK

//OPCODE DATA_SIZE
#define APX_VM_OPCODE_DATA_SIZE         ((uint8_t) 2u)
#define APX_VM_VARIANT_ARRAY_SIZE_U8    ((uint8_t) 0u)
#define APX_VM_VARIANT_ARRAY_SIZE_U16   ((uint8_t) 1u)
#define APX_VM_VARIANT_ARRAY_SIZE_U32   ((uint8_t) 2u)
#define APX_VM_VARIANT_ARRAY_SIZE_LAST  APX_VM_VARIANT_ARRAY_SIZE_U32
#define APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE ((uint8_t) 3u)
#define APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U8 (APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE + APX_VM_VARIANT_UINT8)    // 3
#define APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U16 (APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE + APX_VM_VARIANT_UINT16)  // 4
#define APX_VM_VARIANT_ELEMENT_SIZE_U8_QUEUE_SIZE_U32 (APX_VM_VARIANT_ELEMENT_SIZE_U8_BASE + APX_VM_VARIANT_UINT32)  // 5
#define APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE ((uint8_t) 6u)
#define APX_VM_VARIANT_ELEMENT_SIZE_U16_QUEUE_SIZE_U8 (APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE + APX_VM_VARIANT_UINT8)    // 6
#define APX_VM_VARIANT_ELEMENT_SIZE_U16_QUEUE_SIZE_U16 (APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE + APX_VM_VARIANT_UINT16)  // 7
#define APX_VM_VARIANT_ELEMENT_SIZE_U16_QUEUE_SIZE_U32 (APX_VM_VARIANT_ELEMENT_SIZE_U16_BASE + APX_VM_VARIANT_UINT32)  // 8
#define APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE ((uint8_t) 9u)
#define APX_VM_VARIANT_ELEMENT_SIZE_U32_QUEUE_SIZE_U8 (APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE + APX_VM_VARIANT_UINT8)    // 9
#define APX_VM_VARIANT_ELEMENT_SIZE_U32_QUEUE_SIZE_U16 (APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE + APX_VM_VARIANT_UINT16)  // 10
#define APX_VM_VARIANT_ELEMENT_SIZE_U32_QUEUE_SIZE_U32 (APX_VM_VARIANT_ELEMENT_SIZE_U32_BASE + APX_VM_VARIANT_UINT32)  // 11
#define APX_VM_VARIANT_ELEMENT_SIZE_LAST APX_VM_VARIANT_ELEMENT_SIZE_U32_QUEUE_SIZE_U32
// For variants 0..2: Maximum array size is always encoded into program.
// If flag bit is set then the current array size is serialized into data buffer (as next byte(s)). This is used for dynamic arrays.
// Flag bit is not used for variants 3 through 11

//OPCODE DATA_CTRL
#define APX_VM_OPCODE_DATA_CTRL         ((uint8_t) 3u)
#define APX_VM_VARIANT_RECORD_SELECT    ((uint8_t) 0u)
#define APX_VM_VARIANT_LIMIT_CHECK_NONE ((uint8_t) 0u) //Overlays with APX_VM_VARIANT_RECORD_SELECT (context-specific)
#define APX_VM_VARIANT_LIMIT_CHECK_U8   ((uint8_t) 1u)
#define APX_VM_VARIANT_LIMIT_CHECK_U16  ((uint8_t) 2u)
#define APX_VM_VARIANT_LIMIT_CHECK_U32  ((uint8_t) 3u)
#define APX_VM_VARIANT_LIMIT_CHECK_U64  ((uint8_t) 4u)
#define APX_VM_VARIANT_LIMIT_CHECK_S8   ((uint8_t) 5u)
#define APX_VM_VARIANT_LIMIT_CHECK_S16  ((uint8_t) 6u)
#define APX_VM_VARIANT_LIMIT_CHECK_S32  ((uint8_t) 8u)
#define APX_VM_VARIANT_LIMIT_CHECK_S64  ((uint8_t) 9u)
#define APX_VM_VARIANT_LIMIT_CHECK_LAST APX_VM_VARIANT_LIMIT_CHECK_S64

//OPCODE FLOW_CTRL
#define APX_VM_OPCODE_FLOW_CTRL ((uint8_t) 4u)
#define APX_VM_VARIANT_ARRAY_NEXT ((uint8_t) 0u)
#define APX_VM_VARIANT_FLOW_CTRL_LAST APX_VM_VARIANT_ARRAY_NEXT


//Other VM-related defines
#define APX_VM_INST_SIZE ((uint32_t) sizeof(uint8_t))
#define APX_VM_INST_OPCODE_MASK   7u
#define APX_VM_INST_VARIANT_SHIFT 3u
#define APX_VM_INST_VARIANT_MASK 0xf
#define APX_VM_INST_FLAG         0x80
#define APX_VM_ARRAY_FLAG        APX_VM_INST_FLAG
#define APX_VM_DYN_ARRAY_FLAG    APX_VM_INST_FLAG
#define APX_VM_LAST_FIELD_FLAG   APX_VM_INST_FLAG

#define APX_VM_UINT8_SIZE  ((uint32_t) sizeof(uint8_t))
#define APX_VM_CHAR_SIZE   ((uint32_t) sizeof(char))
#define APX_VM_CHAR8_SIZE  ((uint32_t) sizeof(uint8_t))   //Replaces sizeof(std::char8_t) as seen in C++20
#define APX_VM_CHAR16_SIZE ((uint32_t) sizeof(uint16_t))  //Replaces sizeof(std::char16_t) as seen in C++20
#define APX_VM_CHAR32_SIZE ((uint32_t) sizeof(uint32_t))  //Replaces sizeof(std::char32_t) as seen in C++20
#define APX_VM_BOOL_SIZE   ((uint32_t) sizeof(uint8_t))
#define APX_VM_BYTE_SIZE   ((uint32_t) sizeof(uint8_t))   //Replaces sizeof(std::byte_t) as seen in C++20
#define APX_VM_UINT16_SIZE ((uint32_t) sizeof(uint16_t))
#define APX_VM_UINT32_SIZE ((uint32_t) sizeof(uint32_t))
#define APX_VM_UINT64_SIZE ((uint32_t) sizeof(uint64_t))
#define APX_VM_INT8_SIZE   ((uint32_t) sizeof(int8_t))
#define APX_VM_INT16_SIZE  ((uint32_t) sizeof(int16_t))
#define APX_VM_INT32_SIZE  ((uint32_t) sizeof(int32_t))
#define APX_VM_INT64_SIZE  ((uint32_t) sizeof(int64_t))

typedef uint8_t apx_operationType_t;
#define APX_OPERATION_TYPE_PROGRAM_END         ((apx_operationType_t) 0u)
#define APX_OPERATION_TYPE_UNPACK              ((apx_operationType_t) 1u)
#define APX_OPERATION_TYPE_PACK                ((apx_operationType_t) 2u)
#define APX_OPERATION_TYPE_RANGE_CHECK_INT32   ((apx_operationType_t) 3u)
#define APX_OPERATION_TYPE_RANGE_CHECK_UINT32  ((apx_operationType_t) 4u)
#define APX_OPERATION_TYPE_RANGE_CHECK_INT64   ((apx_operationType_t) 5u)
#define APX_OPERATION_TYPE_RANGE_CHECK_UINT64  ((apx_operationType_t) 6u)
#define APX_OPERATION_TYPE_RECORD_SELECT       ((apx_operationType_t) 7u)
#define APX_OPERATION_TYPE_ARRAY_NEXT          ((apx_operationType_t) 8u)

typedef struct apx_packUnpackOperationInfo_tag
{
   apx_typeCode_t type_code;
   uint32_t array_length;
   bool is_dynamic_array;
} apx_packUnpackOperationInfo_t;

typedef struct apx_rangeCheckUInt32OperationInfo_tag
{
   uint32_t lower_limit;
   uint32_t upper_limit;
} apx_rangeCheckUInt32OperationInfo_t;

typedef struct apx_rangeCheckUInt64OperationInfo_tag
{
   uint64_t lower_limit;
   uint64_t upper_limit;
} apx_rangeCheckUInt64OperationInfo_t;

typedef struct apx_rangeCheckInt32OperationInfo_tag
{
   int32_t lower_limit;
   int32_t upper_limit;
} apx_rangeCheckInt32OperationInfo_t;

typedef struct apx_rangeCheckInt64OperationInfo_tag
{
   int64_t lower_limit;
   int64_t upper_limit;
} apx_rangeCheckInt64OperationInfo_t;




//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


#endif //APX_VM_DEFS_H
