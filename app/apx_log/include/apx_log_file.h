/*****************************************************************************
 * \file      apx_log_file.h
 * \author    Conny Gustafsson
 * \date      2026-11-01
 * \brief     .apxlog container file reader and writer
 *
 * Copyright (c) 2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_LOG_FILE_H
#define APX_LOG_FILE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/error.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#define APX_LOG_MAGIC "\0APXLOG\n"
#define APX_LOG_MAGIC_SIZE 8u
#define APX_LOG_HEADER_SIZE 64u
#define APX_LOG_FORMAT_MAJOR 1u
#define APX_LOG_FORMAT_MINOR 0u

#define APX_LOG_FLAG_FINALIZED 0x00000001u
#define APX_LOG_FLAG_INDEX_PRESENT 0x00000002u

#define APX_LOG_EOF_ERROR APX_NOT_FOUND_ERROR

typedef struct apx_log_file_header_tag
{
  uint8_t magic[APX_LOG_MAGIC_SIZE];
  uint16_t format_major;
  uint16_t format_minor;
  uint32_t header_size;
  uint32_t flags;
  uint32_t reserved;
  uint64_t capture_start_unix_ns;
  uint64_t capture_end_unix_ns;
  uint64_t record_count;
  uint64_t index_offset;
  uint64_t data_offset;
} apx_log_file_header_t;

typedef struct apx_log_writer_tag
{
  FILE *fh;
  char *path;
  apx_log_file_header_t header;
  bool is_open;
} apx_log_writer_t;

typedef struct apx_log_reader_tag
{
  FILE *fh;
  char *path;
  apx_log_file_header_t header;
  uint64_t file_size;
  bool is_open;
} apx_log_reader_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

// Writer functions
void apx_log_writer_create(apx_log_writer_t *self);
void apx_log_writer_destroy(apx_log_writer_t *self);
apx_log_writer_t *apx_log_writer_new(void);
void apx_log_writer_delete(apx_log_writer_t *self);

apx_error_t apx_log_writer_open(apx_log_writer_t *self, const char *path);
void apx_log_writer_set_start_time(apx_log_writer_t *self, uint64_t start_unix_ns);
apx_error_t apx_log_writer_write_record(apx_log_writer_t *self, const uint8_t *data, uint32_t size);
apx_error_t apx_log_writer_finalize(apx_log_writer_t *self, uint64_t end_unix_ns);
void apx_log_writer_close(apx_log_writer_t *self);

// Reader functions
void apx_log_reader_create(apx_log_reader_t *self);
void apx_log_reader_destroy(apx_log_reader_t *self);
apx_log_reader_t *apx_log_reader_new(void);
void apx_log_reader_delete(apx_log_reader_t *self);

apx_error_t apx_log_reader_open(apx_log_reader_t *self, const char *path);
apx_error_t apx_log_reader_read_next_record(apx_log_reader_t *self, uint8_t **out_data, uint32_t *out_size);
void apx_log_reader_close(apx_log_reader_t *self);

#endif // APX_LOG_FILE_H
