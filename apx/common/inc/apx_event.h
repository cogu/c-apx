/*****************************************************************************
* \file:    apx_event.h
* \author:  Conny Gustafsson
* \date:    2018-05-01
* \brief:   Shared header containing event-related definitions
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

#ifndef APX_EVENT_H
#define APX_EVENT_H
#include <stdint.h>
#include "rmf.h"
#include "apx_file.h"
#include "apx_event.h"

typedef struct apx_eventFile_tag
{
   apx_file_t *file; //weak pointer to apx file object
}apx_eventFile_t;

#define APX_EVENT_FILE_NAME "apx_event.stream"
#define APX_EVENT_FILE_LEN 2048                 //maximum message size
#define APX_EVENT_FILE_ADDRESS 0x3FFFF400

#define APX_LOG_MODE_NONE     0u
#define APX_LOG_MODE_CAPTURE  1u
#define APX_LOG_MODE_PLAYBACK 2u

#endif //APX_EVENT_H
