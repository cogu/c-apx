/*****************************************************************************
* \file      apx_eventRecorderSrvTxt.h
* \author    Conny Gustafsson
* \date      2018-08-07
* \brief     Description
*
* Copyright (c) 2018 Conny Gustafsson
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
#ifndef APX_EVENT_RECORDER_SRV_TXT_H
#define APX_EVENT_RECORDER_SRV_TXT_H

//////////////////////////////////////////////////////////////////////////////
// INCLUDES
//////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
# endif
# include <Windows.h>
#else
# include <pthread.h>
# include <semaphore.h>
#endif
#include "osmacro.h"
#include "apx_eventListener.h"

//////////////////////////////////////////////////////////////////////////////
// PUBLIC CONSTANTS AND DATA TYPES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// PUBLIC VARIABLES
//////////////////////////////////////////////////////////////////////////////
//forward declarations
struct apx_server_tag;

typedef struct apx_eventRecorderSrvTxt_t
{
   char *fileName;
   FILE *fp;
   MUTEX_T mutex;
}apx_eventRecorderSrvTxt_t;

//////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES
//////////////////////////////////////////////////////////////////////////////
void apx_eventRecorderSrvTxt_create(apx_eventRecorderSrvTxt_t *self);
void apx_eventRecorderSrvTxt_destroy(apx_eventRecorderSrvTxt_t *self);
apx_eventRecorderSrvTxt_t *apx_eventRecorderSrvTxt_new(void);
void apx_eventRecorderSrvTxt_delete(apx_eventRecorderSrvTxt_t *self);
void apx_eventRecorderSrvTxt_register(apx_eventRecorderSrvTxt_t *self, struct apx_server_tag *server);
void apx_eventRecorderSrvTxt_open(apx_eventRecorderSrvTxt_t *self, const char *fileName);
void apx_eventRecorderSrvTxt_close(apx_eventRecorderSrvTxt_t *self);


#endif //APX_EVENT_RECORDER_SRV_TXT_H
