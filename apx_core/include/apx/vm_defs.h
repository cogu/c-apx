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
      * APX VM 2.1 PROGRAM HEADER (Varies between 4 and 12 bytes)
      *
      * Base Program Header:
      * Byte 0: APX VM major version number ('2' in ASCII, 0x32)
      * Byte 1: APX VM minor version number ('1' in ASCII, 0x31)
      * Byte 2: Program Flags and Type byte
      *         - Bits 0-2: Data size variant (VARIANT_U8, VARIANT_U16, VARIANT_U32)
      *                     Determines the byte width of MaxDataSize directly following Byte 2:
      *                     - 0 (VARIANT_U8):  1 byte (0..255)
      *                     - 1 (VARIANT_U16): 2 bytes, little-endian (0..65535)
      *                     - 2 (VARIANT_U32): 4 bytes, little-endian (0..4294967295)
      *         - Bit 3:    Program type (0 = UNPACK, 1 = PACK)
      *         - Bit 4:    DYNAMIC_DATA flag (0x10): Active if dynamic arrays are present
      *         - Bit 5:    QUEUED_DATA flag (0x20): Active if this is a queued port
      *         - Bits 6-7: Reserved for future use (must be 0)
      * Bytes 3..(N-1): MaxDataSize (Maximum expected data size, 1, 2, or 4 bytes little-endian)
      *
      * Queued Port Header Extension (only when QUEUED_DATA flag is set):
      * Since the length of the previous field varies, we call the next byte "Byte N".
      * Byte N:             Embedded DATA_SIZE instruction header (Opcode 2, Variants 3..11, Flag bit = 0)
      * Bytes (N+1)..(N+M): ElementSize (1, 2, or 4 bytes little-endian unsigned integer)
      *
      * When QUEUED_DATA is set, the queue length can be calculated using the formula:
      *
      * Queue Length = (MaxDataSize - QueueStorageSize) / ElementSize
      *
      * where QueueStorageSize is the header prefix size (in bytes) reserved in the data buffer
      * to store the current queue length:
      *   - 1 byte when Queue Size is UINT8  (Variants 3, 6, 9)
      *   - 2 bytes when Queue Size is UINT16 (Variants 4, 7, 10)
      *   - 4 bytes when Queue Size is UINT32 (Variants 5, 8, 11)
      *
      * The result of Queue Length should always be an integer without fraction.
      */

#define APX_VM_MAJOR_VERSION ((uint8_t) 2u) //NO LONGER USED IN PROGRAM HEADER. Moved to file cache header instead.
#define APX_VM_MINOR_VERSION ((uint8_t) 0u) //NO LONGER USED IN PROGRAM HEADER. Moved to file cache header instead.
#define APX_VM_HEADER_VERSION_MAJOR ((uint8_t) 0x32u) // ASCII '2'
#define APX_VM_HEADER_VERSION_MINOR ((uint8_t) 0x31u) // ASCII '1'
#define APX_VM_VERSION_SIZE 2u              //NO LONGER USED IN PROGRAM HEADER. Moved to file cache header instead.
#define APX_VM_HEADER_DATA_VARIANT_MASK ((uint8_t) 0x07) // Mask for bits 0..2 which can hold APX_VM_VARIANT_UINT8, APX_VM_VARIANT_UINT16 or APX_VM_VARIANT_UINT32
                                                         // (with an extra spare bit for future use)
#define APX_VM_HEADER_PROG_TYPE_UNPACK ((uint8_t) 0x00)
#define APX_VM_HEADER_PROG_TYPE_PACK   ((uint8_t) 0x08)
#define APX_VM_HEADER_FIXED_SIZE 2u

#define APX_VM_HEADER_FLAG_DYNAMIC_DATA ((uint8_t) 0x10) //This is just an indicator if any dynamic arrays are present inside the data.
#define APX_VM_HEADER_FLAG_QUEUED_DATA ((uint8_t) 0x20) //When this is active, the very next instruction must be OPCODE_DATA_SIZE.


/* APX VM 2.1 Instruction Format

          +------------+---------------+----------------+
          | 1 Flag bit | 3 opcode bits | 4 variant bits |
          +------------+---------------+----------------+
          |   Bit 7    |   Bits 4-6    |    Bits 0-3    |
          +------------+---------------+----------------+

          OP CODES
          0: PACK:       14 variants
             FLAG: is_array (true, false)
             0: UINT8
             1: UINT16
             2: UINT32
             3: UINT64
             4: INT8
             5: INT16
             6: INT32
             7: INT64
             8: BOOL
             9: BYTE (raw byte / blob)
             10: RECORD
             11: ARRAY (reserved for nested arrays / future use)
             12: CHAR (ASCII)
             13: CHAR8 (UTF-8)
             14: CHAR16 (UTF-16)
             15: CHAR32 (UTF-32)

          1: UNPACK:     14 variants
             FLAG: is_array (true, false)
             Same variants (0..15) and payload sizes as PACK.

          2: DATA_SIZE:  12 variants
             Variants 0..2: ARRAY_SIZE
             FLAG: is_dynamic_array (0: fixed-length, 1: dynamic-length)
             0: ARRAY_SIZE_UINT8  (1 byte length payload, 0..255)
             1: ARRAY_SIZE_UINT16 (2 bytes length payload, 256..65535, little-endian)
             2: ARRAY_SIZE_UINT32 (4 bytes length payload, > 65535, little-endian)

             Variants 3..11: ELEMENT_SIZE & QUEUE_SIZE (Queued Port Header Extension)
             FLAG: Unused (must be 0)
             3:  ELEMENT_SIZE_U8_QUEUE_SIZE_UINT8   (1B elem size, 1B queue storage)
             4:  ELEMENT_SIZE_U8_QUEUE_SIZE_UINT16  (1B elem size, 2B queue storage)
             5:  ELEMENT_SIZE_U8_QUEUE_SIZE_UINT32  (1B elem size, 4B queue storage)
             6:  ELEMENT_SIZE_U16_QUEUE_SIZE_UINT8  (2B elem size, 1B queue storage)
             7:  ELEMENT_SIZE_U16_QUEUE_SIZE_UINT16 (2B elem size, 2B queue storage)
             8:  ELEMENT_SIZE_U16_QUEUE_SIZE_UINT32 (2B elem size, 4B queue storage)
             9:  ELEMENT_SIZE_U32_QUEUE_SIZE_UINT8  (4B elem size, 1B queue storage)
             10: ELEMENT_SIZE_U32_QUEUE_SIZE_UINT16 (4B elem size, 2B queue storage)
             11: ELEMENT_SIZE_U32_QUEUE_SIZE_UINT32 (4B elem size, 4B queue storage)

          3: DATA_CTRL:  10 variants
             0: RECORD_SELECT (Variable payload: null-terminated ASCII string)
                FLAG: When 1, this is the first field of the record.
                      When 0, subsequent record field.
             1: RECORD_END (0 payload bytes)
                Explicit end-of-record delimiter.
             2: LIMIT_CHECK_UINT8  (2 bytes payload: 1B lower, 1B upper)
             3: LIMIT_CHECK_UINT16 (4 bytes payload: 2B lower, 2B upper, little-endian)
             4: LIMIT_CHECK_UINT32 (8 bytes payload: 4B lower, 4B upper, little-endian)
             5: LIMIT_CHECK_UINT64 (16 bytes payload: 8B lower, 8B upper, little-endian)
             6: LIMIT_CHECK_INT8   (2 bytes payload: 1B lower, 1B upper)
             7: LIMIT_CHECK_INT16  (4 bytes payload: 2B lower, 2B upper, little-endian)
             8: LIMIT_CHECK_INT32  (8 bytes payload: 4B lower, 4B upper, little-endian)
             9: LIMIT_CHECK_INT64  (16 bytes payload: 8B lower, 8B upper, little-endian)
             FLAG (variants 2..9):
                0: Limit check applies to scalar value.
                1: Limit check applies to array of values.

          4: FLOW_CTRL:  1 variant
             0: ARRAY_NEXT (0 payload bytes)
                Advances internal array iterator to the next element.

          5: RESERVED (Reserved for future use)
          6: RESERVED (Reserved for future use)
          7: RESERVED (Reserved for future use)
          */

//OPCODE PACK
#define APX_VM_OPCODE_PACK              ((uint8_t) 0u)
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

//OPCODE UNPACK
#define APX_VM_OPCODE_UNPACK            ((uint8_t) 1u)
//same variants as OPCODE_PACK

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
#define APX_VM_VARIANT_RECORD_END       ((uint8_t) 1u)
#define APX_VM_VARIANT_LIMIT_CHECK_U8   ((uint8_t) 2u)
#define APX_VM_VARIANT_LIMIT_CHECK_U16  ((uint8_t) 3u)
#define APX_VM_VARIANT_LIMIT_CHECK_U32  ((uint8_t) 4u)
#define APX_VM_VARIANT_LIMIT_CHECK_U64  ((uint8_t) 5u)
#define APX_VM_VARIANT_LIMIT_CHECK_S8   ((uint8_t) 6u)
#define APX_VM_VARIANT_LIMIT_CHECK_S16  ((uint8_t) 7u)
#define APX_VM_VARIANT_LIMIT_CHECK_S32  ((uint8_t) 8u)
#define APX_VM_VARIANT_LIMIT_CHECK_S64  ((uint8_t) 9u)
#define APX_VM_VARIANT_LIMIT_CHECK_LAST APX_VM_VARIANT_LIMIT_CHECK_S64

//OPCODE FLOW_CTRL
#define APX_VM_OPCODE_FLOW_CTRL ((uint8_t) 4u)
#define APX_VM_VARIANT_ARRAY_NEXT ((uint8_t) 0u)
#define APX_VM_VARIANT_FLOW_CTRL_LAST APX_VM_VARIANT_ARRAY_NEXT


//Other VM-related defines
#define APX_VM_INST_SIZE          ((uint32_t) sizeof(uint8_t))
#define APX_VM_INST_OPCODE_MASK   7u
#define APX_VM_INST_OPCODE_SHIFT  4u
#define APX_VM_INST_VARIANT_MASK  0x0fu
#define APX_VM_INST_VARIANT_SHIFT 0u
#define APX_VM_INST_FLAG          0x80u
#define APX_VM_ARRAY_FLAG         APX_VM_INST_FLAG
#define APX_VM_DYN_ARRAY_FLAG     APX_VM_INST_FLAG
#define APX_VM_FIRST_FIELD_FLAG   APX_VM_INST_FLAG

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
#define APX_OPERATION_TYPE_RECORD_END          ((apx_operationType_t) 8u)
#define APX_OPERATION_TYPE_ARRAY_NEXT          ((apx_operationType_t) 9u)

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
