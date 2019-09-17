/*****************************************************************************
* \file      apx_textLogBase.c
* \author    Conny Gustafsson
* \date      2019-09-12
* \brief     Base class for text-based event loggers
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
//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include "apx_textLogBase.h"
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
void apx_textLogVTable_init(apx_connectionBaseVTable_t *self, void (*destructor)(void *arg))
{

}
*/

void apx_textLogBase_create(apx_textLogBase_t *self)
{
   if (self != 0)
   {
      self->file = (FILE*) 0;
      self->fileEnabled = false;
      self->syslogEnabled = false;
      self->syslogLabel = (char*) 0;
   }
}

void apx_textLogBase_destroy(apx_textLogBase_t *self)
{
   if (self != 0)
   {
      if ( (self->file != 0) && (self->file != stdout ) )
      {
         fflush(self->file);
         fclose(self->file);
      }
   }
}

void apx_textLogBase_enableSysLog(apx_textLogBase_t *self, const char *label)
{
   if ( (self != 0) && (label != 0))
   {
      self->syslogEnabled = true;
      self->syslogLabel = STRDUP(label);
#if !defined(_WIN32) && !defined(__CYGWIN__)
      openlog(self->syslogLabel, 0, LOG_USER );
#endif
   }
}

void apx_textLogBase_enableStdout(apx_textLogBase_t *self)
{
   if (self != 0)
   {
      self->fileEnabled = true;
      self->file = stdout;
   }
}

void apx_textLogBase_enableFile(apx_textLogBase_t *self, const char *path)
{
   if (self != 0)
   {
      FILE *fh = fopen(path, "w");
      if (fh != 0)
      {
         self->fileEnabled = true;
         self->file = fh;
      }
   }
}

void apx_textLogBase_closeAll(apx_textLogBase_t *self)
{
   if (self != 0)
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

//////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////


