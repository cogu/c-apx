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
typedef struct apx_text_log_base_vtable_tag
{

} apx_text_log_base_vtable_t;
*/

typedef struct apx_text_log_base_tag
{
   bool fileEnabled;
   bool syslogEnabled;
   char *syslogLabel;
   FILE *file; //this can point to stdout if configured
   MUTEX_T mutex;
   char lineEnding[2+1]; //"\n" or "\r\n"
} apx_text_log_base_t;



//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
//void apx_textLogVTable_init(apx_connection_base_vtable_t *self, void (*destructor)(void *arg)); //Keep in case it's needed later
void apx_textLogBase_create(apx_text_log_base_t *self);
void apx_textLogBase_destroy(apx_text_log_base_t *self);
void apx_textLogBase_enableSysLog(apx_text_log_base_t *self, const char *label);
void apx_textLogBase_enableStdout(apx_text_log_base_t *self);
void apx_textLogBase_enableFile(apx_text_log_base_t *self, const char *path);
void apx_textLogBase_closeAll(apx_text_log_base_t *self);
void apx_textLogBase_print(apx_text_log_base_t *self, const char *msg);
void apx_textLogBase_printf(apx_text_log_base_t *self, const char *format, ...);

#endif //APX_TEXT_LOG_BASE_H
