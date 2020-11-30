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
#define APX_VM_MAJOR_VERSION ((uint8_t) 2u)
#define APX_VM_MINOR_VERSION ((uint8_t) 0u)

#define APX_VM_HEADER_SIZE 8u
//byte 0: magic number 0x42 ('A'),
//bytes 1-2: APX_BYTE_CODE_VERSION(uint16_le),
//byte3 (high nibble): program flags
//byte3 (low nibble): program type (0=APX_VM_HEADER_UNPACK_PROG, 1=APX_VM_HEADER_PACK_PROG)
//bytes 4-7: maxDataSize (uint32_le)
#define APX_VM_HEADER_DATA_OFFSET 4
#define APX_VM_MAGIC_NUMBER              ((uint8_t) 'A')
#define APX_VM_HEADER_UNPACK_PROG    0x00u
#define APX_VM_HEADER_PACK_PROG      0x01u
#define APX_VM_HEADER_FLAG_DYNAMIC   0x10u
//reserved flags for future use: 0x20u, 0x40u, 0x80u

#define APX_VM_INSTRUCTION_SIZE          1u
#define APX_VM_OPCODE_SIZE               APX_VM_OPCODE_SIZE


/*
FORMAT:
+-------------+----------------+---------------+
| 1 FLag bit  | 4 variant bits | 3 opcode bits |
+-------------+----------------+---------------+

OP CODES

0: UNPACK:   14 variants
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
   11: BYTES
   12: STR
   13: ZSTR (not yet decided if needed)

1: PACK  14 variants
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
   11: BYTES
   12: STR
   13: ZSTR (not yet decided if needed)

2: ARRAY     : 3 variants
   0: U8
   1: U16
   2: U32

3: DATA_CTRL  : 1 variant
   0: RECORD_SELECT

4: FLOW_CTRL     : 1 variant
   1: ARRAY_NEXT

5: UNPACK2 (reserved for 16 additional data types)
6: PACK2 (reserved for 16 additional data types)
7: INVALID

*/

//OPCODE UNPACK
#define APX_OPCODE_UNPACK               0u
//If flagbit is set it means the next instruction is an opcode ARRAY
//PACK VARIANTS
#define APX_VARIANT_U8                0u
#define APX_VARIANT_U16               1u
#define APX_VARIANT_U32               2u
#define APX_VARIANT_U64               3u
#define APX_VARIANT_S8                4u
#define APX_VARIANT_S16               5u
#define APX_VARIANT_S32               6u
#define APX_VARIANT_S64               7u
#define APX_VARIANT_ARRAY             8u
#define APX_VARIANT_RECORD            9u
#define APX_VARIANT_BOOL              10u
#define APX_VARIANT_BYTES             11u
#define APX_VARIANT_STR               12u //string
#define APX_VARIANT_ZSTR              13u //null-terminated string (not yet decided if needed)
typedef uint8_t apx_vmVariant_t;

//OPCODE PACK
#define APX_OPCODE_PACK             1u
//SAME VARIANTS and FLAG as APX_OPCODE_UNPACK

//OPCODE ARRAY
#define APX_OPCODE_ARRAY            2u
//Uses VARIANT_U8, VARIANT_U16, VARIANT_U32.
//If flag is 0, the array is fixed (array length read from program), ELSE it is dynamic (array length read from data).

//OPCODE DATA_CTRL
#define APX_OPCODE_DATA_CTRL        3u
//DATA_CTRL variants
#define APX_VARIANT_RECORD_SELECT   0u //When flag is 1 it means this is the last field in the record

//OPCODE FLOW_CTRL
#define APX_OPCODE_FLOW_CTRL        4u
//APX_OPCODE_FLOW_CTRL variants
#define APX_VARIANT_ARRAY_NEXT      0u

#define APX_OPCODE_INVALID          7u



#define APX_INST_SIZE                  1u
#define APX_INST_OPCODE_MASK           0x07u
#define APX_INST_VARIANT_SHIFT         3u
#define APX_INST_VARIANT_MASK          0xfu
#define APX_INST_FLAG_MASK             0x01u
#define APX_INST_FLAG_SHIFT            7u
#define APX_INST_FLAG                  0x01u
#define APX_LAST_FIELD_FLAG            APX_INST_FLAG
#define APX_ARRAY_FLAG                 APX_INST_FLAG
#define APX_DYN_ARRAY_FLAG             APX_INST_FLAG
#define APX_INST_NO_FLAG               0

#define APX_VALUE_TYPE_NONE                0
#define APX_VALUE_TYPE_SCALAR              1
#define APX_VALUE_TYPE_ARRAY               2
#define APX_VALUE_TYPE_RECORD              3
typedef uint8_t apx_valueType_t;

#define APX_PROGRAM_GROW_SIZE              64

typedef adt_bytearray_t apx_program_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


#endif //APX_VM_DEFS_H
