/*****************************************************************************
 * \file      text_log_base.h
 * \author    Conny Gustafsson
 * \date      2019-09-12
 * \brief     Base class for text-based event loggers
 *
 * Copyright (c) 2019-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
#ifndef APX_TEXT_LOG_BASE_H
#define APX_TEXT_LOG_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
# include <Windows.h>
#else
# include <pthread.h>
#endif
#include "apx/types.h"
#include "osmacro.h"
#include <stdarg.h>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
/* Keep in case it's needed later
typedef struct apx_text_log_base_vtable_tag
{

} apx_text_log_base_vtable_t;
*/

typedef struct apx_text_log_base_tag
{
  bool fileEnabled;
  bool syslogEnabled;
  bool timestampEnabled;
  apx_log_level_t log_level;
  char *syslogLabel;
  FILE *file; // this can point to stdout if configured
  MUTEX_T mutex;
  char lineEnding[2 + 1]; //"\n" or "\r\n"
} apx_text_log_base_t;


//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
// void apx_text_log_vtable_init(apx_connection_base_vtable_t *self, void (*destructor)(void *arg)); //Keep in case it's
// needed later
void apx_text_log_base_create(apx_text_log_base_t *self);
void apx_text_log_base_destroy(apx_text_log_base_t *self);
void apx_text_log_base_enable_sys_log(apx_text_log_base_t *self, const char *label);
void apx_text_log_base_enable_stdout(apx_text_log_base_t *self);
void apx_text_log_base_enable_file(apx_text_log_base_t *self, const char *path);
void apx_text_log_base_close_all(apx_text_log_base_t *self);
void apx_text_log_base_set_log_level(apx_text_log_base_t *self, apx_log_level_t log_level);
apx_log_level_t apx_text_log_base_get_log_level(apx_text_log_base_t const *self);
void apx_text_log_base_set_timestamp_enabled(apx_text_log_base_t *self, bool enabled);
bool apx_text_log_base_get_timestamp_enabled(apx_text_log_base_t const *self);
void apx_text_log_base_print(apx_text_log_base_t *self, const char *msg);
void apx_text_log_base_printf(apx_text_log_base_t *self, const char *format, ...);
void apx_text_log_base_log(apx_text_log_base_t *self, apx_log_level_t level, const char *label, const char *msg);
#if defined(__GNUC__) || defined(__clang__)
void apx_text_log_base_logf(apx_text_log_base_t *self, apx_log_level_t level, const char *label, const char *format,
  ...) __attribute__((format(printf, 4, 5)));
#else
void apx_text_log_base_logf(
  apx_text_log_base_t *self, apx_log_level_t level, const char *label, const char *format, ...);
#endif

#endif // APX_TEXT_LOG_BASE_H
