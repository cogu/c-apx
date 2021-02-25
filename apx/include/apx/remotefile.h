/*****************************************************************************
* \file      remotefile.h
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
#ifndef REMOTEFILE_H
#define REMOTEFILE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/types.h"
#include "apx/remotefile_cfg.h"

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

typedef uint8_t rmf_fileType_t;
#define RMF_FILE_TYPE_FIXED     ((rmf_fileType_t) 0u)
#define RMF_FILE_TYPE_DYNAMIC8  ((rmf_fileType_t) 1u)
#define RMF_FILE_TYPE_DYNAMIC16 ((rmf_fileType_t) 2u)
#define RMF_FILE_TYPE_DYNAMIC32 ((rmf_fileType_t) 3u)
#define RMF_FILE_TYPE_DEVICE    ((rmf_fileType_t) 4u)
#define RMF_FILE_TYPE_STREAM    ((rmf_fileType_t) 5u)

typedef uint8_t rmf_digestType_t;
#define RMF_DIGEST_TYPE_NONE   ((rmf_digestType_t) 0u)
#define RMF_DIGEST_TYPE_SHA1   ((rmf_digestType_t) 1u)
#define RMF_DIGEST_TYPE_SHA256 ((rmf_digestType_t) 2u)

#define RMF_REMOTE_ADDRESS_BIT    ((uint32_t) 0x80000000) //This is overlayed with RMF_HIGH_ADDR_BIT
#define RMF_INVALID_ADDRESS       ((uint32_t) 0x7FFFFFFF) //This is outside the valid address region of 30 bits
#define RMF_ADDRESS_MASK          ((uint32_t) 0x3FFFFFFF) //A true remote address can be at most 30 bits long
#define RMF_ADDRESS_MASK_INTERNAL ((uint32_t) 0x7FFFFFFF) //This is the address without the remote address bit
#define RMF_SHA1_SIZE             20u
#define RMF_SHA256_SIZE           32u

#define RMF_LOW_ADDR_MAX  ((uint32_t)0x3FFF)
#define RMF_LOW_ADDR_MASK RMF_LOW_ADDR_MAX
#define RMF_LOW_ADDR_SIZE UINT16_SIZE
#define RMF_HIGH_ADDR_MIN ((uint32_t)0x4000)            //16KB
#define RMF_HIGH_ADDR_MAX ((uint32_t)0x3FFFFFFFu)       //1GB
#define RMF_HIGH_ADDR_MASK RMF_HIGH_ADDR_MAX
#define RMF_HIGH_ADDR_SIZE UINT32_SIZE

#define RMF_CMD_AREA_START_ADDRESS ((uint32_t) 0x3FFFFC00)
#define RMF_CMD_AREA_END_ADDRESS   ((uint32_t) 0x3FFFFFFF)
#define RMF_CMD_AREA_SIZE          1024u
#define RMF_CMD_HIGH_BIT           ((uint32_t) 0x80000000)

#define RMF_MORE_BIT_LOW_ADDR      ((uint32_t) 0x4000u)
#define RMF_MORE_BIT_HIGH_ADDR     ((uint32_t) 0x40000000u)
#define RMF_HIGH_ADDR_BIT          ((uint32_t) 0x80000000) //This is overlayed with RMF_REMOTE_ADDRESS_BIT
#define RMF_U8_MORE_BIT            ((uint8_t)0x40)
#define RMF_U8_HIGH_ADDR_BIT       ((uint8_t)0x80)
#define RMF_MAX_FILE_NAME_SIZE     255u
#define RMF_FILE_INFO_HEADER_SIZE  48u
#define RMF_FILE_NAME_MAX_SIZE     RMF_MAX_FILE_NAME_SIZE

#define RMF_CMD_ACK_MSG            ((uint32_t) 0u)
#define RMF_CMD_NACK_MSG           ((uint32_t) 1u)
#define RMF_CMD_PUBLISH_FILE_MSG   ((uint32_t) 3u)
#define RMF_CMD_FILE_INFO_MSG      RMF_CMD_PUBLISH_FILE_MSG
#define RMF_CMD_REVOKE_FILE_MSG    ((uint32_t) 4u)
#define RMF_CMD_OPEN_FILE_MSG      ((uint32_t) 10u)
#define RMF_CMD_CLOSE_FILE_MSG     ((uint32_t) 11u)

#define RMF_FILE_OPEN_CMD_SIZE     UINT32_SIZE
#define RMF_FILE_CLOSE_CMD_SIZE    UINT32_SIZE
#define RMF_CMD_TYPE_SIZE          UINT32_SIZE


#define RMF_U16_FILE_TYPE_FIXED     ((uint16_t) 0u)
#define RMF_U16_FILE_TYPE_DYNAMIC8  ((uint16_t) 1u)
#define RMF_U16_FILE_TYPE_DYNAMIC16 ((uint16_t) 2u)
#define RMF_U16_FILE_TYPE_DYNAMIC32 ((uint16_t) 3u)
#define RMF_U16_FILE_TYPE_DEVICE    ((uint16_t) 4u)
#define RMF_U16_FILE_TYPE_STREAM    ((uint16_t) 5u)
#define RMF_U16_DIGEST_TYPE_NONE    ((uint16_t) 0u)
#define RMF_U16_DIGEST_TYPE_SHA1    ((uint16_t) 1u)
#define RMF_U16_DIGEST_TYPE_SHA256  ((uint16_t) 2u)

#define RMF_NUMHEADER_SIZE_16  UINT16_SIZE
#define RMF_NUMHEADER_SIZE_32  UINT32_SIZE
#define RMF_NUMHEADER_SIZE_DEFAULT RMF_NUMHEADER_SIZE_32

#define RMF_GREETING_MAX_LEN 127
#define RMF_GREETING10_START "RMFP/1.0\n"
#define RMF_GREETING11_START "RMFP/1.1\n"
#define RMF_MESSAGE_FORMAT_HDR "Message-Format"
#define RMF_CONNECTION_TYPE_MONITOR_HDR "Connection-Type"

apx_size_t rmf_needed_encoding_size(uint32_t address);
apx_size_t rmf_address_encode(uint8_t* buf, apx_size_t buf_size, uint32_t address, bool more_bit);
apx_size_t rmf_address_decode(uint8_t const* begin, uint8_t const* end, uint32_t* address, bool* more_bit);
apx_size_t rmf_encode_open_file_cmd(uint8_t* buf, apx_size_t buf_size, uint32_t address);
apx_size_t rmf_encode_acknowledge_cmd(uint8_t* buf, apx_size_t buf_size);
apx_size_t rmf_decode_cmd_type(uint8_t const* begin, uint8_t const* end, uint32_t* cmd_type);





//////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

#endif //REMOTEFILE_H
