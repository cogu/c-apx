/*****************************************************************************
 * \file      text_log_base.c
 * \author    Conny Gustafsson
 * \date      2019-09-12
 * \brief     Base class for text-based event loggers
 *
 * Copyright (c) 2019-2026 Conny Gustafsson
 * SPDX-License-Identifier: MIT
 * See LICENSE in project root for full license terms.
 ******************************************************************************/
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx/extension/text_log_base.h"
#include "apx/util.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <sys/time.h>
# include <time.h>
#endif
#if !defined(_WIN32) && !defined(__CYGWIN__)
# include <syslog.h>
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
# define STRDUP _strdup
#else
# define STRDUP strdup
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
static void format_timestamp(char *buf, size_t buf_size);

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/*
void apx_text_log_vtable_init(apx_connection_base_vtable_t *self, void (*destructor)(void *arg))
{

}
*/

void apx_text_log_base_create(apx_text_log_base_t *self)
{
  if (self != NULL)
  {
    self->file = NULL;
    self->fileEnabled = false;
    self->syslogEnabled = false;
    self->timestampEnabled = false;
    self->log_level = APX_LOG_LEVEL_INFO;
    self->syslogLabel = NULL;
    strcpy(self->lineEnding, "\n");
    MUTEX_INIT(self->mutex);
  }
}

void apx_text_log_base_destroy(apx_text_log_base_t *self)
{
  if (self != NULL)
  {
    apx_text_log_base_close_all(self);
    if (self->syslogLabel != NULL)
    {
      free(self->syslogLabel);
      self->syslogLabel = NULL;
    }
    MUTEX_DESTROY(self->mutex);
  }
}

void apx_text_log_base_enable_sys_log(apx_text_log_base_t *self, const char *label)
{
  if ((self != NULL) && (label != NULL))
  {
    self->syslogEnabled = true;
    self->syslogLabel = STRDUP(label);
#if !defined(_WIN32) && !defined(__CYGWIN__)
    openlog(self->syslogLabel, 0, LOG_USER);
#endif
  }
}

void apx_text_log_base_enable_stdout(apx_text_log_base_t *self)
{
  if (self != NULL)
  {
    self->fileEnabled = true;
    self->file = stdout;
  }
}

void apx_text_log_base_enable_file(apx_text_log_base_t *self, const char *path)
{
  if (self != NULL)
  {
    FILE *fh = fopen(path, "w");
    if (fh != NULL)
    {
      self->fileEnabled = true;
      self->file = fh;
    }
  }
}

void apx_text_log_base_close_all(apx_text_log_base_t *self)
{
  if (self != NULL)
  {
    MUTEX_LOCK(self->mutex);
    if (self->fileEnabled)
    {
      if (self->file != NULL)
      {
        fflush(self->file);
        if (self->file != stdout)
        {
          fclose(self->file);
        }
        self->file = NULL;
      }
      self->fileEnabled = false;
    }

    if (self->syslogEnabled)
    {
#if !defined(_WIN32) && !defined(__CYGWIN__)
      closelog();
#endif
      self->syslogEnabled = false;
    }
    MUTEX_UNLOCK(self->mutex);
  }
}

void apx_text_log_base_print(apx_text_log_base_t *self, const char *msg)
{
  if ((self != NULL) && (msg != NULL))
  {
    MUTEX_LOCK(self->mutex);
    if (self->fileEnabled)
    {
      fprintf(self->file, "%s%s", msg, self->lineEnding);
    }
    MUTEX_UNLOCK(self->mutex);
  }
}

void apx_text_log_base_printf(apx_text_log_base_t *self, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  MUTEX_LOCK(self->mutex);
  if (self->fileEnabled)
  {
    vfprintf(self->file, format, args);
    fprintf(self->file, "%s", self->lineEnding);
  }
  MUTEX_UNLOCK(self->mutex);
  va_end(args);
}


void apx_text_log_base_set_log_level(apx_text_log_base_t *self, apx_log_level_t log_level)
{
  if (self != NULL)
  {
    self->log_level = log_level;
  }
}

apx_log_level_t apx_text_log_base_get_log_level(apx_text_log_base_t const *self)
{
  if (self != NULL)
  {
    return self->log_level;
  }
  return APX_LOG_LEVEL_INVALID;
}

void apx_text_log_base_set_timestamp_enabled(apx_text_log_base_t *self, bool enabled)
{
  if (self != NULL)
  {
    self->timestampEnabled = enabled;
  }
}

bool apx_text_log_base_get_timestamp_enabled(apx_text_log_base_t const *self)
{
  if (self != NULL)
  {
    return self->timestampEnabled;
  }
  return false;
}

void apx_text_log_base_log(apx_text_log_base_t *self, apx_log_level_t level, const char *label, const char *msg)
{
  if ((self != NULL) && (msg != NULL))
  {
    if (level > self->log_level)
    {
      return;
    }
    const char *lvl_str = apx_log_level_to_string(level);
    const char *lbl_str = (label != NULL && *label != '\0') ? label : "SERVER";

    MUTEX_LOCK(self->mutex);
    if (self->fileEnabled && (self->file != NULL))
    {
      if (self->timestampEnabled)
      {
        char ts_buf[32];
        format_timestamp(ts_buf, sizeof(ts_buf));
        fprintf(self->file, "[%s] [%s] [%s] %s%s", ts_buf, lvl_str, lbl_str, msg, self->lineEnding);
      }
      else
      {
        fprintf(self->file, "[%s] [%s] %s%s", lvl_str, lbl_str, msg, self->lineEnding);
      }
      fflush(self->file);
    }
    MUTEX_UNLOCK(self->mutex);
  }
}

void apx_text_log_base_logf(
  apx_text_log_base_t *self, apx_log_level_t level, const char *label, const char *format, ...)
{
  if ((self != NULL) && (format != NULL))
  {
    if (level > self->log_level)
    {
      return;
    }
    char buf[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    apx_text_log_base_log(self, level, label, buf);
  }
}

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
static void format_timestamp(char *buf, size_t buf_size)
{
#ifdef _WIN32
  SYSTEMTIME st;
  GetLocalTime(&st);
  snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d", (int)st.wYear, (int)st.wMonth, (int)st.wDay,
    (int)st.wHour, (int)st.wMinute, (int)st.wSecond, (int)st.wMilliseconds);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm tm_buf;
  localtime_r(&tv.tv_sec, &tm_buf);
  int millis = (int)(tv.tv_usec / 1000);
  char time_str[32];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);
  snprintf(buf, buf_size, "%s.%03d", time_str, millis);
#endif
}
