/*****************************************************************************
* \file      apx_text_log_base.h
* \author    Conny Gustafsson
* \date      2019-09-12
* \brief     Base class for text-based event loggers
*
* Copyright (c) 2019-2020 Conny Gustafsson
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
#ifndef APX_TEXT_LOG_BASE_H
#define APX_TEXT_LOG_BASE_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include "osmacro.h"
#include "apx/types.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////
/* Keep in case it's needed later
typedef struct apx_textLogBaseVTable_tag
{

} apx_textLogBaseVTable_t;
*/

typedef struct apx_textLogBase_tag
{
   bool fileEnabled;
   bool syslogEnabled;
   char *syslogLabel;
   FILE *file; //this can point to stdout if configured
   MUTEX_T mutex;
   char lineEnding[2+1]; //"\n" or "\r\n"
} apx_textLogBase_t;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//void apx_textLogVTable_init(apx_connectionBaseVTable_t *self, void (*destructor)(void *arg)); //Keep in case it's needed later
void apx_textLogBase_create(apx_textLogBase_t *self);
void apx_textLogBase_destroy(apx_textLogBase_t *self);
void apx_textLogBase_enableSysLog(apx_textLogBase_t *self, const char *label);
void apx_textLogBase_enableStdout(apx_textLogBase_t *self);
void apx_textLogBase_enableFile(apx_textLogBase_t *self, const char *path);
void apx_textLogBase_closeAll(apx_textLogBase_t *self);
void apx_textLogBase_print(apx_textLogBase_t *self, const char *msg);
void apx_textLogBase_printf(apx_textLogBase_t *self, const char *format, ...);

#endif //APX_TEXT_LOG_BASE_H
