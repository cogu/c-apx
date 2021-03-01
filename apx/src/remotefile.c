/*****************************************************************************
* \file      remotefile.c
* \author    Conny Gustafsson
* \date      2021-01-20
* \brief     Remotefile layer
*
* Copyright (c) 2021 Conny Gustafsson
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include "apx/remotefile.h"
#include "pack.h"
#ifdef MEM_LEAK_CHECK
#include "CMemLeak.h"
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// LOCAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

apx_size_t rmf_needed_encoding_size(uint32_t address)
{
   return (address < RMF_HIGH_ADDR_MIN) ? UINT16_SIZE : UINT32_SIZE;
}

apx_size_t rmf_address_encode(uint8_t* buf, apx_size_t buf_size, uint32_t address, bool more_bit)
{
   if ((buf == NULL) || (buf_size == 0) || (address > RMF_HIGH_ADDR_MAX))
   {
      return 0; //Invalid argument
   }
   apx_size_t encoding_size = rmf_needed_encoding_size(address);
   if (encoding_size <= buf_size)
   {
      if (encoding_size == UINT16_SIZE)
      {
         uint16_t value = more_bit ? RMF_MORE_BIT_LOW_ADDR : 0u;
         value |= (uint16_t)address;
         packBE(buf, (uint32_t)value, (uint8_t) UINT16_SIZE);
      }
      else
      {
         assert(encoding_size == UINT32_SIZE);
         uint32_t value = more_bit ? (RMF_HIGH_ADDR_BIT | RMF_MORE_BIT_HIGH_ADDR) : RMF_HIGH_ADDR_BIT;
         value |= address;
         packBE(buf, value, (uint8_t)UINT32_SIZE);
      }
   }
   else
   {
      return 0u; //Not enough bytes in buffer
   }
   return encoding_size;
}

apx_size_t rmf_address_decode(uint8_t const* begin, uint8_t const* end, uint32_t* address, bool* more_bit)
{
   apx_size_t retval = 0u;
   if ((begin == NULL) || (end == NULL) || (address == NULL) || (more_bit == NULL) || (begin >= end))
   {
      return 0u; //Invalid argument
   }
   uint8_t const first_byte = *begin;
   *more_bit = (first_byte & RMF_U8_MORE_BIT) ? true : false;
   if (first_byte & RMF_U8_HIGH_ADDR_BIT)
   {
      if (begin +  UINT32_SIZE <= end)
      {
         uint32_t value = unpackBE(begin, (uint8_t) UINT32_SIZE);
         *address = value & RMF_HIGH_ADDR_MASK;
         retval = UINT32_SIZE;
      }
   }
   else
   {
      if (begin + UINT16_SIZE <= end)
      {
         uint32_t value = unpackBE(begin, UINT16_SIZE);
         *address = (value & RMF_LOW_ADDR_MASK);
         retval = UINT16_SIZE;
      }
   }
   return retval;
}

apx_size_t rmf_encode_open_file_cmd(uint8_t* buf, apx_size_t buf_size, uint32_t address)
{
   apx_size_t const required_size = RMF_CMD_TYPE_SIZE + RMF_FILE_OPEN_CMD_SIZE;
   if ((address > RMF_HIGH_ADDR_MAX) || (required_size > buf_size))
   {
      return 0;
   }
   uint8_t* p = buf;
   packLE(p, RMF_CMD_OPEN_FILE_MSG, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   packLE(p, address, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   return required_size;
}

apx_size_t rmf_encode_acknowledge_cmd(uint8_t* buf, apx_size_t buf_size)
{
   apx_size_t const required_size =RMF_CMD_TYPE_SIZE;
   if (required_size > buf_size)
   {
      return 0u;
   }
   packLE(buf, RMF_CMD_ACK_MSG, (uint8_t)UINT32_SIZE);
   return required_size;
}

apx_size_t rmf_encode_header_accepted(uint8_t* buf, apx_size_t buf_size, uint32_t connection_id)
{
   apx_size_t const required_size = RMF_CMD_TYPE_SIZE + UINT32_SIZE;
   if (required_size > buf_size)
   {
      return 0;
   }
   uint8_t* p = buf;
   packLE(p, RMF_CMD_ACCEPT_HEADER, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   packLE(p, connection_id, (uint8_t)UINT32_SIZE); p += UINT32_SIZE;
   return required_size;
}

apx_size_t rmf_decode_cmd_type(uint8_t const* begin, uint8_t const* end, uint32_t* cmd_type)
{
   if ((begin == NULL) || (end == NULL) || (begin >= end) || (cmd_type == NULL))
   {
      return 0u;
   }
   if (begin + RMF_CMD_TYPE_SIZE <= end)
   {
      *cmd_type = unpackLE(begin, UINT32_SIZE);
      return RMF_CMD_TYPE_SIZE;
   }
   return 0u;
}




//////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


