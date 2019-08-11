/*****************************************************************************
* \file      apx_vmDefs.h
* \author    Conny Gustafsson
* \date      2019-01-03
* \brief     APX virtual machine shared definitions
*
* Copyright (c) 2019 Conny Gustafsson
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
#include "apx_types.h"
//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_VM_VERSION                2u
#define APX_OPCODE_SIZE               1u //sizeof(uint8_t)

#define APX_OPCODE_NOP                0u

#define APX_OPCODE_PACK_U8            1u
#define APX_OPCODE_PACK_U16           2u
#define APX_OPCODE_PACK_U32           3u
#define APX_OPCODE_PACK_U64           4u  //reserved but not implemented
#define APX_OPCODE_PACK_S8            5u
#define APX_OPCODE_PACK_S16           6u
#define APX_OPCODE_PACK_S32           7u
#define APX_OPCODE_PACK_S64           8u  //reserved but not implemented
#define APX_OPCODE_PACK_STR           9u  //null-terminated string. An array length must have been set by previous instruction
#define APX_OPCODE_PACK_BOOL          10u

//reserved for future data types
#define APX_OPCODE_UNPACK_U8          64u
#define APX_OPCODE_UNPACK_U16         65u
#define APX_OPCODE_UNPACK_U32         66u
#define APX_OPCODE_UNPACK_U64         67u
#define APX_OPCODE_UNPACK_S8          68u
#define APX_OPCODE_UNPACK_S16         69u
#define APX_OPCODE_UNPACK_S32         70u
#define APX_OPCODE_UNPACK_S64         70u
#define APX_OPCODE_UNPACK_STR         70u
#define APX_OPCODE_UNPACK_BOOL        71u

//reserved for future data types

//flow control op codes
#define APX_OPCODE_U8ARRAY            192u //next 1 byte of program is the length of array
#define APX_OPCODE_U16ARRAY           193u //next 2 bytes of program is the length of array (uint16_le)
#define APX_OPCODE_U32ARRAY           194u //next 4 bytes of program is the length of array (uint32_le)
#define APX_OPCODE_RECORD_PUSH        195u //enter new record element
#define APX_OPCODE_RECORD_POP         196u //leave current record element
#define APX_OPCODE_RECORD_SELECT      197u //Reads key as string from program. Next n bytes in program is the key, it keeps reading until a null-terminator is encountered.

#define APX_OPCODE_PACK_PROG          198u //pack program mode
#define APX_OPCODE_UNPACK_PROG        198u //unpack program mode

//data op codes
#define APX_OPCODE_U8DYNARRAY   198u //next 1 byte of data is the length of array
#define APX_OPCODE_U16DYNARRAY  199u //next 2 bytes of data is the length of array (uint16_le)
#define APX_OPCODE_U32DYNARRAY  200u //next 4 bytes of data is the length of array (uint32_le)

#define UINT8_SIZE   1u
#define UINT16_SIZE  2u
#define UINT32_SIZE  4u
#define UINT64_SIZE  8u
#define SINT8_SIZE   1u
#define SINT16_SIZE  2u
#define SINT32_SIZE  4u
#define SINT64_SIZE  8u
#define BOOL_SIZE    sizeof(bool)

#define APX_INST_PACK_PROG_SIZE         7u //bytes 0-1: VM_VERSION(uint16_le), byte2: program_type, bytes 3-6: expected_data_length (uint32_le)
#define APX_INST_UNPACK_PROG_SIZE       7u //bytes 0-1: VM_VERSION(uint16_le), byte2: program_type, bytes 3-6: expected_data_length (uint32_le)
#define APX_INST_PACK_U8_SIZE           1u
#define APX_INST_PACK_U16_SIZE          1u
#define APX_INST_PACK_U32_SIZE          1u
#define APX_INST_PACK_S8_SIZE           1u
#define APX_INST_PACK_S16_SIZE          1u
#define APX_INST_PACK_S32_SIZE          1u
#define APX_INST_PACK_STR_SIZE          3u
#define APX_INST_PACK_U8AR_SIZE         3u
#define APX_INST_PACK_S8AR_SIZE         3u
#define APX_INST_UNPACK_U8_SIZE         1u
#define APX_INST_UNPACK_U16_SIZE        1u
#define APX_INST_UNPACK_U32_SIZE        1u
#define APX_INST_UNPACK_S8_SIZE         1u
#define APX_INST_UNPACK_S16_SIZE        1u
#define APX_INST_UNPACK_S32_SIZE        1u
#define APX_INST_UNPACK_STR_SIZE        3u
#define APX_INST_UNPACK_U8AR_SIZE       3u
#define APX_INST_UNPACK_S8AR_SIZE       3u
#define APX_INST_RECORD_ENTER_SIZE      1u
#define APX_INST_RECORD_SELECT_SIZE     3u
#define APX_INST_RECORD_LEAVE_SIZE      1u
#define APX_INST_ARRAY_ENTER_SIZE       3u
#define APX_INST_DYNARRAY_ENTER_SIZE    1u
#define APX_INST_ARRAY_NEXT_SIZE        1u

#define APX_MAX_INST_PACK_SIZE          3u
#define APX_MAX_INST_UNPACK_SIZE        3u


#define APX_VALUE_TYPE_NONE                0
#define APX_VALUE_TYPE_SCALAR              1
#define APX_VALUE_TYPE_ARRAY               2
#define APX_VALUE_TYPE_RECORD              3
typedef uint8_t apx_valueType_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


#endif //APX_VM_DEFS_H
