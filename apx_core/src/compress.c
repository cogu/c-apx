/*****************************************************************************
 * \file      compress.c
 * \author    Conny Gustafsson
 * \date      2026-11-04
 * \brief     APX compression and decompression codec implementation
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/compress.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#if defined(APX_COMPRESSION_BACKEND_ZLIB)
# include <zlib.h>
# define BACKEND_NAME "zlib"
#elif defined(APX_COMPRESSION_BACKEND_MINIZ)
# include <miniz.h>
# define BACKEND_NAME "miniz"
#else
# error "No APX compression backend defined. Define APX_COMPRESSION_BACKEND_ZLIB or APX_COMPRESSION_BACKEND_MINIZ."
#endif

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define MIN_ZLIB_STREAM_LEN 6u

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

static apx_error_t validate_zlib_header(const uint8_t *src, size_t src_len)
{
  if (src_len < MIN_ZLIB_STREAM_LEN)
  {
    return APX_UNEXPECTED_END_ERROR;
  }

  uint8_t cmf = src[0];
  uint8_t flg = src[1];

  // Compression Method must be 8 (DEFLATE), CINFO <= 7 (window size <= 32KB)
  if (((cmf & 0x0Fu) != 8u) || (((cmf >> 4u) & 0x0Fu) > 7u))
  {
    return APX_INVALID_HEADER_ERROR;
  }

  // Header check: (CMF * 256 + FLG) % 31 == 0
  if ((((uint32_t)cmf * 256u) + (uint32_t)flg) % 31u != 0u)
  {
    return APX_INVALID_HEADER_ERROR;
  }

  // Preset dictionary (FDICT, bit 5) is rejected per specification
  if ((flg & 0x20u) != 0u)
  {
    return APX_INVALID_HEADER_ERROR;
  }

  return APX_NO_ERROR;
}

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

const char *apx_compress_backend_name(void) { return BACKEND_NAME; }

size_t apx_compress_bound(size_t src_len)
{
  if (src_len > UINT_MAX)
  {
    return 0;
  }
  return (size_t)compressBound((uLong)src_len);
}

apx_error_t apx_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len)
{
  return apx_compress_level(dst, dst_len, src, src_len, APX_COMPRESSION_DEFAULT_LEVEL);
}

apx_error_t apx_compress_level(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len, int level)
{
  if ((dst == NULL) || (dst_len == NULL) || ((src_len > 0) && (src == NULL)))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if ((level != APX_COMPRESSION_DEFAULT_LEVEL) && ((level < 0) || (level > 9)))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if ((src_len > UINT_MAX) || (*dst_len > UINT_MAX))
  {
    return APX_LENGTH_ERROR;
  }

  int comp_level = (level == APX_COMPRESSION_DEFAULT_LEVEL) ? Z_DEFAULT_COMPRESSION : level;

  z_stream strm;
  memset(&strm, 0, sizeof(strm));

  int ret = deflateInit(&strm, comp_level);
  if (ret != Z_OK)
  {
    return (ret == Z_MEM_ERROR) ? APX_MEM_ERROR : APX_COMPRESSION_ERROR;
  }

  strm.next_in = (Bytef *)src;
  strm.avail_in = (uInt)src_len;
  strm.next_out = (Bytef *)dst;
  strm.avail_out = (uInt)*dst_len;

  ret = deflate(&strm, Z_FINISH);
  if (ret == Z_STREAM_END)
  {
    *dst_len = (size_t)strm.total_out;
    deflateEnd(&strm);
    return APX_NO_ERROR;
  }

  deflateEnd(&strm);
  if ((ret == Z_OK) || (ret == Z_BUF_ERROR))
  {
    return APX_BUFFER_FULL_ERROR;
  }
  return (ret == Z_MEM_ERROR) ? APX_MEM_ERROR : APX_COMPRESSION_ERROR;
}

apx_error_t apx_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len, size_t max_output_size)
{
  if ((dst_len == NULL) || ((src_len > 0) && (src == NULL)))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if ((*dst_len > 0) && (dst == NULL))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if ((src_len > UINT_MAX) || (*dst_len > UINT_MAX))
  {
    return APX_LENGTH_ERROR;
  }

  // Validate RFC 1950 zlib header
  apx_error_t header_err = validate_zlib_header(src, src_len);
  if (header_err != APX_NO_ERROR)
  {
    return header_err;
  }

  size_t capacity = *dst_len;
  size_t limit = capacity;
  if ((max_output_size > 0) && (max_output_size < limit))
  {
    limit = max_output_size;
  }

  uint8_t dummy_out = 0;
#if defined(APX_COMPRESSION_BACKEND_MINIZ)
  // The zlib-compatible miniz API discards the precise checksum failure status.
  tinfl_decompressor *decoder = (tinfl_decompressor *)malloc(sizeof(*decoder));
  if (decoder == NULL)
  {
    return APX_MEM_ERROR;
  }
  tinfl_init(decoder);
  size_t input_size = src_len;
  size_t output_size = limit;
  uint8_t *output = limit > 0u ? dst : &dummy_out;
  tinfl_status const status = tinfl_decompress(decoder, src, &input_size, output, output, &output_size,
    TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  free(decoder);
  switch (status)
  {
  case TINFL_STATUS_DONE:
    if (input_size != src_len)
    {
      return APX_UNEXPECTED_DATA_ERROR;
    }
    *dst_len = output_size;
    return APX_NO_ERROR;
  case TINFL_STATUS_ADLER32_MISMATCH:
    return APX_CHECKSUM_ERROR;
  case TINFL_STATUS_NEEDS_MORE_INPUT:
  case TINFL_STATUS_FAILED_CANNOT_MAKE_PROGRESS:
    return APX_UNEXPECTED_END_ERROR;
  case TINFL_STATUS_HAS_MORE_OUTPUT:
    return max_output_size > 0u && limit == max_output_size && limit < capacity ? APX_LENGTH_ERROR :
                                                                                  APX_BUFFER_FULL_ERROR;
  case TINFL_STATUS_BAD_PARAM:
    return APX_INTERNAL_ERROR;
  default:
    return APX_DECOMPRESSION_ERROR;
  }
#else
  z_stream strm;
  memset(&strm, 0, sizeof(strm));

  int ret = inflateInit(&strm);
  if (ret != Z_OK)
  {
    return (ret == Z_MEM_ERROR) ? APX_MEM_ERROR : APX_DECOMPRESSION_ERROR;
  }

  strm.next_in = (Bytef *)src;
  strm.avail_in = (uInt)src_len;
  strm.next_out = (limit > 0) ? (Bytef *)dst : (Bytef *)&dummy_out;
  strm.avail_out = (uInt)limit;

  ret = inflate(&strm, Z_FINISH);

  if (ret == Z_STREAM_END)
  {
    if (strm.avail_in > 0)
    {
      // Trailing unconsumed data after end of zlib stream
      inflateEnd(&strm);
      return APX_UNEXPECTED_DATA_ERROR;
    }

    *dst_len = (size_t)strm.total_out;
    inflateEnd(&strm);
    return APX_NO_ERROR;
  }

  inflateEnd(&strm);

  if (ret == Z_NEED_DICT)
  {
    return APX_INVALID_HEADER_ERROR;
  }
  if (ret == Z_MEM_ERROR)
  {
    return APX_MEM_ERROR;
  }
  if (ret == Z_DATA_ERROR)
  {
    if ((strm.msg != NULL) && (strstr(strm.msg, "check") != NULL))
    {
      return APX_CHECKSUM_ERROR;
    }
    return APX_DECOMPRESSION_ERROR;
  }

  // If inflate stopped without reaching Z_STREAM_END:
  if (strm.avail_out == 0)
  {
    // Output space limit was reached before stream completion
    if ((max_output_size > 0) && (limit == max_output_size) && (max_output_size < capacity))
    {
      return APX_LENGTH_ERROR;
    }
    return APX_BUFFER_FULL_ERROR;
  }

  // avail_out > 0 but avail_in exhausted before Z_STREAM_END
  if (strm.avail_in == 0)
  {
    return APX_UNEXPECTED_END_ERROR;
  }

  return APX_DECOMPRESSION_ERROR;
#endif
}
