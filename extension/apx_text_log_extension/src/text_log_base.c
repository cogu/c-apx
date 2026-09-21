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
#include <string.h>
#include "apx/extension/text_log_base.h"
#if !defined(_WIN32) && !defined(__CYGWIN__)
#include <syslog.h>
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
/*
void apx_textLogVTable_init(apx_connection_base_vtable_t *self, void (*destructor)(void *arg))
{

}
*/

void apx_textLogBase_create(apx_text_log_base_t *self)
{
   if (self != NULL)
   {
      self->file = NULL;
      self->fileEnabled = false;
      self->syslogEnabled = false;
      self->syslogLabel = NULL;
      strcpy(self->lineEnding, "\n");
      MUTEX_INIT(self->mutex);
   }
}

void apx_textLogBase_destroy(apx_text_log_base_t *self)
{
   if (self != NULL)
   {
      if ( (self->file != NULL) && (self->file != stdout ) )
      {
         fflush(self->file);
         fclose(self->file);
      }
      MUTEX_DESTROY(self->mutex);
   }
}

void apx_textLogBase_enableSysLog(apx_text_log_base_t *self, const char *label)
{
   if ( (self != NULL) && (label != NULL))
   {
      self->syslogEnabled = true;
      self->syslogLabel = STRDUP(label);
#if !defined(_WIN32) && !defined(__CYGWIN__)
      openlog(self->syslogLabel, 0, LOG_USER );
#endif
   }
}

void apx_textLogBase_enableStdout(apx_text_log_base_t *self)
{
   if (self != NULL)
   {
      self->fileEnabled = true;
      self->file = stdout;
   }
}

void apx_textLogBase_enableFile(apx_text_log_base_t *self, const char *path)
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

void apx_textLogBase_closeAll(apx_text_log_base_t *self)
{
   if (self != NULL)
   {
      if (self->fileEnabled != 0)
      {
         fflush(self->file);
         if (self->file != stdout)
         {
            fclose(self->file);
         }
         self->fileEnabled = false;
      }

      if (self->syslogEnabled != 0)
      {
#if !defined(_WIN32) && !defined(__CYGWIN__)
         closelog();
#endif
         self->syslogEnabled = false;
      }
   }
}

void apx_textLogBase_print(apx_text_log_base_t *self, const char *msg)
{
   if ( (self != NULL) && (msg != NULL))
   {
      MUTEX_LOCK(self->mutex);
      if(self->fileEnabled)
      {
         fprintf(self->file, "%s%s", msg, self->lineEnding);
      }
      MUTEX_UNLOCK(self->mutex);
   }
}

void apx_textLogBase_printf(apx_text_log_base_t *self, const char *format, ...)
{
   va_list args;
   va_start (args, format);
   MUTEX_LOCK(self->mutex);
   if(self->fileEnabled)
   {
      vfprintf(self->file, format, args);
      fprintf(self->file, "%s", self->lineEnding);
   }
   MUTEX_UNLOCK(self->mutex);
   va_end (args);
}


//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


