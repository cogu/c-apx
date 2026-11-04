/*****************************************************************************
 * \file      compress.h
 * \author    Conny Gustafsson
 * \date      2026-11-04
 * \brief     APX compression and decompression codec interface (RFC 1950 / RFC 1951)
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_COMPRESS_H
#define APX_COMPRESS_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_COMPRESSION_DEFAULT_LEVEL (-1)

//////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

/**
 * Returns the name of the compiled-in compression backend ("zlib" or "miniz").
 */
const char *apx_compress_backend_name(void);

/**
 * Returns the maximum buffer size required to compress src_len bytes.
 *
 * \param src_len Length of uncompressed data in bytes.
 * \return Maximum compressed size in bytes.
 */
size_t apx_compress_bound(size_t src_len);

/**
 * Compresses data using zlib-wrapped DEFLATE (RFC 1950 / RFC 1951) at default level.
 *
 * \param dst Pointer to destination buffer.
 * \param dst_len On entry, capacity of dst in bytes. On exit, actual compressed bytes written.
 * \param src Pointer to input data to compress.
 * \param src_len Length of input data in bytes.
 *
 * \return APX_NO_ERROR on success.
 * \return APX_INVALID_ARGUMENT_ERROR if dst, dst_len, or src (when src_len > 0) is NULL.
 * \return APX_BUFFER_FULL_ERROR if dst capacity is insufficient.
 * \return APX_COMPRESSION_ERROR on other compression errors.
 */
apx_error_t apx_compress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len);

/**
 * Compresses data using zlib-wrapped DEFLATE (RFC 1950 / RFC 1951) with specified level.
 *
 * \param dst Pointer to destination buffer.
 * \param dst_len On entry, capacity of dst in bytes. On exit, actual compressed bytes written.
 * \param src Pointer to input data to compress.
 * \param src_len Length of input data in bytes.
 * \param level Compression level (0-9, or APX_COMPRESSION_DEFAULT_LEVEL for default).
 *
 * \return APX_NO_ERROR on success.
 * \return APX_INVALID_ARGUMENT_ERROR if dst, dst_len, or src (when src_len > 0) is NULL, or invalid level.
 * \return APX_BUFFER_FULL_ERROR if dst capacity is insufficient.
 * \return APX_COMPRESSION_ERROR on other compression errors.
 */
apx_error_t apx_compress_level(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len, int level);

/**
 * Decompresses a zlib-wrapped DEFLATE stream (RFC 1950 / RFC 1951) with bounded output.
 *
 * Enforces:
 * - Exact input consumption: All compressed bytes must be consumed. Trailing data is rejected.
 * - Complete stream termination: The stream must reach Z_STREAM_END. Truncated streams are rejected.
 * - Valid RFC 1950 zlib header and Adler-32 checksum.
 * - Preset dictionaries, raw DEFLATE, and gzip streams are rejected.
 * - Bounded output: Decompression stops and returns an error if output would exceed max_output_size
 *   or dst buffer capacity (*dst_len).
 *
 * \param dst Pointer to destination buffer where decompressed data will be written.
 * \param dst_len On entry, capacity of dst in bytes. On exit, actual number of bytes written.
 * \param src Pointer to compressed input data.
 * \param src_len Length of compressed input data in bytes.
 * \param max_output_size Maximum allowed decompressed size in bytes (0 for no limit beyond dst_len).
 *
 * \return APX_NO_ERROR on success.
 * \return APX_INVALID_ARGUMENT_ERROR if dst, dst_len, or src (when src_len > 0) is NULL.
 * \return APX_BUFFER_FULL_ERROR if output exceeds dst buffer capacity (*dst_len).
 * \return APX_LENGTH_ERROR if decompressed size exceeds max_output_size (when max_output_size > 0).
 * \return APX_UNEXPECTED_END_ERROR if stream is truncated (premature EOF).
 * \return APX_UNEXPECTED_DATA_ERROR if stream has trailing unconsumed bytes after Z_STREAM_END.
 * \return APX_INVALID_HEADER_ERROR if zlib header is invalid (wrong method, preset dict, or gzip/raw).
 * \return APX_CHECKSUM_ERROR if Adler-32 checksum verification fails.
 * \return APX_DECOMPRESSION_ERROR on other inflate failures.
 */
apx_error_t apx_decompress(uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t src_len, size_t max_output_size);

#endif // APX_COMPRESS_H
