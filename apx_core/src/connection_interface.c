/*****************************************************************************
 * \file      connection_interface.c
 * \brief     Shared connection message buffering
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#include "apx/connection_interface.h"
#include "apx/numheader.h"
#include <assert.h>
#include <limits.h>

static apx_error_t append_message(adt_bytearray_t *buffer, apx_size_t max_buffer_size, uint8_t const *header,
  apx_size_t header_size, uint8_t const *data, apx_size_t size, int32_t *bytes_available)
{
  if ((header_size > max_buffer_size) || (size > max_buffer_size - header_size))
  {
    return APX_MSG_TOO_LARGE_ERROR;
  }
  apx_size_t const buffered_size = adt_bytearray_length(buffer);
  if (buffered_size > max_buffer_size)
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  apx_size_t const message_size = header_size + size;
  if (message_size > max_buffer_size - buffered_size)
  {
    return APX_BUFFER_FULL_ERROR;
  }

  // Reserve the whole frame so allocation failure cannot leave a partial message.
  if (adt_bytearray_reserve(buffer, buffered_size + message_size) != ADT_NO_ERROR)
  {
    return APX_MEM_ERROR;
  }
  adt_error_t result = adt_bytearray_append(buffer, header, header_size);
  if ((result == ADT_NO_ERROR) && (size > 0u))
  {
    result = adt_bytearray_append(buffer, data, size);
  }
  if (result != ADT_NO_ERROR)
  {
    adt_error_t const rollback_result = adt_bytearray_resize(buffer, buffered_size);
    assert(rollback_result == ADT_NO_ERROR);
    (void)rollback_result;
    return APX_MEM_ERROR;
  }
  *bytes_available = (int32_t)(max_buffer_size - adt_bytearray_length(buffer));
  return APX_NO_ERROR;
}

apx_error_t apx_connection_buffer_append_data_message(adt_bytearray_t *buffer, apx_size_t max_buffer_size,
  uint32_t write_address, bool more_bit, uint8_t const *data, int32_t size, int32_t *bytes_available)
{
  if ((buffer == NULL) || (bytes_available == NULL) || (size < 0) || ((size > 0) && (data == NULL)) ||
    (max_buffer_size == 0u) || (max_buffer_size > INT32_MAX) || (write_address > RMF_HIGH_ADDR_MAX))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  apx_size_t const address_size = rmf_needed_encoding_size(write_address);
  if (((apx_size_t)size > max_buffer_size) || (address_size > max_buffer_size - (apx_size_t)size))
  {
    return APX_MSG_TOO_LARGE_ERROR;
  }
  uint8_t header[NUMHEADER32_LONG_SIZE + RMF_HIGH_ADDR_SIZE];
  apx_size_t const numheader_size = numheader_encode32(header, sizeof(header), address_size + (apx_size_t)size);
  apx_size_t const encoded_address_size =
    rmf_address_encode(header + numheader_size, sizeof(header) - numheader_size, write_address, more_bit);
  assert(numheader_size > 0u);
  assert(encoded_address_size == address_size);
  return append_message(
    buffer, max_buffer_size, header, numheader_size + encoded_address_size, data, (apx_size_t)size, bytes_available);
}

apx_error_t apx_connection_buffer_append_direct_message(
  adt_bytearray_t *buffer, apx_size_t max_buffer_size, uint8_t const *data, int32_t size, int32_t *bytes_available)
{
  if ((buffer == NULL) || (bytes_available == NULL) || (size < 0) || ((size > 0) && (data == NULL)) ||
    (max_buffer_size == 0u) || (max_buffer_size > INT32_MAX))
  {
    return APX_INVALID_ARGUMENT_ERROR;
  }
  if ((apx_size_t)size > max_buffer_size)
  {
    return APX_MSG_TOO_LARGE_ERROR;
  }
  uint8_t header[NUMHEADER32_LONG_SIZE];
  apx_size_t const header_size = numheader_encode32(header, sizeof(header), (apx_size_t)size);
  assert(header_size > 0u);
  return append_message(buffer, max_buffer_size, header, header_size, data, (apx_size_t)size, bytes_available);
}
